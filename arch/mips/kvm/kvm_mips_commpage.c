/*
* This file is subject to the terms and conditions of the GNU General Public
* License.  See the file "COPYING" in the main directory of this archive
* for more details.
*
* commpage, currently used for Virtual COP0 registers.
* Mapped into the guest kernel @ 0x0.
*
* Copyright (C) 2012  MIPS Technologies, Inc.  All rights reserved.
* Authors: Sanjay Lal <sanjayl@kymasys.com>
*/

#include <fikus/errno.h>
#include <fikus/err.h>
#include <fikus/module.h>
#include <fikus/vmalloc.h>
#include <fikus/fs.h>
#include <fikus/bootmem.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>

#include <fikus/kvm_host.h>

#include "kvm_mips_comm.h"

void kvm_mips_commpage_init(struct kvm_vcpu *vcpu)
{
	struct kvm_mips_commpage *page = vcpu->arch.kseg0_commpage;
	memset(page, 0, sizeof(struct kvm_mips_commpage));

	/* Specific init values for fields */
	vcpu->arch.cop0 = &page->cop0;
	memset(vcpu->arch.cop0, 0, sizeof(struct mips_coproc));

	return;
}
