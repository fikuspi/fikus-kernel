/*
 * UIO driver fo Humusoft MF624 DAQ card.
 * Copyright (C) 2011 Rostislav Lisovy <lisovy@gmail.com>,
 *                    Czech Technical University in Prague
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <fikus/init.h>
#include <fikus/module.h>
#include <fikus/device.h>
#include <fikus/pci.h>
#include <fikus/slab.h>
#include <fikus/io.h>
#include <fikus/kernel.h>
#include <fikus/uio_driver.h>

#define PCI_VENDOR_ID_HUMUSOFT		0x186c
#define PCI_DEVICE_ID_MF624		0x0624
#define PCI_SUBVENDOR_ID_HUMUSOFT	0x186c
#define PCI_SUBDEVICE_DEVICE		0x0624

/* BAR0 Interrupt control/status register */
#define INTCSR				0x4C
#define INTCSR_ADINT_ENABLE		(1 << 0)
#define INTCSR_CTR4INT_ENABLE		(1 << 3)
#define INTCSR_PCIINT_ENABLE		(1 << 6)
#define INTCSR_ADINT_STATUS		(1 << 2)
#define INTCSR_CTR4INT_STATUS		(1 << 5)

enum mf624_interrupt_source {ADC, CTR4, ALL};

void mf624_disable_interrupt(enum mf624_interrupt_source source,
			     struct uio_info *info)
{
	void __iomem *INTCSR_reg = info->mem[0].internal_addr + INTCSR;

	switch (source) {
	case ADC:
		iowrite32(ioread32(INTCSR_reg)
			& ~(INTCSR_ADINT_ENABLE | INTCSR_PCIINT_ENABLE),
			INTCSR_reg);
		break;

	case CTR4:
		iowrite32(ioread32(INTCSR_reg)
			& ~(INTCSR_CTR4INT_ENABLE | INTCSR_PCIINT_ENABLE),
			INTCSR_reg);
		break;

	case ALL:
	default:
		iowrite32(ioread32(INTCSR_reg)
			& ~(INTCSR_ADINT_ENABLE | INTCSR_CTR4INT_ENABLE
			    | INTCSR_PCIINT_ENABLE),
			INTCSR_reg);
		break;
	}
}

void mf624_enable_interrupt(enum mf624_interrupt_source source,
			    struct uio_info *info)
{
	void __iomem *INTCSR_reg = info->mem[0].internal_addr + INTCSR;

	switch (source) {
	case ADC:
		iowrite32(ioread32(INTCSR_reg)
			| INTCSR_ADINT_ENABLE | INTCSR_PCIINT_ENABLE,
			INTCSR_reg);
		break;

	case CTR4:
		iowrite32(ioread32(INTCSR_reg)
			| INTCSR_CTR4INT_ENABLE | INTCSR_PCIINT_ENABLE,
			INTCSR_reg);
		break;

	case ALL:
	default:
		iowrite32(ioread32(INTCSR_reg)
			| INTCSR_ADINT_ENABLE | INTCSR_CTR4INT_ENABLE
			| INTCSR_PCIINT_ENABLE,
			INTCSR_reg);
		break;
	}
}

static irqreturn_t mf624_irq_handler(int irq, struct uio_info *info)
{
	void __iomem *INTCSR_reg = info->mem[0].internal_addr + INTCSR;

	if ((ioread32(INTCSR_reg) & INTCSR_ADINT_ENABLE)
	    && (ioread32(INTCSR_reg) & INTCSR_ADINT_STATUS)) {
		mf624_disable_interrupt(ADC, info);
		return IRQ_HANDLED;
	}

	if ((ioread32(INTCSR_reg) & INTCSR_CTR4INT_ENABLE)
	    && (ioread32(INTCSR_reg) & INTCSR_CTR4INT_STATUS)) {
		mf624_disable_interrupt(CTR4, info);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int mf624_irqcontrol(struct uio_info *info, s32 irq_on)
{
	if (irq_on == 0)
		mf624_disable_interrupt(ALL, info);
	else if (irq_on == 1)
		mf624_enable_interrupt(ALL, info);

	return 0;
}

static int mf624_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct uio_info *info;

	info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	if (pci_enable_device(dev))
		goto out_free;

	if (pci_request_regions(dev, "mf624"))
		goto out_disable;

	info->name = "mf624";
	info->version = "0.0.1";

	/* Note: Datasheet says device uses BAR0, BAR1, BAR2 -- do not trust it */

	/* BAR0 */
	info->mem[0].name = "PCI chipset, interrupts, status "
			"bits, special functions";
	info->mem[0].addr = pci_resource_start(dev, 0);
	if (!info->mem[0].addr)
		goto out_release;
	info->mem[0].size = pci_resource_len(dev, 0);
	info->mem[0].memtype = UIO_MEM_PHYS;
	info->mem[0].internal_addr = pci_ioremap_bar(dev, 0);
	if (!info->mem[0].internal_addr)
		goto out_release;

	/* BAR2 */
	info->mem[1].name = "ADC, DAC, DIO";
	info->mem[1].addr = pci_resource_start(dev, 2);
	if (!info->mem[1].addr)
		goto out_unmap0;
	info->mem[1].size = pci_resource_len(dev, 2);
	info->mem[1].memtype = UIO_MEM_PHYS;
	info->mem[1].internal_addr = pci_ioremap_bar(dev, 2);
	if (!info->mem[1].internal_addr)
		goto out_unmap0;

	/* BAR4 */
	info->mem[2].name = "Counter/timer chip";
	info->mem[2].addr = pci_resource_start(dev, 4);
	if (!info->mem[2].addr)
		goto out_unmap1;
	info->mem[2].size = pci_resource_len(dev, 4);
	info->mem[2].memtype = UIO_MEM_PHYS;
	info->mem[2].internal_addr = pci_ioremap_bar(dev, 4);
	if (!info->mem[2].internal_addr)
		goto out_unmap1;

	info->irq = dev->irq;
	info->irq_flags = IRQF_SHARED;
	info->handler = mf624_irq_handler;

	info->irqcontrol = mf624_irqcontrol;

	if (uio_register_device(&dev->dev, info))
		goto out_unmap2;

	pci_set_drvdata(dev, info);

	return 0;

out_unmap2:
	iounmap(info->mem[2].internal_addr);
out_unmap1:
	iounmap(info->mem[1].internal_addr);
out_unmap0:
	iounmap(info->mem[0].internal_addr);

out_release:
	pci_release_regions(dev);

out_disable:
	pci_disable_device(dev);

out_free:
	kfree(info);
	return -ENODEV;
}

static void mf624_pci_remove(struct pci_dev *dev)
{
	struct uio_info *info = pci_get_drvdata(dev);

	mf624_disable_interrupt(ALL, info);

	uio_unregister_device(info);
	pci_release_regions(dev);
	pci_disable_device(dev);
	pci_set_drvdata(dev, NULL);

	iounmap(info->mem[0].internal_addr);
	iounmap(info->mem[1].internal_addr);
	iounmap(info->mem[2].internal_addr);

	kfree(info);
}

static DEFINE_PCI_DEVICE_TABLE(mf624_pci_id) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_HUMUSOFT, PCI_DEVICE_ID_MF624) },
	{ 0, }
};

static struct pci_driver mf624_pci_driver = {
	.name = "mf624",
	.id_table = mf624_pci_id,
	.probe = mf624_pci_probe,
	.remove = mf624_pci_remove,
};
MODULE_DEVICE_TABLE(pci, mf624_pci_id);

module_pci_driver(mf624_pci_driver);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Rostislav Lisovy <lisovy@gmail.com>");
