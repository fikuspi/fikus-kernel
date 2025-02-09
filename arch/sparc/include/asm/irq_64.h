/* irq.h: IRQ registers on the 64-bit Sparc.
 *
 * Copyright (C) 1996 David S. Miller (davem@davemloft.net)
 * Copyright (C) 1998 Jakub Jelinek (jj@ultra.fikus.cz)
 */

#ifndef _SPARC64_IRQ_H
#define _SPARC64_IRQ_H

#include <fikus/linkage.h>
#include <fikus/kernel.h>
#include <fikus/errno.h>
#include <fikus/interrupt.h>
#include <asm/pil.h>
#include <asm/ptrace.h>

/* IMAP/ICLR register defines */
#define IMAP_VALID		0x80000000UL	/* IRQ Enabled		*/
#define IMAP_TID_UPA		0x7c000000UL	/* UPA TargetID		*/
#define IMAP_TID_JBUS		0x7c000000UL	/* JBUS TargetID	*/
#define IMAP_TID_SHIFT		26
#define IMAP_AID_SAFARI		0x7c000000UL	/* Safari AgentID	*/
#define IMAP_AID_SHIFT		26
#define IMAP_NID_SAFARI		0x03e00000UL	/* Safari NodeID	*/
#define IMAP_NID_SHIFT		21
#define IMAP_IGN		0x000007c0UL	/* IRQ Group Number	*/
#define IMAP_INO		0x0000003fUL	/* IRQ Number		*/
#define IMAP_INR		0x000007ffUL	/* Full interrupt number*/

#define ICLR_IDLE		0x00000000UL	/* Idle state		*/
#define ICLR_TRANSMIT		0x00000001UL	/* Transmit state	*/
#define ICLR_PENDING		0x00000003UL	/* Pending state	*/

/* The largest number of unique interrupt sources we support.
 * If this needs to ever be larger than 255, you need to change
 * the type of ino_bucket->irq as appropriate.
 *
 * ino_bucket->irq allocation is made during {sun4v_,}build_irq().
 */
#define NR_IRQS    255

extern void irq_install_pre_handler(int irq,
				    void (*func)(unsigned int, void *, void *),
				    void *arg1, void *arg2);
#define irq_canonicalize(irq)	(irq)
extern unsigned int build_irq(int inofixup, unsigned long iclr, unsigned long imap);
extern unsigned int sun4v_build_irq(u32 devhandle, unsigned int devino);
extern unsigned int sun4v_build_virq(u32 devhandle, unsigned int devino);
extern unsigned int sun4v_build_msi(u32 devhandle, unsigned int *irq_p,
				    unsigned int msi_devino_start,
				    unsigned int msi_devino_end);
extern void sun4v_destroy_msi(unsigned int irq);
extern unsigned int sun4u_build_msi(u32 portid, unsigned int *irq_p,
				    unsigned int msi_devino_start,
				    unsigned int msi_devino_end,
				    unsigned long imap_base,
				    unsigned long iclr_base);
extern void sun4u_destroy_msi(unsigned int irq);

extern unsigned char irq_alloc(unsigned int dev_handle,
				    unsigned int dev_ino);
#ifdef CONFIG_PCI_MSI
extern void irq_free(unsigned int irq);
#endif

extern void __init init_IRQ(void);
extern void fixup_irqs(void);

static inline void set_softint(unsigned long bits)
{
	__asm__ __volatile__("wr	%0, 0x0, %%set_softint"
			     : /* No outputs */
			     : "r" (bits));
}

static inline void clear_softint(unsigned long bits)
{
	__asm__ __volatile__("wr	%0, 0x0, %%clear_softint"
			     : /* No outputs */
			     : "r" (bits));
}

static inline unsigned long get_softint(void)
{
	unsigned long retval;

	__asm__ __volatile__("rd	%%softint, %0"
			     : "=r" (retval));
	return retval;
}

void arch_trigger_all_cpu_backtrace(void);
#define arch_trigger_all_cpu_backtrace arch_trigger_all_cpu_backtrace

extern void *hardirq_stack[NR_CPUS];
extern void *softirq_stack[NR_CPUS];
#define __ARCH_HAS_DO_SOFTIRQ

#define NO_IRQ		0xffffffff

#endif
