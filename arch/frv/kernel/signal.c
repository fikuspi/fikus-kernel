/* signal.c: FRV specific bits of signal handling
 *
 * Copyright (C) 2003-5 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 * - Derived from arch/m68k/kernel/signal.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
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
#include <fikus/personality.h>
#include <fikus/tracehook.h>
#include <asm/ucontext.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

#define DEBUG_SIG 0

struct fdpic_func_descriptor {
	unsigned long	text;
	unsigned long	GOT;
};

/*
 * Do a signal return; undo the signal stack.
 */

struct sigframe
{
	__sigrestore_t pretcode;
	int sig;
	struct sigcontext sc;
	unsigned long extramask[_NSIG_WORDS-1];
	uint32_t retcode[2];
};

struct rt_sigframe
{
	__sigrestore_t pretcode;
	int sig;
	struct siginfo __user *pinfo;
	void __user *puc;
	struct siginfo info;
	struct ucontext uc;
	uint32_t retcode[2];
};

static int restore_sigcontext(struct sigcontext __user *sc, int *_gr8)
{
	struct user_context *user = current->thread.user;
	unsigned long tbr, psr;

	/* Always make any pending restarted system calls return -EINTR */
	current_thread_info()->restart_block.fn = do_no_restart_syscall;

	tbr = user->i.tbr;
	psr = user->i.psr;
	if (copy_from_user(user, &sc->sc_context, sizeof(sc->sc_context)))
		goto badframe;
	user->i.tbr = tbr;
	user->i.psr = psr;

	restore_user_regs(user);

	user->i.syscallno = -1;		/* disable syscall checks */

	*_gr8 = user->i.gr[8];
	return 0;

 badframe:
	return 1;
}

asmlinkage int sys_sigreturn(void)
{
	struct sigframe __user *frame = (struct sigframe __user *) __frame->sp;
	sigset_t set;
	int gr8;

	if (!access_ok(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__get_user(set.sig[0], &frame->sc.sc_oldmask))
		goto badframe;

	if (_NSIG_WORDS > 1 &&
	    __copy_from_user(&set.sig[1], &frame->extramask, sizeof(frame->extramask)))
		goto badframe;

	set_current_blocked(&set);

	if (restore_sigcontext(&frame->sc, &gr8))
		goto badframe;
	return gr8;

 badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

asmlinkage int sys_rt_sigreturn(void)
{
	struct rt_sigframe __user *frame = (struct rt_sigframe __user *) __frame->sp;
	sigset_t set;
	int gr8;

	if (!access_ok(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	set_current_blocked(&set);

	if (restore_sigcontext(&frame->uc.uc_mcontext, &gr8))
		goto badframe;

	if (restore_altstack(&frame->uc.uc_stack))
		goto badframe;

	return gr8;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

/*
 * Set up a signal frame
 */
static int setup_sigcontext(struct sigcontext __user *sc, unsigned long mask)
{
	save_user_regs(current->thread.user);

	if (copy_to_user(&sc->sc_context, current->thread.user, sizeof(sc->sc_context)) != 0)
		goto badframe;

	/* non-iBCS2 extensions.. */
	if (__put_user(mask, &sc->sc_oldmask) < 0)
		goto badframe;

	return 0;

 badframe:
	return 1;
}

/*****************************************************************************/
/*
 * Determine which stack to use..
 */
static inline void __user *get_sigframe(struct k_sigaction *ka,
					size_t frame_size)
{
	unsigned long sp;

	/* Default to using normal stack */
	sp = __frame->sp;

	/* This is the X/Open sanctioned signal stack switching.  */
	if (ka->sa.sa_flags & SA_ONSTACK) {
		if (! sas_ss_flags(sp))
			sp = current->sas_ss_sp + current->sas_ss_size;
	}

	return (void __user *) ((sp - frame_size) & ~7UL);

} /* end get_sigframe() */

/*****************************************************************************/
/*
 *
 */
static int setup_frame(int sig, struct k_sigaction *ka, sigset_t *set)
{
	struct sigframe __user *frame;
	int rsig;

	set_fs(USER_DS);

	frame = get_sigframe(ka, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	rsig = sig;
	if (sig < 32 &&
	    __current_thread_info->exec_domain &&
	    __current_thread_info->exec_domain->signal_invmap)
		rsig = __current_thread_info->exec_domain->signal_invmap[sig];

	if (__put_user(rsig, &frame->sig) < 0)
		goto give_sigsegv;

	if (setup_sigcontext(&frame->sc, set->sig[0]))
		goto give_sigsegv;

	if (_NSIG_WORDS > 1) {
		if (__copy_to_user(frame->extramask, &set->sig[1],
				   sizeof(frame->extramask)))
			goto give_sigsegv;
	}

	/* Set up to return from userspace.  If provided, use a stub
	 * already in userspace.  */
	if (ka->sa.sa_flags & SA_RESTORER) {
		if (__put_user(ka->sa.sa_restorer, &frame->pretcode) < 0)
			goto give_sigsegv;
	}
	else {
		/* Set up the following code on the stack:
		 *	setlos	#__NR_sigreturn,gr7
		 *	tira	gr0,0
		 */
		if (__put_user((__sigrestore_t)frame->retcode, &frame->pretcode) ||
		    __put_user(0x8efc0000|__NR_sigreturn, &frame->retcode[0]) ||
		    __put_user(0xc0700000, &frame->retcode[1]))
			goto give_sigsegv;

		flush_icache_range((unsigned long) frame->retcode,
				   (unsigned long) (frame->retcode + 2));
	}

	/* Set up registers for the signal handler */
	if (current->personality & FDPIC_FUNCPTRS) {
		struct fdpic_func_descriptor __user *funcptr =
			(struct fdpic_func_descriptor __user *) ka->sa.sa_handler;
		struct fdpic_func_descriptor desc;
		if (copy_from_user(&desc, funcptr, sizeof(desc)))
			goto give_sigsegv;
		__frame->pc = desc.text;
		__frame->gr15 = desc.GOT;
	} else {
		__frame->pc   = (unsigned long) ka->sa.sa_handler;
		__frame->gr15 = 0;
	}

	__frame->sp   = (unsigned long) frame;
	__frame->lr   = (unsigned long) &frame->retcode;
	__frame->gr8  = sig;

#if DEBUG_SIG
	printk("SIG deliver %d (%s:%d): sp=%p pc=%lx ra=%p\n",
	       sig, current->comm, current->pid, frame, __frame->pc,
	       frame->pretcode);
#endif

	return 0;

give_sigsegv:
	force_sigsegv(sig, current);
	return -EFAULT;

} /* end setup_frame() */

/*****************************************************************************/
/*
 *
 */
static int setup_rt_frame(int sig, struct k_sigaction *ka, siginfo_t *info,
			  sigset_t *set)
{
	struct rt_sigframe __user *frame;
	int rsig;

	set_fs(USER_DS);

	frame = get_sigframe(ka, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	rsig = sig;
	if (sig < 32 &&
	    __current_thread_info->exec_domain &&
	    __current_thread_info->exec_domain->signal_invmap)
		rsig = __current_thread_info->exec_domain->signal_invmap[sig];

	if (__put_user(rsig,		&frame->sig) ||
	    __put_user(&frame->info,	&frame->pinfo) ||
	    __put_user(&frame->uc,	&frame->puc))
		goto give_sigsegv;

	if (copy_siginfo_to_user(&frame->info, info))
		goto give_sigsegv;

	/* Create the ucontext.  */
	if (__put_user(0, &frame->uc.uc_flags) ||
	    __put_user(NULL, &frame->uc.uc_link) ||
	    __save_altstack(&frame->uc.uc_stack, __frame->sp))
		goto give_sigsegv;

	if (setup_sigcontext(&frame->uc.uc_mcontext, set->sig[0]))
		goto give_sigsegv;

	if (__copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set)))
		goto give_sigsegv;

	/* Set up to return from userspace.  If provided, use a stub
	 * already in userspace.  */
	if (ka->sa.sa_flags & SA_RESTORER) {
		if (__put_user(ka->sa.sa_restorer, &frame->pretcode))
			goto give_sigsegv;
	}
	else {
		/* Set up the following code on the stack:
		 *	setlos	#__NR_sigreturn,gr7
		 *	tira	gr0,0
		 */
		if (__put_user((__sigrestore_t)frame->retcode, &frame->pretcode) ||
		    __put_user(0x8efc0000|__NR_rt_sigreturn, &frame->retcode[0]) ||
		    __put_user(0xc0700000, &frame->retcode[1]))
			goto give_sigsegv;

		flush_icache_range((unsigned long) frame->retcode,
				   (unsigned long) (frame->retcode + 2));
	}

	/* Set up registers for signal handler */
	if (current->personality & FDPIC_FUNCPTRS) {
		struct fdpic_func_descriptor __user *funcptr =
			(struct fdpic_func_descriptor __user *) ka->sa.sa_handler;
		struct fdpic_func_descriptor desc;
		if (copy_from_user(&desc, funcptr, sizeof(desc)))
			goto give_sigsegv;
		__frame->pc = desc.text;
		__frame->gr15 = desc.GOT;
	} else {
		__frame->pc   = (unsigned long) ka->sa.sa_handler;
		__frame->gr15 = 0;
	}

	__frame->sp  = (unsigned long) frame;
	__frame->lr  = (unsigned long) &frame->retcode;
	__frame->gr8 = sig;
	__frame->gr9 = (unsigned long) &frame->info;

#if DEBUG_SIG
	printk("SIG deliver %d (%s:%d): sp=%p pc=%lx ra=%p\n",
	       sig, current->comm, current->pid, frame, __frame->pc,
	       frame->pretcode);
#endif

	return 0;

give_sigsegv:
	force_sigsegv(sig, current);
	return -EFAULT;

} /* end setup_rt_frame() */

/*****************************************************************************/
/*
 * OK, we're invoking a handler
 */
static void handle_signal(unsigned long sig, siginfo_t *info,
			 struct k_sigaction *ka)
{
	sigset_t *oldset = sigmask_to_save();
	int ret;

	/* Are we from a system call? */
	if (__frame->syscallno != -1) {
		/* If so, check system call restarting.. */
		switch (__frame->gr8) {
		case -ERESTART_RESTARTBLOCK:
		case -ERESTARTNOHAND:
			__frame->gr8 = -EINTR;
			break;

		case -ERESTARTSYS:
			if (!(ka->sa.sa_flags & SA_RESTART)) {
				__frame->gr8 = -EINTR;
				break;
			}

			/* fallthrough */
		case -ERESTARTNOINTR:
			__frame->gr8 = __frame->orig_gr8;
			__frame->pc -= 4;
		}
		__frame->syscallno = -1;
	}

	/* Set up the stack frame */
	if (ka->sa.sa_flags & SA_SIGINFO)
		ret = setup_rt_frame(sig, ka, info, oldset);
	else
		ret = setup_frame(sig, ka, oldset);

	if (ret)
		return;

	signal_delivered(sig, info, ka, __frame,
				 test_thread_flag(TIF_SINGLESTEP));
} /* end handle_signal() */

/*****************************************************************************/
/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 */
static void do_signal(void)
{
	struct k_sigaction ka;
	siginfo_t info;
	int signr;

	signr = get_signal_to_deliver(&info, &ka, __frame, NULL);
	if (signr > 0) {
		handle_signal(signr, &info, &ka);
		return;
	}

	/* Did we come from a system call? */
	if (__frame->syscallno != -1) {
		/* Restart the system call - no handlers present */
		switch (__frame->gr8) {
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			__frame->gr8 = __frame->orig_gr8;
			__frame->pc -= 4;
			break;

		case -ERESTART_RESTARTBLOCK:
			__frame->gr7 = __NR_restart_syscall;
			__frame->pc -= 4;
			break;
		}
		__frame->syscallno = -1;
	}

	/* if there's no signal to deliver, we just put the saved sigmask
	 * back */
	restore_saved_sigmask();
} /* end do_signal() */

/*****************************************************************************/
/*
 * notification of userspace execution resumption
 * - triggered by the TIF_WORK_MASK flags
 */
asmlinkage void do_notify_resume(__u32 thread_info_flags)
{
	/* pending single-step? */
	if (thread_info_flags & _TIF_SINGLESTEP)
		clear_thread_flag(TIF_SINGLESTEP);

	/* deal with pending signal delivery */
	if (thread_info_flags & _TIF_SIGPENDING)
		do_signal();

	/* deal with notification on about to resume userspace execution */
	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(__frame);
	}

} /* end do_notify_resume() */
