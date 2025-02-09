/*
 * Copyright (C) 2004-2006 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/kernel.h>
#include <fikus/gfp.h>
#include <fikus/mm.h>
#include <fikus/swap.h>
#include <fikus/init.h>
#include <fikus/mmzone.h>
#include <fikus/module.h>
#include <fikus/bootmem.h>
#include <fikus/pagemap.h>
#include <fikus/nodemask.h>

#include <asm/page.h>
#include <asm/mmu_context.h>
#include <asm/tlb.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <asm/setup.h>
#include <asm/sections.h>

pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned_data;

struct page *empty_zero_page;
EXPORT_SYMBOL(empty_zero_page);

/*
 * Cache of MMU context last used.
 */
unsigned long mmu_context_cache = NO_CONTEXT;

/*
 * paging_init() sets up the page tables
 *
 * This routine also unmaps the page at virtual kernel address 0, so
 * that we can trap those pesky NULL-reference errors in the kernel.
 */
void __init paging_init(void)
{
	extern unsigned long _evba;
	void *zero_page;
	int nid;

	/*
	 * Make sure we can handle exceptions before enabling
	 * paging. Not that we should ever _get_ any exceptions this
	 * early, but you never know...
	 */
	printk("Exception vectors start at %p\n", &_evba);
	sysreg_write(EVBA, (unsigned long)&_evba);

	/*
	 * Since we are ready to handle exceptions now, we should let
	 * the CPU generate them...
	 */
	__asm__ __volatile__ ("csrf %0" : : "i"(SR_EM_BIT));

	/*
	 * Allocate the zero page. The allocator will panic if it
	 * can't satisfy the request, so no need to check.
	 */
	zero_page = alloc_bootmem_low_pages_node(NODE_DATA(0),
						 PAGE_SIZE);

	sysreg_write(PTBR, (unsigned long)swapper_pg_dir);
	enable_mmu();
	printk ("CPU: Paging enabled\n");

	for_each_online_node(nid) {
		pg_data_t *pgdat = NODE_DATA(nid);
		unsigned long zones_size[MAX_NR_ZONES];
		unsigned long low, start_pfn;

		start_pfn = pgdat->bdata->node_min_pfn;
		low = pgdat->bdata->node_low_pfn;

		memset(zones_size, 0, sizeof(zones_size));
		zones_size[ZONE_NORMAL] = low - start_pfn;

		printk("Node %u: start_pfn = 0x%lx, low = 0x%lx\n",
		       nid, start_pfn, low);

		free_area_init_node(nid, zones_size, start_pfn, NULL);

		printk("Node %u: mem_map starts at %p\n",
		       pgdat->node_id, pgdat->node_mem_map);
	}

	mem_map = NODE_DATA(0)->node_mem_map;

	empty_zero_page = virt_to_page(zero_page);
	flush_dcache_page(empty_zero_page);
}

void __init mem_init(void)
{
	pg_data_t *pgdat;

	high_memory = NULL;
	for_each_online_pgdat(pgdat)
		high_memory = max_t(void *, high_memory,
				    __va(pgdat_end_pfn(pgdat) << PAGE_SHIFT));

	set_max_mapnr(MAP_NR(high_memory));
	free_all_bootmem();
	mem_init_print_info(NULL);
}

void free_initmem(void)
{
	free_initmem_default(-1);
}

#ifdef CONFIG_BLK_DEV_INITRD
void free_initrd_mem(unsigned long start, unsigned long end)
{
	free_reserved_area((void *)start, (void *)end, -1, "initrd");
}
#endif
