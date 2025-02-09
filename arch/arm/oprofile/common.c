/**
 * @file common.c
 *
 * @remark Copyright 2004 Oprofile Authors
 * @remark Copyright 2010 ARM Ltd.
 * @remark Read the file COPYING
 *
 * @author Zwane Mwaikambo
 * @author Will Deacon [move to perf]
 */

#include <fikus/cpumask.h>
#include <fikus/init.h>
#include <fikus/mutex.h>
#include <fikus/oprofile.h>
#include <fikus/perf_event.h>
#include <fikus/platform_device.h>
#include <fikus/slab.h>
#include <asm/stacktrace.h>
#include <fikus/uaccess.h>

#include <asm/perf_event.h>
#include <asm/ptrace.h>

#ifdef CONFIG_HW_PERF_EVENTS

/*
 * OProfile has a curious naming scheme for the ARM PMUs, but they are
 * part of the user ABI so we need to map from the perf PMU name for
 * supported PMUs.
 */
static struct op_perf_name {
	char *perf_name;
	char *op_name;
} op_perf_name_map[] = {
	{ "xscale1",		"arm/xscale1"	},
	{ "xscale1",		"arm/xscale2"	},
	{ "v6",			"arm/armv6"	},
	{ "v6mpcore",		"arm/mpcore"	},
	{ "ARMv7 Cortex-A8",	"arm/armv7"	},
	{ "ARMv7 Cortex-A9",	"arm/armv7-ca9"	},
};

char *op_name_from_perf_id(void)
{
	int i;
	struct op_perf_name names;
	const char *perf_name = perf_pmu_name();

	for (i = 0; i < ARRAY_SIZE(op_perf_name_map); ++i) {
		names = op_perf_name_map[i];
		if (!strcmp(names.perf_name, perf_name))
			return names.op_name;
	}

	return NULL;
}
#endif

static int report_trace(struct stackframe *frame, void *d)
{
	unsigned int *depth = d;

	if (*depth) {
		oprofile_add_trace(frame->pc);
		(*depth)--;
	}

	return *depth == 0;
}

/*
 * The registers we're interested in are at the end of the variable
 * length saved register structure. The fp points at the end of this
 * structure so the address of this struct is:
 * (struct frame_tail *)(xxx->fp)-1
 */
struct frame_tail {
	struct frame_tail *fp;
	unsigned long sp;
	unsigned long lr;
} __attribute__((packed));

static struct frame_tail* user_backtrace(struct frame_tail *tail)
{
	struct frame_tail buftail[2];

	/* Also check accessibility of one struct frame_tail beyond */
	if (!access_ok(VERIFY_READ, tail, sizeof(buftail)))
		return NULL;
	if (__copy_from_user_inatomic(buftail, tail, sizeof(buftail)))
		return NULL;

	oprofile_add_trace(buftail[0].lr);

	/* frame pointers should strictly progress back up the stack
	 * (towards higher addresses) */
	if (tail + 1 >= buftail[0].fp)
		return NULL;

	return buftail[0].fp-1;
}

static void arm_backtrace(struct pt_regs * const regs, unsigned int depth)
{
	struct frame_tail *tail = ((struct frame_tail *) regs->ARM_fp) - 1;

	if (!user_mode(regs)) {
		struct stackframe frame;
		frame.fp = regs->ARM_fp;
		frame.sp = regs->ARM_sp;
		frame.lr = regs->ARM_lr;
		frame.pc = regs->ARM_pc;
		walk_stackframe(&frame, report_trace, &depth);
		return;
	}

	while (depth-- && tail && !((unsigned long) tail & 3))
		tail = user_backtrace(tail);
}

int __init oprofile_arch_init(struct oprofile_operations *ops)
{
	/* provide backtrace support also in timer mode: */
	ops->backtrace		= arm_backtrace;

	return oprofile_perf_init(ops);
}

void oprofile_arch_exit(void)
{
	oprofile_perf_exit();
}
