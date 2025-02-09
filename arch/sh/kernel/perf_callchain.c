/*
 * Performance event callchain support - SuperH architecture code
 *
 * Copyright (C) 2009  Paul Mundt
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <fikus/kernel.h>
#include <fikus/sched.h>
#include <fikus/perf_event.h>
#include <fikus/percpu.h>
#include <asm/unwinder.h>
#include <asm/ptrace.h>

static int callchain_stack(void *data, char *name)
{
	return 0;
}

static void callchain_address(void *data, unsigned long addr, int reliable)
{
	struct perf_callchain_entry *entry = data;

	if (reliable)
		perf_callchain_store(entry, addr);
}

static const struct stacktrace_ops callchain_ops = {
	.stack		= callchain_stack,
	.address	= callchain_address,
};

void
perf_callchain_kernel(struct perf_callchain_entry *entry, struct pt_regs *regs)
{
	perf_callchain_store(entry, regs->pc);

	unwind_stack(NULL, regs, NULL, &callchain_ops, entry);
}
