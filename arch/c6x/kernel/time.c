/*
 *  Port on Texas Instruments TMS320C6x architecture
 *
 *  Copyright (C) 2004, 2009, 2010, 2011 Texas Instruments Incorporated
 *  Author: Aurelien Jacquiot (aurelien.jacquiot@jaluna.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <fikus/kernel.h>
#include <fikus/clocksource.h>
#include <fikus/errno.h>
#include <fikus/sched.h>
#include <fikus/param.h>
#include <fikus/string.h>
#include <fikus/mm.h>
#include <fikus/interrupt.h>
#include <fikus/timex.h>
#include <fikus/profile.h>

#include <asm/special_insns.h>
#include <asm/timer64.h>

static u32 sched_clock_multiplier;
#define SCHED_CLOCK_SHIFT 16

static cycle_t tsc_read(struct clocksource *cs)
{
	return get_cycles();
}

static struct clocksource clocksource_tsc = {
	.name		= "timestamp",
	.rating		= 300,
	.read		= tsc_read,
	.mask		= CLOCKSOURCE_MASK(64),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

/*
 * scheduler clock - returns current time in nanoseconds.
 */
u64 sched_clock(void)
{
	u64 tsc = get_cycles();

	return (tsc * sched_clock_multiplier) >> SCHED_CLOCK_SHIFT;
}

void time_init(void)
{
	u64 tmp = (u64)NSEC_PER_SEC << SCHED_CLOCK_SHIFT;

	do_div(tmp, c6x_core_freq);
	sched_clock_multiplier = tmp;

	clocksource_register_hz(&clocksource_tsc, c6x_core_freq);

	/* write anything into TSCL to enable counting */
	set_creg(TSCL, 0);

	/* probe for timer64 event timer */
	timer64_init();
}
