/*
 * Driver for pcf857x, pca857x, and pca967x I2C GPIO expanders
 *
 * Copyright (C) 2007 David Brownell
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <fikus/gpio.h>
#include <fikus/i2c.h>
#include <fikus/i2c/pcf857x.h>
#include <fikus/interrupt.h>
#include <fikus/irq.h>
#include <fikus/irqdomain.h>
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/slab.h>
#include <fikus/spinlock.h>
#include <fikus/workqueue.h>


static const struct i2c_device_id pcf857x_id[] = {
	{ "pcf8574", 8 },
	{ "pcf8574a", 8 },
	{ "pca8574", 8 },
	{ "pca9670", 8 },
	{ "pca9672", 8 },
	{ "pca9674", 8 },
	{ "pcf8575", 16 },
	{ "pca8575", 16 },
	{ "pca9671", 16 },
	{ "pca9673", 16 },
	{ "pca9675", 16 },
	{ "max7328", 8 },
	{ "max7329", 8 },
	{ "tca9554", 8 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pcf857x_id);

/*
 * The pcf857x, pca857x, and pca967x chips only expose one read and one
 * write register.  Writing a "one" bit (to match the reset state) lets
 * that pin be used as an input; it's not an open-drain model, but acts
 * a bit like one.  This is described as "quasi-bidirectional"; read the
 * chip documentation for details.
 *
 * Many other I2C GPIO expander chips (like the pca953x models) have
 * more complex register models and more conventional circuitry using
 * push/pull drivers.  They often use the same 0x20..0x27 addresses as
 * pcf857x parts, making the "legacy" I2C driver model problematic.
 */
struct pcf857x {
	struct gpio_chip	chip;
	struct i2c_client	*client;
	struct mutex		lock;		/* protect 'out' */
	struct work_struct	work;		/* irq demux work */
	struct irq_domain	*irq_domain;	/* for irq demux  */
	spinlock_t		slock;		/* protect irq demux */
	unsigned		out;		/* software latch */
	unsigned		status;		/* current status */
	int			irq;		/* real irq number */

	int (*write)(struct i2c_client *client, unsigned data);
	int (*read)(struct i2c_client *client);
};

/*-------------------------------------------------------------------------*/

/* Talk to 8-bit I/O expander */

static int i2c_write_le8(struct i2c_client *client, unsigned data)
{
	return i2c_smbus_write_byte(client, data);
}

static int i2c_read_le8(struct i2c_client *client)
{
	return (int)i2c_smbus_read_byte(client);
}

/* Talk to 16-bit I/O expander */

static int i2c_write_le16(struct i2c_client *client, unsigned word)
{
	u8 buf[2] = { word & 0xff, word >> 8, };
	int status;

	status = i2c_master_send(client, buf, 2);
	return (status < 0) ? status : 0;
}

static int i2c_read_le16(struct i2c_client *client)
{
	u8 buf[2];
	int status;

	status = i2c_master_recv(client, buf, 2);
	if (status < 0)
		return status;
	return (buf[1] << 8) | buf[0];
}

/*-------------------------------------------------------------------------*/

static int pcf857x_input(struct gpio_chip *chip, unsigned offset)
{
	struct pcf857x	*gpio = container_of(chip, struct pcf857x, chip);
	int		status;

	mutex_lock(&gpio->lock);
	gpio->out |= (1 << offset);
	status = gpio->write(gpio->client, gpio->out);
	mutex_unlock(&gpio->lock);

	return status;
}

static int pcf857x_get(struct gpio_chip *chip, unsigned offset)
{
	struct pcf857x	*gpio = container_of(chip, struct pcf857x, chip);
	int		value;

	value = gpio->read(gpio->client);
	return (value < 0) ? 0 : (value & (1 << offset));
}

static int pcf857x_output(struct gpio_chip *chip, unsigned offset, int value)
{
	struct pcf857x	*gpio = container_of(chip, struct pcf857x, chip);
	unsigned	bit = 1 << offset;
	int		status;

	mutex_lock(&gpio->lock);
	if (value)
		gpio->out |= bit;
	else
		gpio->out &= ~bit;
	status = gpio->write(gpio->client, gpio->out);
	mutex_unlock(&gpio->lock);

	return status;
}

static void pcf857x_set(struct gpio_chip *chip, unsigned offset, int value)
{
	pcf857x_output(chip, offset, value);
}

/*-------------------------------------------------------------------------*/

static int pcf857x_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct pcf857x *gpio = container_of(chip, struct pcf857x, chip);

	return irq_create_mapping(gpio->irq_domain, offset);
}

static void pcf857x_irq_demux_work(struct work_struct *work)
{
	struct pcf857x *gpio = container_of(work,
					       struct pcf857x,
					       work);
	unsigned long change, i, status, flags;

	status = gpio->read(gpio->client);

	spin_lock_irqsave(&gpio->slock, flags);

	change = gpio->status ^ status;
	for_each_set_bit(i, &change, gpio->chip.ngpio)
		generic_handle_irq(irq_find_mapping(gpio->irq_domain, i));
	gpio->status = status;

	spin_unlock_irqrestore(&gpio->slock, flags);
}

static irqreturn_t pcf857x_irq_demux(int irq, void *data)
{
	struct pcf857x	*gpio = data;

	/*
	 * pcf857x can't read/write data here,
	 * since i2c data access might go to sleep.
	 */
	schedule_work(&gpio->work);

	return IRQ_HANDLED;
}

static int pcf857x_irq_domain_map(struct irq_domain *domain, unsigned int virq,
				 irq_hw_number_t hw)
{
	irq_set_chip_and_handler(virq,
				 &dummy_irq_chip,
				 handle_level_irq);
	return 0;
}

static struct irq_domain_ops pcf857x_irq_domain_ops = {
	.map	= pcf857x_irq_domain_map,
};

static void pcf857x_irq_domain_cleanup(struct pcf857x *gpio)
{
	if (gpio->irq_domain)
		irq_domain_remove(gpio->irq_domain);

	if (gpio->irq)
		free_irq(gpio->irq, gpio);
}

static int pcf857x_irq_domain_init(struct pcf857x *gpio,
				   struct i2c_client *client)
{
	int status;

	gpio->irq_domain = irq_domain_add_linear(client->dev.of_node,
						 gpio->chip.ngpio,
						 &pcf857x_irq_domain_ops,
						 NULL);
	if (!gpio->irq_domain)
		goto fail;

	/* enable real irq */
	status = request_irq(client->irq, pcf857x_irq_demux, 0,
			     dev_name(&client->dev), gpio);
	if (status)
		goto fail;

	/* enable gpio_to_irq() */
	INIT_WORK(&gpio->work, pcf857x_irq_demux_work);
	gpio->chip.to_irq	= pcf857x_to_irq;
	gpio->irq		= client->irq;

	return 0;

fail:
	pcf857x_irq_domain_cleanup(gpio);
	return -EINVAL;
}

/*-------------------------------------------------------------------------*/

static int pcf857x_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct pcf857x_platform_data	*pdata;
	struct pcf857x			*gpio;
	int				status;

	pdata = dev_get_platdata(&client->dev);
	if (!pdata) {
		dev_dbg(&client->dev, "no platform data\n");
	}

	/* Allocate, initialize, and register this gpio_chip. */
	gpio = devm_kzalloc(&client->dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;

	mutex_init(&gpio->lock);
	spin_lock_init(&gpio->slock);

	gpio->chip.base			= pdata ? pdata->gpio_base : -1;
	gpio->chip.can_sleep		= 1;
	gpio->chip.dev			= &client->dev;
	gpio->chip.owner		= THIS_MODULE;
	gpio->chip.get			= pcf857x_get;
	gpio->chip.set			= pcf857x_set;
	gpio->chip.direction_input	= pcf857x_input;
	gpio->chip.direction_output	= pcf857x_output;
	gpio->chip.ngpio		= id->driver_data;

	/* enable gpio_to_irq() if platform has settings */
	if (client->irq) {
		status = pcf857x_irq_domain_init(gpio, client);
		if (status < 0) {
			dev_err(&client->dev, "irq_domain init failed\n");
			goto fail;
		}
	}

	/* NOTE:  the OnSemi jlc1562b is also largely compatible with
	 * these parts, notably for output.  It has a low-resolution
	 * DAC instead of pin change IRQs; and its inputs can be the
	 * result of comparators.
	 */

	/* 8574 addresses are 0x20..0x27; 8574a uses 0x38..0x3f;
	 * 9670, 9672, 9764, and 9764a use quite a variety.
	 *
	 * NOTE: we don't distinguish here between *4 and *4a parts.
	 */
	if (gpio->chip.ngpio == 8) {
		gpio->write	= i2c_write_le8;
		gpio->read	= i2c_read_le8;

		if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE))
			status = -EIO;

		/* fail if there's no chip present */
		else
			status = i2c_smbus_read_byte(client);

	/* '75/'75c addresses are 0x20..0x27, just like the '74;
	 * the '75c doesn't have a current source pulling high.
	 * 9671, 9673, and 9765 use quite a variety of addresses.
	 *
	 * NOTE: we don't distinguish here between '75 and '75c parts.
	 */
	} else if (gpio->chip.ngpio == 16) {
		gpio->write	= i2c_write_le16;
		gpio->read	= i2c_read_le16;

		if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
			status = -EIO;

		/* fail if there's no chip present */
		else
			status = i2c_read_le16(client);

	} else {
		dev_dbg(&client->dev, "unsupported number of gpios\n");
		status = -EINVAL;
	}

	if (status < 0)
		goto fail;

	gpio->chip.label = client->name;

	gpio->client = client;
	i2c_set_clientdata(client, gpio);

	/* NOTE:  these chips have strange "quasi-bidirectional" I/O pins.
	 * We can't actually know whether a pin is configured (a) as output
	 * and driving the signal low, or (b) as input and reporting a low
	 * value ... without knowing the last value written since the chip
	 * came out of reset (if any).  We can't read the latched output.
	 *
	 * In short, the only reliable solution for setting up pin direction
	 * is to do it explicitly.  The setup() method can do that, but it
	 * may cause transient glitching since it can't know the last value
	 * written (some pins may need to be driven low).
	 *
	 * Using pdata->n_latch avoids that trouble.  When left initialized
	 * to zero, our software copy of the "latch" then matches the chip's
	 * all-ones reset state.  Otherwise it flags pins to be driven low.
	 */
	gpio->out = pdata ? ~pdata->n_latch : ~0;
	gpio->status = gpio->out;

	status = gpiochip_add(&gpio->chip);
	if (status < 0)
		goto fail;

	/* Let platform code set up the GPIOs and their users.
	 * Now is the first time anyone could use them.
	 */
	if (pdata && pdata->setup) {
		status = pdata->setup(client,
				gpio->chip.base, gpio->chip.ngpio,
				pdata->context);
		if (status < 0)
			dev_warn(&client->dev, "setup --> %d\n", status);
	}

	dev_info(&client->dev, "probed\n");

	return 0;

fail:
	dev_dbg(&client->dev, "probe error %d for '%s'\n",
			status, client->name);

	if (client->irq)
		pcf857x_irq_domain_cleanup(gpio);

	return status;
}

static int pcf857x_remove(struct i2c_client *client)
{
	struct pcf857x_platform_data	*pdata = dev_get_platdata(&client->dev);
	struct pcf857x			*gpio = i2c_get_clientdata(client);
	int				status = 0;

	if (pdata && pdata->teardown) {
		status = pdata->teardown(client,
				gpio->chip.base, gpio->chip.ngpio,
				pdata->context);
		if (status < 0) {
			dev_err(&client->dev, "%s --> %d\n",
					"teardown", status);
			return status;
		}
	}

	if (client->irq)
		pcf857x_irq_domain_cleanup(gpio);

	status = gpiochip_remove(&gpio->chip);
	if (status)
		dev_err(&client->dev, "%s --> %d\n", "remove", status);
	return status;
}

static struct i2c_driver pcf857x_driver = {
	.driver = {
		.name	= "pcf857x",
		.owner	= THIS_MODULE,
	},
	.probe	= pcf857x_probe,
	.remove	= pcf857x_remove,
	.id_table = pcf857x_id,
};

static int __init pcf857x_init(void)
{
	return i2c_add_driver(&pcf857x_driver);
}
/* register after i2c postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall(pcf857x_init);

static void __exit pcf857x_exit(void)
{
	i2c_del_driver(&pcf857x_driver);
}
module_exit(pcf857x_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Brownell");
