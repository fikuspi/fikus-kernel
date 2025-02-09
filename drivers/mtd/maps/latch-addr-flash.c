/*
 * Interface for NOR flash driver whose high address lines are latched
 *
 * Copyright © 2000 Nicolas Pitre <nico@cam.org>
 * Copyright © 2005-2008 Analog Devices Inc.
 * Copyright © 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/mtd/mtd.h>
#include <fikus/mtd/map.h>
#include <fikus/mtd/partitions.h>
#include <fikus/platform_device.h>
#include <fikus/mtd/latch-addr-flash.h>
#include <fikus/slab.h>

#define DRIVER_NAME "latch-addr-flash"

struct latch_addr_flash_info {
	struct mtd_info		*mtd;
	struct map_info		map;
	struct resource		*res;

	void			(*set_window)(unsigned long offset, void *data);
	void			*data;

	/* cache; could be found out of res */
	unsigned long		win_mask;

	spinlock_t		lock;
};

static map_word lf_read(struct map_info *map, unsigned long ofs)
{
	struct latch_addr_flash_info *info;
	map_word datum;

	info = (struct latch_addr_flash_info *)map->map_priv_1;

	spin_lock(&info->lock);

	info->set_window(ofs, info->data);
	datum = inline_map_read(map, info->win_mask & ofs);

	spin_unlock(&info->lock);

	return datum;
}

static void lf_write(struct map_info *map, map_word datum, unsigned long ofs)
{
	struct latch_addr_flash_info *info;

	info = (struct latch_addr_flash_info *)map->map_priv_1;

	spin_lock(&info->lock);

	info->set_window(ofs, info->data);
	inline_map_write(map, datum, info->win_mask & ofs);

	spin_unlock(&info->lock);
}

static void lf_copy_from(struct map_info *map, void *to,
		unsigned long from, ssize_t len)
{
	struct latch_addr_flash_info *info =
		(struct latch_addr_flash_info *) map->map_priv_1;
	unsigned n;

	while (len > 0) {
		n = info->win_mask + 1 - (from & info->win_mask);
		if (n > len)
			n = len;

		spin_lock(&info->lock);

		info->set_window(from, info->data);
		memcpy_fromio(to, map->virt + (from & info->win_mask), n);

		spin_unlock(&info->lock);

		to += n;
		from += n;
		len -= n;
	}
}

static char *rom_probe_types[] = { "cfi_probe", NULL };

static int latch_addr_flash_remove(struct platform_device *dev)
{
	struct latch_addr_flash_info *info;
	struct latch_addr_flash_data *latch_addr_data;

	info = platform_get_drvdata(dev);
	if (info == NULL)
		return 0;

	latch_addr_data = dev_get_platdata(&dev->dev);

	if (info->mtd != NULL) {
		mtd_device_unregister(info->mtd);
		map_destroy(info->mtd);
	}

	if (info->map.virt != NULL)
		iounmap(info->map.virt);

	if (info->res != NULL)
		release_mem_region(info->res->start, resource_size(info->res));

	kfree(info);

	if (latch_addr_data->done)
		latch_addr_data->done(latch_addr_data->data);

	return 0;
}

static int latch_addr_flash_probe(struct platform_device *dev)
{
	struct latch_addr_flash_data *latch_addr_data;
	struct latch_addr_flash_info *info;
	resource_size_t win_base = dev->resource->start;
	resource_size_t win_size = resource_size(dev->resource);
	char **probe_type;
	int chipsel;
	int err;

	latch_addr_data = dev_get_platdata(&dev->dev);
	if (latch_addr_data == NULL)
		return -ENODEV;

	pr_notice("latch-addr platform flash device: %#llx byte "
		  "window at %#.8llx\n",
		  (unsigned long long)win_size, (unsigned long long)win_base);

	chipsel = dev->id;

	if (latch_addr_data->init) {
		err = latch_addr_data->init(latch_addr_data->data, chipsel);
		if (err != 0)
			return err;
	}

	info = kzalloc(sizeof(struct latch_addr_flash_info), GFP_KERNEL);
	if (info == NULL) {
		err = -ENOMEM;
		goto done;
	}

	platform_set_drvdata(dev, info);

	info->res = request_mem_region(win_base, win_size, DRIVER_NAME);
	if (info->res == NULL) {
		dev_err(&dev->dev, "Could not reserve memory region\n");
		err = -EBUSY;
		goto free_info;
	}

	info->map.name		= DRIVER_NAME;
	info->map.size		= latch_addr_data->size;
	info->map.bankwidth	= latch_addr_data->width;

	info->map.phys		= NO_XIP;
	info->map.virt		= ioremap(win_base, win_size);
	if (!info->map.virt) {
		err = -ENOMEM;
		goto free_res;
	}

	info->map.map_priv_1	= (unsigned long)info;

	info->map.read		= lf_read;
	info->map.copy_from	= lf_copy_from;
	info->map.write		= lf_write;
	info->set_window	= latch_addr_data->set_window;
	info->data		= latch_addr_data->data;
	info->win_mask		= win_size - 1;

	spin_lock_init(&info->lock);

	for (probe_type = rom_probe_types; !info->mtd && *probe_type;
		probe_type++)
		info->mtd = do_map_probe(*probe_type, &info->map);

	if (info->mtd == NULL) {
		dev_err(&dev->dev, "map_probe failed\n");
		err = -ENODEV;
		goto iounmap;
	}
	info->mtd->owner = THIS_MODULE;

	mtd_device_parse_register(info->mtd, NULL, NULL,
				  latch_addr_data->parts,
				  latch_addr_data->nr_parts);
	return 0;

iounmap:
	iounmap(info->map.virt);
free_res:
	release_mem_region(info->res->start, resource_size(info->res));
free_info:
	kfree(info);
done:
	if (latch_addr_data->done)
		latch_addr_data->done(latch_addr_data->data);
	return err;
}

static struct platform_driver latch_addr_flash_driver = {
	.probe		= latch_addr_flash_probe,
	.remove		= latch_addr_flash_remove,
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

module_platform_driver(latch_addr_flash_driver);

MODULE_AUTHOR("David Griego <dgriego@mvista.com>");
MODULE_DESCRIPTION("MTD map driver for flashes addressed physically with upper "
		"address lines being set board specifically");
MODULE_LICENSE("GPL v2");
