/* fikus/arch/arm/mach-s3c2410/nor-simtec.c
 *
 * Copyright (c) 2008 Simtec Electronics
 *	http://armfikus.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Simtec NOR mapping
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <fikus/module.h>
#include <fikus/types.h>
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/platform_device.h>

#include <fikus/mtd/mtd.h>
#include <fikus/mtd/map.h>
#include <fikus/mtd/physmap.h>
#include <fikus/mtd/partitions.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/map.h>

#include "bast.h"
#include "simtec.h"

static void simtec_nor_vpp(struct platform_device *pdev, int vpp)
{
	unsigned int val;

	val = __raw_readb(BAST_VA_CTRL3);

	printk(KERN_DEBUG "%s(%d)\n", __func__, vpp);

	if (vpp)
		val |= BAST_CPLD_CTRL3_ROMWEN;
	else
		val &= ~BAST_CPLD_CTRL3_ROMWEN;

	__raw_writeb(val, BAST_VA_CTRL3);
}

static struct physmap_flash_data simtec_nor_pdata = {
	.width		= 2,
	.set_vpp	= simtec_nor_vpp,
	.nr_parts	= 0,
};

static struct resource simtec_nor_resource[] = {
	[0] = DEFINE_RES_MEM(S3C2410_CS1 + 0x4000000, SZ_8M),
};

static struct platform_device simtec_device_nor = {
	.name		= "physmap-flash",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(simtec_nor_resource),
	.resource	= simtec_nor_resource,
	.dev		= {
		.platform_data = &simtec_nor_pdata,
	},
};

void __init nor_simtec_init(void)
{
	int ret;

	ret = platform_device_register(&simtec_device_nor);
	if (ret < 0)
		printk(KERN_ERR "failed to register physmap-flash device\n");
	else
		simtec_nor_vpp(NULL, 1);
}
