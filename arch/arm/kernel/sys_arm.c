/*
 *  fikus/arch/arm/kernel/sys_arm.c
 *
 *  Copyright (C) People who wrote fikus/arch/i386/kernel/sys_i386.c
 *  Copyright (C) 1995, 1996 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file contains various random system calls that
 *  have a non-standard calling sequence on the Fikus/arm
 *  platform.
 */
#include <fikus/export.h>
#include <fikus/errno.h>
#include <fikus/sched.h>
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
#include <fikus/slab.h>

/*
 * Since loff_t is a 64 bit type we avoid a lot of ABI hassle
 * with a different argument ordering.
 */
asmlinkage long sys_arm_fadvise64_64(int fd, int advice,
				     loff_t offset, loff_t len)
{
	return sys_fadvise64_64(fd, offset, len, advice);
}
