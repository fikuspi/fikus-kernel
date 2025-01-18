/*
 * Copyright (C) 2007-2009 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2007-2009 PetaLogix
 * Copyright (C) 2007 John Williams <john.williams@petalogix.com>
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 *	Yasushi SHOJI <yashi@atmark-techno.com>
 *	Tetsuya OHKAWA <tetsuya@atmark-techno.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <fikus/errno.h>
#include <fikus/export.h>
#include <fikus/mm.h>
#include <fikus/smp.h>
#include <fikus/syscalls.h>
#include <fikus/sem.h>
#include <fikus/msg.h>
#include <fikus/shm.h>
#include <fikus/stat.h>
#include <fikus/mman.h>
#include <fikus/sys.h>
#include <fikus/ipc.h>
#include <fikus/file.h>
#include <fikus/err.h>
#include <fikus/fs.h>
#include <fikus/semaphore.h>
#include <fikus/uaccess.h>
#include <fikus/unistd.h>
#include <fikus/slab.h>
#include <asm/syscalls.h>

asmlinkage long sys_mmap(unsigned long addr, unsigned long len,
			unsigned long prot, unsigned long flags,
			unsigned long fd, off_t pgoff)
{
	if (pgoff & ~PAGE_MASK)
		return -EINVAL;

	return sys_mmap_pgoff(addr, len, prot, flags, fd, pgoff >> PAGE_SHIFT);
}
