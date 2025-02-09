/*
 * Copyright 2010 Tilera Corporation. All Rights Reserved.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 *
 * This file contains various random system calls that
 * have a non-standard calling sequence on the Fikus/TILE
 * platform.
 */

#include <fikus/errno.h>
#include <fikus/sched.h>
#include <fikus/mm.h>
#include <fikus/smp.h>
#include <fikus/syscalls.h>
#include <fikus/mman.h>
#include <fikus/file.h>
#include <fikus/mempolicy.h>
#include <fikus/binfmts.h>
#include <fikus/fs.h>
#include <fikus/compat.h>
#include <fikus/uaccess.h>
#include <fikus/signal.h>
#include <asm/syscalls.h>
#include <asm/pgtable.h>
#include <asm/homecache.h>
#include <asm/cachectl.h>
#include <arch/chip.h>

SYSCALL_DEFINE3(cacheflush, unsigned long, addr, unsigned long, len,
		unsigned long, flags)
{
	/* DCACHE is not particularly effective if not bound to one cpu. */
	if (flags & DCACHE)
		homecache_evict(cpumask_of(raw_smp_processor_id()));

	if (flags & ICACHE)
		flush_remote(0, HV_FLUSH_EVICT_L1I, mm_cpumask(current->mm),
			     0, 0, 0, NULL, NULL, 0);
	return 0;
}

/*
 * Syscalls that pass 64-bit values on 32-bit systems normally
 * pass them as (low,high) word packed into the immediately adjacent
 * registers.  If the low word naturally falls on an even register,
 * our ABI makes it work correctly; if not, we adjust it here.
 * Handling it here means we don't have to fix uclibc AND glibc AND
 * any other standard libcs we want to support.
 */

#if !defined(__tilegx__) || defined(CONFIG_COMPAT)

ssize_t sys32_readahead(int fd, u32 offset_lo, u32 offset_hi, u32 count)
{
	return sys_readahead(fd, ((loff_t)offset_hi << 32) | offset_lo, count);
}

int sys32_fadvise64_64(int fd, u32 offset_lo, u32 offset_hi,
		       u32 len_lo, u32 len_hi, int advice)
{
	return sys_fadvise64_64(fd, ((loff_t)offset_hi << 32) | offset_lo,
				((loff_t)len_hi << 32) | len_lo, advice);
}

#endif /* 32-bit syscall wrappers */

/* Note: used by the compat code even in 64-bit Fikus. */
SYSCALL_DEFINE6(mmap2, unsigned long, addr, unsigned long, len,
		unsigned long, prot, unsigned long, flags,
		unsigned long, fd, unsigned long, off_4k)
{
#define PAGE_ADJUST (PAGE_SHIFT - 12)
	if (off_4k & ((1 << PAGE_ADJUST) - 1))
		return -EINVAL;
	return sys_mmap_pgoff(addr, len, prot, flags, fd,
			      off_4k >> PAGE_ADJUST);
}

#ifdef __tilegx__
SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
		unsigned long, prot, unsigned long, flags,
		unsigned long, fd, off_t, offset)
{
	if (offset & ((1 << PAGE_SHIFT) - 1))
		return -EINVAL;
	return sys_mmap_pgoff(addr, len, prot, flags, fd,
			      offset >> PAGE_SHIFT);
}
#endif


/* Provide the actual syscall number to call mapping. */
#undef __SYSCALL
#define __SYSCALL(nr, call) [nr] = (call),

#ifndef __tilegx__
/* See comments at the top of the file. */
#define sys_fadvise64_64 sys32_fadvise64_64
#define sys_readahead sys32_readahead
#endif

/* Call the assembly trampolines where necessary. */
#undef sys_rt_sigreturn
#define sys_rt_sigreturn _sys_rt_sigreturn
#define sys_clone _sys_clone

/*
 * Note that we can't include <fikus/unistd.h> here since the header
 * guard will defeat us; <asm/unistd.h> checks for __SYSCALL as well.
 */
void *sys_call_table[__NR_syscalls] = {
	[0 ... __NR_syscalls-1] = sys_ni_syscall,
#include <asm/unistd.h>
};
