/*
 * fikus/arch/arm/mach-w90x900/mach-nuc950evb.c
 *
 * Based on mach-s3c2410/mach-smdk2410.c by Jonas Dietsche
 *
 * Copyright (C) 2008 Nuvoton technology corporation.
 *
 * Wan ZongShun <mcuos.com@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation;version 2 of the License.
 *   history:
 *     Wang Qiang (rurality.fikus@gmail.com) add LCD support
 *
 */

#include <fikus/platform_device.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <mach/map.h>
#include <fikus/platform_data/video-nuc900fb.h>

#include "nuc950.h"

static void __init nuc950evb_map_io(void)
{
	nuc950_map_io();
	nuc950_init_clocks();
}

static void __init nuc950evb_init(void)
{
	nuc950_board_init();
}

MACHINE_START(W90P950EVB, "W90P950EVB")
	/* Maintainer: Wan ZongShun */
	.map_io		= nuc950evb_map_io,
	.init_irq	= nuc900_init_irq,
	.init_machine	= nuc950evb_init,
	.init_time	= nuc900_timer_init,
	.restart	= nuc9xx_restart,
MACHINE_END
