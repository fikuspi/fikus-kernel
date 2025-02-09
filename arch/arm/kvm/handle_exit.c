/*
 * Copyright (C) 2012 - Virtual Open Systems and Columbia University
 * Author: Christoffer Dall <c.dall@virtualopensystems.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <fikus/kvm.h>
#include <fikus/kvm_host.h>
#include <asm/kvm_emulate.h>
#include <asm/kvm_coproc.h>
#include <asm/kvm_mmu.h>
#include <asm/kvm_psci.h>
#include <trace/events/kvm.h>

#include "trace.h"

#include "trace.h"

typedef int (*exit_handle_fn)(struct kvm_vcpu *, struct kvm_run *);

static int handle_svc_hyp(struct kvm_vcpu *vcpu, struct kvm_run *run)
{
	/* SVC called from Hyp mode should never get here */
	kvm_debug("SVC called from Hyp mode shouldn't go here\n");
	BUG();
	return -EINVAL; /* Squash warning */
}

static int handle_hvc(struct kvm_vcpu *vcpu, struct kvm_run *run)
{
	trace_kvm_hvc(*vcpu_pc(vcpu), *vcpu_reg(vcpu, 0),
		      kvm_vcpu_hvc_get_imm(vcpu));

	if (kvm_psci_call(vcpu))
		return 1;

	kvm_inject_undefined(vcpu);
	return 1;
}

static int handle_smc(struct kvm_vcpu *vcpu, struct kvm_run *run)
{
	kvm_inject_undefined(vcpu);
	return 1;
}

static int handle_pabt_hyp(struct kvm_vcpu *vcpu, struct kvm_run *run)
{
	/* The hypervisor should never cause aborts */
	kvm_err("Prefetch Abort taken from Hyp mode at %#08lx (HSR: %#08x)\n",
		kvm_vcpu_get_hfar(vcpu), kvm_vcpu_get_hsr(vcpu));
	return -EFAULT;
}

static int handle_dabt_hyp(struct kvm_vcpu *vcpu, struct kvm_run *run)
{
	/* This is either an error in the ws. code or an external abort */
	kvm_err("Data Abort taken from Hyp mode at %#08lx (HSR: %#08x)\n",
		kvm_vcpu_get_hfar(vcpu), kvm_vcpu_get_hsr(vcpu));
	return -EFAULT;
}

/**
 * kvm_handle_wfi - handle a wait-for-interrupts instruction executed by a guest
 * @vcpu:	the vcpu pointer
 * @run:	the kvm_run structure pointer
 *
 * Simply sets the wait_for_interrupts flag on the vcpu structure, which will
 * halt execution of world-switches and schedule other host processes until
 * there is an incoming IRQ or FIQ to the VM.
 */
static int kvm_handle_wfi(struct kvm_vcpu *vcpu, struct kvm_run *run)
{
	trace_kvm_wfi(*vcpu_pc(vcpu));
	kvm_vcpu_block(vcpu);
	return 1;
}

static exit_handle_fn arm_exit_handlers[] = {
	[HSR_EC_WFI]		= kvm_handle_wfi,
	[HSR_EC_CP15_32]	= kvm_handle_cp15_32,
	[HSR_EC_CP15_64]	= kvm_handle_cp15_64,
	[HSR_EC_CP14_MR]	= kvm_handle_cp14_access,
	[HSR_EC_CP14_LS]	= kvm_handle_cp14_load_store,
	[HSR_EC_CP14_64]	= kvm_handle_cp14_access,
	[HSR_EC_CP_0_13]	= kvm_handle_cp_0_13_access,
	[HSR_EC_CP10_ID]	= kvm_handle_cp10_id,
	[HSR_EC_SVC_HYP]	= handle_svc_hyp,
	[HSR_EC_HVC]		= handle_hvc,
	[HSR_EC_SMC]		= handle_smc,
	[HSR_EC_IABT]		= kvm_handle_guest_abort,
	[HSR_EC_IABT_HYP]	= handle_pabt_hyp,
	[HSR_EC_DABT]		= kvm_handle_guest_abort,
	[HSR_EC_DABT_HYP]	= handle_dabt_hyp,
};

static exit_handle_fn kvm_get_exit_handler(struct kvm_vcpu *vcpu)
{
	u8 hsr_ec = kvm_vcpu_trap_get_class(vcpu);

	if (hsr_ec >= ARRAY_SIZE(arm_exit_handlers) ||
	    !arm_exit_handlers[hsr_ec]) {
		kvm_err("Unknown exception class: hsr: %#08x\n",
			(unsigned int)kvm_vcpu_get_hsr(vcpu));
		BUG();
	}

	return arm_exit_handlers[hsr_ec];
}

/*
 * Return > 0 to return to guest, < 0 on error, 0 (and set exit_reason) on
 * proper exit to userspace.
 */
int handle_exit(struct kvm_vcpu *vcpu, struct kvm_run *run,
		       int exception_index)
{
	exit_handle_fn exit_handler;

	switch (exception_index) {
	case ARM_EXCEPTION_IRQ:
		return 1;
	case ARM_EXCEPTION_UNDEFINED:
		kvm_err("Undefined exception in Hyp mode at: %#08lx\n",
			kvm_vcpu_get_hyp_pc(vcpu));
		BUG();
		panic("KVM: Hypervisor undefined exception!\n");
	case ARM_EXCEPTION_DATA_ABORT:
	case ARM_EXCEPTION_PREF_ABORT:
	case ARM_EXCEPTION_HVC:
		/*
		 * See ARM ARM B1.14.1: "Hyp traps on instructions
		 * that fail their condition code check"
		 */
		if (!kvm_condition_valid(vcpu)) {
			kvm_skip_instr(vcpu, kvm_vcpu_trap_il_is32bit(vcpu));
			return 1;
		}

		exit_handler = kvm_get_exit_handler(vcpu);

		return exit_handler(vcpu, run);
	default:
		kvm_pr_unimpl("Unsupported exception type: %d",
			      exception_index);
		run->exit_reason = KVM_EXIT_INTERNAL_ERROR;
		return 0;
	}
}
