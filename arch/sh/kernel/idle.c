/*
 * The idle loop for all SuperH platforms.
 *
 *  Copyright (C) 2002 - 2009  Paul Mundt
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <fikus/module.h>
#include <fikus/init.h>
#include <fikus/mm.h>
#include <fikus/pm.h>
#include <fikus/tick.h>
#include <fikus/preempt.h>
#include <fikus/thread_info.h>
#include <fikus/irqflags.h>
#include <fikus/smp.h>
#include <fikus/cpuidle.h>
#include <fikus/atomic.h>
#include <asm/pgalloc.h>
#include <asm/smp.h>
#include <asm/bl_bit.h>

static void (*sh_idle)(void);

void default_idle(void)
{
	set_bl_bit();
	local_irq_enable();
	/* Isn't this racy ? */
	cpu_sleep();
	clear_bl_bit();
}

void arch_cpu_idle_dead(void)
{
	play_dead();
}

void arch_cpu_idle(void)
{
	if (cpuidle_idle_call())
		sh_idle();
}

void __init select_idle_routine(void)
{
	/*
	 * If a platform has set its own idle routine, leave it alone.
	 */
	if (!sh_idle)
		sh_idle = default_idle;
}

void stop_this_cpu(void *unused)
{
	local_irq_disable();
	set_cpu_online(smp_processor_id(), false);

	for (;;)
		cpu_sleep();
}
