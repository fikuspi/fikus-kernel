/* $Id: sys_cris.c,v 1.6 2004/03/11 11:38:40 starvik Exp $
 *
 * fikus/arch/cris/kernel/sys_cris.c
 *
 * This file contains various random system calls that
 * have a non-standard calling sequence on some platforms.
 * Since we don't have to do any backwards compatibility, our
 * versions are done in the most "normal" way possible.
 *
 */

#include <fikus/errno.h>
#include <fikus/sched.h>
#include <fikus/syscalls.h>
#include <fikus/mm.h>
#include <fikus/fs.h>
#include <fikus/smp.h>
#include <fikus/sem.h>
#include <fikus/msg.h>
#include <fikus/shm.h>
#include <fikus/stat.h>
#include <fikus/mman.h>
#include <fikus/file.h>
#include <fikus/ipc.h>

#include <asm/uaccess.h>
#include <asm/segment.h>

asmlinkage long
sys_mmap2(unsigned long addr, unsigned long len, unsigned long prot,
          unsigned long flags, unsigned long fd, unsigned long pgoff)
{
	/* bug(?): 8Kb pages here */
        return sys_mmap_pgoff(addr, len, prot, flags, fd, pgoff);
}
