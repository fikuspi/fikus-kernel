/*
 * Driver for s390 eadm subchannels
 *
 * Copyright IBM Corp. 2012
 * Author(s): Sebastian Ott <sebott@fikus.vnet.ibm.com>
 */

#include <fikus/kernel_stat.h>
#include <fikus/workqueue.h>
#include <fikus/spinlock.h>
#include <fikus/device.h>
#include <fikus/module.h>
#include <fikus/timer.h>
#include <fikus/slab.h>
#include <fikus/list.h>

#include <asm/css_chars.h>
#include <asm/debug.h>
#include <asm/isc.h>
#include <asm/cio.h>
#include <asm/scsw.h>
#include <asm/eadm.h>

#include "eadm_sch.h"
#include "ioasm.h"
#include "cio.h"
#include "css.h"
#include "orb.h"

MODULE_DESCRIPTION("driver for s390 eadm subchannels");
MODULE_LICENSE("GPL");

#define EADM_TIMEOUT (5 * HZ)
static DEFINE_SPINLOCK(list_lock);
static LIST_HEAD(eadm_list);

static debug_info_t *eadm_debug;

#define EADM_LOG(imp, txt) do {					\
		debug_text_event(eadm_debug, imp, txt);		\
	} while (0)

static void EADM_LOG_HEX(int level, void *data, int length)
{
	if (level > eadm_debug->level)
		return;
	while (length > 0) {
		debug_event(eadm_debug, level, data, length);
		length -= eadm_debug->buf_size;
		data += eadm_debug->buf_size;
	}
}

static void orb_init(union orb *orb)
{
	memset(orb, 0, sizeof(union orb));
	orb->eadm.compat1 = 1;
	orb->eadm.compat2 = 1;
	orb->eadm.fmt = 1;
	orb->eadm.x = 1;
}

static int eadm_subchannel_start(struct subchannel *sch, struct aob *aob)
{
	union orb *orb = &get_eadm_private(sch)->orb;
	int cc;

	orb_init(orb);
	orb->eadm.aob = (u32)__pa(aob);
	orb->eadm.intparm = (u32)(addr_t)sch;
	orb->eadm.key = PAGE_DEFAULT_KEY >> 4;

	EADM_LOG(6, "start");
	EADM_LOG_HEX(6, &sch->schid, sizeof(sch->schid));

	cc = ssch(sch->schid, orb);
	switch (cc) {
	case 0:
		sch->schib.scsw.eadm.actl |= SCSW_ACTL_START_PEND;
		break;
	case 1:		/* status pending */
	case 2:		/* busy */
		return -EBUSY;
	case 3:		/* not operational */
		return -ENODEV;
	}
	return 0;
}

static int eadm_subchannel_clear(struct subchannel *sch)
{
	int cc;

	cc = csch(sch->schid);
	if (cc)
		return -ENODEV;

	sch->schib.scsw.eadm.actl |= SCSW_ACTL_CLEAR_PEND;
	return 0;
}

static void eadm_subchannel_timeout(unsigned long data)
{
	struct subchannel *sch = (struct subchannel *) data;

	spin_lock_irq(sch->lock);
	EADM_LOG(1, "timeout");
	EADM_LOG_HEX(1, &sch->schid, sizeof(sch->schid));
	if (eadm_subchannel_clear(sch))
		EADM_LOG(0, "clear failed");
	spin_unlock_irq(sch->lock);
}

static void eadm_subchannel_set_timeout(struct subchannel *sch, int expires)
{
	struct eadm_private *private = get_eadm_private(sch);

	if (expires == 0) {
		del_timer(&private->timer);
		return;
	}
	if (timer_pending(&private->timer)) {
		if (mod_timer(&private->timer, jiffies + expires))
			return;
	}
	private->timer.function = eadm_subchannel_timeout;
	private->timer.data = (unsigned long) sch;
	private->timer.expires = jiffies + expires;
	add_timer(&private->timer);
}

static void eadm_subchannel_irq(struct subchannel *sch)
{
	struct eadm_private *private = get_eadm_private(sch);
	struct eadm_scsw *scsw = &sch->schib.scsw.eadm;
	struct irb *irb = (struct irb *)&S390_lowcore.irb;
	int error = 0;

	EADM_LOG(6, "irq");
	EADM_LOG_HEX(6, irb, sizeof(*irb));

	inc_irq_stat(IRQIO_ADM);

	if ((scsw->stctl & (SCSW_STCTL_ALERT_STATUS | SCSW_STCTL_STATUS_PEND))
	    && scsw->eswf == 1 && irb->esw.eadm.erw.r)
		error = -EIO;

	if (scsw->fctl & SCSW_FCTL_CLEAR_FUNC)
		error = -ETIMEDOUT;

	eadm_subchannel_set_timeout(sch, 0);

	if (private->state != EADM_BUSY) {
		EADM_LOG(1, "irq unsol");
		EADM_LOG_HEX(1, irb, sizeof(*irb));
		private->state = EADM_NOT_OPER;
		css_sched_sch_todo(sch, SCH_TODO_EVAL);
		return;
	}
	scm_irq_handler((struct aob *)(unsigned long)scsw->aob, error);
	private->state = EADM_IDLE;
}

static struct subchannel *eadm_get_idle_sch(void)
{
	struct eadm_private *private;
	struct subchannel *sch;
	unsigned long flags;

	spin_lock_irqsave(&list_lock, flags);
	list_for_each_entry(private, &eadm_list, head) {
		sch = private->sch;
		spin_lock(sch->lock);
		if (private->state == EADM_IDLE) {
			private->state = EADM_BUSY;
			list_move_tail(&private->head, &eadm_list);
			spin_unlock(sch->lock);
			spin_unlock_irqrestore(&list_lock, flags);

			return sch;
		}
		spin_unlock(sch->lock);
	}
	spin_unlock_irqrestore(&list_lock, flags);

	return NULL;
}

static int eadm_start_aob(struct aob *aob)
{
	struct eadm_private *private;
	struct subchannel *sch;
	unsigned long flags;
	int ret;

	sch = eadm_get_idle_sch();
	if (!sch)
		return -EBUSY;

	spin_lock_irqsave(sch->lock, flags);
	eadm_subchannel_set_timeout(sch, EADM_TIMEOUT);
	ret = eadm_subchannel_start(sch, aob);
	if (!ret)
		goto out_unlock;

	/* Handle start subchannel failure. */
	eadm_subchannel_set_timeout(sch, 0);
	private = get_eadm_private(sch);
	private->state = EADM_NOT_OPER;
	css_sched_sch_todo(sch, SCH_TODO_EVAL);

out_unlock:
	spin_unlock_irqrestore(sch->lock, flags);

	return ret;
}

static int eadm_subchannel_probe(struct subchannel *sch)
{
	struct eadm_private *private;
	int ret;

	private = kzalloc(sizeof(*private), GFP_KERNEL | GFP_DMA);
	if (!private)
		return -ENOMEM;

	INIT_LIST_HEAD(&private->head);
	init_timer(&private->timer);

	spin_lock_irq(sch->lock);
	set_eadm_private(sch, private);
	private->state = EADM_IDLE;
	private->sch = sch;
	sch->isc = EADM_SCH_ISC;
	ret = cio_enable_subchannel(sch, (u32)(unsigned long)sch);
	if (ret) {
		set_eadm_private(sch, NULL);
		spin_unlock_irq(sch->lock);
		kfree(private);
		goto out;
	}
	spin_unlock_irq(sch->lock);

	spin_lock_irq(&list_lock);
	list_add(&private->head, &eadm_list);
	spin_unlock_irq(&list_lock);

	if (dev_get_uevent_suppress(&sch->dev)) {
		dev_set_uevent_suppress(&sch->dev, 0);
		kobject_uevent(&sch->dev.kobj, KOBJ_ADD);
	}
out:
	return ret;
}

static void eadm_quiesce(struct subchannel *sch)
{
	int ret;

	do {
		spin_lock_irq(sch->lock);
		ret = cio_disable_subchannel(sch);
		spin_unlock_irq(sch->lock);
	} while (ret == -EBUSY);
}

static int eadm_subchannel_remove(struct subchannel *sch)
{
	struct eadm_private *private = get_eadm_private(sch);

	spin_lock_irq(&list_lock);
	list_del(&private->head);
	spin_unlock_irq(&list_lock);

	eadm_quiesce(sch);

	spin_lock_irq(sch->lock);
	set_eadm_private(sch, NULL);
	spin_unlock_irq(sch->lock);

	kfree(private);

	return 0;
}

static void eadm_subchannel_shutdown(struct subchannel *sch)
{
	eadm_quiesce(sch);
}

static int eadm_subchannel_freeze(struct subchannel *sch)
{
	return cio_disable_subchannel(sch);
}

static int eadm_subchannel_restore(struct subchannel *sch)
{
	return cio_enable_subchannel(sch, (u32)(unsigned long)sch);
}

/**
 * eadm_subchannel_sch_event - process subchannel event
 * @sch: subchannel
 * @process: non-zero if function is called in process context
 *
 * An unspecified event occurred for this subchannel. Adjust data according
 * to the current operational state of the subchannel. Return zero when the
 * event has been handled sufficiently or -EAGAIN when this function should
 * be called again in process context.
 */
static int eadm_subchannel_sch_event(struct subchannel *sch, int process)
{
	struct eadm_private *private;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(sch->lock, flags);
	if (!device_is_registered(&sch->dev))
		goto out_unlock;

	if (work_pending(&sch->todo_work))
		goto out_unlock;

	if (cio_update_schib(sch)) {
		css_sched_sch_todo(sch, SCH_TODO_UNREG);
		goto out_unlock;
	}
	private = get_eadm_private(sch);
	if (private->state == EADM_NOT_OPER)
		private->state = EADM_IDLE;

out_unlock:
	spin_unlock_irqrestore(sch->lock, flags);

	return ret;
}

static struct css_device_id eadm_subchannel_ids[] = {
	{ .match_flags = 0x1, .type = SUBCHANNEL_TYPE_ADM, },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(css, eadm_subchannel_ids);

static struct css_driver eadm_subchannel_driver = {
	.drv = {
		.name = "eadm_subchannel",
		.owner = THIS_MODULE,
	},
	.subchannel_type = eadm_subchannel_ids,
	.irq = eadm_subchannel_irq,
	.probe = eadm_subchannel_probe,
	.remove = eadm_subchannel_remove,
	.shutdown = eadm_subchannel_shutdown,
	.sch_event = eadm_subchannel_sch_event,
	.freeze = eadm_subchannel_freeze,
	.thaw = eadm_subchannel_restore,
	.restore = eadm_subchannel_restore,
};

static struct eadm_ops eadm_ops = {
	.eadm_start = eadm_start_aob,
	.owner = THIS_MODULE,
};

static int __init eadm_sch_init(void)
{
	int ret;

	if (!css_general_characteristics.eadm)
		return -ENXIO;

	eadm_debug = debug_register("eadm_log", 16, 1, 16);
	if (!eadm_debug)
		return -ENOMEM;

	debug_register_view(eadm_debug, &debug_hex_ascii_view);
	debug_set_level(eadm_debug, 2);

	isc_register(EADM_SCH_ISC);
	ret = css_driver_register(&eadm_subchannel_driver);
	if (ret)
		goto cleanup;

	register_eadm_ops(&eadm_ops);
	return ret;

cleanup:
	isc_unregister(EADM_SCH_ISC);
	debug_unregister(eadm_debug);
	return ret;
}

static void __exit eadm_sch_exit(void)
{
	unregister_eadm_ops(&eadm_ops);
	css_driver_unregister(&eadm_subchannel_driver);
	isc_unregister(EADM_SCH_ISC);
	debug_unregister(eadm_debug);
}
module_init(eadm_sch_init);
module_exit(eadm_sch_exit);
