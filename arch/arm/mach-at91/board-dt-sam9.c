/*
 *  Setup code for AT91SAM Evaluation Kits with Device Tree support
 *
 *  Copyright (C) 2011 Atmel,
 *                2011 Nicolas Ferre <nicolas.ferre@atmel.com>
 *
 * Licensed under GPLv2 or later.
 */

#include <fikus/types.h>
#include <fikus/init.h>
#include <fikus/module.h>
#include <fikus/gpio.h>
#include <fikus/of.h>
#include <fikus/of_irq.h>
#include <fikus/of_platform.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include "at91_aic.h"
#include "board.h"
#include "generic.h"


static const struct of_device_id irq_of_match[] __initconst = {

	{ .compatible = "atmel,at91rm9200-aic", .data = at91_aic_of_init },
	{ /*sentinel*/ }
};

static void __init at91_dt_init_irq(void)
{
	of_irq_init(irq_of_match);
}

static void __init at91_dt_device_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static const char *at91_dt_board_compat[] __initdata = {
	"atmel,at91sam9",
	NULL
};

DT_MACHINE_START(at91sam_dt, "Atmel AT91SAM (Device Tree)")
	/* Maintainer: Atmel */
	.init_time	= at91sam926x_pit_init,
	.map_io		= at91_map_io,
	.handle_irq	= at91_aic_handle_irq,
	.init_early	= at91_dt_initialize,
	.init_irq	= at91_dt_init_irq,
	.init_machine	= at91_dt_device_init,
	.dt_compat	= at91_dt_board_compat,
MACHINE_END
