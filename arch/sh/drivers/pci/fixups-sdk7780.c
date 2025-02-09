/*
 * arch/sh/drivers/pci/fixups-sdk7780.c
 *
 * PCI fixups for the SDK7780SE03
 *
 * Copyright (C) 2003  Lineo uSolutions, Inc.
 * Copyright (C) 2004 - 2006  Paul Mundt
 * Copyright (C) 2006  Nobuhiro Iwamatsu
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <fikus/pci.h>
#include <fikus/io.h>
#include <fikus/sh_intc.h>
#include "pci-sh4.h"

#define IRQ_INTA	evt2irq(0xa20)
#define IRQ_INTB	evt2irq(0xa40)
#define IRQ_INTC	evt2irq(0xa60)
#define IRQ_INTD	evt2irq(0xa80)

/* IDSEL [16][17][18][19][20][21][22][23][24][25][26][27][28][29][30][31] */
static char sdk7780_irq_tab[4][16] __initdata = {
	/* INTA */
	{ IRQ_INTA, IRQ_INTD, IRQ_INTC, IRQ_INTD, -1, -1, -1, -1, -1, -1,
	  -1, -1, -1, -1, -1, -1 },
	/* INTB */
	{ IRQ_INTB, IRQ_INTA, -1, IRQ_INTA, -1, -1, -1, -1, -1, -1, -1, -1,
	  -1, -1, -1, -1 },
	/* INTC */
	{ IRQ_INTC, IRQ_INTB, -1, IRQ_INTB, -1, -1, -1, -1, -1, -1, -1, -1,
	  -1, -1, -1, -1 },
	/* INTD */
	{ IRQ_INTD, IRQ_INTC, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	  -1, -1, -1 },
};

int __init pcibios_map_platform_irq(const struct pci_dev *pdev, u8 slot, u8 pin)
{
       return sdk7780_irq_tab[pin-1][slot];
}
