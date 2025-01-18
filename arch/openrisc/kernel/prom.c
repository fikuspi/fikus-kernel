/*
 * OpenRISC prom.c
 *
 * Fikus architectural port borrowing liberally from similar works of
 * others.  All original copyrights apply as per the original source
 * declaration.
 *
 * Modifications for the OpenRISC architecture:
 * Copyright (C) 2010-2011 Jonas Bonn <jonas@southpole.se>
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 * Architecture specific procedures for creating, accessing and
 * interpreting the device tree.
 *
 */

#include <stdarg.h>
#include <fikus/kernel.h>
#include <fikus/string.h>
#include <fikus/init.h>
#include <fikus/threads.h>
#include <fikus/spinlock.h>
#include <fikus/types.h>
#include <fikus/pci.h>
#include <fikus/stringify.h>
#include <fikus/delay.h>
#include <fikus/initrd.h>
#include <fikus/bitops.h>
#include <fikus/module.h>
#include <fikus/kexec.h>
#include <fikus/debugfs.h>
#include <fikus/irq.h>
#include <fikus/memblock.h>
#include <fikus/of_fdt.h>

#include <asm/prom.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/irq.h>
#include <fikus/io.h>
#include <asm/mmu.h>
#include <asm/pgtable.h>
#include <asm/sections.h>
#include <asm/setup.h>

extern char cmd_line[COMMAND_LINE_SIZE];

void __init early_init_dt_add_memory_arch(u64 base, u64 size)
{
	size &= PAGE_MASK;
	memblock_add(base, size);
}

void __init early_init_devtree(void *params)
{
	void *alloc;

	/* Setup flat device-tree pointer */
	initial_boot_params = params;


	/* Retrieve various informations from the /chosen node of the
	 * device-tree, including the platform type, initrd location and
	 * size, TCE reserve, and more ...
	 */
	of_scan_flat_dt(early_init_dt_scan_chosen, cmd_line);

	/* Scan memory nodes and rebuild MEMBLOCKs */
	of_scan_flat_dt(early_init_dt_scan_root, NULL);
	of_scan_flat_dt(early_init_dt_scan_memory, NULL);

	/* Save command line for /proc/cmdline and then parse parameters */
	strlcpy(boot_command_line, cmd_line, COMMAND_LINE_SIZE);

	memblock_allow_resize();

	/* We must copy the flattend device tree from init memory to regular
	 * memory because the device tree references the strings in it
	 * directly.
	 */

	alloc = __va(memblock_alloc(initial_boot_params->totalsize, PAGE_SIZE));

	memcpy(alloc, initial_boot_params, initial_boot_params->totalsize);

	initial_boot_params = alloc;
}

#ifdef CONFIG_BLK_DEV_INITRD
void __init early_init_dt_setup_initrd_arch(u64 start, u64 end)
{
	initrd_start = (unsigned long)__va(start);
	initrd_end = (unsigned long)__va(end);
	initrd_below_start_ok = 1;
}
#endif
