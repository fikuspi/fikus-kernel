#ifndef __ASM_POWERPC_SYSCALLS_H
#define __ASM_POWERPC_SYSCALLS_H
#ifdef __KERNEL__

#include <fikus/compiler.h>
#include <fikus/linkage.h>
#include <fikus/types.h>

struct rtas_args;

asmlinkage unsigned long sys_mmap(unsigned long addr, size_t len,
		unsigned long prot, unsigned long flags,
		unsigned long fd, off_t offset);
asmlinkage unsigned long sys_mmap2(unsigned long addr, size_t len,
		unsigned long prot, unsigned long flags,
		unsigned long fd, unsigned long pgoff);
asmlinkage long ppc64_personality(unsigned long personality);
asmlinkage int ppc_rtas(struct rtas_args __user *uargs);

#endif /* __KERNEL__ */
#endif /* __ASM_POWERPC_SYSCALLS_H */
