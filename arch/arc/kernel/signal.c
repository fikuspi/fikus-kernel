/*
 * Signal Handling for ARC
 *
 * Copyright (C) 2004, 2007-2010, 2011-2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * vineetg: Jan 2010 (Restarting of timer related syscalls)
 *
 * vineetg: Nov 2009 (Everything needed for TIF_RESTORE_SIGMASK)
 *  -do_signal() supports TIF_RESTORE_SIGMASK
 *  -do_signal() no loner needs oldset, required by OLD sys_sigsuspend
 *  -sys_rt_sigsuspend() now comes from generic code, so discard arch implemen
 *  -sys_sigsuspend() no longer needs to fudge ptregs, hence that arg removed
 *  -sys_sigsuspend() no longer loops for do_signal(), sets TIF_xxx and leaves
 *   the job to do_signal()
 *
 * vineetg: July 2009
 *  -Modified Code to support the uClibc provided userland sigreturn stub
 *   to avoid kernel synthesing it on user stack at runtime, costing TLB
 *   probes and Cache line flushes.
 *
 * vineetg: July 2009
 *  -In stash_usr_regs( ) and restore_usr_regs( ), save/restore of user regs
 *   in done in block copy rather than one word at a time.
 *   This saves around 2K of code and improves LMBench lat_sig <catch>
 *
 * rajeshwarr: Feb 2009
 *  - Support for Realtime Signals
 *
 * vineetg: Aug 11th 2008: Bug #94183
 *  -ViXS were still seeing crashes when using insmod to load drivers.
 *   It turned out that the code to change Execute permssions for TLB entries
 *   of user was not guarded for interrupts (mod_tlb_permission)
 *   This was cauing TLB entries to be overwritten on unrelated indexes
 *
 * Vineetg: July 15th 2008: Bug #94183
 *  -Exception happens in Delay slot of a JMP, and before user space resumes,
 *   Signal is delivered (Ctrl + C) = >SIGINT.
 *   setup_frame( ) sets up PC,SP,BLINK to enable user space signal handler
 *   to run, but doesn't clear the Delay slot bit from status32. As a result,
 *   on resuming user mode, signal handler branches off to BTA of orig JMP
 *  -FIX: clear the DE bit from status32 in setup_frame( )
 *
 * Rahul Trivedi, Kanika Nema: Codito Technologies 2004
 */

#include <fikus/signal.h>
#include <fikus/ptrace.h>
#include <fikus/personality.h>
#include <fikus/uaccess.h>
#include <fikus/syscalls.h>
#include <fikus/tracehook.h>
#include <asm/ucontext.h>

struct rt_sigframe {
	struct siginfo info;
	struct ucontext uc;
#define MAGIC_SIGALTSTK		0x07302004
	unsigned int sigret_magic;
};

static int
stash_usr_regs(struct rt_sigframe __user *sf, struct pt_regs *regs,
	       sigset_t *set)
{
	int err;
	err = __copy_to_user(&(sf->uc.uc_mcontext.regs), regs,
			     sizeof(sf->uc.uc_mcontext.regs.scratch));
	err |= __copy_to_user(&sf->uc.uc_sigmask, set, sizeof(sigset_t));

	return err;
}

static int restore_usr_regs(struct pt_regs *regs, struct rt_sigframe __user *sf)
{
	sigset_t set;
	int err;

	err = __copy_from_user(&set, &sf->uc.uc_sigmask, sizeof(set));
	if (!err)
		set_current_blocked(&set);

	err |= __copy_from_user(regs, &(sf->uc.uc_mcontext.regs),
				sizeof(sf->uc.uc_mcontext.regs.scratch));

	return err;
}

static inline int is_do_ss_needed(unsigned int magic)
{
	if (MAGIC_SIGALTSTK == magic)
		return 1;
	else
		return 0;
}

SYSCALL_DEFINE0(rt_sigreturn)
{
	struct rt_sigframe __user *sf;
	unsigned int magic;
	struct pt_regs *regs = current_pt_regs();

	/* Always make any pending restarted system calls return -EINTR */
	current_thread_info()->restart_block.fn = do_no_restart_syscall;

	/* Since we stacked the signal on a word boundary,
	 * then 'sp' should be word aligned here.  If it's
	 * not, then the user is trying to mess with us.
	 */
	if (regs->sp & 3)
		goto badframe;

	sf = (struct rt_sigframe __force __user *)(regs->sp);

	if (!access_ok(VERIFY_READ, sf, sizeof(*sf)))
		goto badframe;

	if (__get_user(magic, &sf->sigret_magic))
		goto badframe;

	if (unlikely(is_do_ss_needed(magic)))
		if (restore_altstack(&sf->uc.uc_stack))
			goto badframe;

	if (restore_usr_regs(regs, sf))
		goto badframe;

	/* Don't restart from sigreturn */
	syscall_wont_restart(regs);

	return regs->r0;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

/*
 * Determine which stack to use..
 */
static inline void __user *get_sigframe(struct k_sigaction *ka,
					struct pt_regs *regs,
					unsigned long framesize)
{
	unsigned long sp = regs->sp;
	void __user *frame;

	/* This is the X/Open sanctioned signal stack switching */
	if ((ka->sa.sa_flags & SA_ONSTACK) && !sas_ss_flags(sp))
		sp = current->sas_ss_sp + current->sas_ss_size;

	/* No matter what happens, 'sp' must be word
	 * aligned otherwise nasty things could happen
	 */

	/* ATPCS B01 mandates 8-byte alignment */
	frame = (void __user *)((sp - framesize) & ~7);

	/* Check that we can actually write to the signal frame */
	if (!access_ok(VERIFY_WRITE, frame, framesize))
		frame = NULL;

	return frame;
}

/*
 * translate the signal
 */
static inline int map_sig(int sig)
{
	struct thread_info *thread = current_thread_info();
	if (thread->exec_domain && thread->exec_domain->signal_invmap
	    && sig < 32)
		sig = thread->exec_domain->signal_invmap[sig];
	return sig;
}

static int
setup_rt_frame(int signo, struct k_sigaction *ka, siginfo_t *info,
	       sigset_t *set, struct pt_regs *regs)
{
	struct rt_sigframe __user *sf;
	unsigned int magic = 0;
	int err = 0;

	sf = get_sigframe(ka, regs, sizeof(struct rt_sigframe));
	if (!sf)
		return 1;

	/*
	 * w/o SA_SIGINFO, struct ucontext is partially populated (only
	 * uc_mcontext/uc_sigmask) for kernel's normal user state preservation
	 * during signal handler execution. This works for SA_SIGINFO as well
	 * although the semantics are now overloaded (the same reg state can be
	 * inspected by userland: but are they allowed to fiddle with it ?
	 */
	err |= stash_usr_regs(sf, regs, set);

	/*
	 * SA_SIGINFO requires 3 args to signal handler:
	 *  #1: sig-no (common to any handler)
	 *  #2: struct siginfo
	 *  #3: struct ucontext (completely populated)
	 */
	if (unlikely(ka->sa.sa_flags & SA_SIGINFO)) {
		err |= copy_siginfo_to_user(&sf->info, info);
		err |= __put_user(0, &sf->uc.uc_flags);
		err |= __put_user(NULL, &sf->uc.uc_link);
		err |= __save_altstack(&sf->uc.uc_stack, regs->sp);

		/* setup args 2 and 3 for user mode handler */
		regs->r1 = (unsigned long)&sf->info;
		regs->r2 = (unsigned long)&sf->uc;

		/*
		 * small optim to avoid unconditonally calling do_sigaltstack
		 * in sigreturn path, now that we only have rt_sigreturn
		 */
		magic = MAGIC_SIGALTSTK;
	}

	err |= __put_user(magic, &sf->sigret_magic);
	if (err)
		return err;

	/* #1 arg to the user Signal handler */
	regs->r0 = map_sig(signo);

	/* setup PC of user space signal handler */
	regs->ret = (unsigned long)ka->sa.sa_handler;

	/*
	 * handler returns using sigreturn stub provided already by userpsace
	 */
	BUG_ON(!(ka->sa.sa_flags & SA_RESTORER));
	regs->blink = (unsigned long)ka->sa.sa_restorer;

	/* User Stack for signal handler will be above the frame just carved */
	regs->sp = (unsigned long)sf;

	/*
	 * Bug 94183, Clear the DE bit, so that when signal handler
	 * starts to run, it doesn't use BTA
	 */
	regs->status32 &= ~STATUS_DE_MASK;
	regs->status32 |= STATUS_L_MASK;

	return err;
}

static void arc_restart_syscall(struct k_sigaction *ka, struct pt_regs *regs)
{
	switch (regs->r0) {
	case -ERESTART_RESTARTBLOCK:
	case -ERESTARTNOHAND:
		/*
		 * ERESTARTNOHAND means that the syscall should
		 * only be restarted if there was no handler for
		 * the signal, and since we only get here if there
		 * is a handler, we don't restart
		 */
		regs->r0 = -EINTR;   /* ERESTART_xxx is internal */
		break;

	case -ERESTARTSYS:
		/*
		 * ERESTARTSYS means to restart the syscall if
		 * there is no handler or the handler was
		 * registered with SA_RESTART
		 */
		if (!(ka->sa.sa_flags & SA_RESTART)) {
			regs->r0 = -EINTR;
			break;
		}
		/* fallthrough */

	case -ERESTARTNOINTR:
		/*
		 * ERESTARTNOINTR means that the syscall should
		 * be called again after the signal handler returns.
		 * Setup reg state just as it was before doing the trap
		 * r0 has been clobbered with sys call ret code thus it
		 * needs to be reloaded with orig first arg to syscall
		 * in orig_r0. Rest of relevant reg-file:
		 * r8 (syscall num) and (r1 - r7) will be reset to
		 * their orig user space value when we ret from kernel
		 */
		regs->r0 = regs->orig_r0;
		regs->ret -= 4;
		break;
	}
}

/*
 * OK, we're invoking a handler
 */
static void
handle_signal(unsigned long sig, struct k_sigaction *ka, siginfo_t *info,
	      struct pt_regs *regs)
{
	sigset_t *oldset = sigmask_to_save();
	int ret;

	/* Set up the stack frame */
	ret = setup_rt_frame(sig, ka, info, oldset, regs);

	if (ret)
		force_sigsegv(sig, current);
	else
		signal_delivered(sig, info, ka, regs, 0);
}

void do_signal(struct pt_regs *regs)
{
	struct k_sigaction ka;
	siginfo_t info;
	int signr;
	int restart_scall;

	signr = get_signal_to_deliver(&info, &ka, regs, NULL);

	restart_scall = in_syscall(regs) && syscall_restartable(regs);

	if (signr > 0) {
		if (restart_scall) {
			arc_restart_syscall(&ka, regs);
			syscall_wont_restart(regs);	/* No more restarts */
		}
		handle_signal(signr, &ka, &info, regs);
		return;
	}

	if (restart_scall) {
		/* No handler for syscall: restart it */
		if (regs->r0 == -ERESTARTNOHAND ||
		    regs->r0 == -ERESTARTSYS || regs->r0 == -ERESTARTNOINTR) {
			regs->r0 = regs->orig_r0;
			regs->ret -= 4;
		} else if (regs->r0 == -ERESTART_RESTARTBLOCK) {
			regs->r8 = __NR_restart_syscall;
			regs->ret -= 4;
		}
		syscall_wont_restart(regs);	/* No more restarts */
	}

	/* If there's no signal to deliver, restore the saved sigmask back */
	restore_saved_sigmask();
}

void do_notify_resume(struct pt_regs *regs)
{
	/*
	 * ASM glue gaurantees that this is only called when returning to
	 * user mode
	 */
	if (test_and_clear_thread_flag(TIF_NOTIFY_RESUME))
		tracehook_notify_resume(regs);
}
