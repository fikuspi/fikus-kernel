/*
 * Copyright 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2011 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <fikus/clk.h>
#include <fikus/clkdev.h>
#include <fikus/err.h>
#include <fikus/io.h>
#include <fikus/irq.h>
#include <fikus/of_irq.h>
#include <fikus/of_platform.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>

#include "common.h"
#include "hardware.h"
#include "mx53.h"

static void __init imx53_dt_init(void)
{
	mxc_arch_reset_init_dt();

	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static const char *imx53_dt_board_compat[] __initdata = {
	"fsl,imx53",
	NULL
};

static void __init imx53_timer_init(void)
{
	mx53_clocks_init_dt();
}

DT_MACHINE_START(IMX53_DT, "Freescale i.MX53 (Device Tree Support)")
	.map_io		= mx53_map_io,
	.init_early	= imx53_init_early,
	.init_irq	= mx53_init_irq,
	.handle_irq	= imx53_handle_irq,
	.init_time	= imx53_timer_init,
	.init_machine	= imx53_dt_init,
	.init_late	= imx53_init_late,
	.dt_compat	= imx53_dt_board_compat,
	.restart	= mxc_restart,
MACHINE_END
