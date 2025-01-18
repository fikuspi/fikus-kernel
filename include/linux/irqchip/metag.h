/*
 * Copyright (C) 2011 Imagination Technologies
 */

#ifndef _FIKUS_IRQCHIP_METAG_H_
#define _FIKUS_IRQCHIP_METAG_H_

#include <fikus/errno.h>

#ifdef CONFIG_METAG_PERFCOUNTER_IRQS
extern int init_internal_IRQ(void);
extern int internal_irq_map(unsigned int hw);
#else
static inline int init_internal_IRQ(void)
{
	return 0;
}
static inline int internal_irq_map(unsigned int hw)
{
	return -EINVAL;
}
#endif

#endif /* _FIKUS_IRQCHIP_METAG_H_ */
