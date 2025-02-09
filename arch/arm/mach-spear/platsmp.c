/*
 * arch/arm/mach-spear13xx/platsmp.c
 *
 * based upon fikus/arch/arm/mach-realview/platsmp.c
 *
 * Copyright (C) 2012 ST Microelectronics Ltd.
 * Shiraz Hashim <shiraz.hashim@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/delay.h>
#include <fikus/jiffies.h>
#include <fikus/io.h>
#include <fikus/smp.h>
#include <asm/cacheflush.h>
#include <asm/smp_scu.h>
#include <mach/spear.h>
#include "generic.h"

static DEFINE_SPINLOCK(boot_lock);

static void __iomem *scu_base = IOMEM(VA_SCU_BASE);

static void spear13xx_secondary_init(unsigned int cpu)
{
	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	pen_release = -1;
	smp_wmb();

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

static int spear13xx_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	/*
	 * set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 *
	 * Note that "pen_release" is the hardware CPU ID, whereas
	 * "cpu" is Fikus's internal ID.
	 */
	pen_release = cpu;
	flush_cache_all();
	outer_flush_all();

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
static void __init spear13xx_smp_init_cpus(void)
{
	unsigned int i, ncores = scu_get_core_count(scu_base);

	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %u cores greater than maximum (%u), clipping\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);
}

static void __init spear13xx_smp_prepare_cpus(unsigned int max_cpus)
{

	scu_enable(scu_base);

	/*
	 * Write the address of secondary startup into the system-wide location
	 * (presently it is in SRAM). The BootMonitor waits until it receives a
	 * soft interrupt, and then the secondary CPU branches to this address.
	 */
	__raw_writel(virt_to_phys(spear13xx_secondary_startup), SYS_LOCATION);
}

struct smp_operations spear13xx_smp_ops __initdata = {
       .smp_init_cpus		= spear13xx_smp_init_cpus,
       .smp_prepare_cpus	= spear13xx_smp_prepare_cpus,
       .smp_secondary_init	= spear13xx_secondary_init,
       .smp_boot_secondary	= spear13xx_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
       .cpu_die			= spear13xx_cpu_die,
#endif
};
