/*
 * PCI driver for the Synopsys DesignWare DMA Controller
 *
 * Copyright (C) 2013 Intel Corporation
 * Author: Andy Shevchenko <andriy.shevchenko@fikus.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/module.h>
#include <fikus/pci.h>
#include <fikus/device.h>

#include "internal.h"

static struct dw_dma_platform_data dw_pci_pdata = {
	.is_private = 1,
	.chan_allocation_order = CHAN_ALLOCATION_ASCENDING,
	.chan_priority = CHAN_PRIORITY_ASCENDING,
};

static int dw_pci_probe(struct pci_dev *pdev, const struct pci_device_id *pid)
{
	struct dw_dma_chip *chip;
	struct dw_dma_platform_data *pdata = (void *)pid->driver_data;
	int ret;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	ret = pcim_iomap_regions(pdev, 1 << 0, pci_name(pdev));
	if (ret) {
		dev_err(&pdev->dev, "I/O memory remapping failed\n");
		return ret;
	}

	pci_set_master(pdev);
	pci_try_set_mwi(pdev);

	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	ret = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &pdev->dev;
	chip->regs = pcim_iomap_table(pdev)[0];
	chip->irq = pdev->irq;

	ret = dw_dma_probe(chip, pdata);
	if (ret)
		return ret;

	pci_set_drvdata(pdev, chip);

	return 0;
}

static void dw_pci_remove(struct pci_dev *pdev)
{
	struct dw_dma_chip *chip = pci_get_drvdata(pdev);
	int ret;

	ret = dw_dma_remove(chip);
	if (ret)
		dev_warn(&pdev->dev, "can't remove device properly: %d\n", ret);
}

static DEFINE_PCI_DEVICE_TABLE(dw_pci_id_table) = {
	/* Medfield */
	{ PCI_VDEVICE(INTEL, 0x0827), (kernel_ulong_t)&dw_pci_pdata },
	{ PCI_VDEVICE(INTEL, 0x0830), (kernel_ulong_t)&dw_pci_pdata },

	/* BayTrail */
	{ PCI_VDEVICE(INTEL, 0x0f06), (kernel_ulong_t)&dw_pci_pdata },
	{ PCI_VDEVICE(INTEL, 0x0f40), (kernel_ulong_t)&dw_pci_pdata },
	{ }
};
MODULE_DEVICE_TABLE(pci, dw_pci_id_table);

static struct pci_driver dw_pci_driver = {
	.name		= "dw_dmac_pci",
	.id_table	= dw_pci_id_table,
	.probe		= dw_pci_probe,
	.remove		= dw_pci_remove,
};

module_pci_driver(dw_pci_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Synopsys DesignWare DMA Controller PCI driver");
MODULE_AUTHOR("Andy Shevchenko <andriy.shevchenko@fikus.intel.com>");
