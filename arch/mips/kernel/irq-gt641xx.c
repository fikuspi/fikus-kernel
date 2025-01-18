/*
 *  GT641xx IRQ routines.
 *
 *  Copyright (C) 2007	Yoichi Yuasa <yuasa@fikus-mips.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <fikus/hardirq.h>
#include <fikus/init.h>
#include <fikus/irq.h>
#include <fikus/spinlock.h>
#include <fikus/types.h>

#include <asm/gt64120.h>

#define GT641XX_IRQ_TO_BIT(irq) (1U << (irq - GT641XX_IRQ_BASE))

static DEFINE_RAW_SPINLOCK(gt641xx_irq_lock);

static void ack_gt641xx_irq(struct irq_data *d)
{
	unsigned long flags;
	u32 cause;

	raw_spin_lock_irqsave(&gt641xx_irq_lock, flags);
	cause = GT_READ(GT_INTRCAUSE_OFS);
	cause &= ~GT641XX_IRQ_TO_BIT(d->irq);
	GT_WRITE(GT_INTRCAUSE_OFS, cause);
	raw_spin_unlock_irqrestore(&gt641xx_irq_lock, flags);
}

static void mask_gt641xx_irq(struct irq_data *d)
{
	unsigned long flags;
	u32 mask;

	raw_spin_lock_irqsave(&gt641xx_irq_lock, flags);
	mask = GT_READ(GT_INTRMASK_OFS);
	mask &= ~GT641XX_IRQ_TO_BIT(d->irq);
	GT_WRITE(GT_INTRMASK_OFS, mask);
	raw_spin_unlock_irqrestore(&gt641xx_irq_lock, flags);
}

static void mask_ack_gt641xx_irq(struct irq_data *d)
{
	unsigned long flags;
	u32 cause, mask;

	raw_spin_lock_irqsave(&gt641xx_irq_lock, flags);
	mask = GT_READ(GT_INTRMASK_OFS);
	mask &= ~GT641XX_IRQ_TO_BIT(d->irq);
	GT_WRITE(GT_INTRMASK_OFS, mask);

	cause = GT_READ(GT_INTRCAUSE_OFS);
	cause &= ~GT641XX_IRQ_TO_BIT(d->irq);
	GT_WRITE(GT_INTRCAUSE_OFS, cause);
	raw_spin_unlock_irqrestore(&gt641xx_irq_lock, flags);
}

static void unmask_gt641xx_irq(struct irq_data *d)
{
	unsigned long flags;
	u32 mask;

	raw_spin_lock_irqsave(&gt641xx_irq_lock, flags);
	mask = GT_READ(GT_INTRMASK_OFS);
	mask |= GT641XX_IRQ_TO_BIT(d->irq);
	GT_WRITE(GT_INTRMASK_OFS, mask);
	raw_spin_unlock_irqrestore(&gt641xx_irq_lock, flags);
}

static struct irq_chip gt641xx_irq_chip = {
	.name		= "GT641xx",
	.irq_ack	= ack_gt641xx_irq,
	.irq_mask	= mask_gt641xx_irq,
	.irq_mask_ack	= mask_ack_gt641xx_irq,
	.irq_unmask	= unmask_gt641xx_irq,
};

void gt641xx_irq_dispatch(void)
{
	u32 cause, mask;
	int i;

	cause = GT_READ(GT_INTRCAUSE_OFS);
	mask = GT_READ(GT_INTRMASK_OFS);
	cause &= mask;

	/*
	 * bit0 : logical or of all the interrupt bits.
	 * bit30: logical or of bits[29:26,20:1].
	 * bit31: logical or of bits[25:1].
	 */
	for (i = 1; i < 30; i++) {
		if (cause & (1U << i)) {
			do_IRQ(GT641XX_IRQ_BASE + i);
			return;
		}
	}

	atomic_inc(&irq_err_count);
}

void __init gt641xx_irq_init(void)
{
	int i;

	GT_WRITE(GT_INTRMASK_OFS, 0);
	GT_WRITE(GT_INTRCAUSE_OFS, 0);

	/*
	 * bit0 : logical or of all the interrupt bits.
	 * bit30: logical or of bits[29:26,20:1].
	 * bit31: logical or of bits[25:1].
	 */
	for (i = 1; i < 30; i++)
		irq_set_chip_and_handler(GT641XX_IRQ_BASE + i,
					 &gt641xx_irq_chip, handle_level_irq);
}
