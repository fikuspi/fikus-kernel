/*
 * STMicroelectronics pressures driver
 *
 * Copyright 2013 STMicroelectronics Inc.
 *
 * Denis Ciocca <denis.ciocca@st.com>
 *
 * Licensed under the GPL-2.
 */

#include <fikus/module.h>
#include <fikus/kernel.h>
#include <fikus/slab.h>
#include <fikus/stat.h>
#include <fikus/interrupt.h>
#include <fikus/i2c.h>
#include <fikus/delay.h>
#include <fikus/iio/iio.h>
#include <fikus/iio/buffer.h>
#include <fikus/iio/trigger_consumer.h>
#include <fikus/iio/triggered_buffer.h>

#include <fikus/iio/common/st_sensors.h>
#include "st_pressure.h"

int st_press_trig_set_state(struct iio_trigger *trig, bool state)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);

	return st_sensors_set_dataready_irq(indio_dev, state);
}

static int st_press_buffer_preenable(struct iio_dev *indio_dev)
{
	int err;

	err = st_sensors_set_enable(indio_dev, true);
	if (err < 0)
		goto st_press_set_enable_error;

	err = iio_sw_buffer_preenable(indio_dev);

st_press_set_enable_error:
	return err;
}

static int st_press_buffer_postenable(struct iio_dev *indio_dev)
{
	int err;
	struct st_sensor_data *pdata = iio_priv(indio_dev);

	pdata->buffer_data = kmalloc(indio_dev->scan_bytes, GFP_KERNEL);
	if (pdata->buffer_data == NULL) {
		err = -ENOMEM;
		goto allocate_memory_error;
	}

	err = iio_triggered_buffer_postenable(indio_dev);
	if (err < 0)
		goto st_press_buffer_postenable_error;

	return err;

st_press_buffer_postenable_error:
	kfree(pdata->buffer_data);
allocate_memory_error:
	return err;
}

static int st_press_buffer_predisable(struct iio_dev *indio_dev)
{
	int err;
	struct st_sensor_data *pdata = iio_priv(indio_dev);

	err = iio_triggered_buffer_predisable(indio_dev);
	if (err < 0)
		goto st_press_buffer_predisable_error;

	err = st_sensors_set_enable(indio_dev, false);

st_press_buffer_predisable_error:
	kfree(pdata->buffer_data);
	return err;
}

static const struct iio_buffer_setup_ops st_press_buffer_setup_ops = {
	.preenable = &st_press_buffer_preenable,
	.postenable = &st_press_buffer_postenable,
	.predisable = &st_press_buffer_predisable,
};

int st_press_allocate_ring(struct iio_dev *indio_dev)
{
	return iio_triggered_buffer_setup(indio_dev, &iio_pollfunc_store_time,
		&st_sensors_trigger_handler, &st_press_buffer_setup_ops);
}

void st_press_deallocate_ring(struct iio_dev *indio_dev)
{
	iio_triggered_buffer_cleanup(indio_dev);
}

MODULE_AUTHOR("Denis Ciocca <denis.ciocca@st.com>");
MODULE_DESCRIPTION("STMicroelectronics pressures buffer");
MODULE_LICENSE("GPL v2");
