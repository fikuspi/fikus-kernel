/*
 * Copyright (C) 2011. Freescale Inc. All rights reserved.
 *
 * Authors:
 *    Alexander Graf <agraf@suse.de>
 *    Paul Mackerras <paulus@samba.org>
 *
 * Description:
 *
 * Hypercall handling for running PAPR guests in PR KVM on Book 3S
 * processors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 */

#include <fikus/anon_inodes.h>

#include <asm/uaccess.h>
#include <asm/kvm_ppc.h>
#include <asm/kvm_book3s.h>

static unsigned long get_pteg_addr(struct kvm_vcpu *vcpu, long pte_index)
{
	struct kvmppc_vcpu_book3s *vcpu_book3s = to_book3s(vcpu);
	unsigned long pteg_addr;

	pte_index <<= 4;
	pte_index &= ((1 << ((vcpu_book3s->sdr1 & 0x1f) + 11)) - 1) << 7 | 0x70;
	pteg_addr = vcpu_book3s->sdr1 & 0xfffffffffffc0000ULL;
	pteg_addr |= pte_index;

	return pteg_addr;
}

static int kvmppc_h_pr_enter(struct kvm_vcpu *vcpu)
{
	long flags = kvmppc_get_gpr(vcpu, 4);
	long pte_index = kvmppc_get_gpr(vcpu, 5);
	unsigned long pteg[2 * 8];
	unsigned long pteg_addr, i, *hpte;

	pte_index &= ~7UL;
	pteg_addr = get_pteg_addr(vcpu, pte_index);

	copy_from_user(pteg, (void __user *)pteg_addr, sizeof(pteg));
	hpte = pteg;

	if (likely((flags & H_EXACT) == 0)) {
		pte_index &= ~7UL;
		for (i = 0; ; ++i) {
			if (i == 8)
				return H_PTEG_FULL;
			if ((*hpte & HPTE_V_VALID) == 0)
				break;
			hpte += 2;
		}
	} else {
		i = kvmppc_get_gpr(vcpu, 5) & 7UL;
		hpte += i * 2;
	}

	hpte[0] = kvmppc_get_gpr(vcpu, 6);
	hpte[1] = kvmppc_get_gpr(vcpu, 7);
	copy_to_user((void __user *)pteg_addr, pteg, sizeof(pteg));
	kvmppc_set_gpr(vcpu, 3, H_SUCCESS);
	kvmppc_set_gpr(vcpu, 4, pte_index | i);

	return EMULATE_DONE;
}

static int kvmppc_h_pr_remove(struct kvm_vcpu *vcpu)
{
	unsigned long flags= kvmppc_get_gpr(vcpu, 4);
	unsigned long pte_index = kvmppc_get_gpr(vcpu, 5);
	unsigned long avpn = kvmppc_get_gpr(vcpu, 6);
	unsigned long v = 0, pteg, rb;
	unsigned long pte[2];

	pteg = get_pteg_addr(vcpu, pte_index);
	copy_from_user(pte, (void __user *)pteg, sizeof(pte));

	if ((pte[0] & HPTE_V_VALID) == 0 ||
	    ((flags & H_AVPN) && (pte[0] & ~0x7fUL) != avpn) ||
	    ((flags & H_ANDCOND) && (pte[0] & avpn) != 0)) {
		kvmppc_set_gpr(vcpu, 3, H_NOT_FOUND);
		return EMULATE_DONE;
	}

	copy_to_user((void __user *)pteg, &v, sizeof(v));

	rb = compute_tlbie_rb(pte[0], pte[1], pte_index);
	vcpu->arch.mmu.tlbie(vcpu, rb, rb & 1 ? true : false);

	kvmppc_set_gpr(vcpu, 3, H_SUCCESS);
	kvmppc_set_gpr(vcpu, 4, pte[0]);
	kvmppc_set_gpr(vcpu, 5, pte[1]);

	return EMULATE_DONE;
}

/* Request defs for kvmppc_h_pr_bulk_remove() */
#define H_BULK_REMOVE_TYPE             0xc000000000000000ULL
#define   H_BULK_REMOVE_REQUEST        0x4000000000000000ULL
#define   H_BULK_REMOVE_RESPONSE       0x8000000000000000ULL
#define   H_BULK_REMOVE_END            0xc000000000000000ULL
#define H_BULK_REMOVE_CODE             0x3000000000000000ULL
#define   H_BULK_REMOVE_SUCCESS        0x0000000000000000ULL
#define   H_BULK_REMOVE_NOT_FOUND      0x1000000000000000ULL
#define   H_BULK_REMOVE_PARM           0x2000000000000000ULL
#define   H_BULK_REMOVE_HW             0x3000000000000000ULL
#define H_BULK_REMOVE_RC               0x0c00000000000000ULL
#define H_BULK_REMOVE_FLAGS            0x0300000000000000ULL
#define   H_BULK_REMOVE_ABSOLUTE       0x0000000000000000ULL
#define   H_BULK_REMOVE_ANDCOND        0x0100000000000000ULL
#define   H_BULK_REMOVE_AVPN           0x0200000000000000ULL
#define H_BULK_REMOVE_PTEX             0x00ffffffffffffffULL
#define H_BULK_REMOVE_MAX_BATCH        4

static int kvmppc_h_pr_bulk_remove(struct kvm_vcpu *vcpu)
{
	int i;
	int paramnr = 4;
	int ret = H_SUCCESS;

	for (i = 0; i < H_BULK_REMOVE_MAX_BATCH; i++) {
		unsigned long tsh = kvmppc_get_gpr(vcpu, paramnr+(2*i));
		unsigned long tsl = kvmppc_get_gpr(vcpu, paramnr+(2*i)+1);
		unsigned long pteg, rb, flags;
		unsigned long pte[2];
		unsigned long v = 0;

		if ((tsh & H_BULK_REMOVE_TYPE) == H_BULK_REMOVE_END) {
			break; /* Exit success */
		} else if ((tsh & H_BULK_REMOVE_TYPE) !=
			   H_BULK_REMOVE_REQUEST) {
			ret = H_PARAMETER;
			break; /* Exit fail */
		}

		tsh &= H_BULK_REMOVE_PTEX | H_BULK_REMOVE_FLAGS;
		tsh |= H_BULK_REMOVE_RESPONSE;

		if ((tsh & H_BULK_REMOVE_ANDCOND) &&
		    (tsh & H_BULK_REMOVE_AVPN)) {
			tsh |= H_BULK_REMOVE_PARM;
			kvmppc_set_gpr(vcpu, paramnr+(2*i), tsh);
			ret = H_PARAMETER;
			break; /* Exit fail */
		}

		pteg = get_pteg_addr(vcpu, tsh & H_BULK_REMOVE_PTEX);
		copy_from_user(pte, (void __user *)pteg, sizeof(pte));

		/* tsl = AVPN */
		flags = (tsh & H_BULK_REMOVE_FLAGS) >> 26;

		if ((pte[0] & HPTE_V_VALID) == 0 ||
		    ((flags & H_AVPN) && (pte[0] & ~0x7fUL) != tsl) ||
		    ((flags & H_ANDCOND) && (pte[0] & tsl) != 0)) {
			tsh |= H_BULK_REMOVE_NOT_FOUND;
		} else {
			/* Splat the pteg in (userland) hpt */
			copy_to_user((void __user *)pteg, &v, sizeof(v));

			rb = compute_tlbie_rb(pte[0], pte[1],
					      tsh & H_BULK_REMOVE_PTEX);
			vcpu->arch.mmu.tlbie(vcpu, rb, rb & 1 ? true : false);
			tsh |= H_BULK_REMOVE_SUCCESS;
			tsh |= (pte[1] & (HPTE_R_C | HPTE_R_R)) << 43;
		}
		kvmppc_set_gpr(vcpu, paramnr+(2*i), tsh);
	}
	kvmppc_set_gpr(vcpu, 3, ret);

	return EMULATE_DONE;
}

static int kvmppc_h_pr_protect(struct kvm_vcpu *vcpu)
{
	unsigned long flags = kvmppc_get_gpr(vcpu, 4);
	unsigned long pte_index = kvmppc_get_gpr(vcpu, 5);
	unsigned long avpn = kvmppc_get_gpr(vcpu, 6);
	unsigned long rb, pteg, r, v;
	unsigned long pte[2];

	pteg = get_pteg_addr(vcpu, pte_index);
	copy_from_user(pte, (void __user *)pteg, sizeof(pte));

	if ((pte[0] & HPTE_V_VALID) == 0 ||
	    ((flags & H_AVPN) && (pte[0] & ~0x7fUL) != avpn)) {
		kvmppc_set_gpr(vcpu, 3, H_NOT_FOUND);
		return EMULATE_DONE;
	}

	v = pte[0];
	r = pte[1];
	r &= ~(HPTE_R_PP0 | HPTE_R_PP | HPTE_R_N | HPTE_R_KEY_HI |
	       HPTE_R_KEY_LO);
	r |= (flags << 55) & HPTE_R_PP0;
	r |= (flags << 48) & HPTE_R_KEY_HI;
	r |= flags & (HPTE_R_PP | HPTE_R_N | HPTE_R_KEY_LO);

	pte[1] = r;

	rb = compute_tlbie_rb(v, r, pte_index);
	vcpu->arch.mmu.tlbie(vcpu, rb, rb & 1 ? true : false);
	copy_to_user((void __user *)pteg, pte, sizeof(pte));

	kvmppc_set_gpr(vcpu, 3, H_SUCCESS);

	return EMULATE_DONE;
}

static int kvmppc_h_pr_put_tce(struct kvm_vcpu *vcpu)
{
	unsigned long liobn = kvmppc_get_gpr(vcpu, 4);
	unsigned long ioba = kvmppc_get_gpr(vcpu, 5);
	unsigned long tce = kvmppc_get_gpr(vcpu, 6);
	long rc;

	rc = kvmppc_h_put_tce(vcpu, liobn, ioba, tce);
	if (rc == H_TOO_HARD)
		return EMULATE_FAIL;
	kvmppc_set_gpr(vcpu, 3, rc);
	return EMULATE_DONE;
}

static int kvmppc_h_pr_xics_hcall(struct kvm_vcpu *vcpu, u32 cmd)
{
	long rc = kvmppc_xics_hcall(vcpu, cmd);
	kvmppc_set_gpr(vcpu, 3, rc);
	return EMULATE_DONE;
}

int kvmppc_h_pr(struct kvm_vcpu *vcpu, unsigned long cmd)
{
	switch (cmd) {
	case H_ENTER:
		return kvmppc_h_pr_enter(vcpu);
	case H_REMOVE:
		return kvmppc_h_pr_remove(vcpu);
	case H_PROTECT:
		return kvmppc_h_pr_protect(vcpu);
	case H_BULK_REMOVE:
		return kvmppc_h_pr_bulk_remove(vcpu);
	case H_PUT_TCE:
		return kvmppc_h_pr_put_tce(vcpu);
	case H_CEDE:
		vcpu->arch.shared->msr |= MSR_EE;
		kvm_vcpu_block(vcpu);
		clear_bit(KVM_REQ_UNHALT, &vcpu->requests);
		vcpu->stat.halt_wakeup++;
		return EMULATE_DONE;
	case H_XIRR:
	case H_CPPR:
	case H_EOI:
	case H_IPI:
	case H_IPOLL:
	case H_XIRR_X:
		if (kvmppc_xics_enabled(vcpu))
			return kvmppc_h_pr_xics_hcall(vcpu, cmd);
		break;
	case H_RTAS:
		if (list_empty(&vcpu->kvm->arch.rtas_tokens))
			return RESUME_HOST;
		if (kvmppc_rtas_hcall(vcpu))
			break;
		kvmppc_set_gpr(vcpu, 3, 0);
		return EMULATE_DONE;
	}

	return EMULATE_FAIL;
}
