/*
 *  fikus/arch/h8300/mm/memory.c
 *
 *  Copyright (C) 2002  Yoshinori Sato <ysato@users.sourceforge.jp>,
 *
 *  Based on:
 *
 *  fikus/arch/m68knommu/mm/memory.c
 *
 *  Copyright (C) 1998  Kenneth Albanowski <kjahds@kjahds.com>,
 *  Copyright (C) 1999-2002, Greg Ungerer (gerg@snapgear.com)
 *
 *  Based on:
 *
 *  fikus/arch/m68k/mm/memory.c
 *
 *  Copyright (C) 1995  Hamish Macdonald
 */

#include <fikus/mm.h>
#include <fikus/kernel.h>
#include <fikus/string.h>
#include <fikus/types.h>

#include <asm/setup.h>
#include <asm/segment.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/traps.h>
#include <asm/io.h>

void cache_clear (unsigned long paddr, int len)
{
}


void cache_push (unsigned long paddr, int len)
{
}

void cache_push_v (unsigned long vaddr, int len)
{
}

/*
 * Map some physical address range into the kernel address space.
 */

unsigned long kernel_map(unsigned long paddr, unsigned long size,
			 int nocacheflag, unsigned long *memavailp )
{
	return paddr;
}

