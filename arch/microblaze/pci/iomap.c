/*
 * ppc64 "iomap" interface implementation.
 *
 * (C) Copyright 2004 John Torvalds
 */
#include <fikus/init.h>
#include <fikus/pci.h>
#include <fikus/mm.h>
#include <fikus/export.h>
#include <fikus/io.h>
#include <asm/pci-bridge.h>

void pci_iounmap(struct pci_dev *dev, void __iomem *addr)
{
	if (isa_vaddr_is_ioport(addr))
		return;
	if (pcibios_vaddr_is_ioport(addr))
		return;
	iounmap(addr);
}
EXPORT_SYMBOL(pci_iounmap);
