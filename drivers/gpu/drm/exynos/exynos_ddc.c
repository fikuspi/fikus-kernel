/*
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Authors:
 *	Seung-Woo Kim <sw0312.kim@samsung.com>
 *	Inki Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <drm/drmP.h>

#include <fikus/kernel.h>
#include <fikus/i2c.h>
#include <fikus/of.h>

#include "exynos_drm_drv.h"
#include "exynos_hdmi.h"

static int s5p_ddc_probe(struct i2c_client *client,
			const struct i2c_device_id *dev_id)
{
	hdmi_attach_ddc_client(client);

	dev_info(&client->adapter->dev,
		"attached %s into i2c adapter successfully\n",
		client->name);

	return 0;
}

static int s5p_ddc_remove(struct i2c_client *client)
{
	dev_info(&client->adapter->dev,
		"detached %s from i2c adapter successfully\n",
		client->name);

	return 0;
}

static struct of_device_id hdmiddc_match_types[] = {
	{
		.compatible = "samsung,exynos5-hdmiddc",
	}, {
		.compatible = "samsung,exynos4210-hdmiddc",
	}, {
		/* end node */
	}
};

struct i2c_driver ddc_driver = {
	.driver = {
		.name = "exynos-hdmiddc",
		.owner = THIS_MODULE,
		.of_match_table = hdmiddc_match_types,
	},
	.probe		= s5p_ddc_probe,
	.remove		= s5p_ddc_remove,
	.command		= NULL,
};
