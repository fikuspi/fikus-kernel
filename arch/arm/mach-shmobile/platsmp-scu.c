/*
 * SMP support for SoCs with SCU covered by mach-shmobile
 *
 * Copyright (C) 2013  Magnus Damm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <fikus/delay.h>
#include <fikus/init.h>
#include <fikus/io.h>
#include <fikus/smp.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <mach/common.h>

void __init shmobile_smp_scu_prepare_cpus(unsigned int max_cpus)
{
	/* install boot code shared by all CPUs */
	shmobile_boot_fn = virt_to_phys(shmobile_smp_boot);
	shmobile_boot_arg = MPIDR_HWID_BITMASK;

	/* enable SCU and cache coherency on booting CPU */
	scu_enable(shmobile_scu_base);
	scu_power_mode(shmobile_scu_base, SCU_PM_NORMAL);
}

int shmobile_smp_scu_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	/* For this particular CPU register SCU boot vector */
	shmobile_smp_hook(cpu, virt_to_phys(shmobile_boot_scu),
			  (unsigned long)shmobile_scu_base);
	return 0;
}

#ifdef CONFIG_HOTPLUG_CPU
void shmobile_smp_scu_cpu_die(unsigned int cpu)
{
	/* For this particular CPU deregister boot vector */
	shmobile_smp_hook(cpu, 0, 0);

	dsb();
	flush_cache_all();

	/* disable cache coherency */
	scu_power_mode(shmobile_scu_base, SCU_PM_POWEROFF);

	/* jump to shared mach-shmobile sleep / reset code */
	shmobile_smp_sleep();
}

static int shmobile_smp_scu_psr_core_disabled(int cpu)
{
	unsigned long mask = SCU_PM_POWEROFF << (cpu * 8);

	if ((__raw_readl(shmobile_scu_base + 8) & mask) == mask)
		return 1;

	return 0;
}

int shmobile_smp_scu_cpu_kill(unsigned int cpu)
{
	int k;

	/* this function is running on another CPU than the offline target,
	 * here we need wait for shutdown code in platform_cpu_die() to
	 * finish before asking SoC-specific code to power off the CPU core.
	 */
	for (k = 0; k < 1000; k++) {
		if (shmobile_smp_scu_psr_core_disabled(cpu))
			return 1;

		mdelay(1);
	}

	return 0;
}
#endif
