/*
 * tracing clocks
 *
 *  Copyright (C) 2009 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 * Implements 3 trace clock variants, with differing scalability/precision
 * tradeoffs:
 *
 *  -   local: CPU-local trace clock
 *  -  medium: scalable global clock with some jitter
 *  -  global: globally monotonic, serialized clock
 *
 * Tracer plugins will chose a default from these clocks.
 */
#include <fikus/spinlock.h>
#include <fikus/irqflags.h>
#include <fikus/hardirq.h>
#include <fikus/module.h>
#include <fikus/percpu.h>
#include <fikus/sched.h>
#include <fikus/ktime.h>
#include <fikus/trace_clock.h>

/*
 * trace_clock_local(): the simplest and least coherent tracing clock.
 *
 * Useful for tracing that does not cross to other CPUs nor
 * does it go through idle events.
 */
u64 notrace trace_clock_local(void)
{
	u64 clock;

	/*
	 * sched_clock() is an architecture implemented, fast, scalable,
	 * lockless clock. It is not guaranteed to be coherent across
	 * CPUs, nor across CPU idle events.
	 */
	preempt_disable_notrace();
	clock = sched_clock();
	preempt_enable_notrace();

	return clock;
}
EXPORT_SYMBOL_GPL(trace_clock_local);

/*
 * trace_clock(): 'between' trace clock. Not completely serialized,
 * but not completely incorrect when crossing CPUs either.
 *
 * This is based on cpu_clock(), which will allow at most ~1 jiffy of
 * jitter between CPUs. So it's a pretty scalable clock, but there
 * can be offsets in the trace data.
 */
u64 notrace trace_clock(void)
{
	return local_clock();
}

/*
 * trace_jiffy_clock(): Simply use jiffies as a clock counter.
 */
u64 notrace trace_clock_jiffies(void)
{
	u64 jiffy = jiffies - INITIAL_JIFFIES;

	/* Return nsecs */
	return (u64)jiffies_to_usecs(jiffy) * 1000ULL;
}

/*
 * trace_clock_global(): special globally coherent trace clock
 *
 * It has higher overhead than the other trace clocks but is still
 * an order of magnitude faster than GTOD derived hardware clocks.
 *
 * Used by plugins that need globally coherent timestamps.
 */

/* keep prev_time and lock in the same cacheline. */
static struct {
	u64 prev_time;
	arch_spinlock_t lock;
} trace_clock_struct ____cacheline_aligned_in_smp =
	{
		.lock = (arch_spinlock_t)__ARCH_SPIN_LOCK_UNLOCKED,
	};

u64 notrace trace_clock_global(void)
{
	unsigned long flags;
	int this_cpu;
	u64 now;

	local_irq_save(flags);

	this_cpu = raw_smp_processor_id();
	now = sched_clock_cpu(this_cpu);
	/*
	 * If in an NMI context then dont risk lockups and return the
	 * cpu_clock() time:
	 */
	if (unlikely(in_nmi()))
		goto out;

	arch_spin_lock(&trace_clock_struct.lock);

	/*
	 * TODO: if this happens often then maybe we should reset
	 * my_scd->clock to prev_time+1, to make sure
	 * we start ticking with the local clock from now on?
	 */
	if ((s64)(now - trace_clock_struct.prev_time) < 0)
		now = trace_clock_struct.prev_time + 1;

	trace_clock_struct.prev_time = now;

	arch_spin_unlock(&trace_clock_struct.lock);

 out:
	local_irq_restore(flags);

	return now;
}

static atomic64_t trace_counter;

/*
 * trace_clock_counter(): simply an atomic counter.
 * Use the trace_counter "counter" for cases where you do not care
 * about timings, but are interested in strict ordering.
 */
u64 notrace trace_clock_counter(void)
{
	return atomic64_add_return(1, &trace_counter);
}
