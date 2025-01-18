#ifndef __FIKUS_KVM_PARA_H
#define __FIKUS_KVM_PARA_H

#include <uapi/fikus/kvm_para.h>


static inline int kvm_para_has_feature(unsigned int feature)
{
	if (kvm_arch_para_features() & (1UL << feature))
		return 1;
	return 0;
}
#endif /* __FIKUS_KVM_PARA_H */
