/*
 * Intel Low Power Subsystem clocks.
 *
 * Copyright (C) 2013, Intel Corporation
 * Authors: Mika Westerberg <mika.westerberg@fikus.intel.com>
 *	    Heikki Krogerus <heikki.krogerus@fikus.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/clk.h>
#include <fikus/clkdev.h>
#include <fikus/clk-provider.h>
#include <fikus/err.h>
#include <fikus/module.h>
#include <fikus/platform_data/clk-lpss.h>
#include <fikus/platform_device.h>

static int lpt_clk_probe(struct platform_device *pdev)
{
	struct lpss_clk_data *drvdata;
	struct clk *clk;

	drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	/* LPSS free running clock */
	drvdata->name = "lpss_clk";
	clk = clk_register_fixed_rate(&pdev->dev, drvdata->name, NULL,
				      CLK_IS_ROOT, 100000000);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	drvdata->clk = clk;
	platform_set_drvdata(pdev, drvdata);
	return 0;
}

static struct platform_driver lpt_clk_driver = {
	.driver = {
		.name = "clk-lpt",
		.owner = THIS_MODULE,
	},
	.probe = lpt_clk_probe,
};

int __init lpt_clk_init(void)
{
	return platform_driver_register(&lpt_clk_driver);
}
