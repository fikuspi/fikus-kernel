/*
 * NXP SC18IS602/603 SPI driver
 *
 * Copyright (C) Guenter Roeck <fikus@roeck-us.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fikus/kernel.h>
#include <fikus/err.h>
#include <fikus/module.h>
#include <fikus/spi/spi.h>
#include <fikus/i2c.h>
#include <fikus/delay.h>
#include <fikus/pm_runtime.h>
#include <fikus/of.h>
#include <fikus/platform_data/sc18is602.h>

enum chips { sc18is602, sc18is602b, sc18is603 };

#define SC18IS602_BUFSIZ		200
#define SC18IS602_CLOCK			7372000

#define SC18IS602_MODE_CPHA		BIT(2)
#define SC18IS602_MODE_CPOL		BIT(3)
#define SC18IS602_MODE_LSB_FIRST	BIT(5)
#define SC18IS602_MODE_CLOCK_DIV_4	0x0
#define SC18IS602_MODE_CLOCK_DIV_16	0x1
#define SC18IS602_MODE_CLOCK_DIV_64	0x2
#define SC18IS602_MODE_CLOCK_DIV_128	0x3

struct sc18is602 {
	struct spi_master	*master;
	struct device		*dev;
	u8			ctrl;
	u32			freq;
	u32			speed;

	/* I2C data */
	struct i2c_client	*client;
	enum chips		id;
	u8			buffer[SC18IS602_BUFSIZ + 1];
	int			tlen;	/* Data queued for tx in buffer */
	int			rindex;	/* Receive data index in buffer */
};

static int sc18is602_wait_ready(struct sc18is602 *hw, int len)
{
	int i, err;
	int usecs = 1000000 * len / hw->speed + 1;
	u8 dummy[1];

	for (i = 0; i < 10; i++) {
		err = i2c_master_recv(hw->client, dummy, 1);
		if (err >= 0)
			return 0;
		usleep_range(usecs, usecs * 2);
	}
	return -ETIMEDOUT;
}

static int sc18is602_txrx(struct sc18is602 *hw, struct spi_message *msg,
			  struct spi_transfer *t, bool do_transfer)
{
	unsigned int len = t->len;
	int ret;

	if (hw->tlen == 0) {
		/* First byte (I2C command) is chip select */
		hw->buffer[0] = 1 << msg->spi->chip_select;
		hw->tlen = 1;
		hw->rindex = 0;
	}
	/*
	 * We can not immediately send data to the chip, since each I2C message
	 * resembles a full SPI message (from CS active to CS inactive).
	 * Enqueue messages up to the first read or until do_transfer is true.
	 */
	if (t->tx_buf) {
		memcpy(&hw->buffer[hw->tlen], t->tx_buf, len);
		hw->tlen += len;
		if (t->rx_buf)
			do_transfer = true;
		else
			hw->rindex = hw->tlen - 1;
	} else if (t->rx_buf) {
		/*
		 * For receive-only transfers we still need to perform a dummy
		 * write to receive data from the SPI chip.
		 * Read data starts at the end of transmit data (minus 1 to
		 * account for CS).
		 */
		hw->rindex = hw->tlen - 1;
		memset(&hw->buffer[hw->tlen], 0, len);
		hw->tlen += len;
		do_transfer = true;
	}

	if (do_transfer && hw->tlen > 1) {
		ret = sc18is602_wait_ready(hw, SC18IS602_BUFSIZ);
		if (ret < 0)
			return ret;
		ret = i2c_master_send(hw->client, hw->buffer, hw->tlen);
		if (ret < 0)
			return ret;
		if (ret != hw->tlen)
			return -EIO;

		if (t->rx_buf) {
			int rlen = hw->rindex + len;

			ret = sc18is602_wait_ready(hw, hw->tlen);
			if (ret < 0)
				return ret;
			ret = i2c_master_recv(hw->client, hw->buffer, rlen);
			if (ret < 0)
				return ret;
			if (ret != rlen)
				return -EIO;
			memcpy(t->rx_buf, &hw->buffer[hw->rindex], len);
		}
		hw->tlen = 0;
	}
	return len;
}

static int sc18is602_setup_transfer(struct sc18is602 *hw, u32 hz, u8 mode)
{
	u8 ctrl = 0;
	int ret;

	if (mode & SPI_CPHA)
		ctrl |= SC18IS602_MODE_CPHA;
	if (mode & SPI_CPOL)
		ctrl |= SC18IS602_MODE_CPOL;
	if (mode & SPI_LSB_FIRST)
		ctrl |= SC18IS602_MODE_LSB_FIRST;

	/* Find the closest clock speed */
	if (hz >= hw->freq / 4) {
		ctrl |= SC18IS602_MODE_CLOCK_DIV_4;
		hw->speed = hw->freq / 4;
	} else if (hz >= hw->freq / 16) {
		ctrl |= SC18IS602_MODE_CLOCK_DIV_16;
		hw->speed = hw->freq / 16;
	} else if (hz >= hw->freq / 64) {
		ctrl |= SC18IS602_MODE_CLOCK_DIV_64;
		hw->speed = hw->freq / 64;
	} else {
		ctrl |= SC18IS602_MODE_CLOCK_DIV_128;
		hw->speed = hw->freq / 128;
	}

	/*
	 * Don't do anything if the control value did not change. The initial
	 * value of 0xff for hw->ctrl ensures that the correct mode will be set
	 * with the first call to this function.
	 */
	if (ctrl == hw->ctrl)
		return 0;

	ret = i2c_smbus_write_byte_data(hw->client, 0xf0, ctrl);
	if (ret < 0)
		return ret;

	hw->ctrl = ctrl;

	return 0;
}

static int sc18is602_check_transfer(struct spi_device *spi,
				    struct spi_transfer *t, int tlen)
{
	int bpw;
	uint32_t hz;

	if (t && t->len + tlen > SC18IS602_BUFSIZ)
		return -EINVAL;

	bpw = spi->bits_per_word;
	if (t && t->bits_per_word)
		bpw = t->bits_per_word;
	if (bpw != 8)
		return -EINVAL;

	hz = spi->max_speed_hz;
	if (t && t->speed_hz)
		hz = t->speed_hz;
	if (hz == 0)
		return -EINVAL;

	return 0;
}

static int sc18is602_transfer_one(struct spi_master *master,
				  struct spi_message *m)
{
	struct sc18is602 *hw = spi_master_get_devdata(master);
	struct spi_device *spi = m->spi;
	struct spi_transfer *t;
	int status = 0;

	/* SC18IS602 does not support CS2 */
	if (hw->id == sc18is602 && spi->chip_select == 2) {
		status = -ENXIO;
		goto error;
	}

	hw->tlen = 0;
	list_for_each_entry(t, &m->transfers, transfer_list) {
		u32 hz = t->speed_hz ? : spi->max_speed_hz;
		bool do_transfer;

		status = sc18is602_check_transfer(spi, t, hw->tlen);
		if (status < 0)
			break;

		status = sc18is602_setup_transfer(hw, hz, spi->mode);
		if (status < 0)
			break;

		do_transfer = t->cs_change || list_is_last(&t->transfer_list,
							   &m->transfers);

		if (t->len) {
			status = sc18is602_txrx(hw, m, t, do_transfer);
			if (status < 0)
				break;
			m->actual_length += status;
		}
		status = 0;

		if (t->delay_usecs)
			udelay(t->delay_usecs);
	}
error:
	m->status = status;
	spi_finalize_current_message(master);

	return status;
}

static int sc18is602_setup(struct spi_device *spi)
{
	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	if (spi->mode & ~(SPI_CPHA | SPI_CPOL | SPI_LSB_FIRST))
		return -EINVAL;

	return sc18is602_check_transfer(spi, NULL, 0);
}

static int sc18is602_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;
	struct sc18is602_platform_data *pdata = dev_get_platdata(dev);
	struct sc18is602 *hw;
	struct spi_master *master;
	int error;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C |
				     I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		return -EINVAL;

	master = spi_alloc_master(dev, sizeof(struct sc18is602));
	if (!master)
		return -ENOMEM;

	hw = spi_master_get_devdata(master);
	i2c_set_clientdata(client, hw);

	hw->master = master;
	hw->client = client;
	hw->dev = dev;
	hw->ctrl = 0xff;

	hw->id = id->driver_data;

	switch (hw->id) {
	case sc18is602:
	case sc18is602b:
		master->num_chipselect = 4;
		hw->freq = SC18IS602_CLOCK;
		break;
	case sc18is603:
		master->num_chipselect = 2;
		if (pdata) {
			hw->freq = pdata->clock_frequency;
		} else {
			const __be32 *val;
			int len;

			val = of_get_property(np, "clock-frequency", &len);
			if (val && len >= sizeof(__be32))
				hw->freq = be32_to_cpup(val);
		}
		if (!hw->freq)
			hw->freq = SC18IS602_CLOCK;
		break;
	}
	master->bus_num = client->adapter->nr;
	master->mode_bits = SPI_CPHA | SPI_CPOL | SPI_LSB_FIRST;
	master->setup = sc18is602_setup;
	master->transfer_one_message = sc18is602_transfer_one;
	master->dev.of_node = np;

	error = spi_register_master(master);
	if (error)
		goto error_reg;

	return 0;

error_reg:
	spi_master_put(master);
	return error;
}

static int sc18is602_remove(struct i2c_client *client)
{
	struct sc18is602 *hw = i2c_get_clientdata(client);
	struct spi_master *master = hw->master;

	spi_unregister_master(master);

	return 0;
}

static const struct i2c_device_id sc18is602_id[] = {
	{ "sc18is602", sc18is602 },
	{ "sc18is602b", sc18is602b },
	{ "sc18is603", sc18is603 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc18is602_id);

static struct i2c_driver sc18is602_driver = {
	.driver = {
		.name = "sc18is602",
	},
	.probe = sc18is602_probe,
	.remove = sc18is602_remove,
	.id_table = sc18is602_id,
};

module_i2c_driver(sc18is602_driver);

MODULE_DESCRIPTION("SC18IC602/603 SPI Master Driver");
MODULE_AUTHOR("Guenter Roeck");
MODULE_LICENSE("GPL");
