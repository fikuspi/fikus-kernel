/*
 *  fikus/arch/cris/kernel/ptrace.c
 *
 * Parts taken from the m68k port.
 *
 * Copyright (c) 2000, 2001, 2002 Axis Communications AB
 *
 * Authors:   Bjorn Wesen
 *
 */

#include <fikus/kernel.h>
#include <fikus/sched.h>
#include <fikus/mm.h>
#include <fikus/smp.h>
#include <fikus/errno.h>
#include <fikus/ptrace.h>
#include <fikus/user.h>
#include <fikus/tracehook.h>

#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/processor.h>


/* notification of userspace execution resumption
 * - triggered by current->work.notify_resume
 */
extern int do_signal(int canrestart, struct pt_regs *regs);


void do_notify_resume(int canrestart, struct pt_regs *regs,
		      __u32 thread_info_flags)
{
	/* deal with pending signal delivery */
	if (thread_info_flags & _TIF_SIGPENDING)
		do_signal(canrestart,regs);

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}
