/*
 * Functions for ST-RAM allocations
 *
 * Copyright 1994-97 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#include <fikus/types.h>
#include <fikus/kernel.h>
#include <fikus/mm.h>
#include <fikus/kdev_t.h>
#include <fikus/major.h>
#include <fikus/init.h>
#include <fikus/slab.h>
#include <fikus/vmalloc.h>
#include <fikus/pagemap.h>
#include <fikus/bootmem.h>
#include <fikus/mount.h>
#include <fikus/blkdev.h>
#include <fikus/module.h>

#include <asm/setup.h>
#include <asm/machdep.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/atarihw.h>
#include <asm/atari_stram.h>
#include <asm/io.h>


/*
 * The ST-RAM allocator allocates memory from a pool of reserved ST-RAM of
 * configurable size, set aside on ST-RAM init.
 * As long as this pool is not exhausted, allocation of real ST-RAM can be
 * guaranteed.
 */

/* set if kernel is in ST-RAM */
static int kernel_in_stram;

static struct resource stram_pool = {
	.name = "ST-RAM Pool"
};

static unsigned long pool_size = 1024*1024;


static int __init atari_stram_setup(char *arg)
{
	if (!MACH_IS_ATARI)
		return 0;

	pool_size = memparse(arg, NULL);
	return 0;
}

early_param("stram_pool", atari_stram_setup);


/*
 * This init function is called very early by atari/config.c
 * It initializes some internal variables needed for stram_alloc()
 */
void __init atari_stram_init(void)
{
	int i;
	void *stram_start;

	/*
	 * determine whether kernel code resides in ST-RAM
	 * (then ST-RAM is the first memory block at virtual 0x0)
	 */
	stram_start = phys_to_virt(0);
	kernel_in_stram = (stram_start == 0);

	for (i = 0; i < m68k_num_memory; ++i) {
		if (m68k_memory[i].addr == 0) {
			return;
		}
	}

	/* Should never come here! (There is always ST-Ram!) */
	panic("atari_stram_init: no ST-RAM found!");
}


/*
 * This function is called from setup_arch() to reserve the pages needed for
 * ST-RAM management.
 */
void __init atari_stram_reserve_pages(void *start_mem)
{
	/*
	 * always reserve first page of ST-RAM, the first 2 KiB are
	 * supervisor-only!
	 */
	if (!kernel_in_stram)
		reserve_bootmem(0, PAGE_SIZE, BOOTMEM_DEFAULT);

	stram_pool.start = (resource_size_t)alloc_bootmem_low_pages(pool_size);
	stram_pool.end = stram_pool.start + pool_size - 1;
	request_resource(&iomem_resource, &stram_pool);

	pr_debug("atari_stram pool: size = %lu bytes, resource = %pR\n",
		 pool_size, &stram_pool);
}


void *atari_stram_alloc(unsigned long size, const char *owner)
{
	struct resource *res;
	int error;

	pr_debug("atari_stram_alloc: allocate %lu bytes\n", size);

	/* round up */
	size = PAGE_ALIGN(size);

	res = kzalloc(sizeof(struct resource), GFP_KERNEL);
	if (!res)
		return NULL;

	res->name = owner;
	error = allocate_resource(&stram_pool, res, size, 0, UINT_MAX,
				  PAGE_SIZE, NULL, NULL);
	if (error < 0) {
		pr_err("atari_stram_alloc: allocate_resource() failed %d!\n",
		       error);
		kfree(res);
		return NULL;
	}

	pr_debug("atari_stram_alloc: returning %pR\n", res);
	return (void *)res->start;
}
EXPORT_SYMBOL(atari_stram_alloc);


void atari_stram_free(void *addr)
{
	unsigned long start = (unsigned long)addr;
	struct resource *res;
	unsigned long size;

	res = lookup_resource(&stram_pool, start);
	if (!res) {
		pr_err("atari_stram_free: trying to free nonexistent region "
		       "at %p\n", addr);
		return;
	}

	size = resource_size(res);
	pr_debug("atari_stram_free: free %lu bytes at %p\n", size, addr);
	release_resource(res);
	kfree(res);
}
EXPORT_SYMBOL(atari_stram_free);
