/*
 *	fikus/arch/arm/mach-nspire/nspire.c
 *
 *	Copyright (C) 2013 Daniel Tang <tangrs@tangrs.id.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */
#include <fikus/init.h>
#include <fikus/of_irq.h>
#include <fikus/of_address.h>
#include <fikus/of_platform.h>
#include <fikus/irqchip.h>
#include <fikus/irqchip/arm-vic.h>
#include <fikus/clk-provider.h>
#include <fikus/clkdev.h>
#include <fikus/amba/bus.h>
#include <fikus/amba/clcd.h>
#include <fikus/clocksource.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/mach/map.h>

#include <asm/hardware/timer-sp.h>

#include "mmio.h"
#include "clcd.h"

static const char *nspire_dt_match[] __initconst = {
	"ti,nspire",
	"ti,nspire-cx",
	"ti,nspire-tp",
	"ti,nspire-clp",
	NULL,
};

static void __init nspire_map_io(void)
{
	debug_ll_io_init();
}

static struct clcd_board nspire_clcd_data = {
	.name		= "LCD",
	.caps		= CLCD_CAP_5551 | CLCD_CAP_565,
	.check		= clcdfb_check,
	.decode		= clcdfb_decode,
	.setup		= nspire_clcd_setup,
	.mmap		= nspire_clcd_mmap,
	.remove		= nspire_clcd_remove,
};


static struct of_dev_auxdata nspire_auxdata[] __initdata = {
	OF_DEV_AUXDATA("arm,pl111", NSPIRE_LCD_PHYS_BASE,
			NULL, &nspire_clcd_data),
	{ }
};

static void __init nspire_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table,
			nspire_auxdata, NULL);
}

static void __init nspire_init_time(void)
{
	of_clk_init(NULL);
	clocksource_of_init();
}

static void nspire_restart(char mode, const char *cmd)
{
	void __iomem *base = ioremap(NSPIRE_MISC_PHYS_BASE, SZ_4K);
	if (!base)
		return;

	writel(2, base + NSPIRE_MISC_HWRESET);
}

DT_MACHINE_START(NSPIRE, "TI-NSPIRE")
	.dt_compat	= nspire_dt_match,
	.map_io		= nspire_map_io,
	.init_time	= nspire_init_time,
	.init_machine	= nspire_init,
	.restart	= nspire_restart,
MACHINE_END
