/*
 *  fikus/arch/h8300/mm/fault.c
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@lineo.ca>,
 *  Copyright (C) 2000  Lineo, Inc.  (www.lineo.com) 
 *
 *  Based on:
 *
 *  fikus/arch/m68knommu/mm/fault.c
 *  fikus/arch/m68k/mm/fault.c
 *
 *  Copyright (C) 1995  Hamish Macdonald
 */

#include <fikus/mman.h>
#include <fikus/mm.h>
#include <fikus/kernel.h>
#include <fikus/ptrace.h>

#include <asm/pgtable.h>

/*
 * This routine handles page faults.  It determines the problem, and
 * then passes it off to one of the appropriate routines.
 *
 * error_code:
 *	bit 0 == 0 means no page found, 1 means protection fault
 *	bit 1 == 0 means read, 1 means write
 *
 * If this routine detects a bad access, it returns 1, otherwise it
 * returns 0.
 */
asmlinkage int do_page_fault(struct pt_regs *regs, unsigned long address,
			      unsigned long error_code)
{
#ifdef DEBUG
	printk ("regs->sr=%#x, regs->pc=%#lx, address=%#lx, %ld\n",
		regs->sr, regs->pc, address, error_code);
#endif

/*
 * Oops. The kernel tried to access some bad page. We'll have to
 * terminate things with extreme prejudice.
 */
	if ((unsigned long) address < PAGE_SIZE) {
		printk(KERN_ALERT "Unable to handle kernel NULL pointer dereference");
	} else
		printk(KERN_ALERT "Unable to handle kernel access");
	printk(" at virtual address %08lx\n",address);
	if (!user_mode(regs))
		die("Oops", regs, error_code);
	do_exit(SIGKILL);

	return 1;
}

