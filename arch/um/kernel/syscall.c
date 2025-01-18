/*
 * Copyright (C) 2000 - 2007 Jeff Dike (jdike@{addtoit,fikus.intel}.com)
 * Licensed under the GPL
 */

#include <fikus/file.h>
#include <fikus/fs.h>
#include <fikus/mm.h>
#include <fikus/sched.h>
#include <fikus/utsname.h>
#include <fikus/syscalls.h>
#include <asm/current.h>
#include <asm/mman.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>

long old_mmap(unsigned long addr, unsigned long len,
	      unsigned long prot, unsigned long flags,
	      unsigned long fd, unsigned long offset)
{
	long err = -EINVAL;
	if (offset & ~PAGE_MASK)
		goto out;

	err = sys_mmap_pgoff(addr, len, prot, flags, fd, offset >> PAGE_SHIFT);
 out:
	return err;
}
