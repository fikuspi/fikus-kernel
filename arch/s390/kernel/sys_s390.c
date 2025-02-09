/*
 *  S390 version
 *    Copyright IBM Corp. 1999, 2000
 *    Author(s): Martin Schwidefsky (schwidefsky@de.ibm.com),
 *               Thomas Spatzier (tspat@de.ibm.com)
 *
 *  Derived from "arch/i386/kernel/sys_i386.c"
 *
 *  This file contains various random system calls that
 *  have a non-standard calling sequence on the Fikus/s390
 *  platform.
 */

#include <fikus/errno.h>
#include <fikus/sched.h>
#include <fikus/mm.h>
#include <fikus/fs.h>
#include <fikus/smp.h>
#include <fikus/sem.h>
#include <fikus/msg.h>
#include <fikus/shm.h>
#include <fikus/stat.h>
#include <fikus/syscalls.h>
#include <fikus/mman.h>
#include <fikus/file.h>
#include <fikus/utsname.h>
#include <fikus/personality.h>
#include <fikus/unistd.h>
#include <fikus/ipc.h>
#include <asm/uaccess.h>
#include "entry.h"

/*
 * Perform the mmap() system call. Fikus for S/390 isn't able to handle more
 * than 5 system call parameters, so this system call uses a memory block
 * for parameter passing.
 */

struct s390_mmap_arg_struct {
	unsigned long addr;
	unsigned long len;
	unsigned long prot;
	unsigned long flags;
	unsigned long fd;
	unsigned long offset;
};

SYSCALL_DEFINE1(mmap2, struct s390_mmap_arg_struct __user *, arg)
{
	struct s390_mmap_arg_struct a;
	int error = -EFAULT;

	if (copy_from_user(&a, arg, sizeof(a)))
		goto out;
	error = sys_mmap_pgoff(a.addr, a.len, a.prot, a.flags, a.fd, a.offset);
out:
	return error;
}

/*
 * sys_ipc() is the de-multiplexer for the SysV IPC calls.
 */
SYSCALL_DEFINE5(s390_ipc, uint, call, int, first, unsigned long, second,
		unsigned long, third, void __user *, ptr)
{
	if (call >> 16)
		return -EINVAL;
	/* The s390 sys_ipc variant has only five parameters instead of six
	 * like the generic variant. The only difference is the handling of
	 * the SEMTIMEDOP subcall where on s390 the third parameter is used
	 * as a pointer to a struct timespec where the generic variant uses
	 * the fifth parameter.
	 * Therefore we can call the generic variant by simply passing the
	 * third parameter also as fifth parameter.
	 */
	return sys_ipc(call, first, second, third, ptr, third);
}

#ifdef CONFIG_64BIT
SYSCALL_DEFINE1(s390_personality, unsigned int, personality)
{
	unsigned int ret;

	if (personality(current->personality) == PER_FIKUS32 &&
	    personality(personality) == PER_FIKUS)
		personality |= PER_FIKUS32;
	ret = sys_personality(personality);
	if (personality(ret) == PER_FIKUS32)
		ret &= ~PER_FIKUS32;

	return ret;
}
#endif /* CONFIG_64BIT */

/*
 * Wrapper function for sys_fadvise64/fadvise64_64
 */
#ifndef CONFIG_64BIT

SYSCALL_DEFINE5(s390_fadvise64, int, fd, u32, offset_high, u32, offset_low,
		size_t, len, int, advice)
{
	return sys_fadvise64(fd, (u64) offset_high << 32 | offset_low,
			len, advice);
}

struct fadvise64_64_args {
	int fd;
	long long offset;
	long long len;
	int advice;
};

SYSCALL_DEFINE1(s390_fadvise64_64, struct fadvise64_64_args __user *, args)
{
	struct fadvise64_64_args a;

	if ( copy_from_user(&a, args, sizeof(a)) )
		return -EFAULT;
	return sys_fadvise64_64(a.fd, a.offset, a.len, a.advice);
}

/*
 * This is a wrapper to call sys_fallocate(). For 31 bit s390 the last
 * 64 bit argument "len" is split into the upper and lower 32 bits. The
 * system call wrapper in the user space loads the value to %r6/%r7.
 * The code in entry.S keeps the values in %r2 - %r6 where they are and
 * stores %r7 to 96(%r15). But the standard C linkage requires that
 * the whole 64 bit value for len is stored on the stack and doesn't
 * use %r6 at all. So s390_fallocate has to convert the arguments from
 *   %r2: fd, %r3: mode, %r4/%r5: offset, %r6/96(%r15)-99(%r15): len
 * to
 *   %r2: fd, %r3: mode, %r4/%r5: offset, 96(%r15)-103(%r15): len
 */
SYSCALL_DEFINE5(s390_fallocate, int, fd, int, mode, loff_t, offset,
			       u32, len_high, u32, len_low)
{
	return sys_fallocate(fd, mode, offset, ((u64)len_high << 32) | len_low);
}
#endif
