/*
 * Support functions for the SH5 PCI hardware.
 *
 * Copyright (C) 2001 David J. Mckay (david.mckay@st.com)
 * Copyright (C) 2003, 2004 Paul Mundt
 * Copyright (C) 2004 Richard Curnow
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See fikus/COPYING for more information.
 */
#include <fikus/kernel.h>
#include <fikus/rwsem.h>
#include <fikus/smp.h>
#include <fikus/interrupt.h>
#include <fikus/init.h>
#include <fikus/errno.h>
#include <fikus/pci.h>
#include <fikus/delay.h>
#include <fikus/types.h>
#include <fikus/irq.h>
#include <asm/pci.h>
#include <asm/io.h>
#include "pci-sh5.h"

static int sh5pci_read(struct pci_bus *bus, unsigned int devfn, int where,
			int size, u32 *val)
{
	SH5PCI_WRITE(PAR, CONFIG_CMD(bus, devfn, where));

	switch (size) {
		case 1:
			*val = (u8)SH5PCI_READ_BYTE(PDR + (where & 3));
			break;
		case 2:
			*val = (u16)SH5PCI_READ_SHORT(PDR + (where & 2));
			break;
		case 4:
			*val = SH5PCI_READ(PDR);
			break;
	}

	return PCIBIOS_SUCCESSFUL;
}

static int sh5pci_write(struct pci_bus *bus, unsigned int devfn, int where,
			 int size, u32 val)
{
	SH5PCI_WRITE(PAR, CONFIG_CMD(bus, devfn, where));

	switch (size) {
		case 1:
			SH5PCI_WRITE_BYTE(PDR + (where & 3), (u8)val);
			break;
		case 2:
			SH5PCI_WRITE_SHORT(PDR + (where & 2), (u16)val);
			break;
		case 4:
			SH5PCI_WRITE(PDR, val);
			break;
	}

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops sh5_pci_ops = {
	.read		= sh5pci_read,
	.write		= sh5pci_write,
};
