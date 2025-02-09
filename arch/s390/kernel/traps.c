/*
 *  S390 version
 *    Copyright IBM Corp. 1999, 2000
 *    Author(s): Martin Schwidefsky (schwidefsky@de.ibm.com),
 *               Denis Joseph Barrow (djbarrow@de.ibm.com,barrow_dj@yahoo.com),
 *
 *  Derived from "arch/i386/kernel/traps.c"
 *    Copyright (C) 1991, 1992 John Torvalds
 */

/*
 * 'Traps.c' handles hardware traps and faults after we have saved some
 * state in 'asm.s'.
 */
#include <fikus/kprobes.h>
#include <fikus/kdebug.h>
#include <fikus/module.h>
#include <fikus/ptrace.h>
#include <fikus/sched.h>
#include <fikus/mm.h>
#include "entry.h"

int show_unhandled_signals = 1;

static inline void __user *get_trap_ip(struct pt_regs *regs)
{
#ifdef CONFIG_64BIT
	unsigned long address;

	if (regs->int_code & 0x200)
		address = *(unsigned long *)(current->thread.trap_tdb + 24);
	else
		address = regs->psw.addr;
	return (void __user *)
		((address - (regs->int_code >> 16)) & PSW_ADDR_INSN);
#else
	return (void __user *)
		((regs->psw.addr - (regs->int_code >> 16)) & PSW_ADDR_INSN);
#endif
}

static inline void report_user_fault(struct pt_regs *regs, int signr)
{
	if ((task_pid_nr(current) > 1) && !show_unhandled_signals)
		return;
	if (!unhandled_signal(current, signr))
		return;
	if (!printk_ratelimit())
		return;
	printk("User process fault: interruption code 0x%X ", regs->int_code);
	print_vma_addr("in ", regs->psw.addr & PSW_ADDR_INSN);
	printk("\n");
	show_regs(regs);
}

int is_valid_bugaddr(unsigned long addr)
{
	return 1;
}

static void __kprobes do_trap(struct pt_regs *regs,
			      int si_signo, int si_code, char *str)
{
	siginfo_t info;

	if (notify_die(DIE_TRAP, str, regs, 0,
		       regs->int_code, si_signo) == NOTIFY_STOP)
		return;

	if (user_mode(regs)) {
		info.si_signo = si_signo;
		info.si_errno = 0;
		info.si_code = si_code;
		info.si_addr = get_trap_ip(regs);
		force_sig_info(si_signo, &info, current);
		report_user_fault(regs, si_signo);
        } else {
                const struct exception_table_entry *fixup;
                fixup = search_exception_tables(regs->psw.addr & PSW_ADDR_INSN);
                if (fixup)
			regs->psw.addr = extable_fixup(fixup) | PSW_ADDR_AMODE;
		else {
			enum bug_trap_type btt;

			btt = report_bug(regs->psw.addr & PSW_ADDR_INSN, regs);
			if (btt == BUG_TRAP_TYPE_WARN)
				return;
			die(regs, str);
		}
        }
}

void __kprobes do_per_trap(struct pt_regs *regs)
{
	siginfo_t info;

	if (notify_die(DIE_SSTEP, "sstep", regs, 0, 0, SIGTRAP) == NOTIFY_STOP)
		return;
	if (!current->ptrace)
		return;
	info.si_signo = SIGTRAP;
	info.si_errno = 0;
	info.si_code = TRAP_HWBKPT;
	info.si_addr =
		(void __force __user *) current->thread.per_event.address;
	force_sig_info(SIGTRAP, &info, current);
}

void default_trap_handler(struct pt_regs *regs)
{
	if (user_mode(regs)) {
		report_user_fault(regs, SIGSEGV);
		do_exit(SIGSEGV);
	} else
		die(regs, "Unknown program exception");
}

#define DO_ERROR_INFO(name, signr, sicode, str) \
void name(struct pt_regs *regs)			\
{						\
	do_trap(regs, signr, sicode, str);	\
}

DO_ERROR_INFO(addressing_exception, SIGILL, ILL_ILLADR,
	      "addressing exception")
DO_ERROR_INFO(execute_exception, SIGILL, ILL_ILLOPN,
	      "execute exception")
DO_ERROR_INFO(divide_exception, SIGFPE, FPE_INTDIV,
	      "fixpoint divide exception")
DO_ERROR_INFO(overflow_exception, SIGFPE, FPE_INTOVF,
	      "fixpoint overflow exception")
DO_ERROR_INFO(hfp_overflow_exception, SIGFPE, FPE_FLTOVF,
	      "HFP overflow exception")
DO_ERROR_INFO(hfp_underflow_exception, SIGFPE, FPE_FLTUND,
	      "HFP underflow exception")
DO_ERROR_INFO(hfp_significance_exception, SIGFPE, FPE_FLTRES,
	      "HFP significance exception")
DO_ERROR_INFO(hfp_divide_exception, SIGFPE, FPE_FLTDIV,
	      "HFP divide exception")
DO_ERROR_INFO(hfp_sqrt_exception, SIGFPE, FPE_FLTINV,
	      "HFP square root exception")
DO_ERROR_INFO(operand_exception, SIGILL, ILL_ILLOPN,
	      "operand exception")
DO_ERROR_INFO(privileged_op, SIGILL, ILL_PRVOPC,
	      "privileged operation")
DO_ERROR_INFO(special_op_exception, SIGILL, ILL_ILLOPN,
	      "special operation exception")
DO_ERROR_INFO(translation_exception, SIGILL, ILL_ILLOPN,
	      "translation exception")

#ifdef CONFIG_64BIT
DO_ERROR_INFO(transaction_exception, SIGILL, ILL_ILLOPN,
	      "transaction constraint exception")
#endif

static inline void do_fp_trap(struct pt_regs *regs, int fpc)
{
	int si_code = 0;
	/* FPC[2] is Data Exception Code */
	if ((fpc & 0x00000300) == 0) {
		/* bits 6 and 7 of DXC are 0 iff IEEE exception */
		if (fpc & 0x8000) /* invalid fp operation */
			si_code = FPE_FLTINV;
		else if (fpc & 0x4000) /* div by 0 */
			si_code = FPE_FLTDIV;
		else if (fpc & 0x2000) /* overflow */
			si_code = FPE_FLTOVF;
		else if (fpc & 0x1000) /* underflow */
			si_code = FPE_FLTUND;
		else if (fpc & 0x0800) /* inexact */
			si_code = FPE_FLTRES;
	}
	do_trap(regs, SIGFPE, si_code, "floating point exception");
}

void __kprobes illegal_op(struct pt_regs *regs)
{
	siginfo_t info;
        __u8 opcode[6];
	__u16 __user *location;
	int signal = 0;

	location = get_trap_ip(regs);

	if (user_mode(regs)) {
		if (get_user(*((__u16 *) opcode), (__u16 __user *) location))
			return;
		if (*((__u16 *) opcode) == S390_BREAKPOINT_U16) {
			if (current->ptrace) {
				info.si_signo = SIGTRAP;
				info.si_errno = 0;
				info.si_code = TRAP_BRKPT;
				info.si_addr = location;
				force_sig_info(SIGTRAP, &info, current);
			} else
				signal = SIGILL;
#ifdef CONFIG_MATHEMU
		} else if (opcode[0] == 0xb3) {
			if (get_user(*((__u16 *) (opcode+2)), location+1))
				return;
			signal = math_emu_b3(opcode, regs);
                } else if (opcode[0] == 0xed) {
			if (get_user(*((__u32 *) (opcode+2)),
				     (__u32 __user *)(location+1)))
				return;
			signal = math_emu_ed(opcode, regs);
		} else if (*((__u16 *) opcode) == 0xb299) {
			if (get_user(*((__u16 *) (opcode+2)), location+1))
				return;
			signal = math_emu_srnm(opcode, regs);
		} else if (*((__u16 *) opcode) == 0xb29c) {
			if (get_user(*((__u16 *) (opcode+2)), location+1))
				return;
			signal = math_emu_stfpc(opcode, regs);
		} else if (*((__u16 *) opcode) == 0xb29d) {
			if (get_user(*((__u16 *) (opcode+2)), location+1))
				return;
			signal = math_emu_lfpc(opcode, regs);
#endif
		} else
			signal = SIGILL;
	} else {
		/*
		 * If we get an illegal op in kernel mode, send it through the
		 * kprobes notifier. If kprobes doesn't pick it up, SIGILL
		 */
		if (notify_die(DIE_BPT, "bpt", regs, 0,
			       3, SIGTRAP) != NOTIFY_STOP)
			signal = SIGILL;
	}

#ifdef CONFIG_MATHEMU
        if (signal == SIGFPE)
		do_fp_trap(regs, current->thread.fp_regs.fpc);
	else if (signal == SIGSEGV)
		do_trap(regs, signal, SEGV_MAPERR, "user address fault");
	else
#endif
	if (signal)
		do_trap(regs, signal, ILL_ILLOPC, "illegal operation");
}


#ifdef CONFIG_MATHEMU
void specification_exception(struct pt_regs *regs)
{
        __u8 opcode[6];
	__u16 __user *location = NULL;
	int signal = 0;

	location = (__u16 __user *) get_trap_ip(regs);

	if (user_mode(regs)) {
		get_user(*((__u16 *) opcode), location);
		switch (opcode[0]) {
		case 0x28: /* LDR Rx,Ry   */
			signal = math_emu_ldr(opcode);
			break;
		case 0x38: /* LER Rx,Ry   */
			signal = math_emu_ler(opcode);
			break;
		case 0x60: /* STD R,D(X,B) */
			get_user(*((__u16 *) (opcode+2)), location+1);
			signal = math_emu_std(opcode, regs);
			break;
		case 0x68: /* LD R,D(X,B) */
			get_user(*((__u16 *) (opcode+2)), location+1);
			signal = math_emu_ld(opcode, regs);
			break;
		case 0x70: /* STE R,D(X,B) */
			get_user(*((__u16 *) (opcode+2)), location+1);
			signal = math_emu_ste(opcode, regs);
			break;
		case 0x78: /* LE R,D(X,B) */
			get_user(*((__u16 *) (opcode+2)), location+1);
			signal = math_emu_le(opcode, regs);
			break;
		default:
			signal = SIGILL;
			break;
                }
        } else
		signal = SIGILL;

        if (signal == SIGFPE)
		do_fp_trap(regs, current->thread.fp_regs.fpc);
	else if (signal)
		do_trap(regs, signal, ILL_ILLOPN, "specification exception");
}
#else
DO_ERROR_INFO(specification_exception, SIGILL, ILL_ILLOPN,
	      "specification exception");
#endif

void data_exception(struct pt_regs *regs)
{
	__u16 __user *location;
	int signal = 0;

	location = get_trap_ip(regs);

	if (MACHINE_HAS_IEEE)
		asm volatile("stfpc %0" : "=m" (current->thread.fp_regs.fpc));

#ifdef CONFIG_MATHEMU
	else if (user_mode(regs)) {
        	__u8 opcode[6];
		get_user(*((__u16 *) opcode), location);
		switch (opcode[0]) {
		case 0x28: /* LDR Rx,Ry   */
			signal = math_emu_ldr(opcode);
			break;
		case 0x38: /* LER Rx,Ry   */
			signal = math_emu_ler(opcode);
			break;
		case 0x60: /* STD R,D(X,B) */
			get_user(*((__u16 *) (opcode+2)), location+1);
			signal = math_emu_std(opcode, regs);
			break;
		case 0x68: /* LD R,D(X,B) */
			get_user(*((__u16 *) (opcode+2)), location+1);
			signal = math_emu_ld(opcode, regs);
			break;
		case 0x70: /* STE R,D(X,B) */
			get_user(*((__u16 *) (opcode+2)), location+1);
			signal = math_emu_ste(opcode, regs);
			break;
		case 0x78: /* LE R,D(X,B) */
			get_user(*((__u16 *) (opcode+2)), location+1);
			signal = math_emu_le(opcode, regs);
			break;
		case 0xb3:
			get_user(*((__u16 *) (opcode+2)), location+1);
			signal = math_emu_b3(opcode, regs);
			break;
                case 0xed:
			get_user(*((__u32 *) (opcode+2)),
				 (__u32 __user *)(location+1));
			signal = math_emu_ed(opcode, regs);
			break;
	        case 0xb2:
			if (opcode[1] == 0x99) {
				get_user(*((__u16 *) (opcode+2)), location+1);
				signal = math_emu_srnm(opcode, regs);
			} else if (opcode[1] == 0x9c) {
				get_user(*((__u16 *) (opcode+2)), location+1);
				signal = math_emu_stfpc(opcode, regs);
			} else if (opcode[1] == 0x9d) {
				get_user(*((__u16 *) (opcode+2)), location+1);
				signal = math_emu_lfpc(opcode, regs);
			} else
				signal = SIGILL;
			break;
		default:
			signal = SIGILL;
			break;
                }
        }
#endif 
	if (current->thread.fp_regs.fpc & FPC_DXC_MASK)
		signal = SIGFPE;
	else
		signal = SIGILL;
        if (signal == SIGFPE)
		do_fp_trap(regs, current->thread.fp_regs.fpc);
	else if (signal)
		do_trap(regs, signal, ILL_ILLOPN, "data exception");
}

void space_switch_exception(struct pt_regs *regs)
{
	/* Set user psw back to home space mode. */
	if (user_mode(regs))
		regs->psw.mask |= PSW_ASC_HOME;
	/* Send SIGILL. */
	do_trap(regs, SIGILL, ILL_PRVOPC, "space switch event");
}

void __kprobes kernel_stack_overflow(struct pt_regs * regs)
{
	bust_spinlocks(1);
	printk("Kernel stack overflow.\n");
	show_regs(regs);
	bust_spinlocks(0);
	panic("Corrupt kernel stack, can't continue.");
}

void __init trap_init(void)
{
	local_mcck_enable();
}
