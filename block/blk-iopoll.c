/*
 * Functions related to interrupt-poll handling in the block layer. This
 * is similar to NAPI for network devices.
 */
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/init.h>
#include <fikus/bio.h>
#include <fikus/blkdev.h>
#include <fikus/interrupt.h>
#include <fikus/cpu.h>
#include <fikus/blk-iopoll.h>
#include <fikus/delay.h>

#include "blk.h"

int blk_iopoll_enabled = 1;
EXPORT_SYMBOL(blk_iopoll_enabled);

static unsigned int blk_iopoll_budget __read_mostly = 256;

static DEFINE_PER_CPU(struct list_head, blk_cpu_iopoll);

/**
 * blk_iopoll_sched - Schedule a run of the iopoll handler
 * @iop:      The parent iopoll structure
 *
 * Description:
 *     Add this blk_iopoll structure to the pending poll list and trigger the
 *     raise of the blk iopoll softirq. The driver must already have gotten a
 *     successful return from blk_iopoll_sched_prep() before calling this.
 **/
void blk_iopoll_sched(struct blk_iopoll *iop)
{
	unsigned long flags;

	local_irq_save(flags);
	list_add_tail(&iop->list, &__get_cpu_var(blk_cpu_iopoll));
	__raise_softirq_irqoff(BLOCK_IOPOLL_SOFTIRQ);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(blk_iopoll_sched);

/**
 * __blk_iopoll_complete - Mark this @iop as un-polled again
 * @iop:      The parent iopoll structure
 *
 * Description:
 *     See blk_iopoll_complete(). This function must be called with interrupts
 *     disabled.
 **/
void __blk_iopoll_complete(struct blk_iopoll *iop)
{
	list_del(&iop->list);
	smp_mb__before_clear_bit();
	clear_bit_unlock(IOPOLL_F_SCHED, &iop->state);
}
EXPORT_SYMBOL(__blk_iopoll_complete);

/**
 * blk_iopoll_complete - Mark this @iop as un-polled again
 * @iop:      The parent iopoll structure
 *
 * Description:
 *     If a driver consumes less than the assigned budget in its run of the
 *     iopoll handler, it'll end the polled mode by calling this function. The
 *     iopoll handler will not be invoked again before blk_iopoll_sched_prep()
 *     is called.
 **/
void blk_iopoll_complete(struct blk_iopoll *iopoll)
{
	unsigned long flags;

	local_irq_save(flags);
	__blk_iopoll_complete(iopoll);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(blk_iopoll_complete);

static void blk_iopoll_softirq(struct softirq_action *h)
{
	struct list_head *list = &__get_cpu_var(blk_cpu_iopoll);
	int rearm = 0, budget = blk_iopoll_budget;
	unsigned long start_time = jiffies;

	local_irq_disable();

	while (!list_empty(list)) {
		struct blk_iopoll *iop;
		int work, weight;

		/*
		 * If softirq window is exhausted then punt.
		 */
		if (budget <= 0 || time_after(jiffies, start_time)) {
			rearm = 1;
			break;
		}

		local_irq_enable();

		/* Even though interrupts have been re-enabled, this
		 * access is safe because interrupts can only add new
		 * entries to the tail of this list, and only ->poll()
		 * calls can remove this head entry from the list.
		 */
		iop = list_entry(list->next, struct blk_iopoll, list);

		weight = iop->weight;
		work = 0;
		if (test_bit(IOPOLL_F_SCHED, &iop->state))
			work = iop->poll(iop, weight);

		budget -= work;

		local_irq_disable();

		/*
		 * Drivers must not modify the iopoll state, if they
		 * consume their assigned weight (or more, some drivers can't
		 * easily just stop processing, they have to complete an
		 * entire mask of commands).In such cases this code
		 * still "owns" the iopoll instance and therefore can
		 * move the instance around on the list at-will.
		 */
		if (work >= weight) {
			if (blk_iopoll_disable_pending(iop))
				__blk_iopoll_complete(iop);
			else
				list_move_tail(&iop->list, list);
		}
	}

	if (rearm)
		__raise_softirq_irqoff(BLOCK_IOPOLL_SOFTIRQ);

	local_irq_enable();
}

/**
 * blk_iopoll_disable - Disable iopoll on this @iop
 * @iop:      The parent iopoll structure
 *
 * Description:
 *     Disable io polling and wait for any pending callbacks to have completed.
 **/
void blk_iopoll_disable(struct blk_iopoll *iop)
{
	set_bit(IOPOLL_F_DISABLE, &iop->state);
	while (test_and_set_bit(IOPOLL_F_SCHED, &iop->state))
		msleep(1);
	clear_bit(IOPOLL_F_DISABLE, &iop->state);
}
EXPORT_SYMBOL(blk_iopoll_disable);

/**
 * blk_iopoll_enable - Enable iopoll on this @iop
 * @iop:      The parent iopoll structure
 *
 * Description:
 *     Enable iopoll on this @iop. Note that the handler run will not be
 *     scheduled, it will only mark it as active.
 **/
void blk_iopoll_enable(struct blk_iopoll *iop)
{
	BUG_ON(!test_bit(IOPOLL_F_SCHED, &iop->state));
	smp_mb__before_clear_bit();
	clear_bit_unlock(IOPOLL_F_SCHED, &iop->state);
}
EXPORT_SYMBOL(blk_iopoll_enable);

/**
 * blk_iopoll_init - Initialize this @iop
 * @iop:      The parent iopoll structure
 * @weight:   The default weight (or command completion budget)
 * @poll_fn:  The handler to invoke
 *
 * Description:
 *     Initialize this blk_iopoll structure. Before being actively used, the
 *     driver must call blk_iopoll_enable().
 **/
void blk_iopoll_init(struct blk_iopoll *iop, int weight, blk_iopoll_fn *poll_fn)
{
	memset(iop, 0, sizeof(*iop));
	INIT_LIST_HEAD(&iop->list);
	iop->weight = weight;
	iop->poll = poll_fn;
	set_bit(IOPOLL_F_SCHED, &iop->state);
}
EXPORT_SYMBOL(blk_iopoll_init);

static int blk_iopoll_cpu_notify(struct notifier_block *self,
				 unsigned long action, void *hcpu)
{
	/*
	 * If a CPU goes away, splice its entries to the current CPU
	 * and trigger a run of the softirq
	 */
	if (action == CPU_DEAD || action == CPU_DEAD_FROZEN) {
		int cpu = (unsigned long) hcpu;

		local_irq_disable();
		list_splice_init(&per_cpu(blk_cpu_iopoll, cpu),
				 &__get_cpu_var(blk_cpu_iopoll));
		__raise_softirq_irqoff(BLOCK_IOPOLL_SOFTIRQ);
		local_irq_enable();
	}

	return NOTIFY_OK;
}

static struct notifier_block blk_iopoll_cpu_notifier = {
	.notifier_call	= blk_iopoll_cpu_notify,
};

static __init int blk_iopoll_setup(void)
{
	int i;

	for_each_possible_cpu(i)
		INIT_LIST_HEAD(&per_cpu(blk_cpu_iopoll, i));

	open_softirq(BLOCK_IOPOLL_SOFTIRQ, blk_iopoll_softirq);
	register_hotcpu_notifier(&blk_iopoll_cpu_notifier);
	return 0;
}
subsys_initcall(blk_iopoll_setup);
