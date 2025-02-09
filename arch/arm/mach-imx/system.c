/*
 * Copyright (C) 1999 ARM Limited
 * Copyright (C) 2000 Deep Blue Solutions Ltd
 * Copyright 2006-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2008 Juergen Beisert, kernel@pengutronix.de
 * Copyright 2009 Ilya Yanok, Emcraft Systems Ltd, yanok@emcraft.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <fikus/kernel.h>
#include <fikus/clk.h>
#include <fikus/io.h>
#include <fikus/err.h>
#include <fikus/delay.h>
#include <fikus/of.h>
#include <fikus/of_address.h>

#include <asm/system_misc.h>
#include <asm/proc-fns.h>
#include <asm/mach-types.h>
#include <asm/hardware/cache-l2x0.h>

#include "common.h"
#include "hardware.h"

static void __iomem *wdog_base;
static struct clk *wdog_clk;

/*
 * Reset the system. It is called by machine_restart().
 */
void mxc_restart(enum reboot_mode mode, const char *cmd)
{
	unsigned int wcr_enable;

	if (wdog_clk)
		clk_enable(wdog_clk);

	if (cpu_is_mx1())
		wcr_enable = (1 << 0);
	else
		wcr_enable = (1 << 2);

	/* Assert SRS signal */
	__raw_writew(wcr_enable, wdog_base);

	/* wait for reset to assert... */
	mdelay(500);

	pr_err("%s: Watchdog reset failed to assert reset\n", __func__);

	/* delay to allow the serial port to show the message */
	mdelay(50);

	/* we'll take a jump through zero as a poor second */
	soft_restart(0);
}

void __init mxc_arch_reset_init(void __iomem *base)
{
	wdog_base = base;

	wdog_clk = clk_get_sys("imx2-wdt.0", NULL);
	if (IS_ERR(wdog_clk)) {
		pr_warn("%s: failed to get wdog clock\n", __func__);
		wdog_clk = NULL;
		return;
	}

	clk_prepare(wdog_clk);
}

void __init mxc_arch_reset_init_dt(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx21-wdt");
	wdog_base = of_iomap(np, 0);
	WARN_ON(!wdog_base);

	wdog_clk = of_clk_get(np, 0);
	if (IS_ERR(wdog_clk)) {
		pr_warn("%s: failed to get wdog clock\n", __func__);
		wdog_clk = NULL;
		return;
	}

	clk_prepare(wdog_clk);
}

#ifdef CONFIG_CACHE_L2X0
void __init imx_init_l2cache(void)
{
	void __iomem *l2x0_base;
	struct device_node *np;
	unsigned int val;

	np = of_find_compatible_node(NULL, NULL, "arm,pl310-cache");
	if (!np)
		goto out;

	l2x0_base = of_iomap(np, 0);
	if (!l2x0_base) {
		of_node_put(np);
		goto out;
	}

	/* Configure the L2 PREFETCH and POWER registers */
	val = readl_relaxed(l2x0_base + L2X0_PREFETCH_CTRL);
	val |= 0x70800000;
	/*
	 * The L2 cache controller(PL310) version on the i.MX6D/Q is r3p1-50rel0
	 * The L2 cache controller(PL310) version on the i.MX6DL/SOLO/SL is r3p2
	 * But according to ARM PL310 errata: 752271
	 * ID: 752271: Double linefill feature can cause data corruption
	 * Fault Status: Present in: r3p0, r3p1, r3p1-50rel0. Fixed in r3p2
	 * Workaround: The only workaround to this erratum is to disable the
	 * double linefill feature. This is the default behavior.
	 */
	if (cpu_is_imx6q())
		val &= ~(1 << 30 | 1 << 23);
	writel_relaxed(val, l2x0_base + L2X0_PREFETCH_CTRL);
	val = L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN;
	writel_relaxed(val, l2x0_base + L2X0_POWER_CTRL);

	iounmap(l2x0_base);
	of_node_put(np);

out:
	l2x0_of_init(0, ~0UL);
}
#endif
