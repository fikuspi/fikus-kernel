/*
 * Dummy local timer
 *
 * Copyright (C) 2008  Paul Mundt
 *
 * cloned from:
 *
 *  fikus/arch/arm/mach-realview/localtimer.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/delay.h>
#include <fikus/device.h>
#include <fikus/smp.h>
#include <fikus/jiffies.h>
#include <fikus/percpu.h>
#include <fikus/clockchips.h>
#include <fikus/hardirq.h>
#include <fikus/irq.h>

static DEFINE_PER_CPU(struct clock_event_device, local_clockevent);

/*
 * Used on SMP for either the local timer or SMP_MSG_TIMER
 */
void local_timer_interrupt(void)
{
	struct clock_event_device *clk = &__get_cpu_var(local_clockevent);

	irq_enter();
	clk->event_handler(clk);
	irq_exit();
}

static void dummy_timer_set_mode(enum clock_event_mode mode,
				 struct clock_event_device *clk)
{
}

void local_timer_setup(unsigned int cpu)
{
	struct clock_event_device *clk = &per_cpu(local_clockevent, cpu);

	clk->name		= "dummy_timer";
	clk->features		= CLOCK_EVT_FEAT_ONESHOT |
				  CLOCK_EVT_FEAT_PERIODIC |
				  CLOCK_EVT_FEAT_DUMMY;
	clk->rating		= 400;
	clk->mult		= 1;
	clk->set_mode		= dummy_timer_set_mode;
	clk->broadcast		= smp_timer_broadcast;
	clk->cpumask		= cpumask_of(cpu);

	clockevents_register_device(clk);
}

void local_timer_stop(unsigned int cpu)
{
}
