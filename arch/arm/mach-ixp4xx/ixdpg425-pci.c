/*
 * arch/arm/mach-ixp4xx/ixdpg425-pci.c
 *
 * PCI setup routines for Intel IXDPG425 Platform
 *
 * Copyright (C) 2004 MontaVista Softwrae, Inc.
 *
 * Maintainer: Deepak Saxena <dsaxena@plexity.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <fikus/kernel.h>
#include <fikus/pci.h>
#include <fikus/init.h>
#include <fikus/irq.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>

#include <asm/mach/pci.h>

void __init ixdpg425_pci_preinit(void)
{
	irq_set_irq_type(IRQ_IXP4XX_GPIO6, IRQ_TYPE_LEVEL_LOW);
	irq_set_irq_type(IRQ_IXP4XX_GPIO7, IRQ_TYPE_LEVEL_LOW);

	ixp4xx_pci_preinit();
}

static int __init ixdpg425_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	if (slot == 12 || slot == 13)
		return IRQ_IXP4XX_GPIO7;
	else if (slot == 14)
		return IRQ_IXP4XX_GPIO6;
	else return -1;
}

struct hw_pci ixdpg425_pci __initdata = {
	.nr_controllers = 1,
	.ops		= &ixp4xx_ops,
	.preinit =        ixdpg425_pci_preinit,
	.setup =          ixp4xx_setup,
	.map_irq =        ixdpg425_map_irq,
};

int __init ixdpg425_pci_init(void)
{
	if (machine_is_ixdpg425())
		pci_common_init(&ixdpg425_pci);
	return 0;
}

subsys_initcall(ixdpg425_pci_init);
