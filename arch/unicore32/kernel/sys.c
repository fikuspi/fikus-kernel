/*
 * fikus/arch/unicore32/kernel/sys.c
 *
 * Code specific to PKUnity SoC and UniCore ISA
 *
 * Copyright (C) 2001-2010 GUAN Xue-tao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <fikus/module.h>
#include <fikus/errno.h>
#include <fikus/sched.h>
#include <fikus/slab.h>
#include <fikus/mm.h>
#include <fikus/sem.h>
#include <fikus/msg.h>
#include <fikus/shm.h>
#include <fikus/stat.h>
#include <fikus/syscalls.h>
#include <fikus/mman.h>
#include <fikus/fs.h>
#include <fikus/file.h>
#include <fikus/ipc.h>
#include <fikus/uaccess.h>

#include <asm/syscalls.h>
#include <asm/cacheflush.h>

/* Provide the actual syscall number to call mapping. */
#undef __SYSCALL
#define __SYSCALL(nr, call)	[nr] = (call),

#define sys_mmap2 sys_mmap_pgoff
/* Note that we don't include <fikus/unistd.h> but <asm/unistd.h> */
void *sys_call_table[__NR_syscalls] = {
	[0 ... __NR_syscalls-1] = sys_ni_syscall,
#include <asm/unistd.h>
};
