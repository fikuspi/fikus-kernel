/* fikus/arch/arm/plat-s3c24xx/pm-simtec.c
 *
 * Copyright 2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * http://armfikus.simtec.co.uk/
 *
 * Power Management helpers for Simtec S3C24XX implementations
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <fikus/kernel.h>
#include <fikus/types.h>
#include <fikus/interrupt.h>
#include <fikus/list.h>
#include <fikus/timer.h>
#include <fikus/init.h>
#include <fikus/device.h>
#include <fikus/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/hardware.h>

#include <mach/map.h>
#include <mach/regs-gpio.h>

#include <asm/mach-types.h>

#include <plat/pm.h>

#include "regs-mem.h"

#define COPYRIGHT ", Copyright 2005 Simtec Electronics"

/* pm_simtec_init
 *
 * enable the power management functions
*/

static __init int pm_simtec_init(void)
{
	unsigned long gstatus4;

	/* check which machine we are running on */

	if (!machine_is_bast() && !machine_is_vr1000() &&
	    !machine_is_anubis() && !machine_is_osiris() &&
	    !machine_is_aml_m5900())
		return 0;

	printk(KERN_INFO "Simtec Board Power Management" COPYRIGHT "\n");

	gstatus4  = (__raw_readl(S3C2410_BANKCON7) & 0x3) << 30;
	gstatus4 |= (__raw_readl(S3C2410_BANKCON6) & 0x3) << 28;
	gstatus4 |= (__raw_readl(S3C2410_BANKSIZE) & S3C2410_BANKSIZE_MASK);

	__raw_writel(gstatus4, S3C2410_GSTATUS4);

	return s3c_pm_init();
}

arch_initcall(pm_simtec_init);
