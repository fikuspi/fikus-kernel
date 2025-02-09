/*
 * arch/powerpc/platforms/83xx/mpc834x_itx.c
 *
 * MPC834x ITX board specific routines
 *
 * Maintainer: Kumar Gala <galak@kernel.crashing.org>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <fikus/stddef.h>
#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/errno.h>
#include <fikus/reboot.h>
#include <fikus/pci.h>
#include <fikus/kdev_t.h>
#include <fikus/major.h>
#include <fikus/console.h>
#include <fikus/delay.h>
#include <fikus/seq_file.h>
#include <fikus/root_dev.h>
#include <fikus/of_platform.h>

#include <fikus/atomic.h>
#include <asm/time.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/ipic.h>
#include <asm/irq.h>
#include <asm/prom.h>
#include <asm/udbg.h>
#include <sysdev/fsl_soc.h>
#include <sysdev/fsl_pci.h>

#include "mpc83xx.h"

static struct of_device_id __initdata mpc834x_itx_ids[] = {
	{ .compatible = "fsl,pq2pro-localbus", },
	{},
};

static int __init mpc834x_itx_declare_of_platform_devices(void)
{
	mpc83xx_declare_of_platform_devices();
	return of_platform_bus_probe(NULL, mpc834x_itx_ids, NULL);
}
machine_device_initcall(mpc834x_itx, mpc834x_itx_declare_of_platform_devices);

/* ************************************************************************
 *
 * Setup the architecture
 *
 */
static void __init mpc834x_itx_setup_arch(void)
{
	if (ppc_md.progress)
		ppc_md.progress("mpc834x_itx_setup_arch()", 0);

	mpc83xx_setup_pci();

	mpc834x_usb_cfg();
}

/*
 * Called very early, MMU is off, device-tree isn't unflattened
 */
static int __init mpc834x_itx_probe(void)
{
        unsigned long root = of_get_flat_dt_root();

        return of_flat_dt_is_compatible(root, "MPC834xMITX");
}

define_machine(mpc834x_itx) {
	.name			= "MPC834x ITX",
	.probe			= mpc834x_itx_probe,
	.setup_arch		= mpc834x_itx_setup_arch,
	.init_IRQ		= mpc83xx_ipic_init_IRQ,
	.get_irq		= ipic_get_irq,
	.restart		= mpc83xx_restart,
	.time_init		= mpc83xx_time_init,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= udbg_progress,
};
