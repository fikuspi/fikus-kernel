/*
 * arch/arm/plat-iop/time.c
 *
 * Timer code for IOP32x and IOP33x based systems
 *
 * Author: Deepak Saxena <dsaxena@mvista.com>
 *
 * Copyright 2002-2003 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <fikus/kernel.h>
#include <fikus/interrupt.h>
#include <fikus/time.h>
#include <fikus/init.h>
#include <fikus/timex.h>
#include <fikus/io.h>
#include <fikus/clocksource.h>
#include <fikus/clockchips.h>
#include <fikus/export.h>
#include <fikus/sched_clock.h>
#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <mach/time.h>

/*
 * Minimum clocksource/clockevent timer range in seconds
 */
#define IOP_MIN_RANGE 4

/*
 * IOP clocksource (free-running timer 1).
 */
static cycle_t notrace iop_clocksource_read(struct clocksource *unused)
{
	return 0xffffffffu - read_tcr1();
}

static struct clocksource iop_clocksource = {
	.name 		= "iop_timer1",
	.rating		= 300,
	.read		= iop_clocksource_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

/*
 * IOP sched_clock() implementation via its clocksource.
 */
static u32 notrace iop_read_sched_clock(void)
{
	return 0xffffffffu - read_tcr1();
}

/*
 * IOP clockevents (interrupting timer 0).
 */
static int iop_set_next_event(unsigned long delta,
			      struct clock_event_device *unused)
{
	u32 tmr = IOP_TMR_PRIVILEGED | IOP_TMR_RATIO_1_1;

	BUG_ON(delta == 0);
	write_tmr0(tmr & ~(IOP_TMR_EN | IOP_TMR_RELOAD));
	write_tcr0(delta);
	write_tmr0((tmr & ~IOP_TMR_RELOAD) | IOP_TMR_EN);

	return 0;
}

static unsigned long ticks_per_jiffy;

static void iop_set_mode(enum clock_event_mode mode,
			 struct clock_event_device *unused)
{
	u32 tmr = read_tmr0();

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		write_tmr0(tmr & ~IOP_TMR_EN);
		write_tcr0(ticks_per_jiffy - 1);
		write_trr0(ticks_per_jiffy - 1);
		tmr |= (IOP_TMR_RELOAD | IOP_TMR_EN);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		/* ->set_next_event sets period and enables timer */
		tmr &= ~(IOP_TMR_RELOAD | IOP_TMR_EN);
		break;
	case CLOCK_EVT_MODE_RESUME:
		tmr |= IOP_TMR_EN;
		break;
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_UNUSED:
	default:
		tmr &= ~IOP_TMR_EN;
		break;
	}

	write_tmr0(tmr);
}

static struct clock_event_device iop_clockevent = {
	.name		= "iop_timer0",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating         = 300,
	.set_next_event	= iop_set_next_event,
	.set_mode	= iop_set_mode,
};

static irqreturn_t
iop_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;

	write_tisr(1);
	evt->event_handler(evt);
	return IRQ_HANDLED;
}

static struct irqaction iop_timer_irq = {
	.name		= "IOP Timer Tick",
	.handler	= iop_timer_interrupt,
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.dev_id		= &iop_clockevent,
};

static unsigned long iop_tick_rate;
unsigned long get_iop_tick_rate(void)
{
	return iop_tick_rate;
}
EXPORT_SYMBOL(get_iop_tick_rate);

void __init iop_init_time(unsigned long tick_rate)
{
	u32 timer_ctl;

	setup_sched_clock(iop_read_sched_clock, 32, tick_rate);

	ticks_per_jiffy = DIV_ROUND_CLOSEST(tick_rate, HZ);
	iop_tick_rate = tick_rate;

	timer_ctl = IOP_TMR_EN | IOP_TMR_PRIVILEGED |
			IOP_TMR_RELOAD | IOP_TMR_RATIO_1_1;

	/*
	 * Set up interrupting clockevent timer 0.
	 */
	write_tmr0(timer_ctl & ~IOP_TMR_EN);
	write_tisr(1);
	setup_irq(IRQ_IOP_TIMER0, &iop_timer_irq);
	iop_clockevent.cpumask = cpumask_of(0);
	clockevents_config_and_register(&iop_clockevent, tick_rate,
					0xf, 0xfffffffe);

	/*
	 * Set up free-running clocksource timer 1.
	 */
	write_trr1(0xffffffff);
	write_tcr1(0xffffffff);
	write_tmr1(timer_ctl);
	clocksource_register_hz(&iop_clocksource, tick_rate);
}
