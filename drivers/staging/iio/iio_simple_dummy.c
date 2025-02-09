/**
 * Copyright (c) 2011 Jonathan Cameron
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * A reference industrial I/O driver to illustrate the functionality available.
 *
 * There are numerous real drivers to illustrate the finer points.
 * The purpose of this driver is to provide a driver with far more comments
 * and explanatory notes than any 'real' driver would have.
 * Anyone starting out writing an IIO driver should first make sure they
 * understand all of this driver except those bits specifically marked
 * as being present to allow us to 'fake' the presence of hardware.
 */
#include <fikus/kernel.h>
#include <fikus/slab.h>
#include <fikus/module.h>
#include <fikus/moduleparam.h>

#include <fikus/iio/iio.h>
#include <fikus/iio/sysfs.h>
#include <fikus/iio/events.h>
#include <fikus/iio/buffer.h>
#include "iio_simple_dummy.h"

/*
 * A few elements needed to fake a bus for this driver
 * Note instances parameter controls how many of these
 * dummy devices are registered.
 */
static unsigned instances = 1;
module_param(instances, int, 0);

/* Pointer array used to fake bus elements */
static struct iio_dev **iio_dummy_devs;

/* Fake a name for the part number, usually obtained from the id table */
static const char *iio_dummy_part_number = "iio_dummy_part_no";

/**
 * struct iio_dummy_accel_calibscale - realworld to register mapping
 * @val: first value in read_raw - here integer part.
 * @val2: second value in read_raw etc - here micro part.
 * @regval: register value - magic device specific numbers.
 */
struct iio_dummy_accel_calibscale {
	int val;
	int val2;
	int regval; /* what would be written to hardware */
};

static const struct iio_dummy_accel_calibscale dummy_scales[] = {
	{ 0, 100, 0x8 }, /* 0.000100 */
	{ 0, 133, 0x7 }, /* 0.000133 */
	{ 733, 13, 0x9 }, /* 733.000013 */
};

/*
 * iio_dummy_channels - Description of available channels
 *
 * This array of structures tells the IIO core about what the device
 * actually provides for a given channel.
 */
static const struct iio_chan_spec iio_dummy_channels[] = {
	/* indexed ADC channel in_voltage0_raw etc */
	{
		.type = IIO_VOLTAGE,
		/* Channel has a numeric index of 0 */
		.indexed = 1,
		.channel = 0,
		/* What other information is available? */
		.info_mask_separate =
		/*
		 * in_voltage0_raw
		 * Raw (unscaled no bias removal etc) measurement
		 * from the device.
		 */
		BIT(IIO_CHAN_INFO_RAW) |
		/*
		 * in_voltage0_offset
		 * Offset for userspace to apply prior to scale
		 * when converting to standard units (microvolts)
		 */
		BIT(IIO_CHAN_INFO_OFFSET) |
		/*
		 * in_voltage0_scale
		 * Multipler for userspace to apply post offset
		 * when converting to standard units (microvolts)
		 */
		BIT(IIO_CHAN_INFO_SCALE),
		/* The ordering of elements in the buffer via an enum */
		.scan_index = voltage0,
		.scan_type = { /* Description of storage in buffer */
			.sign = 'u', /* unsigned */
			.realbits = 13, /* 13 bits */
			.storagebits = 16, /* 16 bits used for storage */
			.shift = 0, /* zero shift */
		},
#ifdef CONFIG_IIO_SIMPLE_DUMMY_EVENTS
		/*
		 * simple event - triggered when value rises above
		 * a threshold
		 */
		.event_mask = IIO_EV_BIT(IIO_EV_TYPE_THRESH,
					 IIO_EV_DIR_RISING),
#endif /* CONFIG_IIO_SIMPLE_DUMMY_EVENTS */
	},
	/* Differential ADC channel in_voltage1-voltage2_raw etc*/
	{
		.type = IIO_VOLTAGE,
		.differential = 1,
		/*
		 * Indexing for differential channels uses channel
		 * for the positive part, channel2 for the negative.
		 */
		.indexed = 1,
		.channel = 1,
		.channel2 = 2,
		/*
		 * in_voltage1-voltage2_raw
		 * Raw (unscaled no bias removal etc) measurement
		 * from the device.
		 */
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		/*
		 * in_voltage-voltage_scale
		 * Shared version of scale - shared by differential
		 * input channels of type IIO_VOLTAGE.
		 */
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = diffvoltage1m2,
		.scan_type = { /* Description of storage in buffer */
			.sign = 's', /* signed */
			.realbits = 12, /* 12 bits */
			.storagebits = 16, /* 16 bits used for storage */
			.shift = 0, /* zero shift */
		},
	},
	/* Differential ADC channel in_voltage3-voltage4_raw etc*/
	{
		.type = IIO_VOLTAGE,
		.differential = 1,
		.indexed = 1,
		.channel = 3,
		.channel2 = 4,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = diffvoltage3m4,
		.scan_type = {
			.sign = 's',
			.realbits = 11,
			.storagebits = 16,
			.shift = 0,
		},
	},
	/*
	 * 'modified' (i.e. axis specified) acceleration channel
	 * in_accel_z_raw
	 */
	{
		.type = IIO_ACCEL,
		.modified = 1,
		/* Channel 2 is use for modifiers */
		.channel2 = IIO_MOD_X,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
		/*
		 * Internal bias and gain correction values. Applied
		 * by the hardware or driver prior to userspace
		 * seeing the readings. Typically part of hardware
		 * calibration.
		 */
		BIT(IIO_CHAN_INFO_CALIBSCALE) |
		BIT(IIO_CHAN_INFO_CALIBBIAS),
		.scan_index = accelx,
		.scan_type = { /* Description of storage in buffer */
			.sign = 's', /* signed */
			.realbits = 16, /* 16 bits */
			.storagebits = 16, /* 16 bits used for storage */
			.shift = 0, /* zero shift */
		},
	},
	/*
	 * Convenience macro for timestamps. 4 is the index in
	 * the buffer.
	 */
	IIO_CHAN_SOFT_TIMESTAMP(4),
	/* DAC channel out_voltage0_raw */
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.output = 1,
		.indexed = 1,
		.channel = 0,
	},
};

/**
 * iio_dummy_read_raw() - data read function.
 * @indio_dev:	the struct iio_dev associated with this device instance
 * @chan:	the channel whose data is to be read
 * @val:	first element of returned value (typically INT)
 * @val2:	second element of returned value (typically MICRO)
 * @mask:	what we actually want to read as per the info_mask_*
 *		in iio_chan_spec.
 */
static int iio_dummy_read_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan,
			      int *val,
			      int *val2,
			      long mask)
{
	struct iio_dummy_state *st = iio_priv(indio_dev);
	int ret = -EINVAL;

	mutex_lock(&st->lock);
	switch (mask) {
	case IIO_CHAN_INFO_RAW: /* magic value - channel value read */
		switch (chan->type) {
		case IIO_VOLTAGE:
			if (chan->output) {
				/* Set integer part to cached value */
				*val = st->dac_val;
				ret = IIO_VAL_INT;
			} else if (chan->differential) {
				if (chan->channel == 1)
					*val = st->differential_adc_val[0];
				else
					*val = st->differential_adc_val[1];
				ret = IIO_VAL_INT;
			} else {
				*val = st->single_ended_adc_val;
				ret = IIO_VAL_INT;
			}
			break;
		case IIO_ACCEL:
			*val = st->accel_val;
			ret = IIO_VAL_INT;
			break;
		default:
			break;
		}
		break;
	case IIO_CHAN_INFO_OFFSET:
		/* only single ended adc -> 7 */
		*val = 7;
		ret = IIO_VAL_INT;
		break;
	case IIO_CHAN_INFO_SCALE:
		switch (chan->differential) {
		case 0:
			/* only single ended adc -> 0.001333 */
			*val = 0;
			*val2 = 1333;
			ret = IIO_VAL_INT_PLUS_MICRO;
			break;
		case 1:
			/* all differential adc channels -> 0.000001344 */
			*val = 0;
			*val2 = 1344;
			ret = IIO_VAL_INT_PLUS_NANO;
		}
		break;
	case IIO_CHAN_INFO_CALIBBIAS:
		/* only the acceleration axis - read from cache */
		*val = st->accel_calibbias;
		ret = IIO_VAL_INT;
		break;
	case IIO_CHAN_INFO_CALIBSCALE:
		*val = st->accel_calibscale->val;
		*val2 = st->accel_calibscale->val2;
		ret = IIO_VAL_INT_PLUS_MICRO;
		break;
	default:
		break;
	}
	mutex_unlock(&st->lock);
	return ret;
}

/**
 * iio_dummy_write_raw() - data write function.
 * @indio_dev:	the struct iio_dev associated with this device instance
 * @chan:	the channel whose data is to be written
 * @val:	first element of value to set (typically INT)
 * @val2:	second element of value to set (typically MICRO)
 * @mask:	what we actually want to write as per the info_mask_*
 *		in iio_chan_spec.
 *
 * Note that all raw writes are assumed IIO_VAL_INT and info mask elements
 * are assumed to be IIO_INT_PLUS_MICRO unless the callback write_raw_get_fmt
 * in struct iio_info is provided by the driver.
 */
static int iio_dummy_write_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan,
			       int val,
			       int val2,
			       long mask)
{
	int i;
	int ret = 0;
	struct iio_dummy_state *st = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (chan->output == 0)
			return -EINVAL;

		/* Locking not required as writing single value */
		mutex_lock(&st->lock);
		st->dac_val = val;
		mutex_unlock(&st->lock);
		return 0;
	case IIO_CHAN_INFO_CALIBSCALE:
		mutex_lock(&st->lock);
		/* Compare against table - hard matching here */
		for (i = 0; i < ARRAY_SIZE(dummy_scales); i++)
			if (val == dummy_scales[i].val &&
			    val2 == dummy_scales[i].val2)
				break;
		if (i == ARRAY_SIZE(dummy_scales))
			ret = -EINVAL;
		else
			st->accel_calibscale = &dummy_scales[i];
		mutex_unlock(&st->lock);
		return ret;
	case IIO_CHAN_INFO_CALIBBIAS:
		mutex_lock(&st->lock);
		st->accel_calibbias = val;
		mutex_unlock(&st->lock);
		return 0;

	default:
		return -EINVAL;
	}
}

/*
 * Device type specific information.
 */
static const struct iio_info iio_dummy_info = {
	.driver_module = THIS_MODULE,
	.read_raw = &iio_dummy_read_raw,
	.write_raw = &iio_dummy_write_raw,
#ifdef CONFIG_IIO_SIMPLE_DUMMY_EVENTS
	.read_event_config = &iio_simple_dummy_read_event_config,
	.write_event_config = &iio_simple_dummy_write_event_config,
	.read_event_value = &iio_simple_dummy_read_event_value,
	.write_event_value = &iio_simple_dummy_write_event_value,
#endif /* CONFIG_IIO_SIMPLE_DUMMY_EVENTS */
};

/**
 * iio_dummy_init_device() - device instance specific init
 * @indio_dev: the iio device structure
 *
 * Most drivers have one of these to set up default values,
 * reset the device to known state etc.
 */
static int iio_dummy_init_device(struct iio_dev *indio_dev)
{
	struct iio_dummy_state *st = iio_priv(indio_dev);

	st->dac_val = 0;
	st->single_ended_adc_val = 73;
	st->differential_adc_val[0] = 33;
	st->differential_adc_val[1] = -34;
	st->accel_val = 34;
	st->accel_calibbias = -7;
	st->accel_calibscale = &dummy_scales[0];

	return 0;
}

/**
 * iio_dummy_probe() - device instance probe
 * @index: an id number for this instance.
 *
 * Arguments are bus type specific.
 * I2C: iio_dummy_probe(struct i2c_client *client,
 *                      const struct i2c_device_id *id)
 * SPI: iio_dummy_probe(struct spi_device *spi)
 */
static int iio_dummy_probe(int index)
{
	int ret;
	struct iio_dev *indio_dev;
	struct iio_dummy_state *st;

	/*
	 * Allocate an IIO device.
	 *
	 * This structure contains all generic state
	 * information about the device instance.
	 * It also has a region (accessed by iio_priv()
	 * for chip specific state information.
	 */
	indio_dev = iio_device_alloc(sizeof(*st));
	if (indio_dev == NULL) {
		ret = -ENOMEM;
		goto error_ret;
	}

	st = iio_priv(indio_dev);
	mutex_init(&st->lock);

	iio_dummy_init_device(indio_dev);
	/*
	 * With hardware: Set the parent device.
	 * indio_dev->dev.parent = &spi->dev;
	 * indio_dev->dev.parent = &client->dev;
	 */

	 /*
	 * Make the iio_dev struct available to remove function.
	 * Bus equivalents
	 * i2c_set_clientdata(client, indio_dev);
	 * spi_set_drvdata(spi, indio_dev);
	 */
	iio_dummy_devs[index] = indio_dev;


	/*
	 * Set the device name.
	 *
	 * This is typically a part number and obtained from the module
	 * id table.
	 * e.g. for i2c and spi:
	 *    indio_dev->name = id->name;
	 *    indio_dev->name = spi_get_device_id(spi)->name;
	 */
	indio_dev->name = iio_dummy_part_number;

	/* Provide description of available channels */
	indio_dev->channels = iio_dummy_channels;
	indio_dev->num_channels = ARRAY_SIZE(iio_dummy_channels);

	/*
	 * Provide device type specific interface functions and
	 * constant data.
	 */
	indio_dev->info = &iio_dummy_info;

	/* Specify that device provides sysfs type interfaces */
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = iio_simple_dummy_events_register(indio_dev);
	if (ret < 0)
		goto error_free_device;

	/*
	 * Configure buffered capture support and register the channels with the
	 * buffer, but avoid the output channel being registered by reducing the
	 * number of channels by 1.
	 */
	ret = iio_simple_dummy_configure_buffer(indio_dev, iio_dummy_channels, 5);
	if (ret < 0)
		goto error_unregister_events;

	ret = iio_device_register(indio_dev);
	if (ret < 0)
		goto error_unconfigure_buffer;

	return 0;
error_unconfigure_buffer:
	iio_simple_dummy_unconfigure_buffer(indio_dev);
error_unregister_events:
	iio_simple_dummy_events_unregister(indio_dev);
error_free_device:
	iio_device_free(indio_dev);
error_ret:
	return ret;
}

/**
 * iio_dummy_remove() - device instance removal function
 * @index: device index.
 *
 * Parameters follow those of iio_dummy_probe for buses.
 */
static int iio_dummy_remove(int index)
{
	int ret;
	/*
	 * Get a pointer to the device instance iio_dev structure
	 * from the bus subsystem. E.g.
	 * struct iio_dev *indio_dev = i2c_get_clientdata(client);
	 * struct iio_dev *indio_dev = spi_get_drvdata(spi);
	 */
	struct iio_dev *indio_dev = iio_dummy_devs[index];


	/* Unregister the device */
	iio_device_unregister(indio_dev);

	/* Device specific code to power down etc */

	/* Buffered capture related cleanup */
	iio_simple_dummy_unconfigure_buffer(indio_dev);

	ret = iio_simple_dummy_events_unregister(indio_dev);
	if (ret)
		goto error_ret;

	/* Free all structures */
	iio_device_free(indio_dev);

error_ret:
	return ret;
}

/**
 * iio_dummy_init() -  device driver registration
 *
 * Varies depending on bus type of the device. As there is no device
 * here, call probe directly. For information on device registration
 * i2c:
 * Documentation/i2c/writing-clients
 * spi:
 * Documentation/spi/spi-summary
 */
static __init int iio_dummy_init(void)
{
	int i, ret;
	if (instances > 10) {
		instances = 1;
		return -EINVAL;
	}

	/* Fake a bus */
	iio_dummy_devs = kcalloc(instances, sizeof(*iio_dummy_devs),
				 GFP_KERNEL);
	/* Here we have no actual device so call probe */
	for (i = 0; i < instances; i++) {
		ret = iio_dummy_probe(i);
		if (ret < 0)
			return ret;
	}
	return 0;
}
module_init(iio_dummy_init);

/**
 * iio_dummy_exit() - device driver removal
 *
 * Varies depending on bus type of the device.
 * As there is no device here, call remove directly.
 */
static __exit void iio_dummy_exit(void)
{
	int i;
	for (i = 0; i < instances; i++)
		iio_dummy_remove(i);
	kfree(iio_dummy_devs);
}
module_exit(iio_dummy_exit);

MODULE_AUTHOR("Jonathan Cameron <jic23@kernel.org>");
MODULE_DESCRIPTION("IIO dummy driver");
MODULE_LICENSE("GPL v2");
