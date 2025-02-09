/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * Copyright 2012 Paul Mackerras, IBM Corp. <paulus@au1.ibm.com>
 */

#include <fikus/types.h>
#include <fikus/string.h>
#include <fikus/kvm.h>
#include <fikus/kvm_host.h>
#include <fikus/kernel.h>
#include <asm/opal.h>

/* SRR1 bits for machine check on POWER7 */
#define SRR1_MC_LDSTERR		(1ul << (63-42))
#define SRR1_MC_IFETCH_SH	(63-45)
#define SRR1_MC_IFETCH_MASK	0x7
#define SRR1_MC_IFETCH_SLBPAR		2	/* SLB parity error */
#define SRR1_MC_IFETCH_SLBMULTI		3	/* SLB multi-hit */
#define SRR1_MC_IFETCH_SLBPARMULTI	4	/* SLB parity + multi-hit */
#define SRR1_MC_IFETCH_TLBMULTI		5	/* I-TLB multi-hit */

/* DSISR bits for machine check on POWER7 */
#define DSISR_MC_DERAT_MULTI	0x800		/* D-ERAT multi-hit */
#define DSISR_MC_TLB_MULTI	0x400		/* D-TLB multi-hit */
#define DSISR_MC_SLB_PARITY	0x100		/* SLB parity error */
#define DSISR_MC_SLB_MULTI	0x080		/* SLB multi-hit */
#define DSISR_MC_SLB_PARMULTI	0x040		/* SLB parity + multi-hit */

/* POWER7 SLB flush and reload */
static void reload_slb(struct kvm_vcpu *vcpu)
{
	struct slb_shadow *slb;
	unsigned long i, n;

	/* First clear out SLB */
	asm volatile("slbmte %0,%0; slbia" : : "r" (0));

	/* Do they have an SLB shadow buffer registered? */
	slb = vcpu->arch.slb_shadow.pinned_addr;
	if (!slb)
		return;

	/* Sanity check */
	n = min_t(u32, slb->persistent, SLB_MIN_SIZE);
	if ((void *) &slb->save_area[n] > vcpu->arch.slb_shadow.pinned_end)
		return;

	/* Load up the SLB from that */
	for (i = 0; i < n; ++i) {
		unsigned long rb = slb->save_area[i].esid;
		unsigned long rs = slb->save_area[i].vsid;

		rb = (rb & ~0xFFFul) | i;	/* insert entry number */
		asm volatile("slbmte %0,%1" : : "r" (rs), "r" (rb));
	}
}

/* POWER7 TLB flush */
static void flush_tlb_power7(struct kvm_vcpu *vcpu)
{
	unsigned long i, rb;

	rb = TLBIEL_INVAL_SET_LPID;
	for (i = 0; i < POWER7_TLB_SETS; ++i) {
		asm volatile("tlbiel %0" : : "r" (rb));
		rb += 1 << TLBIEL_INVAL_SET_SHIFT;
	}
}

/*
 * On POWER7, see if we can handle a machine check that occurred inside
 * the guest in real mode, without switching to the host partition.
 *
 * Returns: 0 => exit guest, 1 => deliver machine check to guest
 */
static long kvmppc_realmode_mc_power7(struct kvm_vcpu *vcpu)
{
	unsigned long srr1 = vcpu->arch.shregs.msr;
#ifdef CONFIG_PPC_POWERNV
	struct opal_machine_check_event *opal_evt;
#endif
	long handled = 1;

	if (srr1 & SRR1_MC_LDSTERR) {
		/* error on load/store */
		unsigned long dsisr = vcpu->arch.shregs.dsisr;

		if (dsisr & (DSISR_MC_SLB_PARMULTI | DSISR_MC_SLB_MULTI |
			     DSISR_MC_SLB_PARITY | DSISR_MC_DERAT_MULTI)) {
			/* flush and reload SLB; flushes D-ERAT too */
			reload_slb(vcpu);
			dsisr &= ~(DSISR_MC_SLB_PARMULTI | DSISR_MC_SLB_MULTI |
				   DSISR_MC_SLB_PARITY | DSISR_MC_DERAT_MULTI);
		}
		if (dsisr & DSISR_MC_TLB_MULTI) {
			flush_tlb_power7(vcpu);
			dsisr &= ~DSISR_MC_TLB_MULTI;
		}
		/* Any other errors we don't understand? */
		if (dsisr & 0xffffffffUL)
			handled = 0;
	}

	switch ((srr1 >> SRR1_MC_IFETCH_SH) & SRR1_MC_IFETCH_MASK) {
	case 0:
		break;
	case SRR1_MC_IFETCH_SLBPAR:
	case SRR1_MC_IFETCH_SLBMULTI:
	case SRR1_MC_IFETCH_SLBPARMULTI:
		reload_slb(vcpu);
		break;
	case SRR1_MC_IFETCH_TLBMULTI:
		flush_tlb_power7(vcpu);
		break;
	default:
		handled = 0;
	}

#ifdef CONFIG_PPC_POWERNV
	/*
	 * See if OPAL has already handled the condition.
	 * We assume that if the condition is recovered then OPAL
	 * will have generated an error log event that we will pick
	 * up and log later.
	 */
	opal_evt = local_paca->opal_mc_evt;
	if (opal_evt->version == OpalMCE_V1 &&
	    (opal_evt->severity == OpalMCE_SEV_NO_ERROR ||
	     opal_evt->disposition == OpalMCE_DISPOSITION_RECOVERED))
		handled = 1;

	if (handled)
		opal_evt->in_use = 0;
#endif

	return handled;
}

long kvmppc_realmode_machine_check(struct kvm_vcpu *vcpu)
{
	if (cpu_has_feature(CPU_FTR_ARCH_206))
		return kvmppc_realmode_mc_power7(vcpu);

	return 0;
}
