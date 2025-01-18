/*
 * Flash support for OMAP1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/io.h>
#include <fikus/mtd/mtd.h>
#include <fikus/mtd/map.h>

#include <mach/tc.h>
#include <mach/flash.h>

#include <mach/hardware.h>

void omap1_set_vpp(struct platform_device *pdev, int enable)
{
	u32 l;

	l = omap_readl(EMIFS_CONFIG);
	if (enable)
		l |= OMAP_EMIFS_CONFIG_WP;
	else
		l &= ~OMAP_EMIFS_CONFIG_WP;
	omap_writel(l, EMIFS_CONFIG);
}
