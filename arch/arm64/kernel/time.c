/*
 * Based on arch/arm/kernel/time.c
 *
 * Copyright (C) 1991, 1992, 1995  John Torvalds
 * Modifications for ARM (C) 1994-2001 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fikus/export.h>
#include <fikus/kernel.h>
#include <fikus/interrupt.h>
#include <fikus/time.h>
#include <fikus/init.h>
#include <fikus/sched.h>
#include <fikus/smp.h>
#include <fikus/timex.h>
#include <fikus/errno.h>
#include <fikus/profile.h>
#include <fikus/syscore_ops.h>
#include <fikus/timer.h>
#include <fikus/irq.h>
#include <fikus/delay.h>
#include <fikus/clocksource.h>

#include <clocksource/arm_arch_timer.h>

#include <asm/thread_info.h>
#include <asm/stacktrace.h>

#ifdef CONFIG_SMP
unsigned long profile_pc(struct pt_regs *regs)
{
	struct stackframe frame;

	if (!in_lock_functions(regs->pc))
		return regs->pc;

	frame.fp = regs->regs[29];
	frame.sp = regs->sp;
	frame.pc = regs->pc;
	do {
		int ret = unwind_frame(&frame);
		if (ret < 0)
			return 0;
	} while (in_lock_functions(frame.pc));

	return frame.pc;
}
EXPORT_SYMBOL(profile_pc);
#endif

static u64 sched_clock_mult __read_mostly;

unsigned long long notrace sched_clock(void)
{
	return arch_timer_read_counter() * sched_clock_mult;
}

void __init time_init(void)
{
	u32 arch_timer_rate;

	clocksource_of_init();

	arch_timer_rate = arch_timer_get_rate();
	if (!arch_timer_rate)
		panic("Unable to initialise architected timer.\n");

	/* Cache the sched_clock multiplier to save a divide in the hot path. */
	sched_clock_mult = NSEC_PER_SEC / arch_timer_rate;

	/* Calibrate the delay loop directly */
	lpj_fine = arch_timer_rate / HZ;
}
