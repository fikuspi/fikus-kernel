/* mga_drv.c -- Matrox G200/G400 driver -*- fikus-c -*-
 * Created: Mon Dec 13 01:56:22 1999 by jhartmann@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Fikus Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA FIKUS SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Rickard E. (Rik) Faith <faith@vafikus.com>
 *    Gareth Hughes <gareth@vafikus.com>
 */

#include <fikus/module.h>

#include <drm/drmP.h>
#include <drm/mga_drm.h>
#include "mga_drv.h"

#include <drm/drm_pciids.h>

static int mga_driver_device_is_agp(struct drm_device *dev);

static struct pci_device_id pciidlist[] = {
	mga_PCI_IDS
};

static const struct file_operations mga_driver_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.mmap = drm_mmap,
	.poll = drm_poll,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mga_compat_ioctl,
#endif
	.llseek = noop_llseek,
};

static struct drm_driver driver = {
	.driver_features =
	    DRIVER_USE_AGP | DRIVER_PCI_DMA |
	    DRIVER_HAVE_DMA | DRIVER_HAVE_IRQ | DRIVER_IRQ_SHARED,
	.dev_priv_size = sizeof(drm_mga_buf_priv_t),
	.load = mga_driver_load,
	.unload = mga_driver_unload,
	.lastclose = mga_driver_lastclose,
	.dma_quiescent = mga_driver_dma_quiescent,
	.device_is_agp = mga_driver_device_is_agp,
	.get_vblank_counter = mga_get_vblank_counter,
	.enable_vblank = mga_enable_vblank,
	.disable_vblank = mga_disable_vblank,
	.irq_preinstall = mga_driver_irq_preinstall,
	.irq_postinstall = mga_driver_irq_postinstall,
	.irq_uninstall = mga_driver_irq_uninstall,
	.irq_handler = mga_driver_irq_handler,
	.ioctls = mga_ioctls,
	.dma_ioctl = mga_dma_buffers,
	.fops = &mga_driver_fops,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

static struct pci_driver mga_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
};

static int __init mga_init(void)
{
	driver.num_ioctls = mga_max_ioctl;
	return drm_pci_init(&driver, &mga_pci_driver);
}

static void __exit mga_exit(void)
{
	drm_pci_exit(&driver, &mga_pci_driver);
}

module_init(mga_init);
module_exit(mga_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");

/**
 * Determine if the device really is AGP or not.
 *
 * In addition to the usual tests performed by \c drm_device_is_agp, this
 * function detects PCI G450 cards that appear to the system exactly like
 * AGP G450 cards.
 *
 * \param dev   The device to be tested.
 *
 * \returns
 * If the device is a PCI G450, zero is returned.  Otherwise 2 is returned.
 */
static int mga_driver_device_is_agp(struct drm_device *dev)
{
	const struct pci_dev *const pdev = dev->pdev;

	/* There are PCI versions of the G450.  These cards have the
	 * same PCI ID as the AGP G450, but have an additional PCI-to-PCI
	 * bridge chip.  We detect these cards, which are not currently
	 * supported by this driver, by looking at the device ID of the
	 * bus the "card" is on.  If vendor is 0x3388 (Hint Corp) and the
	 * device is 0x0021 (HB6 Universal PCI-PCI bridge), we reject the
	 * device.
	 */

	if ((pdev->device == 0x0525) && pdev->bus->self
	    && (pdev->bus->self->vendor == 0x3388)
	    && (pdev->bus->self->device == 0x0021)) {
		return 0;
	}

	return 2;
}
