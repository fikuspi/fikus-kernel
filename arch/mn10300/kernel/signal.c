/* MN10300 Signal handling
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#include <fikus/sched.h>
#include <fikus/mm.h>
#include <fikus/smp.h>
#include <fikus/kernel.h>
#include <fikus/signal.h>
#include <fikus/errno.h>
#include <fikus/wait.h>
#include <fikus/ptrace.h>
#include <fikus/unistd.h>
#include <fikus/stddef.h>
#include <fikus/tty.h>
#include <fikus/personality.h>
#include <fikus/suspend.h>
#include <fikus/tracehook.h>
#include <asm/cacheflush.h>
#include <asm/ucontext.h>
#include <asm/uaccess.h>
#include <asm/fpu.h>
#include "sigframe.h"

#define DEBUG_SIG 0

/*
 * do a signal return; undo the signal stack.
 */
static int restore_sigcontext(struct pt_regs *regs,
			      struct sigcontext __user *sc, long *_d0)
{
	unsigned int err = 0;

	/* Always make any pending restarted system calls return -EINTR */
	current_thread_info()->restart_block.fn = do_no_restart_syscall;

	if (is_using_fpu(current))
		fpu_kill_state(current);

#define COPY(x) err |= __get_user(regs->x, &sc->x)
	COPY(d1); COPY(d2); COPY(d3);
	COPY(a0); COPY(a1); COPY(a2); COPY(a3);
	COPY(e0); COPY(e1); COPY(e2); COPY(e3);
	COPY(e4); COPY(e5); COPY(e6); COPY(e7);
	COPY(lar); COPY(lir);
	COPY(mdr); COPY(mdrq);
	COPY(mcvf); COPY(mcrl); COPY(mcrh);
	COPY(sp); COPY(pc);
#undef COPY

	{
		unsigned int tmpflags;
#ifndef CONFIG_MN10300_USING_JTAG
#define USER_EPSW (EPSW_FLAG_Z | EPSW_FLAG_N | EPSW_FLAG_C | EPSW_FLAG_V | \
		   EPSW_T | EPSW_nAR)
#else
#define USER_EPSW (EPSW_FLAG_Z | EPSW_FLAG_N | EPSW_FLAG_C | EPSW_FLAG_V | \
		   EPSW_nAR)
#endif
		err |= __get_user(tmpflags, &sc->epsw);
		regs->epsw = (regs->epsw & ~USER_EPSW) |
		  (tmpflags & USER_EPSW);
		regs->orig_d0 = -1;		/* disable syscall checks */
	}

	{
		struct fpucontext *buf;
		err |= __get_user(buf, &sc->fpucontext);
		if (buf) {
			if (verify_area(VERIFY_READ, buf, sizeof(*buf)))
				goto badframe;
			err |= fpu_restore_sigcontext(buf);
		}
	}

	err |= __get_user(*_d0, &sc->d0);
	return err;

badframe:
	return 1;
}

/*
 * standard signal return syscall
 */
asmlinkage long sys_sigreturn(void)
{
	struct sigframe __user *frame;
	sigset_t set;
	long d0;

	frame = (struct sigframe __user *) current_frame()->sp;
	if (verify_area(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__get_user(set.sig[0], &frame->sc.oldmask))
		goto badframe;

	if (_NSIG_WORDS > 1 &&
	    __copy_from_user(&set.sig[1], &frame->extramask,
			     sizeof(frame->extramask)))
		goto badframe;

	set_current_blocked(&set);

	if (restore_sigcontext(current_frame(), &frame->sc, &d0))
		goto badframe;

	return d0;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

/*
 * realtime signal return syscall
 */
asmlinkage long sys_rt_sigreturn(void)
{
	struct rt_sigframe __user *frame;
	sigset_t set;
	long d0;

	frame = (struct rt_sigframe __user *) current_frame()->sp;
	if (verify_area(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	set_current_blocked(&set);

	if (restore_sigcontext(current_frame(), &frame->uc.uc_mcontext, &d0))
		goto badframe;

	if (restore_altstack(&frame->uc.uc_stack))
		goto badframe;

	return d0;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

/*
 * store the userspace context into a signal frame
 */
static int setup_sigcontext(struct sigcontext __user *sc,
			    struct fpucontext *fpuctx,
			    struct pt_regs *regs,
			    unsigned long mask)
{
	int tmp, err = 0;

#define COPY(x) err |= __put_user(regs->x, &sc->x)
	COPY(d0); COPY(d1); COPY(d2); COPY(d3);
	COPY(a0); COPY(a1); COPY(a2); COPY(a3);
	COPY(e0); COPY(e1); COPY(e2); COPY(e3);
	COPY(e4); COPY(e5); COPY(e6); COPY(e7);
	COPY(lar); COPY(lir);
	COPY(mdr); COPY(mdrq);
	COPY(mcvf); COPY(mcrl); COPY(mcrh);
	COPY(sp); COPY(epsw); COPY(pc);
#undef COPY

	tmp = fpu_setup_sigcontext(fpuctx);
	if (tmp < 0)
		err = 1;
	else
		err |= __put_user(tmp ? fpuctx : NULL, &sc->fpucontext);

	/* non-iBCS2 extensions.. */
	err |= __put_user(mask, &sc->oldmask);

	return err;
}

/*
 * determine which stack to use..
 */
static inline void __user *get_sigframe(struct k_sigaction *ka,
					struct pt_regs *regs,
					size_t frame_size)
{
	unsigned long sp;

	/* default to using normal stack */
	sp = regs->sp;

	/* this is the X/Open sanctioned signal stack switching.  */
	if (ka->sa.sa_flags & SA_ONSTACK) {
		if (sas_ss_flags(sp) == 0)
			sp = current->sas_ss_sp + current->sas_ss_size;
	}

	return (void __user *) ((sp - frame_size) & ~7UL);
}

/*
 * set up a normal signal frame
 */
static int setup_frame(int sig, struct k_sigaction *ka, sigset_t *set,
		       struct pt_regs *regs)
{
	struct sigframe __user *frame;
	int rsig;

	frame = get_sigframe(ka, regs, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	rsig = sig;
	if (sig < 32 &&
	    current_thread_info()->exec_domain &&
	    current_thread_info()->exec_domain->signal_invmap)
		rsig = current_thread_info()->exec_domain->signal_invmap[sig];

	if (__put_user(rsig, &frame->sig) < 0 ||
	    __put_user(&frame->sc, &frame->psc) < 0)
		goto give_sigsegv;

	if (setup_sigcontext(&frame->sc, &frame->fpuctx, regs, set->sig[0]))
		goto give_sigsegv;

	if (_NSIG_WORDS > 1) {
		if (__copy_to_user(frame->extramask, &set->sig[1],
				   sizeof(frame->extramask)))
			goto give_sigsegv;
	}

	/* set up to return from userspace.  If provided, use a stub already in
	 * userspace */
	if (ka->sa.sa_flags & SA_RESTORER) {
		if (__put_user(ka->sa.sa_restorer, &frame->pretcode))
			goto give_sigsegv;
	} else {
		if (__put_user((void (*)(void))frame->retcode,
			       &frame->pretcode))
			goto give_sigsegv;
		/* this is mov $,d0; syscall 0 */
		if (__put_user(0x2c, (char *)(frame->retcode + 0)) ||
		    __put_user(__NR_sigreturn, (char *)(frame->retcode + 1)) ||
		    __put_user(0x00, (char *)(frame->retcode + 2)) ||
		    __put_user(0xf0, (char *)(frame->retcode + 3)) ||
		    __put_user(0xe0, (char *)(frame->retcode + 4)))
			goto give_sigsegv;
		flush_icache_range((unsigned long) frame->retcode,
				   (unsigned long) frame->retcode + 5);
	}

	/* set up registers for signal handler */
	regs->sp = (unsigned long) frame;
	regs->pc = (unsigned long) ka->sa.sa_handler;
	regs->d0 = sig;
	regs->d1 = (unsigned long) &frame->sc;

#if DEBUG_SIG
	printk(KERN_DEBUG "SIG deliver %d (%s:%d): sp=%p pc=%lx ra=%p\n",
	       sig, current->comm, current->pid, frame, regs->pc,
	       frame->pretcode);
#endif

	return 0;

give_sigsegv:
	force_sigsegv(sig, current);
	return -EFAULT;
}

/*
 * set up a realtime signal frame
 */
static int setup_rt_frame(int sig, struct k_sigaction *ka, siginfo_t *info,
			  sigset_t *set, struct pt_regs *regs)
{
	struct rt_sigframe __user *frame;
	int rsig;

	frame = get_sigframe(ka, regs, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	rsig = sig;
	if (sig < 32 &&
	    current_thread_info()->exec_domain &&
	    current_thread_info()->exec_domain->signal_invmap)
		rsig = current_thread_info()->exec_domain->signal_invmap[sig];

	if (__put_user(rsig, &frame->sig) ||
	    __put_user(&frame->info, &frame->pinfo) ||
	    __put_user(&frame->uc, &frame->puc) ||
	    copy_siginfo_to_user(&frame->info, info))
		goto give_sigsegv;

	/* create the ucontext.  */
	if (__put_user(0, &frame->uc.uc_flags) ||
	    __put_user(0, &frame->uc.uc_link) ||
	    __save_altstack(&frame->uc.uc_stack, regs->sp) ||
	    setup_sigcontext(&frame->uc.uc_mcontext,
			     &frame->fpuctx, regs, set->sig[0]) ||
	    __copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set)))
		goto give_sigsegv;

	/* set up to return from userspace.  If provided, use a stub already in
	 * userspace */
	if (ka->sa.sa_flags & SA_RESTORER) {
		if (__put_user(ka->sa.sa_restorer, &frame->pretcode))
			goto give_sigsegv;
	} else {
		if (__put_user((void(*)(void))frame->retcode,
			       &frame->pretcode) ||
		    /* This is mov $,d0; syscall 0 */
		    __put_user(0x2c, (char *)(frame->retcode + 0)) ||
		    __put_user(__NR_rt_sigreturn,
			       (char *)(frame->retcode + 1)) ||
		    __put_user(0x00, (char *)(frame->retcode + 2)) ||
		    __put_user(0xf0, (char *)(frame->retcode + 3)) ||
		    __put_user(0xe0, (char *)(frame->retcode + 4)))
			goto give_sigsegv;

		flush_icache_range((u_long) frame->retcode,
				   (u_long) frame->retcode + 5);
	}

	/* Set up registers for signal handler */
	regs->sp = (unsigned long) frame;
	regs->pc = (unsigned long) ka->sa.sa_handler;
	regs->d0 = sig;
	regs->d1 = (long) &frame->info;

#if DEBUG_SIG
	printk(KERN_DEBUG "SIG deliver %d (%s:%d): sp=%p pc=%lx ra=%p\n",
	       sig, current->comm, current->pid, frame, regs->pc,
	       frame->pretcode);
#endif

	return 0;

give_sigsegv:
	force_sigsegv(sig, current);
	return -EFAULT;
}

static inline void stepback(struct pt_regs *regs)
{
	regs->pc -= 2;
	regs->orig_d0 = -1;
}

/*
 * handle the actual delivery of a signal to userspace
 */
static int handle_signal(int sig,
			 siginfo_t *info, struct k_sigaction *ka,
			 struct pt_regs *regs)
{
	sigset_t *oldset = sigmask_to_save();
	int ret;

	/* Are we from a system call? */
	if (regs->orig_d0 >= 0) {
		/* If so, check system call restarting.. */
		switch (regs->d0) {
		case -ERESTART_RESTARTBLOCK:
		case -ERESTARTNOHAND:
			regs->d0 = -EINTR;
			break;

		case -ERESTARTSYS:
			if (!(ka->sa.sa_flags & SA_RESTART)) {
				regs->d0 = -EINTR;
				break;
			}

			/* fallthrough */
		case -ERESTARTNOINTR:
			regs->d0 = regs->orig_d0;
			stepback(regs);
		}
	}

	/* Set up the stack frame */
	if (ka->sa.sa_flags & SA_SIGINFO)
		ret = setup_rt_frame(sig, ka, info, oldset, regs);
	else
		ret = setup_frame(sig, ka, oldset, regs);
	if (ret)
		return ret;

	signal_delivered(sig, info, ka, regs,
			 test_thread_flag(TIF_SINGLESTEP));
	return 0;
}

/*
 * handle a potential signal
 */
static void do_signal(struct pt_regs *regs)
{
	struct k_sigaction ka;
	siginfo_t info;
	int signr;

	signr = get_signal_to_deliver(&info, &ka, regs, NULL);
	if (signr > 0) {
		if (handle_signal(signr, &info, &ka, regs) == 0) {
		}

		return;
	}

	/* did we come from a system call? */
	if (regs->orig_d0 >= 0) {
		/* restart the system call - no handlers present */
		switch (regs->d0) {
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			regs->d0 = regs->orig_d0;
			stepback(regs);
			break;

		case -ERESTART_RESTARTBLOCK:
			regs->d0 = __NR_restart_syscall;
			stepback(regs);
			break;
		}
	}

	/* if there's no signal to deliver, we just put the saved sigmask
	 * back */
	restore_saved_sigmask();
}

/*
 * notification of userspace execution resumption
 * - triggered by current->work.notify_resume
 */
asmlinkage void do_notify_resume(struct pt_regs *regs, u32 thread_info_flags)
{
	/* Pending single-step? */
	if (thread_info_flags & _TIF_SINGLESTEP) {
#ifndef CONFIG_MN10300_USING_JTAG
		regs->epsw |= EPSW_T;
		clear_thread_flag(TIF_SINGLESTEP);
#else
		BUG(); /* no h/w single-step if using JTAG unit */
#endif
	}

	/* deal with pending signal delivery */
	if (thread_info_flags & _TIF_SIGPENDING)
		do_signal(regs);

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(current_frame());
	}
}
