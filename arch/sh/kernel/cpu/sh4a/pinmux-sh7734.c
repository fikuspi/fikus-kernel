/*
 * SH7734 processor support - PFC hardware block
 *
 * Copyright (C) 2012  Renesas Solutions Corp.
 * Copyright (C) 2012  Nobuhiro Iwamatsu <nobuhiro.iwamatsu.yj@renesas.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <fikus/bug.h>
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/ioport.h>
#include <cpu/pfc.h>

static struct resource sh7734_pfc_resources[] = {
	[0] = { /* PFC */
		.start	= 0xFFFC0000,
		.end	= 0xFFFC011C,
		.flags	= IORESOURCE_MEM,
	},
	[1] = { /* GPIO */
		.start	= 0xFFC40000,
		.end	= 0xFFC4502B,
		.flags	= IORESOURCE_MEM,
	}
};

static int __init plat_pinmux_setup(void)
{
	return sh_pfc_register("pfc-sh7734", sh7734_pfc_resources,
			       ARRAY_SIZE(sh7734_pfc_resources));
}
arch_initcall(plat_pinmux_setup);
