/* fikus/arch/arm/mach-s3c2410/bast-ide.c
 *
 * Copyright 2007 Simtec Electronics
 *	http://www.simtec.co.uk/products/EB2410ITX/
 *	http://armfikus.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <fikus/kernel.h>
#include <fikus/types.h>
#include <fikus/init.h>
#include <fikus/interrupt.h>

#include <fikus/platform_device.h>
#include <fikus/ata_platform.h>

#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/map.h>

#include "bast.h"

/* IDE ports */

static struct pata_platform_info bast_ide_platdata = {
	.ioport_shift	= 5,
};

static struct resource bast_ide0_resource[] = {
	[0] = DEFINE_RES_MEM(BAST_IDE_CS + BAST_PA_IDEPRI, 8 * 0x20),
	[1] = DEFINE_RES_MEM(BAST_IDE_CS + BAST_PA_IDEPRIAUX + (6 * 0x20), 0x20),
	[2] = DEFINE_RES_IRQ(BAST_IRQ_IDE0),
};

static struct platform_device bast_device_ide0 = {
	.name		= "pata_platform",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(bast_ide0_resource),
	.resource	= bast_ide0_resource,
	.dev		= {
		.platform_data = &bast_ide_platdata,
		.coherent_dma_mask = ~0,
	}

};

static struct resource bast_ide1_resource[] = {
	[0] = DEFINE_RES_MEM(BAST_IDE_CS + BAST_PA_IDESEC, 8 * 0x20),
	[1] = DEFINE_RES_MEM(BAST_IDE_CS + BAST_PA_IDESECAUX + (6 * 0x20), 0x20),
	[2] = DEFINE_RES_IRQ(BAST_IRQ_IDE1),
};

static struct platform_device bast_device_ide1 = {
	.name		= "pata_platform",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(bast_ide1_resource),
	.resource	= bast_ide1_resource,
	.dev		= {
		.platform_data = &bast_ide_platdata,
		.coherent_dma_mask = ~0,
	}
};

static struct platform_device *bast_ide_devices[] __initdata = {
	&bast_device_ide0,
	&bast_device_ide1,
};

static __init int bast_ide_init(void)
{
	if (machine_is_bast() || machine_is_vr1000())
		return platform_add_devices(bast_ide_devices,
					    ARRAY_SIZE(bast_ide_devices));

	return 0;
}

fs_initcall(bast_ide_init);
