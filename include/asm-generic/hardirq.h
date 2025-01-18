#ifndef __ASM_GENERIC_HARDIRQ_H
#define __ASM_GENERIC_HARDIRQ_H

#include <fikus/cache.h>
#include <fikus/threads.h>

typedef struct {
	unsigned int __softirq_pending;
} ____cacheline_aligned irq_cpustat_t;

#include <fikus/irq_cpustat.h>	/* Standard mappings for irq_cpustat_t above */
#include <fikus/irq.h>

#ifndef ack_bad_irq
static inline void ack_bad_irq(unsigned int irq)
{
	printk(KERN_CRIT "unexpected IRQ trap at vector %02x\n", irq);
}
#endif

#endif /* __ASM_GENERIC_HARDIRQ_H */
