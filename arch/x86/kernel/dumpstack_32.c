/*
 *  Copyright (C) 1991, 1992  John Torvalds
 *  Copyright (C) 2000, 2001, 2002 Andi Kleen, SuSE Labs
 */
#include <fikus/kallsyms.h>
#include <fikus/kprobes.h>
#include <fikus/uaccess.h>
#include <fikus/hardirq.h>
#include <fikus/kdebug.h>
#include <fikus/module.h>
#include <fikus/ptrace.h>
#include <fikus/kexec.h>
#include <fikus/sysfs.h>
#include <fikus/bug.h>
#include <fikus/nmi.h>

#include <asm/stacktrace.h>


void dump_trace(struct task_struct *task, struct pt_regs *regs,
		unsigned long *stack, unsigned long bp,
		const struct stacktrace_ops *ops, void *data)
{
	int graph = 0;

	if (!task)
		task = current;

	if (!stack) {
		unsigned long dummy;

		stack = &dummy;
		if (task && task != current)
			stack = (unsigned long *)task->thread.sp;
	}

	if (!bp)
		bp = stack_frame(task, regs);

	for (;;) {
		struct thread_info *context;

		context = (struct thread_info *)
			((unsigned long)stack & (~(THREAD_SIZE - 1)));
		bp = ops->walk_stack(context, stack, bp, ops, data, NULL, &graph);

		stack = (unsigned long *)context->previous_esp;
		if (!stack)
			break;
		if (ops->stack(data, "IRQ") < 0)
			break;
		touch_nmi_watchdog();
	}
}
EXPORT_SYMBOL(dump_trace);

void
show_stack_log_lvl(struct task_struct *task, struct pt_regs *regs,
		   unsigned long *sp, unsigned long bp, char *log_lvl)
{
	unsigned long *stack;
	int i;

	if (sp == NULL) {
		if (task)
			sp = (unsigned long *)task->thread.sp;
		else
			sp = (unsigned long *)&sp;
	}

	stack = sp;
	for (i = 0; i < kstack_depth_to_print; i++) {
		if (kstack_end(stack))
			break;
		if (i && ((i % STACKSLOTS_PER_LINE) == 0))
			pr_cont("\n");
		pr_cont(" %08lx", *stack++);
		touch_nmi_watchdog();
	}
	pr_cont("\n");
	show_trace_log_lvl(task, regs, sp, bp, log_lvl);
}


void show_regs(struct pt_regs *regs)
{
	int i;

	show_regs_print_info(KERN_EMERG);
	__show_regs(regs, !user_mode_vm(regs));

	/*
	 * When in-kernel, we also print out the stack and code at the
	 * time of the fault..
	 */
	if (!user_mode_vm(regs)) {
		unsigned int code_prologue = code_bytes * 43 / 64;
		unsigned int code_len = code_bytes;
		unsigned char c;
		u8 *ip;

		pr_emerg("Stack:\n");
		show_stack_log_lvl(NULL, regs, &regs->sp, 0, KERN_EMERG);

		pr_emerg("Code:");

		ip = (u8 *)regs->ip - code_prologue;
		if (ip < (u8 *)PAGE_OFFSET || probe_kernel_address(ip, c)) {
			/* try starting at IP */
			ip = (u8 *)regs->ip;
			code_len = code_len - code_prologue + 1;
		}
		for (i = 0; i < code_len; i++, ip++) {
			if (ip < (u8 *)PAGE_OFFSET ||
					probe_kernel_address(ip, c)) {
				pr_cont("  Bad EIP value.");
				break;
			}
			if (ip == (u8 *)regs->ip)
				pr_cont(" <%02x>", c);
			else
				pr_cont(" %02x", c);
		}
	}
	pr_cont("\n");
}

int is_valid_bugaddr(unsigned long ip)
{
	unsigned short ud2;

	if (ip < PAGE_OFFSET)
		return 0;
	if (probe_kernel_address((unsigned short *)ip, ud2))
		return 0;

	return ud2 == 0x0b0f;
}
