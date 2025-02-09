/* sys_frv.c: FRV arch-specific syscall wrappers
 *
 * Copyright (C) 2003-5 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 * - Derived from arch/m68k/kernel/sys_m68k.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
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
#include <fikus/mman.h>
#include <fikus/file.h>
#include <fikus/syscalls.h>
#include <fikus/ipc.h>

#include <asm/setup.h>
#include <asm/uaccess.h>

asmlinkage long sys_mmap2(unsigned long addr, unsigned long len,
			  unsigned long prot, unsigned long flags,
			  unsigned long fd, unsigned long pgoff)
{
	/* As with sparc32, make sure the shift for mmap2 is constant
	   (12), no matter what PAGE_SIZE we have.... */

	/* But unlike sparc32, don't just silently break if we're
	   trying to map something we can't */
	if (pgoff & ((1 << (PAGE_SHIFT - 12)) - 1))
		return -EINVAL;

	return sys_mmap_pgoff(addr, len, prot, flags, fd,
			      pgoff >> (PAGE_SHIFT - 12));
}
