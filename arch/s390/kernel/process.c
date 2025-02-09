/*
 * This file handles the architecture dependent parts of process handling.
 *
 *    Copyright IBM Corp. 1999, 2009
 *    Author(s): Martin Schwidefsky <schwidefsky@de.ibm.com>,
 *		 Hartmut Penner <hp@de.ibm.com>,
 *		 Denis Joseph Barrow,
 */

#include <fikus/compiler.h>
#include <fikus/cpu.h>
#include <fikus/sched.h>
#include <fikus/kernel.h>
#include <fikus/mm.h>
#include <fikus/elfcore.h>
#include <fikus/smp.h>
#include <fikus/slab.h>
#include <fikus/interrupt.h>
#include <fikus/tick.h>
#include <fikus/personality.h>
#include <fikus/syscalls.h>
#include <fikus/compat.h>
#include <fikus/kprobes.h>
#include <fikus/random.h>
#include <fikus/module.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <asm/vtimer.h>
#include <asm/exec.h>
#include <asm/irq.h>
#include <asm/nmi.h>
#include <asm/smp.h>
#include <asm/switch_to.h>
#include <asm/runtime_instr.h>
#include "entry.h"

asmlinkage void ret_from_fork(void) asm ("ret_from_fork");

/*
 * Return saved PC of a blocked thread. used in kernel/sched.
 * resume in entry.S does not create a new stack frame, it
 * just stores the registers %r6-%r15 to the frame given by
 * schedule. We want to return the address of the caller of
 * schedule, so we have to walk the backchain one time to
 * find the frame schedule() store its return address.
 */
unsigned long thread_saved_pc(struct task_struct *tsk)
{
	struct stack_frame *sf, *low, *high;

	if (!tsk || !task_stack_page(tsk))
		return 0;
	low = task_stack_page(tsk);
	high = (struct stack_frame *) task_pt_regs(tsk);
	sf = (struct stack_frame *) (tsk->thread.ksp & PSW_ADDR_INSN);
	if (sf <= low || sf > high)
		return 0;
	sf = (struct stack_frame *) (sf->back_chain & PSW_ADDR_INSN);
	if (sf <= low || sf > high)
		return 0;
	return sf->gprs[8];
}

void arch_cpu_idle(void)
{
	local_mcck_disable();
	if (test_thread_flag(TIF_MCCK_PENDING)) {
		local_mcck_enable();
		local_irq_enable();
		return;
	}
	/* Halt the cpu and keep track of cpu time accounting. */
	vtime_stop_cpu();
	local_irq_enable();
}

void arch_cpu_idle_exit(void)
{
	if (test_thread_flag(TIF_MCCK_PENDING))
		s390_handle_mcck();
}

void arch_cpu_idle_dead(void)
{
	cpu_die();
}

extern void __kprobes kernel_thread_starter(void);

/*
 * Free current thread data structures etc..
 */
void exit_thread(void)
{
	exit_thread_runtime_instr();
}

void flush_thread(void)
{
}

void release_thread(struct task_struct *dead_task)
{
}

int copy_thread(unsigned long clone_flags, unsigned long new_stackp,
		unsigned long arg, struct task_struct *p)
{
	struct thread_info *ti;
	struct fake_frame
	{
		struct stack_frame sf;
		struct pt_regs childregs;
	} *frame;

	frame = container_of(task_pt_regs(p), struct fake_frame, childregs);
	p->thread.ksp = (unsigned long) frame;
	/* Save access registers to new thread structure. */
	save_access_regs(&p->thread.acrs[0]);
	/* start new process with ar4 pointing to the correct address space */
	p->thread.mm_segment = get_fs();
	/* Don't copy debug registers */
	memset(&p->thread.per_user, 0, sizeof(p->thread.per_user));
	memset(&p->thread.per_event, 0, sizeof(p->thread.per_event));
	clear_tsk_thread_flag(p, TIF_SINGLE_STEP);
	clear_tsk_thread_flag(p, TIF_PER_TRAP);
	/* Initialize per thread user and system timer values */
	ti = task_thread_info(p);
	ti->user_timer = 0;
	ti->system_timer = 0;

	frame->sf.back_chain = 0;
	/* new return point is ret_from_fork */
	frame->sf.gprs[8] = (unsigned long) ret_from_fork;
	/* fake return stack for resume(), don't go back to schedule */
	frame->sf.gprs[9] = (unsigned long) frame;

	/* Store access registers to kernel stack of new process. */
	if (unlikely(p->flags & PF_KTHREAD)) {
		/* kernel thread */
		memset(&frame->childregs, 0, sizeof(struct pt_regs));
		frame->childregs.psw.mask = psw_kernel_bits | PSW_MASK_DAT |
				PSW_MASK_IO | PSW_MASK_EXT | PSW_MASK_MCHECK;
		frame->childregs.psw.addr = PSW_ADDR_AMODE |
				(unsigned long) kernel_thread_starter;
		frame->childregs.gprs[9] = new_stackp; /* function */
		frame->childregs.gprs[10] = arg;
		frame->childregs.gprs[11] = (unsigned long) do_exit;
		frame->childregs.orig_gpr2 = -1;

		return 0;
	}
	frame->childregs = *current_pt_regs();
	frame->childregs.gprs[2] = 0;	/* child returns 0 on fork. */
	if (new_stackp)
		frame->childregs.gprs[15] = new_stackp;

	/* Don't copy runtime instrumentation info */
	p->thread.ri_cb = NULL;
	p->thread.ri_signum = 0;
	frame->childregs.psw.mask &= ~PSW_MASK_RI;

#ifndef CONFIG_64BIT
	/*
	 * save fprs to current->thread.fp_regs to merge them with
	 * the emulated registers and then copy the result to the child.
	 */
	save_fp_regs(&current->thread.fp_regs);
	memcpy(&p->thread.fp_regs, &current->thread.fp_regs,
	       sizeof(s390_fp_regs));
	/* Set a new TLS ?  */
	if (clone_flags & CLONE_SETTLS)
		p->thread.acrs[0] = frame->childregs.gprs[6];
#else /* CONFIG_64BIT */
	/* Save the fpu registers to new thread structure. */
	save_fp_regs(&p->thread.fp_regs);
	/* Set a new TLS ?  */
	if (clone_flags & CLONE_SETTLS) {
		unsigned long tls = frame->childregs.gprs[6];
		if (is_compat_task()) {
			p->thread.acrs[0] = (unsigned int)tls;
		} else {
			p->thread.acrs[0] = (unsigned int)(tls >> 32);
			p->thread.acrs[1] = (unsigned int)tls;
		}
	}
#endif /* CONFIG_64BIT */
	return 0;
}

asmlinkage void execve_tail(void)
{
	current->thread.fp_regs.fpc = 0;
	if (MACHINE_HAS_IEEE)
		asm volatile("sfpc %0,%0" : : "d" (0));
}

/*
 * fill in the FPU structure for a core dump.
 */
int dump_fpu (struct pt_regs * regs, s390_fp_regs *fpregs)
{
#ifndef CONFIG_64BIT
	/*
	 * save fprs to current->thread.fp_regs to merge them with
	 * the emulated registers and then copy the result to the dump.
	 */
	save_fp_regs(&current->thread.fp_regs);
	memcpy(fpregs, &current->thread.fp_regs, sizeof(s390_fp_regs));
#else /* CONFIG_64BIT */
	save_fp_regs(fpregs);
#endif /* CONFIG_64BIT */
	return 1;
}
EXPORT_SYMBOL(dump_fpu);

unsigned long get_wchan(struct task_struct *p)
{
	struct stack_frame *sf, *low, *high;
	unsigned long return_address;
	int count;

	if (!p || p == current || p->state == TASK_RUNNING || !task_stack_page(p))
		return 0;
	low = task_stack_page(p);
	high = (struct stack_frame *) task_pt_regs(p);
	sf = (struct stack_frame *) (p->thread.ksp & PSW_ADDR_INSN);
	if (sf <= low || sf > high)
		return 0;
	for (count = 0; count < 16; count++) {
		sf = (struct stack_frame *) (sf->back_chain & PSW_ADDR_INSN);
		if (sf <= low || sf > high)
			return 0;
		return_address = sf->gprs[8] & PSW_ADDR_INSN;
		if (!in_sched_functions(return_address))
			return return_address;
	}
	return 0;
}

unsigned long arch_align_stack(unsigned long sp)
{
	if (!(current->personality & ADDR_NO_RANDOMIZE) && randomize_va_space)
		sp -= get_random_int() & ~PAGE_MASK;
	return sp & ~0xf;
}

static inline unsigned long brk_rnd(void)
{
	/* 8MB for 32bit, 1GB for 64bit */
	if (is_32bit_task())
		return (get_random_int() & 0x7ffUL) << PAGE_SHIFT;
	else
		return (get_random_int() & 0x3ffffUL) << PAGE_SHIFT;
}

unsigned long arch_randomize_brk(struct mm_struct *mm)
{
	unsigned long ret = PAGE_ALIGN(mm->brk + brk_rnd());

	if (ret < mm->brk)
		return mm->brk;
	return ret;
}

unsigned long randomize_et_dyn(unsigned long base)
{
	unsigned long ret = PAGE_ALIGN(base + brk_rnd());

	if (!(current->flags & PF_RANDOMIZE))
		return base;
	if (ret < base)
		return base;
	return ret;
}
