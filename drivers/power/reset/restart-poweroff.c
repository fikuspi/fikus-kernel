/*
 * Power off by restarting and let u-boot keep hold of the machine
 * until the user presses a button for example.
 *
 * Andrew Lunn <andrew@lunn.ch>
 *
 * Copyright (C) 2012 Andrew Lunn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/platform_device.h>
#include <fikus/of_platform.h>
#include <fikus/module.h>
#include <fikus/reboot.h>
#include <asm/system_misc.h>

static void restart_poweroff_do_poweroff(void)
{
	arm_pm_restart(REBOOT_HARD, NULL);
}

static int restart_poweroff_probe(struct platform_device *pdev)
{
	/* If a pm_power_off function has already been added, leave it alone */
	if (pm_power_off != NULL) {
		dev_err(&pdev->dev,
			"pm_power_off function already registered");
		return -EBUSY;
	}

	pm_power_off = &restart_poweroff_do_poweroff;
	return 0;
}

static int restart_poweroff_remove(struct platform_device *pdev)
{
	if (pm_power_off == &restart_poweroff_do_poweroff)
		pm_power_off = NULL;

	return 0;
}

static const struct of_device_id of_restart_poweroff_match[] = {
	{ .compatible = "restart-poweroff", },
	{},
};

static struct platform_driver restart_poweroff_driver = {
	.probe = restart_poweroff_probe,
	.remove = restart_poweroff_remove,
	.driver = {
		.name = "poweroff-restart",
		.owner = THIS_MODULE,
		.of_match_table = of_restart_poweroff_match,
	},
};
module_platform_driver(restart_poweroff_driver);

MODULE_AUTHOR("Andrew Lunn <andrew@lunn.ch");
MODULE_DESCRIPTION("restart poweroff driver");
MODULE_LICENSE("GPLv2");
MODULE_ALIAS("platform:poweroff-restart");
