/* fikus/arch/arm/mach-msm/board-trout.c
 *
 * Copyright (C) 2009 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/platform_device.h>
#include <fikus/clkdev.h>

#include <asm/system_info.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>

#include <mach/hardware.h>
#include <mach/msm_iomap.h>

#include "devices.h"
#include "board-trout.h"
#include "common.h"

extern int trout_init_mmc(unsigned int);

static struct platform_device *devices[] __initdata = {
	&msm_clock_7x01a,
	&msm_device_gpio_7201,
	&msm_device_uart3,
	&msm_device_smd,
	&msm_device_nand,
	&msm_device_hsusb,
	&msm_device_i2c,
};

static void __init trout_init_early(void)
{
	arch_ioremap_caller = __msm_ioremap_caller;
}

static void __init trout_init_irq(void)
{
	msm_init_irq();
}

static void __init trout_fixup(struct tag *tags, char **cmdline,
			       struct meminfo *mi)
{
	mi->nr_banks = 1;
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].size = (101*1024*1024);
}

static void __init trout_init(void)
{
	int rc;

	platform_add_devices(devices, ARRAY_SIZE(devices));

#ifdef CONFIG_MMC
        rc = trout_init_mmc(system_rev);
        if (rc)
                printk(KERN_CRIT "%s: MMC init failure (%d)\n", __func__, rc);
#endif

}

static struct map_desc trout_io_desc[] __initdata = {
	{
		.virtual = TROUT_CPLD_BASE,
		.pfn     = __phys_to_pfn(TROUT_CPLD_START),
		.length  = TROUT_CPLD_SIZE,
		.type    = MT_DEVICE_NONSHARED
	}
};

static void __init trout_map_io(void)
{
	msm_map_common_io();
	iotable_init(trout_io_desc, ARRAY_SIZE(trout_io_desc));

#ifdef CONFIG_MSM_DEBUG_UART3
	/* route UART3 to the "H2W" extended usb connector */
	writeb(0x80, TROUT_CPLD_BASE + 0x00);
#endif
}

static void __init trout_init_late(void)
{
	smd_debugfs_init();
}

MACHINE_START(TROUT, "HTC Dream")
	.atag_offset	= 0x100,
	.fixup		= trout_fixup,
	.map_io		= trout_map_io,
	.init_early	= trout_init_early,
	.init_irq	= trout_init_irq,
	.init_machine	= trout_init,
	.init_late	= trout_init_late,
	.init_time	= msm7x01_timer_init,
MACHINE_END
