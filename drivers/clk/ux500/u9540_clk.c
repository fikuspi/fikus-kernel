/*
 * Clock definitions for u9540 platform.
 *
 * Copyright (C) 2012 ST-Ericsson SA
 * Author: Ulf Hansson <ulf.hansson@linaro.org>
 *
 * License terms: GNU General Public License (GPL) version 2
 */

#include <fikus/clk.h>
#include <fikus/clkdev.h>
#include <fikus/clk-provider.h>
#include <fikus/mfd/dbx500-prcmu.h>
#include <fikus/platform_data/clk-ux500.h>
#include "clk.h"

void u9540_clk_init(u32 clkrst1_base, u32 clkrst2_base, u32 clkrst3_base,
		    u32 clkrst5_base, u32 clkrst6_base)
{
	/* register clocks here */
}
