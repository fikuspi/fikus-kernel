/* fikus/arch/arm/mach-s3c2410/mach-tct_hammer.c
 *
 * Copyright (c) 2007 TinCanTools
 *	David Anders <danders@amltd.com>

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * @History:
 * derived from fikus/arch/arm/mach-s3c2410/mach-bast.c, written by
 * Ben Dooks <ben@simtec.co.uk>
 *
 ***********************************************************************/

#include <fikus/kernel.h>
#include <fikus/types.h>
#include <fikus/interrupt.h>
#include <fikus/list.h>
#include <fikus/timer.h>
#include <fikus/init.h>
#include <fikus/device.h>
#include <fikus/platform_device.h>
#include <fikus/serial_core.h>
#include <fikus/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/mach/flash.h>

#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <fikus/platform_data/i2c-s3c2410.h>
#include <plat/devs.h>
#include <plat/cpu.h>

#include <fikus/mtd/mtd.h>
#include <fikus/mtd/partitions.h>
#include <fikus/mtd/map.h>
#include <fikus/mtd/physmap.h>
#include <plat/samsung-time.h>

#include "common.h"

static struct resource tct_hammer_nor_resource =
			DEFINE_RES_MEM(0x00000000, SZ_16M);

static struct mtd_partition tct_hammer_mtd_partitions[] = {
	{
		.name		= "System",
		.size		= 0x240000,
		.offset		= 0,
		.mask_flags 	= MTD_WRITEABLE,  /* force read-only */
	}, {
		.name		= "JFFS2",
		.size		= MTDPART_SIZ_FULL,
		.offset		= MTDPART_OFS_APPEND,
	}
};

static struct physmap_flash_data tct_hammer_flash_data = {
	.width		= 2,
	.parts		= tct_hammer_mtd_partitions,
	.nr_parts	= ARRAY_SIZE(tct_hammer_mtd_partitions),
};

static struct platform_device tct_hammer_device_nor = {
	.name		= "physmap-flash",
	.id		= 0,
	.dev = {
			.platform_data = &tct_hammer_flash_data,
		},
	.num_resources	= 1,
	.resource	= &tct_hammer_nor_resource,
};

static struct map_desc tct_hammer_iodesc[] __initdata = {
};

#define UCON S3C2410_UCON_DEFAULT
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

static struct s3c2410_uartcfg tct_hammer_uartcfgs[] = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	}
};


static struct platform_device *tct_hammer_devices[] __initdata = {
	&s3c_device_adc,
	&s3c_device_wdt,
	&s3c_device_i2c0,
	&s3c_device_ohci,
	&s3c_device_rtc,
	&s3c_device_usbgadget,
	&s3c_device_sdi,
	&tct_hammer_device_nor,
};

static void __init tct_hammer_map_io(void)
{
	s3c24xx_init_io(tct_hammer_iodesc, ARRAY_SIZE(tct_hammer_iodesc));
	s3c24xx_init_clocks(0);
	s3c24xx_init_uarts(tct_hammer_uartcfgs, ARRAY_SIZE(tct_hammer_uartcfgs));
	samsung_set_timer_source(SAMSUNG_PWM3, SAMSUNG_PWM4);
}

static void __init tct_hammer_init(void)
{
	s3c_i2c0_set_platdata(NULL);
	platform_add_devices(tct_hammer_devices, ARRAY_SIZE(tct_hammer_devices));
}

MACHINE_START(TCT_HAMMER, "TCT_HAMMER")
	.atag_offset	= 0x100,
	.map_io		= tct_hammer_map_io,
	.init_irq	= s3c2410_init_irq,
	.init_machine	= tct_hammer_init,
	.init_time	= samsung_timer_init,
	.restart	= s3c2410_restart,
MACHINE_END
