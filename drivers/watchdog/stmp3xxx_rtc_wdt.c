/*
 * Watchdog driver for the RTC based watchdog in STMP3xxx and i.MX23/28
 *
 * Author: Wolfram Sang <w.sang@pengutronix.de>
 *
 * Copyright (C) 2011-12 Wolfram Sang, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/miscdevice.h>
#include <fikus/watchdog.h>
#include <fikus/platform_device.h>
#include <fikus/stmp3xxx_rtc_wdt.h>

#define WDOG_TICK_RATE 1000 /* 1 kHz clock */
#define STMP3XXX_DEFAULT_TIMEOUT 19
#define STMP3XXX_MAX_TIMEOUT (UINT_MAX / WDOG_TICK_RATE)

static int heartbeat = STMP3XXX_DEFAULT_TIMEOUT;
module_param(heartbeat, uint, 0);
MODULE_PARM_DESC(heartbeat, "Watchdog heartbeat period in seconds from 1 to "
		 __MODULE_STRING(STMP3XXX_MAX_TIMEOUT) ", default "
		 __MODULE_STRING(STMP3XXX_DEFAULT_TIMEOUT));

static int wdt_start(struct watchdog_device *wdd)
{
	struct device *dev = watchdog_get_drvdata(wdd);
	struct stmp3xxx_wdt_pdata *pdata = dev->platform_data;

	pdata->wdt_set_timeout(dev->parent, wdd->timeout * WDOG_TICK_RATE);
	return 0;
}

static int wdt_stop(struct watchdog_device *wdd)
{
	struct device *dev = watchdog_get_drvdata(wdd);
	struct stmp3xxx_wdt_pdata *pdata = dev->platform_data;

	pdata->wdt_set_timeout(dev->parent, 0);
	return 0;
}

static int wdt_set_timeout(struct watchdog_device *wdd, unsigned new_timeout)
{
	wdd->timeout = new_timeout;
	return wdt_start(wdd);
}

static const struct watchdog_info stmp3xxx_wdt_ident = {
	.options = WDIOF_MAGICCLOSE | WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
	.identity = "STMP3XXX RTC Watchdog",
};

static const struct watchdog_ops stmp3xxx_wdt_ops = {
	.owner = THIS_MODULE,
	.start = wdt_start,
	.stop = wdt_stop,
	.set_timeout = wdt_set_timeout,
};

static struct watchdog_device stmp3xxx_wdd = {
	.info = &stmp3xxx_wdt_ident,
	.ops = &stmp3xxx_wdt_ops,
	.min_timeout = 1,
	.max_timeout = STMP3XXX_MAX_TIMEOUT,
	.status = WATCHDOG_NOWAYOUT_INIT_STATUS,
};

static int stmp3xxx_wdt_probe(struct platform_device *pdev)
{
	int ret;

	watchdog_set_drvdata(&stmp3xxx_wdd, &pdev->dev);

	stmp3xxx_wdd.timeout = clamp_t(unsigned, heartbeat, 1, STMP3XXX_MAX_TIMEOUT);

	ret = watchdog_register_device(&stmp3xxx_wdd);
	if (ret < 0) {
		dev_err(&pdev->dev, "cannot register watchdog device\n");
		return ret;
	}

	dev_info(&pdev->dev, "initialized watchdog with heartbeat %ds\n",
			stmp3xxx_wdd.timeout);
	return 0;
}

static int stmp3xxx_wdt_remove(struct platform_device *pdev)
{
	watchdog_unregister_device(&stmp3xxx_wdd);
	return 0;
}

static struct platform_driver stmp3xxx_wdt_driver = {
	.driver = {
		.name = "stmp3xxx_rtc_wdt",
	},
	.probe = stmp3xxx_wdt_probe,
	.remove = stmp3xxx_wdt_remove,
};
module_platform_driver(stmp3xxx_wdt_driver);

MODULE_DESCRIPTION("STMP3XXX RTC Watchdog Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Wolfram Sang <w.sang@pengutronix.de>");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
