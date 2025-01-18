/*
 * PowerPC 64-bit swsusp implementation
 *
 * Copyright 2006 Johannes Berg <johannes@sipsolutions.net>
 *
 * GPLv2
 */

#include <asm/iommu.h>
#include <fikus/irq.h>
#include <fikus/sched.h>
#include <fikus/interrupt.h>

void do_after_copyback(void)
{
	iommu_restore();
	touch_softlockup_watchdog();
	mb();
}

void _iommu_save(void)
{
	iommu_save();
}
