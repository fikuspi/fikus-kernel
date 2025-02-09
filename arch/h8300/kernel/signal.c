/*
 *  fikus/arch/h8300/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  John Torvalds
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

/*
 * uCfikus H8/300 support by Yoshinori Sato <ysato@users.sourceforge.jp>
 *                and David McCullough <davidm@snapgear.com>
 *
 * Based on
 * Fikus/m68k by Hamish Macdonald
 */

/*
 * ++roman (07/09/96): implemented signal stacks (specially for tosemu on
 * Atari :-) Current limitation: Only one sigstack can be active at one time.
 * If a second signal with SA_ONSTACK set arrives while working on a sigstack,
 * SA_ONSTACK is ignored. This behaviour avoids lots of trouble with nested
 * signal handlers!
 */

#include <fikus/sched.h>
#include <fikus/mm.h>
#include <fikus/kernel.h>
#include <fikus/signal.h>
#include <fikus/syscalls.h>
#include <fikus/errno.h>
#include <fikus/wait.h>
#include <fikus/ptrace.h>
#include <fikus/unistd.h>
#include <fikus/stddef.h>
#include <fikus/highuid.h>
#include <fikus/personality.h>
#include <fikus/tty.h>
#include <fikus/binfmts.h>
#include <fikus/tracehook.h>

#include <asm/setup.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/traps.h>
#include <asm/ucontext.h>

/*
 * Do a signal return; undo the signal stack.
 *
 * Keep the return code on the stack quadword aligned!
 * That makes the cache flush below easier.
 */

struct sigframe
{
	long dummy_er0;
	long dummy_vector;
#if defined(CONFIG_CPU_H8S)
	short dummy_exr;
#endif
	long dummy_pc;
	char *pretcode;
	unsigned char retcode[8];
	unsigned long extramask[_NSIG_WORDS-1];
	struct sigcontext sc;
	int sig;
} __attribute__((aligned(2),packed));

struct rt_sigframe
{
	long dummy_er0;
	long dummy_vector;
#if defined(CONFIG_CPU_H8S)
	short dummy_exr;
#endif
	long dummy_pc;
	char *pretcode;
	struct siginfo *pinfo;
	void *puc;
	unsigned char retcode[8];
	struct siginfo info;
	struct ucontext uc;
	int sig;
} __attribute__((aligned(2),packed));

static inline int
restore_sigcontext(struct sigcontext *usc, int *pd0)
{
	struct pt_regs *regs = current_pt_regs();
	int err = 0;
	unsigned int ccr;
	unsigned int usp;
	unsigned int er0;

	/* Always make any pending restarted system calls return -EINTR */
	current_thread_info()->restart_block.fn = do_no_restart_syscall;

#define COPY(r) err |= __get_user(regs->r, &usc->sc_##r)    /* restore passed registers */
	COPY(er1);
	COPY(er2);
	COPY(er3);
	COPY(er5);
	COPY(pc);
	ccr = regs->ccr & 0x10;
	COPY(ccr);
#undef COPY
	regs->ccr &= 0xef;
	regs->ccr |= ccr;
	regs->orig_er0 = -1;		/* disable syscall checks */
	err |= __get_user(usp, &usc->sc_usp);
	wrusp(usp);

	err |= __get_user(er0, &usc->sc_er0);
	*pd0 = er0;
	return err;
}

asmlinkage int sys_sigreturn(void)
{
	unsigned long usp = rdusp();
	struct sigframe *frame = (struct sigframe *)(usp - 4);
	sigset_t set;
	int er0;

	if (!access_ok(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__get_user(set.sig[0], &frame->sc.sc_mask) ||
	    (_NSIG_WORDS > 1 &&
	     __copy_from_user(&set.sig[1], &frame->extramask,
			      sizeof(frame->extramask))))
		goto badframe;

	set_current_blocked(&set);
	
	if (restore_sigcontext(&frame->sc, &er0))
		goto badframe;
	return er0;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

asmlinkage int sys_rt_sigreturn(void)
{
	unsigned long usp = rdusp();
	struct rt_sigframe *frame = (struct rt_sigframe *)(usp - 4);
	sigset_t set;
	int er0;

	if (!access_ok(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	set_current_blocked(&set);
	
	if (restore_sigcontext(&frame->uc.uc_mcontext, &er0))
		goto badframe;

	if (restore_altstack(&frame->uc.uc_stack))
		goto badframe;

	return er0;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

static int setup_sigcontext(struct sigcontext __user *sc, struct pt_regs *regs,
			     unsigned long mask)
{
	int err = 0;

	err |= __put_user(regs->er0, &sc->sc_er0);
	err |= __put_user(regs->er1, &sc->sc_er1);
	err |= __put_user(regs->er2, &sc->sc_er2);
	err |= __put_user(regs->er3, &sc->sc_er3);
	err |= __put_user(regs->er4, &sc->sc_er4);
	err |= __put_user(regs->er5, &sc->sc_er5);
	err |= __put_user(regs->er6, &sc->sc_er6);
	err |= __put_user(rdusp(),   &sc->sc_usp);
	err |= __put_user(regs->pc,  &sc->sc_pc);
	err |= __put_user(regs->ccr, &sc->sc_ccr);
	err |= __put_user(mask,      &sc->sc_mask);

	return err;
}

static inline void *
get_sigframe(struct k_sigaction *ka, struct pt_regs *regs, size_t frame_size)
{
	unsigned long usp;

	/* Default to using normal stack.  */
	usp = rdusp();

	/* This is the X/Open sanctioned signal stack switching.  */
	if (ka->sa.sa_flags & SA_ONSTACK) {
		if (!sas_ss_flags(usp))
			usp = current->sas_ss_sp + current->sas_ss_size;
	}
	return (void *)((usp - frame_size) & -8UL);
}

static int setup_frame (int sig, struct k_sigaction *ka,
			 sigset_t *set, struct pt_regs *regs)
{
	struct sigframe *frame;
	int err = 0;
	int usig;
	unsigned char *ret;

	frame = get_sigframe(ka, regs, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	usig = current_thread_info()->exec_domain
		&& current_thread_info()->exec_domain->signal_invmap
		&& sig < 32
		? current_thread_info()->exec_domain->signal_invmap[sig]
		: sig;

	err |= __put_user(usig, &frame->sig);
	if (err)
		goto give_sigsegv;

	err |= setup_sigcontext(&frame->sc, regs, set->sig[0]);
	if (err)
		goto give_sigsegv;

	if (_NSIG_WORDS > 1) {
		err |= copy_to_user(frame->extramask, &set->sig[1],
				    sizeof(frame->extramask));
		if (err)
			goto give_sigsegv;
	}

	ret = frame->retcode;
	if (ka->sa.sa_flags & SA_RESTORER)
		ret = (unsigned char *)(ka->sa.sa_restorer);
	else {
		/* sub.l er0,er0; mov.b #__NR_sigreturn,r0l; trapa #0 */
		err |= __put_user(0x1a80f800 + (__NR_sigreturn & 0xff),
				  (unsigned long *)(frame->retcode + 0));
		err |= __put_user(0x5700, (unsigned short *)(frame->retcode + 4));
	}

	/* Set up to return from userspace.  */
	err |= __put_user(ret, &frame->pretcode);

	if (err)
		goto give_sigsegv;

	/* Set up registers for signal handler */
	wrusp ((unsigned long) frame);
	regs->pc = (unsigned long) ka->sa.sa_handler;
	regs->er0 = (current_thread_info()->exec_domain
			   && current_thread_info()->exec_domain->signal_invmap
			   && sig < 32
			   ? current_thread_info()->exec_domain->signal_invmap[sig]
		          : sig);
	regs->er1 = (unsigned long)&(frame->sc);
	regs->er5 = current->mm->start_data;	/* GOT base */

	return 0;

give_sigsegv:
	force_sigsegv(sig, current);
	return -EFAULT;
}

static int setup_rt_frame (int sig, struct k_sigaction *ka, siginfo_t *info,
			    sigset_t *set, struct pt_regs *regs)
{
	struct rt_sigframe *frame;
	int err = 0;
	int usig;
	unsigned char *ret;

	frame = get_sigframe(ka, regs, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	usig = current_thread_info()->exec_domain
		&& current_thread_info()->exec_domain->signal_invmap
		&& sig < 32
		? current_thread_info()->exec_domain->signal_invmap[sig]
		: sig;

	err |= __put_user(usig, &frame->sig);
	if (err)
		goto give_sigsegv;

	err |= __put_user(&frame->info, &frame->pinfo);
	err |= __put_user(&frame->uc, &frame->puc);
	err |= copy_siginfo_to_user(&frame->info, info);
	if (err)
		goto give_sigsegv;

	/* Create the ucontext.  */
	err |= __put_user(0, &frame->uc.uc_flags);
	err |= __put_user(0, &frame->uc.uc_link);
	err |= __save_altstack(&frame->uc.uc_stack, rdusp());
	err |= setup_sigcontext(&frame->uc.uc_mcontext, regs, set->sig[0]);
	err |= copy_to_user (&frame->uc.uc_sigmask, set, sizeof(*set));
	if (err)
		goto give_sigsegv;

	/* Set up to return from userspace.  */
	ret = frame->retcode;
	if (ka->sa.sa_flags & SA_RESTORER)
		ret = (unsigned char *)(ka->sa.sa_restorer);
	else {
		/* sub.l er0,er0; mov.b #__NR_sigreturn,r0l; trapa #0 */
		err |= __put_user(0x1a80f800 + (__NR_sigreturn & 0xff),
				  (unsigned long *)(frame->retcode + 0));
		err |= __put_user(0x5700, (unsigned short *)(frame->retcode + 4));
	}
	err |= __put_user(ret, &frame->pretcode);

	if (err)
		goto give_sigsegv;

	/* Set up registers for signal handler */
	wrusp ((unsigned long) frame);
	regs->pc  = (unsigned long) ka->sa.sa_handler;
	regs->er0 = (current_thread_info()->exec_domain
		     && current_thread_info()->exec_domain->signal_invmap
		     && sig < 32
		     ? current_thread_info()->exec_domain->signal_invmap[sig]
		     : sig);
	regs->er1 = (unsigned long)&(frame->info);
	regs->er2 = (unsigned long)&frame->uc;
	regs->er5 = current->mm->start_data;	/* GOT base */

	return 0;

give_sigsegv:
	force_sigsegv(sig, current);
	return -EFAULT;
}

/*
 * OK, we're invoking a handler
 */
static void
handle_signal(unsigned long sig, siginfo_t *info, struct k_sigaction *ka,
	      struct pt_regs * regs)
{
	sigset_t *oldset = sigmask_to_save();
	int ret;
	/* are we from a system call? */
	if (regs->orig_er0 >= 0) {
		switch (regs->er0) {
		        case -ERESTART_RESTARTBLOCK:
			case -ERESTARTNOHAND:
				regs->er0 = -EINTR;
				break;

			case -ERESTARTSYS:
				if (!(ka->sa.sa_flags & SA_RESTART)) {
					regs->er0 = -EINTR;
					break;
				}
			/* fallthrough */
			case -ERESTARTNOINTR:
				regs->er0 = regs->orig_er0;
				regs->pc -= 2;
		}
	}

	/* set up the stack frame */
	if (ka->sa.sa_flags & SA_SIGINFO)
		ret = setup_rt_frame(sig, ka, info, oldset, regs);
	else
		ret = setup_frame(sig, ka, oldset, regs);

	if (!ret)
		signal_delivered(sig, info, ka, regs, 0);
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 */
static void do_signal(struct pt_regs *regs)
{
	siginfo_t info;
	int signr;
	struct k_sigaction ka;

	/*
	 * We want the common case to go fast, which
	 * is why we may in certain cases get here from
	 * kernel mode. Just return without doing anything
	 * if so.
	 */
	if ((regs->ccr & 0x10))
		return;

	current->thread.esp0 = (unsigned long) regs;

	signr = get_signal_to_deliver(&info, &ka, regs, NULL);
	if (signr > 0) {
		/* Whee!  Actually deliver the signal.  */
		handle_signal(signr, &info, &ka, regs);
		return;
	}
	/* Did we come from a system call? */
	if (regs->orig_er0 >= 0) {
		/* Restart the system call - no handlers present */
		if (regs->er0 == -ERESTARTNOHAND ||
		    regs->er0 == -ERESTARTSYS ||
		    regs->er0 == -ERESTARTNOINTR) {
			regs->er0 = regs->orig_er0;
			regs->pc -= 2;
		}
		if (regs->er0 == -ERESTART_RESTARTBLOCK){
			regs->er0 = __NR_restart_syscall;
			regs->pc -= 2;
		}
	}

	/* If there's no signal to deliver, we just restore the saved mask.  */
	restore_saved_sigmask();
}

asmlinkage void do_notify_resume(struct pt_regs *regs, u32 thread_info_flags)
{
	if (thread_info_flags & _TIF_SIGPENDING)
		do_signal(regs);

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}
