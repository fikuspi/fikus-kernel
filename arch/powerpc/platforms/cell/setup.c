/*
 *  fikus/arch/powerpc/platforms/cell/cell_setup.c
 *
 *  Copyright (C) 1995  John Torvalds
 *  Adapted from 'alpha' version by Gary Thomas
 *  Modified by Cort Dougan (cort@cs.nmt.edu)
 *  Modified by PPC64 Team, IBM Corp
 *  Modified by Cell Team, IBM Deutschland Entwicklung GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#undef DEBUG

#include <fikus/sched.h>
#include <fikus/kernel.h>
#include <fikus/mm.h>
#include <fikus/stddef.h>
#include <fikus/export.h>
#include <fikus/unistd.h>
#include <fikus/user.h>
#include <fikus/reboot.h>
#include <fikus/init.h>
#include <fikus/delay.h>
#include <fikus/irq.h>
#include <fikus/seq_file.h>
#include <fikus/root_dev.h>
#include <fikus/console.h>
#include <fikus/mutex.h>
#include <fikus/memory_hotplug.h>
#include <fikus/of_platform.h>

#include <asm/mmu.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/prom.h>
#include <asm/rtas.h>
#include <asm/pci-bridge.h>
#include <asm/iommu.h>
#include <asm/dma.h>
#include <asm/machdep.h>
#include <asm/time.h>
#include <asm/nvram.h>
#include <asm/cputable.h>
#include <asm/ppc-pci.h>
#include <asm/irq.h>
#include <asm/spu.h>
#include <asm/spu_priv1.h>
#include <asm/udbg.h>
#include <asm/mpic.h>
#include <asm/cell-regs.h>
#include <asm/io-workarounds.h>

#include "interrupt.h"
#include "pervasive.h"
#include "ras.h"

#ifdef DEBUG
#define DBG(fmt...) udbg_printf(fmt)
#else
#define DBG(fmt...)
#endif

static void cell_show_cpuinfo(struct seq_file *m)
{
	struct device_node *root;
	const char *model = "";

	root = of_find_node_by_path("/");
	if (root)
		model = of_get_property(root, "model", NULL);
	seq_printf(m, "machine\t\t: CHRP %s\n", model);
	of_node_put(root);
}

static void cell_progress(char *s, unsigned short hex)
{
	printk("*** %04x : %s\n", hex, s ? s : "");
}

static void cell_fixup_pcie_rootcomplex(struct pci_dev *dev)
{
	struct pci_controller *hose;
	const char *s;
	int i;

	if (!machine_is(cell))
		return;

	/* We're searching for a direct child of the PHB */
	if (dev->bus->self != NULL || dev->devfn != 0)
		return;

	hose = pci_bus_to_host(dev->bus);
	if (hose == NULL)
		return;

	/* Only on PCIE */
	if (!of_device_is_compatible(hose->dn, "pciex"))
		return;

	/* And only on axon */
	s = of_get_property(hose->dn, "model", NULL);
	if (!s || strcmp(s, "Axon") != 0)
		return;

	for (i = 0; i < PCI_BRIDGE_RESOURCES; i++) {
		dev->resource[i].start = dev->resource[i].end = 0;
		dev->resource[i].flags = 0;
	}

	printk(KERN_DEBUG "PCI: Hiding resources on Axon PCIE RC %s\n",
	       pci_name(dev));
}
DECLARE_PCI_FIXUP_HEADER(PCI_ANY_ID, PCI_ANY_ID, cell_fixup_pcie_rootcomplex);

static int cell_setup_phb(struct pci_controller *phb)
{
	const char *model;
	struct device_node *np;

	int rc = rtas_setup_phb(phb);
	if (rc)
		return rc;

	np = phb->dn;
	model = of_get_property(np, "model", NULL);
	if (model == NULL || strcmp(np->name, "pci"))
		return 0;

	/* Setup workarounds for spider */
	if (strcmp(model, "Spider"))
		return 0;

	iowa_register_bus(phb, &spiderpci_ops, &spiderpci_iowa_init,
				  (void *)SPIDER_PCI_REG_BASE);
	return 0;
}

static const struct of_device_id cell_bus_ids[] __initconst = {
	{ .type = "soc", },
	{ .compatible = "soc", },
	{ .type = "spider", },
	{ .type = "axon", },
	{ .type = "plb5", },
	{ .type = "plb4", },
	{ .type = "opb", },
	{ .type = "ebc", },
	{},
};

static int __init cell_publish_devices(void)
{
	struct device_node *root = of_find_node_by_path("/");
	struct device_node *np;
	int node;

	/* Publish OF platform devices for southbridge IOs */
	of_platform_bus_probe(NULL, cell_bus_ids, NULL);

	/* On spider based blades, we need to manually create the OF
	 * platform devices for the PCI host bridges
	 */
	for_each_child_of_node(root, np) {
		if (np->type == NULL || (strcmp(np->type, "pci") != 0 &&
					 strcmp(np->type, "pciex") != 0))
			continue;
		of_platform_device_create(np, NULL, NULL);
	}

	/* There is no device for the MIC memory controller, thus we create
	 * a platform device for it to attach the EDAC driver to.
	 */
	for_each_online_node(node) {
		if (cbe_get_cpu_mic_tm_regs(cbe_node_to_cpu(node)) == NULL)
			continue;
		platform_device_register_simple("cbe-mic", node, NULL, 0);
	}

	return 0;
}
machine_subsys_initcall(cell, cell_publish_devices);

static void __init mpic_init_IRQ(void)
{
	struct device_node *dn;
	struct mpic *mpic;

	for (dn = NULL;
	     (dn = of_find_node_by_name(dn, "interrupt-controller"));) {
		if (!of_device_is_compatible(dn, "CBEA,platform-open-pic"))
			continue;

		/* The MPIC driver will get everything it needs from the
		 * device-tree, just pass 0 to all arguments
		 */
		mpic = mpic_alloc(dn, 0, MPIC_SECONDARY | MPIC_NO_RESET,
				0, 0, " MPIC     ");
		if (mpic == NULL)
			continue;
		mpic_init(mpic);
	}
}


static void __init cell_init_irq(void)
{
	iic_init_IRQ();
	spider_init_IRQ();
	mpic_init_IRQ();
}

static void __init cell_set_dabrx(void)
{
	mtspr(SPRN_DABRX, DABRX_KERNEL | DABRX_USER);
}

static void __init cell_setup_arch(void)
{
#ifdef CONFIG_SPU_BASE
	spu_priv1_ops = &spu_priv1_mmio_ops;
	spu_management_ops = &spu_management_of_ops;
#endif

	cbe_regs_init();

	cell_set_dabrx();

#ifdef CONFIG_CBE_RAS
	cbe_ras_init();
#endif

#ifdef CONFIG_SMP
	smp_init_cell();
#endif
	/* init to some ~sane value until calibrate_delay() runs */
	loops_per_jiffy = 50000000;

	/* Find and initialize PCI host bridges */
	init_pci_config_tokens();

	cbe_pervasive_init();
#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

	mmio_nvram_init();
}

static int __init cell_probe(void)
{
	unsigned long root = of_get_flat_dt_root();

	if (!of_flat_dt_is_compatible(root, "IBM,CBEA") &&
	    !of_flat_dt_is_compatible(root, "IBM,CPBW-1.0"))
		return 0;

	hpte_init_native();

	return 1;
}

define_machine(cell) {
	.name			= "Cell",
	.probe			= cell_probe,
	.setup_arch		= cell_setup_arch,
	.show_cpuinfo		= cell_show_cpuinfo,
	.restart		= rtas_restart,
	.power_off		= rtas_power_off,
	.halt			= rtas_halt,
	.get_boot_time		= rtas_get_boot_time,
	.get_rtc_time		= rtas_get_rtc_time,
	.set_rtc_time		= rtas_set_rtc_time,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= cell_progress,
	.init_IRQ       	= cell_init_irq,
	.pci_setup_phb		= cell_setup_phb,
};
