/*
 * PTP 1588 clock using the IXP46X
 *
 * Copyright (C) 2010 OMICRON electronics GmbH
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <fikus/device.h>
#include <fikus/err.h>
#include <fikus/gpio.h>
#include <fikus/init.h>
#include <fikus/interrupt.h>
#include <fikus/io.h>
#include <fikus/irq.h>
#include <fikus/kernel.h>
#include <fikus/module.h>

#include <fikus/ptp_clock_kernel.h>
#include <mach/ixp46x_ts.h>

#define DRIVER		"ptp_ixp46x"
#define N_EXT_TS	2
#define MASTER_GPIO	8
#define MASTER_IRQ	25
#define SLAVE_GPIO	7
#define SLAVE_IRQ	24

struct ixp_clock {
	struct ixp46x_ts_regs *regs;
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info caps;
	int exts0_enabled;
	int exts1_enabled;
};

DEFINE_SPINLOCK(register_lock);

/*
 * Register access functions
 */

static u64 ixp_systime_read(struct ixp46x_ts_regs *regs)
{
	u64 ns;
	u32 lo, hi;

	lo = __raw_readl(&regs->systime_lo);
	hi = __raw_readl(&regs->systime_hi);

	ns = ((u64) hi) << 32;
	ns |= lo;
	ns <<= TICKS_NS_SHIFT;

	return ns;
}

static void ixp_systime_write(struct ixp46x_ts_regs *regs, u64 ns)
{
	u32 hi, lo;

	ns >>= TICKS_NS_SHIFT;
	hi = ns >> 32;
	lo = ns & 0xffffffff;

	__raw_writel(lo, &regs->systime_lo);
	__raw_writel(hi, &regs->systime_hi);
}

/*
 * Interrupt service routine
 */

static irqreturn_t isr(int irq, void *priv)
{
	struct ixp_clock *ixp_clock = priv;
	struct ixp46x_ts_regs *regs = ixp_clock->regs;
	struct ptp_clock_event event;
	u32 ack = 0, lo, hi, val;

	val = __raw_readl(&regs->event);

	if (val & TSER_SNS) {
		ack |= TSER_SNS;
		if (ixp_clock->exts0_enabled) {
			hi = __raw_readl(&regs->asms_hi);
			lo = __raw_readl(&regs->asms_lo);
			event.type = PTP_CLOCK_EXTTS;
			event.index = 0;
			event.timestamp = ((u64) hi) << 32;
			event.timestamp |= lo;
			event.timestamp <<= TICKS_NS_SHIFT;
			ptp_clock_event(ixp_clock->ptp_clock, &event);
		}
	}

	if (val & TSER_SNM) {
		ack |= TSER_SNM;
		if (ixp_clock->exts1_enabled) {
			hi = __raw_readl(&regs->amms_hi);
			lo = __raw_readl(&regs->amms_lo);
			event.type = PTP_CLOCK_EXTTS;
			event.index = 1;
			event.timestamp = ((u64) hi) << 32;
			event.timestamp |= lo;
			event.timestamp <<= TICKS_NS_SHIFT;
			ptp_clock_event(ixp_clock->ptp_clock, &event);
		}
	}

	if (val & TTIPEND)
		ack |= TTIPEND; /* this bit seems to be always set */

	if (ack) {
		__raw_writel(ack, &regs->event);
		return IRQ_HANDLED;
	} else
		return IRQ_NONE;
}

/*
 * PTP clock operations
 */

static int ptp_ixp_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	u64 adj;
	u32 diff, addend;
	int neg_adj = 0;
	struct ixp_clock *ixp_clock = container_of(ptp, struct ixp_clock, caps);
	struct ixp46x_ts_regs *regs = ixp_clock->regs;

	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}
	addend = DEFAULT_ADDEND;
	adj = addend;
	adj *= ppb;
	diff = div_u64(adj, 1000000000ULL);

	addend = neg_adj ? addend - diff : addend + diff;

	__raw_writel(addend, &regs->addend);

	return 0;
}

static int ptp_ixp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	s64 now;
	unsigned long flags;
	struct ixp_clock *ixp_clock = container_of(ptp, struct ixp_clock, caps);
	struct ixp46x_ts_regs *regs = ixp_clock->regs;

	spin_lock_irqsave(&register_lock, flags);

	now = ixp_systime_read(regs);
	now += delta;
	ixp_systime_write(regs, now);

	spin_unlock_irqrestore(&register_lock, flags);

	return 0;
}

static int ptp_ixp_gettime(struct ptp_clock_info *ptp, struct timespec *ts)
{
	u64 ns;
	u32 remainder;
	unsigned long flags;
	struct ixp_clock *ixp_clock = container_of(ptp, struct ixp_clock, caps);
	struct ixp46x_ts_regs *regs = ixp_clock->regs;

	spin_lock_irqsave(&register_lock, flags);

	ns = ixp_systime_read(regs);

	spin_unlock_irqrestore(&register_lock, flags);

	ts->tv_sec = div_u64_rem(ns, 1000000000, &remainder);
	ts->tv_nsec = remainder;
	return 0;
}

static int ptp_ixp_settime(struct ptp_clock_info *ptp,
			   const struct timespec *ts)
{
	u64 ns;
	unsigned long flags;
	struct ixp_clock *ixp_clock = container_of(ptp, struct ixp_clock, caps);
	struct ixp46x_ts_regs *regs = ixp_clock->regs;

	ns = ts->tv_sec * 1000000000ULL;
	ns += ts->tv_nsec;

	spin_lock_irqsave(&register_lock, flags);

	ixp_systime_write(regs, ns);

	spin_unlock_irqrestore(&register_lock, flags);

	return 0;
}

static int ptp_ixp_enable(struct ptp_clock_info *ptp,
			  struct ptp_clock_request *rq, int on)
{
	struct ixp_clock *ixp_clock = container_of(ptp, struct ixp_clock, caps);

	switch (rq->type) {
	case PTP_CLK_REQ_EXTTS:
		switch (rq->extts.index) {
		case 0:
			ixp_clock->exts0_enabled = on ? 1 : 0;
			break;
		case 1:
			ixp_clock->exts1_enabled = on ? 1 : 0;
			break;
		default:
			return -EINVAL;
		}
		return 0;
	default:
		break;
	}

	return -EOPNOTSUPP;
}

static struct ptp_clock_info ptp_ixp_caps = {
	.owner		= THIS_MODULE,
	.name		= "IXP46X timer",
	.max_adj	= 66666655,
	.n_ext_ts	= N_EXT_TS,
	.pps		= 0,
	.adjfreq	= ptp_ixp_adjfreq,
	.adjtime	= ptp_ixp_adjtime,
	.gettime	= ptp_ixp_gettime,
	.settime	= ptp_ixp_settime,
	.enable		= ptp_ixp_enable,
};

/* module operations */

static struct ixp_clock ixp_clock;

static int setup_interrupt(int gpio)
{
	int irq;

	gpio_line_config(gpio, IXP4XX_GPIO_IN);

	irq = gpio_to_irq(gpio);

	if (NO_IRQ == irq)
		return NO_IRQ;

	if (irq_set_irq_type(irq, IRQF_TRIGGER_FALLING)) {
		pr_err("cannot set trigger type for irq %d\n", irq);
		return NO_IRQ;
	}

	if (request_irq(irq, isr, 0, DRIVER, &ixp_clock)) {
		pr_err("request_irq failed for irq %d\n", irq);
		return NO_IRQ;
	}

	return irq;
}

static void __exit ptp_ixp_exit(void)
{
	free_irq(MASTER_IRQ, &ixp_clock);
	free_irq(SLAVE_IRQ, &ixp_clock);
	ixp46x_phc_index = -1;
	ptp_clock_unregister(ixp_clock.ptp_clock);
}

static int __init ptp_ixp_init(void)
{
	if (!cpu_is_ixp46x())
		return -ENODEV;

	ixp_clock.regs =
		(struct ixp46x_ts_regs __iomem *) IXP4XX_TIMESYNC_BASE_VIRT;

	ixp_clock.caps = ptp_ixp_caps;

	ixp_clock.ptp_clock = ptp_clock_register(&ixp_clock.caps, NULL);

	if (IS_ERR(ixp_clock.ptp_clock))
		return PTR_ERR(ixp_clock.ptp_clock);

	ixp46x_phc_index = ptp_clock_index(ixp_clock.ptp_clock);

	__raw_writel(DEFAULT_ADDEND, &ixp_clock.regs->addend);
	__raw_writel(1, &ixp_clock.regs->trgt_lo);
	__raw_writel(0, &ixp_clock.regs->trgt_hi);
	__raw_writel(TTIPEND, &ixp_clock.regs->event);

	if (MASTER_IRQ != setup_interrupt(MASTER_GPIO)) {
		pr_err("failed to setup gpio %d as irq\n", MASTER_GPIO);
		goto no_master;
	}
	if (SLAVE_IRQ != setup_interrupt(SLAVE_GPIO)) {
		pr_err("failed to setup gpio %d as irq\n", SLAVE_GPIO);
		goto no_slave;
	}

	return 0;
no_slave:
	free_irq(MASTER_IRQ, &ixp_clock);
no_master:
	ptp_clock_unregister(ixp_clock.ptp_clock);
	return -ENODEV;
}

module_init(ptp_ixp_init);
module_exit(ptp_ixp_exit);

MODULE_AUTHOR("Richard Cochran <richardcochran@gmail.com>");
MODULE_DESCRIPTION("PTP clock using the IXP46X timer");
MODULE_LICENSE("GPL");
