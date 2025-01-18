/****************************************************************************/

/*
 *	ucfikus.c -- generic memory mapped MTD driver for ucfikus
 *
 *	(C) Copyright 2002, Greg Ungerer (gerg@snapgear.com)
 */

/****************************************************************************/

#include <fikus/module.h>
#include <fikus/types.h>
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/fs.h>
#include <fikus/mm.h>
#include <fikus/major.h>
#include <fikus/mtd/mtd.h>
#include <fikus/mtd/map.h>
#include <fikus/mtd/partitions.h>
#include <asm/io.h>
#include <asm/sections.h>

/****************************************************************************/

#ifdef CONFIG_MTD_ROM
#define MAP_NAME "rom"
#else
#define MAP_NAME "ram"
#endif

/*
 * Blackfin uses ucfikus_ram_map during startup, so it must not be static.
 * Provide a dummy declaration to make sparse happy.
 */
extern struct map_info ucfikus_ram_map;

struct map_info ucfikus_ram_map = {
	.name = MAP_NAME,
	.size = 0,
};

static unsigned long physaddr = -1;
module_param(physaddr, ulong, S_IRUGO);

static struct mtd_info *ucfikus_ram_mtdinfo;

/****************************************************************************/

static struct mtd_partition ucfikus_romfs[] = {
	{ .name = "ROMfs" }
};

#define	NUM_PARTITIONS	ARRAY_SIZE(ucfikus_romfs)

/****************************************************************************/

static int ucfikus_point(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, void **virt, resource_size_t *phys)
{
	struct map_info *map = mtd->priv;
	*virt = map->virt + from;
	if (phys)
		*phys = map->phys + from;
	*retlen = len;
	return(0);
}

/****************************************************************************/

static int __init ucfikus_mtd_init(void)
{
	struct mtd_info *mtd;
	struct map_info *mapp;

	mapp = &ucfikus_ram_map;

	if (physaddr == -1)
		mapp->phys = (resource_size_t)__bss_stop;
	else
		mapp->phys = physaddr;

	if (!mapp->size)
		mapp->size = PAGE_ALIGN(ntohl(*((unsigned long *)(mapp->phys + 8))));
	mapp->bankwidth = 4;

	printk("ucfikus[mtd]: probe address=0x%x size=0x%x\n",
	       	(int) mapp->phys, (int) mapp->size);

	/*
	 * The filesystem is guaranteed to be in direct mapped memory. It is
	 * directly following the kernels own bss region. Following the same
	 * mechanism used by architectures setting up traditional initrds we
	 * use phys_to_virt to get the virtual address of its start.
	 */
	mapp->virt = phys_to_virt(mapp->phys);

	if (mapp->virt == 0) {
		printk("ucfikus[mtd]: no virtual mapping?\n");
		return(-EIO);
	}

	simple_map_init(mapp);

	mtd = do_map_probe("map_" MAP_NAME, mapp);
	if (!mtd) {
		printk("ucfikus[mtd]: failed to find a mapping?\n");
		return(-ENXIO);
	}

	mtd->owner = THIS_MODULE;
	mtd->_point = ucfikus_point;
	mtd->priv = mapp;

	ucfikus_ram_mtdinfo = mtd;
	mtd_device_register(mtd, ucfikus_romfs, NUM_PARTITIONS);

	return(0);
}

/****************************************************************************/

static void __exit ucfikus_mtd_cleanup(void)
{
	if (ucfikus_ram_mtdinfo) {
		mtd_device_unregister(ucfikus_ram_mtdinfo);
		map_destroy(ucfikus_ram_mtdinfo);
		ucfikus_ram_mtdinfo = NULL;
	}
	if (ucfikus_ram_map.virt)
		ucfikus_ram_map.virt = 0;
}

/****************************************************************************/

module_init(ucfikus_mtd_init);
module_exit(ucfikus_mtd_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Greg Ungerer <gerg@snapgear.com>");
MODULE_DESCRIPTION("Generic MTD for uCfikus");

/****************************************************************************/
