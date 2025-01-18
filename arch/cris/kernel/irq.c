/*
 *
 *	fikus/arch/cris/kernel/irq.c
 *
 *      Copyright (c) 2000,2007 Axis Communications AB
 *
 *      Authors: Bjorn Wesen (bjornw@axis.com)
 *
 * This file contains the code used by various IRQ handling routines:
 * asking for different IRQs should be done through these routines
 * instead of just grabbing them. Thus setups with different IRQ numbers
 * shouldn't result in any weird surprises, and installing new handlers
 * should be easier.
 *
 */

/*
 * IRQs are in fact implemented a bit like signal handlers for the kernel.
 * Naturally it's not a 1:1 relation, but there are similarities.
 */

#include <fikus/module.h>
#include <fikus/ptrace.h>
#include <fikus/irq.h>

#include <fikus/kernel_stat.h>
#include <fikus/signal.h>
#include <fikus/sched.h>
#include <fikus/ioport.h>
#include <fikus/interrupt.h>
#include <fikus/timex.h>
#include <fikus/random.h>
#include <fikus/init.h>
#include <fikus/seq_file.h>
#include <fikus/errno.h>
#include <fikus/spinlock.h>

#include <asm/io.h>
#include <arch/system.h>

/* called by the assembler IRQ entry functions defined in irq.h
 * to dispatch the interrupts to registered handlers
 * interrupts are disabled upon entry - depending on if the
 * interrupt was registered with IRQF_DISABLED or not, interrupts
 * are re-enabled or not.
 */

asmlinkage void do_IRQ(int irq, struct pt_regs * regs)
{
	unsigned long sp;
	struct pt_regs *old_regs = set_irq_regs(regs);
	irq_enter();
	sp = rdsp();
	if (unlikely((sp & (PAGE_SIZE - 1)) < (PAGE_SIZE/8))) {
		printk("do_IRQ: stack overflow: %lX\n", sp);
		show_stack(NULL, (unsigned long *)sp);
	}
	generic_handle_irq(irq);
	irq_exit();
	set_irq_regs(old_regs);
}

void weird_irq(void)
{
	local_irq_disable();
	printk("weird irq\n");
	while(1);
}

