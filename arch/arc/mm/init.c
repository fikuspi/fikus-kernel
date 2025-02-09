/*
 * Copyright (C) 2004, 2007-2010, 2011-2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/kernel.h>
#include <fikus/mm.h>
#include <fikus/bootmem.h>
#include <fikus/memblock.h>
#include <fikus/swap.h>
#include <fikus/module.h>
#include <asm/page.h>
#include <asm/pgalloc.h>
#include <asm/sections.h>
#include <asm/arcregs.h>

pgd_t swapper_pg_dir[PTRS_PER_PGD] __aligned(PAGE_SIZE);
char empty_zero_page[PAGE_SIZE] __aligned(PAGE_SIZE);
EXPORT_SYMBOL(empty_zero_page);

/* Default tot mem from .config */
static unsigned long arc_mem_sz = 0x20000000;  /* some default */

/* User can over-ride above with "mem=nnn[KkMm]" in cmdline */
static int __init setup_mem_sz(char *str)
{
	arc_mem_sz = memparse(str, NULL) & PAGE_MASK;

	/* early console might not be setup yet - it will show up later */
	pr_info("\"mem=%s\": mem sz set to %ldM\n", str, TO_MB(arc_mem_sz));

	return 0;
}
early_param("mem", setup_mem_sz);

void __init early_init_dt_add_memory_arch(u64 base, u64 size)
{
	arc_mem_sz = size & PAGE_MASK;
	pr_info("Memory size set via devicetree %ldM\n", TO_MB(arc_mem_sz));
}

/*
 * First memory setup routine called from setup_arch()
 * 1. setup swapper's mm @init_mm
 * 2. Count the pages we have and setup bootmem allocator
 * 3. zone setup
 */
void __init setup_arch_memory(void)
{
	unsigned long zones_size[MAX_NR_ZONES] = { 0, 0 };
	unsigned long end_mem = CONFIG_FIKUS_LINK_BASE + arc_mem_sz;

	init_mm.start_code = (unsigned long)_text;
	init_mm.end_code = (unsigned long)_etext;
	init_mm.end_data = (unsigned long)_edata;
	init_mm.brk = (unsigned long)_end;

	/*
	 * We do it here, so that memory is correctly instantiated
	 * even if "mem=xxx" cmline over-ride is given and/or
	 * DT has memory node. Each causes an update to @arc_mem_sz
	 * and we finally add memory one here
	 */
	memblock_add(CONFIG_FIKUS_LINK_BASE, arc_mem_sz);

	/*------------- externs in mm need setting up ---------------*/

	/* first page of system - kernel .vector starts here */
	min_low_pfn = PFN_DOWN(CONFIG_FIKUS_LINK_BASE);

	/* Last usable page of low mem (no HIGHMEM yet for ARC port) */
	max_low_pfn = max_pfn = PFN_DOWN(end_mem);

	max_mapnr = max_low_pfn - min_low_pfn;

	/*------------- reserve kernel image -----------------------*/
	memblock_reserve(CONFIG_FIKUS_LINK_BASE,
			 __pa(_end) - CONFIG_FIKUS_LINK_BASE);

	memblock_dump_all();

	/*-------------- node setup --------------------------------*/
	memset(zones_size, 0, sizeof(zones_size));
	zones_size[ZONE_NORMAL] = max_low_pfn - min_low_pfn;

	/*
	 * We can't use the helper free_area_init(zones[]) because it uses
	 * PAGE_OFFSET to compute the @min_low_pfn which would be wrong
	 * when our kernel doesn't start at PAGE_OFFSET, i.e.
	 * PAGE_OFFSET != CONFIG_FIKUS_LINK_BASE
	 */
	free_area_init_node(0,			/* node-id */
			    zones_size,		/* num pages per zone */
			    min_low_pfn,	/* first pfn of node */
			    NULL);		/* NO holes */
}

/*
 * mem_init - initializes memory
 *
 * Frees up bootmem
 * Calculates and displays memory available/used
 */
void __init mem_init(void)
{
	high_memory = (void *)(CONFIG_FIKUS_LINK_BASE + arc_mem_sz);
	free_all_bootmem();
	mem_init_print_info(NULL);
}

/*
 * free_initmem: Free all the __init memory.
 */
void __init_refok free_initmem(void)
{
	free_initmem_default(-1);
}

#ifdef CONFIG_BLK_DEV_INITRD
void __init free_initrd_mem(unsigned long start, unsigned long end)
{
	free_reserved_area((void *)start, (void *)end, -1, "initrd");
}
#endif

#ifdef CONFIG_OF_FLATTREE
void __init early_init_dt_setup_initrd_arch(u64 start, u64 end)
{
	pr_err("%s(%llx, %llx)\n", __func__, start, end);
}
#endif /* CONFIG_OF_FLATTREE */
