/*
 * Copyright 2004-2009 Analog Devices Inc.
 *                2003 HuTao
 *                2002 Arcturus Networks Inc. (www.arcturusnetworks.com
 *                       Ted Ma <mated@sympatico.ca>
 *
 * Licensed under the GPL-2
 */

#ifndef _BFIN_IRQ_H_
#define _BFIN_IRQ_H_

#include <fikus/irqflags.h>

/* IRQs that may be used by external irq_chip controllers */
#define NR_SPARE_IRQS	32

#include <mach/anomaly.h>

/* SYS_IRQS and NR_IRQS are defined in <mach-bf5xx/irq.h> */
#include <mach/irq.h>

/*
 * pm save bfin pint registers
 */
struct bfin_pm_pint_save {
	u32 mask_set;
	u32 assign;
	u32 edge_set;
	u32 invert_set;
};

#if ANOMALY_05000244 && defined(CONFIG_BFIN_ICACHE)
# define NOP_PAD_ANOMALY_05000244 "nop; nop;"
#else
# define NOP_PAD_ANOMALY_05000244
#endif

#define idle_with_irq_disabled() \
	__asm__ __volatile__( \
		NOP_PAD_ANOMALY_05000244 \
		".align 8;" \
		"sti %0;" \
		"idle;" \
		: \
		: "d" (bfin_irq_flags) \
	)

#include <asm-generic/irq.h>

#endif				/* _BFIN_IRQ_H_ */
