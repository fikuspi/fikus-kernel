/*
 * Copyright (C) 2012,2013 - ARM Ltd
 * Author: Marc Zyngier <marc.zyngier@arm.com>
 *
 * Derived from arch/arm/kvm/guest.c:
 * Copyright (C) 2012 - Virtual Open Systems and Columbia University
 * Author: Christoffer Dall <c.dall@virtualopensystems.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fikus/errno.h>
#include <fikus/err.h>
#include <fikus/kvm_host.h>
#include <fikus/module.h>
#include <fikus/vmalloc.h>
#include <fikus/fs.h>
#include <asm/cputype.h>
#include <asm/uaccess.h>
#include <asm/kvm.h>
#include <asm/kvm_asm.h>
#include <asm/kvm_emulate.h>
#include <asm/kvm_coproc.h>

struct kvm_stats_debugfs_item debugfs_entries[] = {
	{ NULL }
};

int kvm_arch_vcpu_setup(struct kvm_vcpu *vcpu)
{
	vcpu->arch.hcr_el2 = HCR_GUEST_FLAGS;
	return 0;
}

static u64 core_reg_offset_from_id(u64 id)
{
	return id & ~(KVM_REG_ARCH_MASK | KVM_REG_SIZE_MASK | KVM_REG_ARM_CORE);
}

static int get_core_reg(struct kvm_vcpu *vcpu, const struct kvm_one_reg *reg)
{
	/*
	 * Because the kvm_regs structure is a mix of 32, 64 and
	 * 128bit fields, we index it as if it was a 32bit
	 * array. Hence below, nr_regs is the number of entries, and
	 * off the index in the "array".
	 */
	__u32 __user *uaddr = (__u32 __user *)(unsigned long)reg->addr;
	struct kvm_regs *regs = vcpu_gp_regs(vcpu);
	int nr_regs = sizeof(*regs) / sizeof(__u32);
	u32 off;

	/* Our ID is an index into the kvm_regs struct. */
	off = core_reg_offset_from_id(reg->id);
	if (off >= nr_regs ||
	    (off + (KVM_REG_SIZE(reg->id) / sizeof(__u32))) >= nr_regs)
		return -ENOENT;

	if (copy_to_user(uaddr, ((u32 *)regs) + off, KVM_REG_SIZE(reg->id)))
		return -EFAULT;

	return 0;
}

static int set_core_reg(struct kvm_vcpu *vcpu, const struct kvm_one_reg *reg)
{
	__u32 __user *uaddr = (__u32 __user *)(unsigned long)reg->addr;
	struct kvm_regs *regs = vcpu_gp_regs(vcpu);
	int nr_regs = sizeof(*regs) / sizeof(__u32);
	__uint128_t tmp;
	void *valp = &tmp;
	u64 off;
	int err = 0;

	/* Our ID is an index into the kvm_regs struct. */
	off = core_reg_offset_from_id(reg->id);
	if (off >= nr_regs ||
	    (off + (KVM_REG_SIZE(reg->id) / sizeof(__u32))) >= nr_regs)
		return -ENOENT;

	if (KVM_REG_SIZE(reg->id) > sizeof(tmp))
		return -EINVAL;

	if (copy_from_user(valp, uaddr, KVM_REG_SIZE(reg->id))) {
		err = -EFAULT;
		goto out;
	}

	if (off == KVM_REG_ARM_CORE_REG(regs.pstate)) {
		u32 mode = (*(u32 *)valp) & COMPAT_PSR_MODE_MASK;
		switch (mode) {
		case COMPAT_PSR_MODE_USR:
		case COMPAT_PSR_MODE_FIQ:
		case COMPAT_PSR_MODE_IRQ:
		case COMPAT_PSR_MODE_SVC:
		case COMPAT_PSR_MODE_ABT:
		case COMPAT_PSR_MODE_UND:
		case PSR_MODE_EL0t:
		case PSR_MODE_EL1t:
		case PSR_MODE_EL1h:
			break;
		default:
			err = -EINVAL;
			goto out;
		}
	}

	memcpy((u32 *)regs + off, valp, KVM_REG_SIZE(reg->id));
out:
	return err;
}

int kvm_arch_vcpu_ioctl_get_regs(struct kvm_vcpu *vcpu, struct kvm_regs *regs)
{
	return -EINVAL;
}

int kvm_arch_vcpu_ioctl_set_regs(struct kvm_vcpu *vcpu, struct kvm_regs *regs)
{
	return -EINVAL;
}

static unsigned long num_core_regs(void)
{
	return sizeof(struct kvm_regs) / sizeof(__u32);
}

/**
 * kvm_arm_num_regs - how many registers do we present via KVM_GET_ONE_REG
 *
 * This is for all registers.
 */
unsigned long kvm_arm_num_regs(struct kvm_vcpu *vcpu)
{
	return num_core_regs() + kvm_arm_num_sys_reg_descs(vcpu);
}

/**
 * kvm_arm_copy_reg_indices - get indices of all registers.
 *
 * We do core registers right here, then we apppend system regs.
 */
int kvm_arm_copy_reg_indices(struct kvm_vcpu *vcpu, u64 __user *uindices)
{
	unsigned int i;
	const u64 core_reg = KVM_REG_ARM64 | KVM_REG_SIZE_U64 | KVM_REG_ARM_CORE;

	for (i = 0; i < sizeof(struct kvm_regs) / sizeof(__u32); i++) {
		if (put_user(core_reg | i, uindices))
			return -EFAULT;
		uindices++;
	}

	return kvm_arm_copy_sys_reg_indices(vcpu, uindices);
}

int kvm_arm_get_reg(struct kvm_vcpu *vcpu, const struct kvm_one_reg *reg)
{
	/* We currently use nothing arch-specific in upper 32 bits */
	if ((reg->id & ~KVM_REG_SIZE_MASK) >> 32 != KVM_REG_ARM64 >> 32)
		return -EINVAL;

	/* Register group 16 means we want a core register. */
	if ((reg->id & KVM_REG_ARM_COPROC_MASK) == KVM_REG_ARM_CORE)
		return get_core_reg(vcpu, reg);

	return kvm_arm_sys_reg_get_reg(vcpu, reg);
}

int kvm_arm_set_reg(struct kvm_vcpu *vcpu, const struct kvm_one_reg *reg)
{
	/* We currently use nothing arch-specific in upper 32 bits */
	if ((reg->id & ~KVM_REG_SIZE_MASK) >> 32 != KVM_REG_ARM64 >> 32)
		return -EINVAL;

	/* Register group 16 means we set a core register. */
	if ((reg->id & KVM_REG_ARM_COPROC_MASK) == KVM_REG_ARM_CORE)
		return set_core_reg(vcpu, reg);

	return kvm_arm_sys_reg_set_reg(vcpu, reg);
}

int kvm_arch_vcpu_ioctl_get_sregs(struct kvm_vcpu *vcpu,
				  struct kvm_sregs *sregs)
{
	return -EINVAL;
}

int kvm_arch_vcpu_ioctl_set_sregs(struct kvm_vcpu *vcpu,
				  struct kvm_sregs *sregs)
{
	return -EINVAL;
}

int __attribute_const__ kvm_target_cpu(void)
{
	unsigned long implementor = read_cpuid_implementor();
	unsigned long part_number = read_cpuid_part_number();

	if (implementor != ARM_CPU_IMP_ARM)
		return -EINVAL;

	switch (part_number) {
	case ARM_CPU_PART_AEM_V8:
		return KVM_ARM_TARGET_AEM_V8;
	case ARM_CPU_PART_FOUNDATION:
		return KVM_ARM_TARGET_FOUNDATION_V8;
	case ARM_CPU_PART_CORTEX_A57:
		/* Currently handled by the generic backend */
		return KVM_ARM_TARGET_CORTEX_A57;
	default:
		return -EINVAL;
	}
}

int kvm_vcpu_set_target(struct kvm_vcpu *vcpu,
			const struct kvm_vcpu_init *init)
{
	unsigned int i;
	int phys_target = kvm_target_cpu();

	if (init->target != phys_target)
		return -EINVAL;

	vcpu->arch.target = phys_target;
	bitmap_zero(vcpu->arch.features, KVM_VCPU_MAX_FEATURES);

	/* -ENOENT for unknown features, -EINVAL for invalid combinations. */
	for (i = 0; i < sizeof(init->features) * 8; i++) {
		if (init->features[i / 32] & (1 << (i % 32))) {
			if (i >= KVM_VCPU_MAX_FEATURES)
				return -ENOENT;
			set_bit(i, vcpu->arch.features);
		}
	}

	/* Now we know what it is, we can reset it. */
	return kvm_reset_vcpu(vcpu);
}

int kvm_arch_vcpu_ioctl_get_fpu(struct kvm_vcpu *vcpu, struct kvm_fpu *fpu)
{
	return -EINVAL;
}

int kvm_arch_vcpu_ioctl_set_fpu(struct kvm_vcpu *vcpu, struct kvm_fpu *fpu)
{
	return -EINVAL;
}

int kvm_arch_vcpu_ioctl_translate(struct kvm_vcpu *vcpu,
				  struct kvm_translation *tr)
{
	return -EINVAL;
}
