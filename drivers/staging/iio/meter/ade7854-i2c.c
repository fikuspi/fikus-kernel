/*
 * ADE7854/58/68/78 Polyphase Multifunction Energy Metering IC Driver (I2C Bus)
 *
 * Copyright 2010 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <fikus/device.h>
#include <fikus/kernel.h>
#include <fikus/i2c.h>
#include <fikus/slab.h>
#include <fikus/module.h>

#include <fikus/iio/iio.h>
#include "ade7854.h"

static int ade7854_i2c_write_reg_8(struct device *dev,
		u16 reg_address,
		u8 value)
{
	int ret;
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ade7854_state *st = iio_priv(indio_dev);

	mutex_lock(&st->buf_lock);
	st->tx[0] = (reg_address >> 8) & 0xFF;
	st->tx[1] = reg_address & 0xFF;
	st->tx[2] = value;

	ret = i2c_master_send(st->i2c, st->tx, 3);
	mutex_unlock(&st->buf_lock);

	return ret;
}

static int ade7854_i2c_write_reg_16(struct device *dev,
		u16 reg_address,
		u16 value)
{
	int ret;
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ade7854_state *st = iio_priv(indio_dev);

	mutex_lock(&st->buf_lock);
	st->tx[0] = (reg_address >> 8) & 0xFF;
	st->tx[1] = reg_address & 0xFF;
	st->tx[2] = (value >> 8) & 0xFF;
	st->tx[3] = value & 0xFF;

	ret = i2c_master_send(st->i2c, st->tx, 4);
	mutex_unlock(&st->buf_lock);

	return ret;
}

static int ade7854_i2c_write_reg_24(struct device *dev,
		u16 reg_address,
		u32 value)
{
	int ret;
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ade7854_state *st = iio_priv(indio_dev);

	mutex_lock(&st->buf_lock);
	st->tx[0] = (reg_address >> 8) & 0xFF;
	st->tx[1] = reg_address & 0xFF;
	st->tx[2] = (value >> 16) & 0xFF;
	st->tx[3] = (value >> 8) & 0xFF;
	st->tx[4] = value & 0xFF;

	ret = i2c_master_send(st->i2c, st->tx, 5);
	mutex_unlock(&st->buf_lock);

	return ret;
}

static int ade7854_i2c_write_reg_32(struct device *dev,
		u16 reg_address,
		u32 value)
{
	int ret;
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ade7854_state *st = iio_priv(indio_dev);

	mutex_lock(&st->buf_lock);
	st->tx[0] = (reg_address >> 8) & 0xFF;
	st->tx[1] = reg_address & 0xFF;
	st->tx[2] = (value >> 24) & 0xFF;
	st->tx[3] = (value >> 16) & 0xFF;
	st->tx[4] = (value >> 8) & 0xFF;
	st->tx[5] = value & 0xFF;

	ret = i2c_master_send(st->i2c, st->tx, 6);
	mutex_unlock(&st->buf_lock);

	return ret;
}

static int ade7854_i2c_read_reg_8(struct device *dev,
		u16 reg_address,
		u8 *val)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ade7854_state *st = iio_priv(indio_dev);
	int ret;

	mutex_lock(&st->buf_lock);
	st->tx[0] = (reg_address >> 8) & 0xFF;
	st->tx[1] = reg_address & 0xFF;

	ret = i2c_master_send(st->i2c, st->tx, 2);
	if (ret)
		goto out;

	ret = i2c_master_recv(st->i2c, st->rx, 1);
	if (ret)
		goto out;

	*val = st->rx[0];
out:
	mutex_unlock(&st->buf_lock);
	return ret;
}

static int ade7854_i2c_read_reg_16(struct device *dev,
		u16 reg_address,
		u16 *val)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ade7854_state *st = iio_priv(indio_dev);
	int ret;

	mutex_lock(&st->buf_lock);
	st->tx[0] = (reg_address >> 8) & 0xFF;
	st->tx[1] = reg_address & 0xFF;

	ret = i2c_master_send(st->i2c, st->tx, 2);
	if (ret)
		goto out;

	ret = i2c_master_recv(st->i2c, st->rx, 2);
	if (ret)
		goto out;

	*val = (st->rx[0] << 8) | st->rx[1];
out:
	mutex_unlock(&st->buf_lock);
	return ret;
}

static int ade7854_i2c_read_reg_24(struct device *dev,
		u16 reg_address,
		u32 *val)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ade7854_state *st = iio_priv(indio_dev);
	int ret;

	mutex_lock(&st->buf_lock);
	st->tx[0] = (reg_address >> 8) & 0xFF;
	st->tx[1] = reg_address & 0xFF;

	ret = i2c_master_send(st->i2c, st->tx, 2);
	if (ret)
		goto out;

	ret = i2c_master_recv(st->i2c, st->rx, 3);
	if (ret)
		goto out;

	*val = (st->rx[0] << 16) | (st->rx[1] << 8) | st->rx[2];
out:
	mutex_unlock(&st->buf_lock);
	return ret;
}

static int ade7854_i2c_read_reg_32(struct device *dev,
		u16 reg_address,
		u32 *val)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ade7854_state *st = iio_priv(indio_dev);
	int ret;

	mutex_lock(&st->buf_lock);
	st->tx[0] = (reg_address >> 8) & 0xFF;
	st->tx[1] = reg_address & 0xFF;

	ret = i2c_master_send(st->i2c, st->tx, 2);
	if (ret)
		goto out;

	ret = i2c_master_recv(st->i2c, st->rx, 3);
	if (ret)
		goto out;

	*val = (st->rx[0] << 24) | (st->rx[1] << 16) | (st->rx[2] << 8) | st->rx[3];
out:
	mutex_unlock(&st->buf_lock);
	return ret;
}

static int ade7854_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret;
	struct ade7854_state *st;
	struct iio_dev *indio_dev;

	indio_dev = iio_device_alloc(sizeof(*st));
	if (indio_dev == NULL)
		return -ENOMEM;
	st = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	st->read_reg_8 = ade7854_i2c_read_reg_8;
	st->read_reg_16 = ade7854_i2c_read_reg_16;
	st->read_reg_24 = ade7854_i2c_read_reg_24;
	st->read_reg_32 = ade7854_i2c_read_reg_32;
	st->write_reg_8 = ade7854_i2c_write_reg_8;
	st->write_reg_16 = ade7854_i2c_write_reg_16;
	st->write_reg_24 = ade7854_i2c_write_reg_24;
	st->write_reg_32 = ade7854_i2c_write_reg_32;
	st->i2c = client;
	st->irq = client->irq;

	ret = ade7854_probe(indio_dev, &client->dev);
	if (ret)
		iio_device_free(indio_dev);

	return ret;
}

static int ade7854_i2c_remove(struct i2c_client *client)
{
	return ade7854_remove(i2c_get_clientdata(client));
}

static const struct i2c_device_id ade7854_id[] = {
	{ "ade7854", 0 },
	{ "ade7858", 0 },
	{ "ade7868", 0 },
	{ "ade7878", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ade7854_id);

static struct i2c_driver ade7854_i2c_driver = {
	.driver = {
		.name = "ade7854",
	},
	.probe    = ade7854_i2c_probe,
	.remove   = ade7854_i2c_remove,
	.id_table = ade7854_id,
};
module_i2c_driver(ade7854_i2c_driver);

MODULE_AUTHOR("Barry Song <21cnbao@gmail.com>");
MODULE_DESCRIPTION("Analog Devices ADE7854/58/68/78 Polyphase Multifunction Energy Metering IC I2C Driver");
MODULE_LICENSE("GPL v2");
