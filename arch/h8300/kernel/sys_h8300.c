/*
 * fikus/arch/h8300/kernel/sys_h8300.c
 *
 * This file contains various random system calls that
 * have a non-standard calling sequence on the H8/300
 * platform.
 */

#include <fikus/errno.h>
#include <fikus/sched.h>
#include <fikus/mm.h>
#include <fikus/smp.h>
#include <fikus/sem.h>
#include <fikus/msg.h>
#include <fikus/shm.h>
#include <fikus/stat.h>
#include <fikus/syscalls.h>
#include <fikus/mman.h>
#include <fikus/file.h>
#include <fikus/fs.h>
#include <fikus/ipc.h>

#include <asm/setup.h>
#include <asm/uaccess.h>
#include <asm/cachectl.h>
#include <asm/traps.h>
#include <asm/unistd.h>

/* sys_cacheflush -- no support.  */
asmlinkage int
sys_cacheflush (unsigned long addr, int scope, int cache, unsigned long len)
{
	return -EINVAL;
}

asmlinkage int sys_getpagesize(void)
{
	return PAGE_SIZE;
}

#if defined(CONFIG_SYSCALL_PRINT)
asmlinkage void syscall_print(void *dummy,...)
{
	struct pt_regs *regs = (struct pt_regs *) ((unsigned char *)&dummy-4);
	printk("call %06lx:%ld 1:%08lx,2:%08lx,3:%08lx,ret:%08lx\n",
               ((regs->pc)&0xffffff)-2,regs->orig_er0,regs->er1,regs->er2,regs->er3,regs->er0);
}
#endif
