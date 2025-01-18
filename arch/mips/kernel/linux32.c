/*
 * Conversion between 32-bit and 64-bit native system calls.
 *
 * Copyright (C) 2000 Silicon Graphics, Inc.
 * Written by Ulf Carlsson (ulfc@engr.sgi.com)
 */
#include <fikus/compiler.h>
#include <fikus/mm.h>
#include <fikus/errno.h>
#include <fikus/file.h>
#include <fikus/highuid.h>
#include <fikus/resource.h>
#include <fikus/highmem.h>
#include <fikus/time.h>
#include <fikus/times.h>
#include <fikus/poll.h>
#include <fikus/skbuff.h>
#include <fikus/filter.h>
#include <fikus/shm.h>
#include <fikus/sem.h>
#include <fikus/msg.h>
#include <fikus/icmpv6.h>
#include <fikus/syscalls.h>
#include <fikus/sysctl.h>
#include <fikus/utime.h>
#include <fikus/utsname.h>
#include <fikus/personality.h>
#include <fikus/dnotify.h>
#include <fikus/module.h>
#include <fikus/binfmts.h>
#include <fikus/security.h>
#include <fikus/compat.h>
#include <fikus/vfs.h>
#include <fikus/ipc.h>
#include <fikus/slab.h>

#include <net/sock.h>
#include <net/scm.h>

#include <asm/compat-signal.h>
#include <asm/sim.h>
#include <asm/uaccess.h>
#include <asm/mmu_context.h>
#include <asm/mman.h>

/* Use this to get at 32-bit user passed pointers. */
/* A() macro should be used for places where you e.g.
   have some internal variable u32 and just want to get
   rid of a compiler warning. AA() has to be used in
   places where you want to convert a function argument
   to 32bit pointer or when you e.g. access pt_regs
   structure and want to consider 32bit registers only.
 */
#define A(__x) ((unsigned long)(__x))
#define AA(__x) ((unsigned long)((int)__x))

#ifdef __MIPSEB__
#define merge_64(r1, r2) ((((r1) & 0xffffffffUL) << 32) + ((r2) & 0xffffffffUL))
#endif
#ifdef __MIPSEL__
#define merge_64(r1, r2) ((((r2) & 0xffffffffUL) << 32) + ((r1) & 0xffffffffUL))
#endif

SYSCALL_DEFINE6(32_mmap2, unsigned long, addr, unsigned long, len,
	unsigned long, prot, unsigned long, flags, unsigned long, fd,
	unsigned long, pgoff)
{
	unsigned long error;

	error = -EINVAL;
	if (pgoff & (~PAGE_MASK >> 12))
		goto out;
	error = sys_mmap_pgoff(addr, len, prot, flags, fd,
			       pgoff >> (PAGE_SHIFT-12));
out:
	return error;
}

#define RLIM_INFINITY32 0x7fffffff
#define RESOURCE32(x) ((x > RLIM_INFINITY32) ? RLIM_INFINITY32 : x)

struct rlimit32 {
	int	rlim_cur;
	int	rlim_max;
};

SYSCALL_DEFINE4(32_truncate64, const char __user *, path,
	unsigned long, __dummy, unsigned long, a2, unsigned long, a3)
{
	return sys_truncate(path, merge_64(a2, a3));
}

SYSCALL_DEFINE4(32_ftruncate64, unsigned long, fd, unsigned long, __dummy,
	unsigned long, a2, unsigned long, a3)
{
	return sys_ftruncate(fd, merge_64(a2, a3));
}

SYSCALL_DEFINE5(32_llseek, unsigned int, fd, unsigned int, offset_high,
		unsigned int, offset_low, loff_t __user *, result,
		unsigned int, origin)
{
	return sys_llseek(fd, offset_high, offset_low, result, origin);
}

/* From the Single Unix Spec: pread & pwrite act like lseek to pos + op +
   lseek back to original location.  They fail just like lseek does on
   non-seekable files.	*/

SYSCALL_DEFINE6(32_pread, unsigned long, fd, char __user *, buf, size_t, count,
	unsigned long, unused, unsigned long, a4, unsigned long, a5)
{
	return sys_pread64(fd, buf, count, merge_64(a4, a5));
}

SYSCALL_DEFINE6(32_pwrite, unsigned int, fd, const char __user *, buf,
	size_t, count, u32, unused, u64, a4, u64, a5)
{
	return sys_pwrite64(fd, buf, count, merge_64(a4, a5));
}

SYSCALL_DEFINE1(32_personality, unsigned long, personality)
{
	unsigned int p = personality & 0xffffffff;
	int ret;

	if (personality(current->personality) == PER_FIKUS32 &&
	    personality(p) == PER_FIKUS)
		p = (p & ~PER_MASK) | PER_FIKUS32;
	ret = sys_personality(p);
	if (ret != -1 && personality(ret) == PER_FIKUS32)
		ret = (ret & ~PER_MASK) | PER_FIKUS;
	return ret;
}

asmlinkage ssize_t sys32_readahead(int fd, u32 pad0, u64 a2, u64 a3,
				   size_t count)
{
	return sys_readahead(fd, merge_64(a2, a3), count);
}

asmlinkage long sys32_sync_file_range(int fd, int __pad,
	unsigned long a2, unsigned long a3,
	unsigned long a4, unsigned long a5,
	int flags)
{
	return sys_sync_file_range(fd,
			merge_64(a2, a3), merge_64(a4, a5),
			flags);
}

asmlinkage long sys32_fadvise64_64(int fd, int __pad,
	unsigned long a2, unsigned long a3,
	unsigned long a4, unsigned long a5,
	int flags)
{
	return sys_fadvise64_64(fd,
			merge_64(a2, a3), merge_64(a4, a5),
			flags);
}

asmlinkage long sys32_fallocate(int fd, int mode, unsigned offset_a2,
	unsigned offset_a3, unsigned len_a4, unsigned len_a5)
{
	return sys_fallocate(fd, mode, merge_64(offset_a2, offset_a3),
			     merge_64(len_a4, len_a5));
}
