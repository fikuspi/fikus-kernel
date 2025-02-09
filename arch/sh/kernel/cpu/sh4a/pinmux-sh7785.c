/*
 * SH7785 Pinmux
 *
 *  Copyright (C) 2008  Magnus Damm
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

static struct resource sh7785_pfc_resources[] = {
	[0] = {
		.start	= 0xffe70000,
		.end	= 0xffe7008f,
		.flags	= IORESOURCE_MEM,
	},
};

static int __init plat_pinmux_setup(void)
{
	return sh_pfc_register("pfc-sh7785", sh7785_pfc_resources,
			       ARRAY_SIZE(sh7785_pfc_resources));
}
arch_initcall(plat_pinmux_setup);
