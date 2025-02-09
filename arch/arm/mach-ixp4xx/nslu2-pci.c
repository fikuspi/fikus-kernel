/*
 * arch/arm/mach-ixp4xx/nslu2-pci.c
 *
 * NSLU2 board-level PCI initialization
 *
 * based on ixdp425-pci.c:
 *	Copyright (C) 2002 Intel Corporation.
 *	Copyright (C) 2003-2004 MontaVista Software, Inc.
 *
 * Maintainer: http://www.nslu2-fikus.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <fikus/pci.h>
#include <fikus/init.h>
#include <fikus/irq.h>
#include <asm/mach/pci.h>
#include <asm/mach-types.h>

#define MAX_DEV		3
#define IRQ_LINES	3

/* PCI controller GPIO to IRQ pin mappings */
#define INTA		11
#define INTB		10
#define INTC		9
#define INTD		8

void __init nslu2_pci_preinit(void)
{
	irq_set_irq_type(IXP4XX_GPIO_IRQ(INTA), IRQ_TYPE_LEVEL_LOW);
	irq_set_irq_type(IXP4XX_GPIO_IRQ(INTB), IRQ_TYPE_LEVEL_LOW);
	irq_set_irq_type(IXP4XX_GPIO_IRQ(INTC), IRQ_TYPE_LEVEL_LOW);
	ixp4xx_pci_preinit();
}

static int __init nslu2_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	static int pci_irq_table[IRQ_LINES] = {
		IXP4XX_GPIO_IRQ(INTA),
		IXP4XX_GPIO_IRQ(INTB),
		IXP4XX_GPIO_IRQ(INTC),
	};

	if (slot >= 1 && slot <= MAX_DEV && pin >= 1 && pin <= IRQ_LINES)
		return pci_irq_table[(slot + pin - 2) % IRQ_LINES];

	return -1;
}

struct hw_pci __initdata nslu2_pci = {
	.nr_controllers = 1,
	.ops		= &ixp4xx_ops,
	.preinit	= nslu2_pci_preinit,
	.setup		= ixp4xx_setup,
	.map_irq	= nslu2_map_irq,
};

int __init nslu2_pci_init(void) /* monkey see, monkey do */
{
	if (machine_is_nslu2())
		pci_common_init(&nslu2_pci);

	return 0;
}

subsys_initcall(nslu2_pci_init);
