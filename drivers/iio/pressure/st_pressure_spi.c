/*
 * STMicroelectronics pressures driver
 *
 * Copyright 2013 STMicroelectronics Inc.
 *
 * Denis Ciocca <denis.ciocca@st.com>
 *
 * Licensed under the GPL-2.
 */

#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/slab.h>
#include <fikus/spi/spi.h>
#include <fikus/iio/iio.h>

#include <fikus/iio/common/st_sensors.h>
#include <fikus/iio/common/st_sensors_spi.h>
#include "st_pressure.h"

static int st_press_spi_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct st_sensor_data *pdata;
	int err;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*pdata));
	if (indio_dev == NULL)
		return -ENOMEM;

	pdata = iio_priv(indio_dev);
	pdata->dev = &spi->dev;

	st_sensors_spi_configure(indio_dev, spi, pdata);

	err = st_press_common_probe(indio_dev, spi->dev.platform_data);
	if (err < 0)
		return err;

	return 0;
}

static int st_press_spi_remove(struct spi_device *spi)
{
	st_press_common_remove(spi_get_drvdata(spi));

	return 0;
}

static const struct spi_device_id st_press_id_table[] = {
	{ LPS331AP_PRESS_DEV_NAME },
	{},
};
MODULE_DEVICE_TABLE(spi, st_press_id_table);

static struct spi_driver st_press_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "st-press-spi",
	},
	.probe = st_press_spi_probe,
	.remove = st_press_spi_remove,
	.id_table = st_press_id_table,
};
module_spi_driver(st_press_driver);

MODULE_AUTHOR("Denis Ciocca <denis.ciocca@st.com>");
MODULE_DESCRIPTION("STMicroelectronics pressures spi driver");
MODULE_LICENSE("GPL v2");
