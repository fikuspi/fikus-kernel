/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <fikus/gpio.h>
#include <fikus/kernel.h>
#include <fikus/irq.h>
#include <fikus/platform_device.h>
#include <fikus/delay.h>
#include <fikus/io.h>
#include <fikus/smsc911x.h>
#include <fikus/usb/msm_hsusb.h>
#include <fikus/clkdev.h>
#include <fikus/memblock.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/memory.h>
#include <asm/setup.h>

#include <mach/msm_iomap.h>
#include <mach/dma.h>

#include <mach/vreg.h>
#include "devices.h"
#include "gpiomux.h"
#include "proc_comm.h"
#include "common.h"

static void __init msm7x30_fixup(struct tag *tag, char **cmdline,
		struct meminfo *mi)
{
	for (; tag->hdr.size; tag = tag_next(tag))
		if (tag->hdr.tag == ATAG_MEM && tag->u.mem.start == 0x200000) {
			tag->u.mem.start = 0;
			tag->u.mem.size += SZ_2M;
		}
}

static void __init msm7x30_reserve(void)
{
	memblock_remove(0x0, SZ_2M);
}

static int hsusb_phy_init_seq[] = {
	0x30, 0x32,	/* Enable and set Pre-Emphasis Depth to 20% */
	0x02, 0x36,	/* Disable CDR Auto Reset feature */
	-1
};

static struct msm_otg_platform_data msm_otg_pdata = {
	.phy_init_seq		= hsusb_phy_init_seq,
	.mode                   = USB_PERIPHERAL,
	.otg_control		= OTG_PHY_CONTROL,
};

struct msm_gpiomux_config msm_gpiomux_configs[GPIOMUX_NGPIOS] = {
#ifdef CONFIG_SERIAL_MSM_CONSOLE
	[49] = { /* UART2 RFR */
		.suspended = GPIOMUX_DRV_2MA | GPIOMUX_PULL_DOWN |
			     GPIOMUX_FUNC_2 | GPIOMUX_VALID,
	},
	[50] = { /* UART2 CTS */
		.suspended = GPIOMUX_DRV_2MA | GPIOMUX_PULL_DOWN |
			     GPIOMUX_FUNC_2 | GPIOMUX_VALID,
	},
	[51] = { /* UART2 RX */
		.suspended = GPIOMUX_DRV_2MA | GPIOMUX_PULL_DOWN |
			     GPIOMUX_FUNC_2 | GPIOMUX_VALID,
	},
	[52] = { /* UART2 TX */
		.suspended = GPIOMUX_DRV_2MA | GPIOMUX_PULL_DOWN |
			     GPIOMUX_FUNC_2 | GPIOMUX_VALID,
	},
#endif
};

static struct platform_device *devices[] __initdata = {
	&msm_clock_7x30,
	&msm_device_gpio_7x30,
#if defined(CONFIG_SERIAL_MSM) || defined(CONFIG_MSM_SERIAL_DEBUGGER)
        &msm_device_uart2,
#endif
	&msm_device_smd,
	&msm_device_otg,
	&msm_device_hsusb,
	&msm_device_hsusb_host,
};

static void __init msm7x30_init_irq(void)
{
	msm_init_irq();
}

static void __init msm7x30_init(void)
{
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
	msm_device_hsusb.dev.parent = &msm_device_otg.dev;
	msm_device_hsusb_host.dev.parent = &msm_device_otg.dev;

	platform_add_devices(devices, ARRAY_SIZE(devices));
}

static void __init msm7x30_map_io(void)
{
	msm_map_msm7x30_io();
}

static void __init msm7x30_init_late(void)
{
	smd_debugfs_init();
}

MACHINE_START(MSM7X30_SURF, "QCT MSM7X30 SURF")
	.atag_offset = 0x100,
	.fixup = msm7x30_fixup,
	.reserve = msm7x30_reserve,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.init_late = msm7x30_init_late,
	.init_time	= msm7x30_timer_init,
MACHINE_END

MACHINE_START(MSM7X30_FFA, "QCT MSM7X30 FFA")
	.atag_offset = 0x100,
	.fixup = msm7x30_fixup,
	.reserve = msm7x30_reserve,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.init_late = msm7x30_init_late,
	.init_time	= msm7x30_timer_init,
MACHINE_END

MACHINE_START(MSM7X30_FLUID, "QCT MSM7X30 FLUID")
	.atag_offset = 0x100,
	.fixup = msm7x30_fixup,
	.reserve = msm7x30_reserve,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.init_late = msm7x30_init_late,
	.init_time	= msm7x30_timer_init,
MACHINE_END
