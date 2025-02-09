/*
 * Based on fikus/arch/mips/txx9/rbtx4938/setup.c,
 *	    and RBTX49xx patch from CELF patch archive.
 *
 * Copyright 2001, 2003-2005 MontaVista Software Inc.
 * Copyright (C) 2004 by Ralf Baechle (ralf@fikus-mips.org)
 * (C) Copyright TOSHIBA CORPORATION 2000-2001, 2004-2007
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <fikus/init.h>
#include <fikus/pci.h>
#include <fikus/kernel.h>
#include <fikus/interrupt.h>
#include <asm/txx9/generic.h>
#include <asm/txx9/tx4938.h>

int __init tx4938_report_pciclk(void)
{
	int pciclk = 0;

	printk(KERN_INFO "PCIC --%s PCICLK:",
	       (__raw_readq(&tx4938_ccfgptr->ccfg) & TX4938_CCFG_PCI66) ?
	       " PCI66" : "");
	if (__raw_readq(&tx4938_ccfgptr->pcfg) & TX4938_PCFG_PCICLKEN_ALL) {
		u64 ccfg = __raw_readq(&tx4938_ccfgptr->ccfg);
		switch ((unsigned long)ccfg &
			TX4938_CCFG_PCIDIVMODE_MASK) {
		case TX4938_CCFG_PCIDIVMODE_4:
			pciclk = txx9_cpu_clock / 4; break;
		case TX4938_CCFG_PCIDIVMODE_4_5:
			pciclk = txx9_cpu_clock * 2 / 9; break;
		case TX4938_CCFG_PCIDIVMODE_5:
			pciclk = txx9_cpu_clock / 5; break;
		case TX4938_CCFG_PCIDIVMODE_5_5:
			pciclk = txx9_cpu_clock * 2 / 11; break;
		case TX4938_CCFG_PCIDIVMODE_8:
			pciclk = txx9_cpu_clock / 8; break;
		case TX4938_CCFG_PCIDIVMODE_9:
			pciclk = txx9_cpu_clock / 9; break;
		case TX4938_CCFG_PCIDIVMODE_10:
			pciclk = txx9_cpu_clock / 10; break;
		case TX4938_CCFG_PCIDIVMODE_11:
			pciclk = txx9_cpu_clock / 11; break;
		}
		printk("Internal(%u.%uMHz)",
		       (pciclk + 50000) / 1000000,
		       ((pciclk + 50000) / 100000) % 10);
	} else {
		printk("External");
		pciclk = -1;
	}
	printk("\n");
	return pciclk;
}

void __init tx4938_report_pci1clk(void)
{
	__u64 ccfg = __raw_readq(&tx4938_ccfgptr->ccfg);
	unsigned int pciclk =
		txx9_gbus_clock / ((ccfg & TX4938_CCFG_PCI1DMD) ? 4 : 2);

	printk(KERN_INFO "PCIC1 -- %sPCICLK:%u.%uMHz\n",
	       (ccfg & TX4938_CCFG_PCI1_66) ? "PCI66 " : "",
	       (pciclk + 50000) / 1000000,
	       ((pciclk + 50000) / 100000) % 10);
}

int __init tx4938_pciclk66_setup(void)
{
	int pciclk;

	/* Assert M66EN */
	tx4938_ccfg_set(TX4938_CCFG_PCI66);
	/* Double PCICLK (if possible) */
	if (__raw_readq(&tx4938_ccfgptr->pcfg) & TX4938_PCFG_PCICLKEN_ALL) {
		unsigned int pcidivmode = 0;
		u64 ccfg = __raw_readq(&tx4938_ccfgptr->ccfg);
		pcidivmode = (unsigned long)ccfg &
			TX4938_CCFG_PCIDIVMODE_MASK;
		switch (pcidivmode) {
		case TX4938_CCFG_PCIDIVMODE_8:
		case TX4938_CCFG_PCIDIVMODE_4:
			pcidivmode = TX4938_CCFG_PCIDIVMODE_4;
			pciclk = txx9_cpu_clock / 4;
			break;
		case TX4938_CCFG_PCIDIVMODE_9:
		case TX4938_CCFG_PCIDIVMODE_4_5:
			pcidivmode = TX4938_CCFG_PCIDIVMODE_4_5;
			pciclk = txx9_cpu_clock * 2 / 9;
			break;
		case TX4938_CCFG_PCIDIVMODE_10:
		case TX4938_CCFG_PCIDIVMODE_5:
			pcidivmode = TX4938_CCFG_PCIDIVMODE_5;
			pciclk = txx9_cpu_clock / 5;
			break;
		case TX4938_CCFG_PCIDIVMODE_11:
		case TX4938_CCFG_PCIDIVMODE_5_5:
		default:
			pcidivmode = TX4938_CCFG_PCIDIVMODE_5_5;
			pciclk = txx9_cpu_clock * 2 / 11;
			break;
		}
		tx4938_ccfg_change(TX4938_CCFG_PCIDIVMODE_MASK,
				   pcidivmode);
		printk(KERN_DEBUG "PCICLK: ccfg:%08lx\n",
		       (unsigned long)__raw_readq(&tx4938_ccfgptr->ccfg));
	} else
		pciclk = -1;
	return pciclk;
}

int __init tx4938_pcic1_map_irq(const struct pci_dev *dev, u8 slot)
{
	if (get_tx4927_pcicptr(dev->bus->sysdata) == tx4938_pcic1ptr) {
		switch (slot) {
		case TX4927_PCIC_IDSEL_AD_TO_SLOT(31):
			if (__raw_readq(&tx4938_ccfgptr->pcfg) &
			    TX4938_PCFG_ETH0_SEL)
				return TXX9_IRQ_BASE + TX4938_IR_ETH0;
			break;
		case TX4927_PCIC_IDSEL_AD_TO_SLOT(30):
			if (__raw_readq(&tx4938_ccfgptr->pcfg) &
			    TX4938_PCFG_ETH1_SEL)
				return TXX9_IRQ_BASE + TX4938_IR_ETH1;
			break;
		}
		return 0;
	}
	return -1;
}

void __init tx4938_setup_pcierr_irq(void)
{
	if (request_irq(TXX9_IRQ_BASE + TX4938_IR_PCIERR,
			tx4927_pcierr_interrupt,
			0, "PCI error",
			(void *)TX4927_PCIC_REG))
		printk(KERN_WARNING "Failed to request irq for PCIERR\n");
}
