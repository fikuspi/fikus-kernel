/*
 *  GT641xx clockevent routines.
 *
 *  Copyright (C) 2007	Yoichi Yuasa <yuasa@fikus-mips.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <fikus/clockchips.h>
#include <fikus/init.h>
#include <fikus/interrupt.h>
#include <fikus/spinlock.h>
#include <fikus/irq.h>

#include <asm/gt64120.h>
#include <asm/time.h>

static DEFINE_RAW_SPINLOCK(gt641xx_timer_lock);
static unsigned int gt641xx_base_clock;

void gt641xx_set_base_clock(unsigned int clock)
{
	gt641xx_base_clock = clock;
}

int gt641xx_timer0_state(void)
{
	if (GT_READ(GT_TC0_OFS))
		return 0;

	GT_WRITE(GT_TC0_OFS, gt641xx_base_clock / HZ);
	GT_WRITE(GT_TC_CONTROL_OFS, GT_TC_CONTROL_ENTC0_MSK);

	return 1;
}

static int gt641xx_timer0_set_next_event(unsigned long delta,
					 struct clock_event_device *evt)
{
	u32 ctrl;

	raw_spin_lock(&gt641xx_timer_lock);

	ctrl = GT_READ(GT_TC_CONTROL_OFS);
	ctrl &= ~(GT_TC_CONTROL_ENTC0_MSK | GT_TC_CONTROL_SELTC0_MSK);
	ctrl |= GT_TC_CONTROL_ENTC0_MSK;

	GT_WRITE(GT_TC0_OFS, delta);
	GT_WRITE(GT_TC_CONTROL_OFS, ctrl);

	raw_spin_unlock(&gt641xx_timer_lock);

	return 0;
}

static void gt641xx_timer0_set_mode(enum clock_event_mode mode,
				    struct clock_event_device *evt)
{
	u32 ctrl;

	raw_spin_lock(&gt641xx_timer_lock);

	ctrl = GT_READ(GT_TC_CONTROL_OFS);
	ctrl &= ~(GT_TC_CONTROL_ENTC0_MSK | GT_TC_CONTROL_SELTC0_MSK);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		ctrl |= GT_TC_CONTROL_ENTC0_MSK | GT_TC_CONTROL_SELTC0_MSK;
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		ctrl |= GT_TC_CONTROL_ENTC0_MSK;
		break;
	default:
		break;
	}

	GT_WRITE(GT_TC_CONTROL_OFS, ctrl);

	raw_spin_unlock(&gt641xx_timer_lock);
}

static void gt641xx_timer0_event_handler(struct clock_event_device *dev)
{
}

static struct clock_event_device gt641xx_timer0_clockevent = {
	.name		= "gt641xx-timer0",
	.features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.irq		= GT641XX_TIMER0_IRQ,
	.set_next_event = gt641xx_timer0_set_next_event,
	.set_mode	= gt641xx_timer0_set_mode,
	.event_handler	= gt641xx_timer0_event_handler,
};

static irqreturn_t gt641xx_timer0_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *cd = &gt641xx_timer0_clockevent;

	cd->event_handler(cd);

	return IRQ_HANDLED;
}

static struct irqaction gt641xx_timer0_irqaction = {
	.handler	= gt641xx_timer0_interrupt,
	.flags		= IRQF_PERCPU | IRQF_TIMER,
	.name		= "gt641xx_timer0",
};

static int __init gt641xx_timer0_clockevent_init(void)
{
	struct clock_event_device *cd;

	if (!gt641xx_base_clock)
		return 0;

	GT_WRITE(GT_TC0_OFS, gt641xx_base_clock / HZ);

	cd = &gt641xx_timer0_clockevent;
	cd->rating = 200 + gt641xx_base_clock / 10000000;
	clockevent_set_clock(cd, gt641xx_base_clock);
	cd->max_delta_ns = clockevent_delta2ns(0x7fffffff, cd);
	cd->min_delta_ns = clockevent_delta2ns(0x300, cd);
	cd->cpumask = cpumask_of(0);

	clockevents_register_device(&gt641xx_timer0_clockevent);

	return setup_irq(GT641XX_TIMER0_IRQ, &gt641xx_timer0_irqaction);
}
arch_initcall(gt641xx_timer0_clockevent_init);
