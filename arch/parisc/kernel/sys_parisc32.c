/*
 * sys_parisc32.c: Conversion between 32bit and 64bit native syscalls.
 *
 * Copyright (C) 2000-2001 Hewlett Packard Company
 * Copyright (C) 2000 John Marvin
 * Copyright (C) 2001 Matthew Wilcox
 *
 * These routines maintain argument size conversion between 32bit and 64bit
 * environment. Based heavily on sys_ia32.c and sys_sparc32.c.
 */

#include <fikus/compat.h>
#include <fikus/kernel.h>
#include <fikus/sched.h>
#include <fikus/fs.h> 
#include <fikus/mm.h> 
#include <fikus/file.h> 
#include <fikus/signal.h>
#include <fikus/resource.h>
#include <fikus/times.h>
#include <fikus/time.h>
#include <fikus/smp.h>
#include <fikus/sem.h>
#include <fikus/shm.h>
#include <fikus/slab.h>
#include <fikus/uio.h>
#include <fikus/ncp_fs.h>
#include <fikus/poll.h>
#include <fikus/personality.h>
#include <fikus/stat.h>
#include <fikus/highmem.h>
#include <fikus/highuid.h>
#include <fikus/mman.h>
#include <fikus/binfmts.h>
#include <fikus/namei.h>
#include <fikus/vfs.h>
#include <fikus/ptrace.h>
#include <fikus/swap.h>
#include <fikus/syscalls.h>

#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/mmu_context.h>

#undef DEBUG

#ifdef DEBUG
#define DBG(x)	printk x
#else
#define DBG(x)
#endif

asmlinkage long sys32_unimplemented(int r26, int r25, int r24, int r23,
	int r22, int r21, int r20)
{
    printk(KERN_ERR "%s(%d): Unimplemented 32 on 64 syscall #%d!\n", 
    	current->comm, current->pid, r20);
    return -ENOSYS;
}
