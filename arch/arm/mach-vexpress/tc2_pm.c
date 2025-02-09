/*
 * arch/arm/mach-vexpress/tc2_pm.c - TC2 power management support
 *
 * Created by:	Nicolas Pitre, October 2012
 * Copyright:	(C) 2012-2013  Linaro Limited
 *
 * Some portions of this file were originally written by Achin Gupta
 * Copyright:   (C) 2012  ARM Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/init.h>
#include <fikus/io.h>
#include <fikus/kernel.h>
#include <fikus/of_address.h>
#include <fikus/spinlock.h>
#include <fikus/errno.h>
#include <fikus/irqchip/arm-gic.h>

#include <asm/mcpm.h>
#include <asm/proc-fns.h>
#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/cp15.h>

#include <fikus/arm-cci.h>

#include "spc.h"

/* SCC conf registers */
#define A15_CONF		0x400
#define A7_CONF			0x500
#define SYS_INFO		0x700
#define SPC_BASE		0xb00

/*
 * We can't use regular spinlocks. In the switcher case, it is possible
 * for an outbound CPU to call power_down() after its inbound counterpart
 * is already live using the same logical CPU number which trips lockdep
 * debugging.
 */
static arch_spinlock_t tc2_pm_lock = __ARCH_SPIN_LOCK_UNLOCKED;

#define TC2_CLUSTERS			2
#define TC2_MAX_CPUS_PER_CLUSTER	3

static unsigned int tc2_nr_cpus[TC2_CLUSTERS];

/* Keep per-cpu usage count to cope with unordered up/down requests */
static int tc2_pm_use_count[TC2_MAX_CPUS_PER_CLUSTER][TC2_CLUSTERS];

#define tc2_cluster_unused(cluster) \
	(!tc2_pm_use_count[0][cluster] && \
	 !tc2_pm_use_count[1][cluster] && \
	 !tc2_pm_use_count[2][cluster])

static int tc2_pm_power_up(unsigned int cpu, unsigned int cluster)
{
	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	if (cluster >= TC2_CLUSTERS || cpu >= tc2_nr_cpus[cluster])
		return -EINVAL;

	/*
	 * Since this is called with IRQs enabled, and no arch_spin_lock_irq
	 * variant exists, we need to disable IRQs manually here.
	 */
	local_irq_disable();
	arch_spin_lock(&tc2_pm_lock);

	if (tc2_cluster_unused(cluster))
		ve_spc_powerdown(cluster, false);

	tc2_pm_use_count[cpu][cluster]++;
	if (tc2_pm_use_count[cpu][cluster] == 1) {
		ve_spc_set_resume_addr(cluster, cpu,
				       virt_to_phys(mcpm_entry_point));
		ve_spc_cpu_wakeup_irq(cluster, cpu, true);
	} else if (tc2_pm_use_count[cpu][cluster] != 2) {
		/*
		 * The only possible values are:
		 * 0 = CPU down
		 * 1 = CPU (still) up
		 * 2 = CPU requested to be up before it had a chance
		 *     to actually make itself down.
		 * Any other value is a bug.
		 */
		BUG();
	}

	arch_spin_unlock(&tc2_pm_lock);
	local_irq_enable();

	return 0;
}

static void tc2_pm_down(u64 residency)
{
	unsigned int mpidr, cpu, cluster;
	bool last_man = false, skip_wfi = false;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	BUG_ON(cluster >= TC2_CLUSTERS || cpu >= TC2_MAX_CPUS_PER_CLUSTER);

	__mcpm_cpu_going_down(cpu, cluster);

	arch_spin_lock(&tc2_pm_lock);
	BUG_ON(__mcpm_cluster_state(cluster) != CLUSTER_UP);
	tc2_pm_use_count[cpu][cluster]--;
	if (tc2_pm_use_count[cpu][cluster] == 0) {
		ve_spc_cpu_wakeup_irq(cluster, cpu, true);
		if (tc2_cluster_unused(cluster)) {
			ve_spc_powerdown(cluster, true);
			ve_spc_global_wakeup_irq(true);
			last_man = true;
		}
	} else if (tc2_pm_use_count[cpu][cluster] == 1) {
		/*
		 * A power_up request went ahead of us.
		 * Even if we do not want to shut this CPU down,
		 * the caller expects a certain state as if the WFI
		 * was aborted.  So let's continue with cache cleaning.
		 */
		skip_wfi = true;
	} else
		BUG();

	/*
	 * If the CPU is committed to power down, make sure
	 * the power controller will be in charge of waking it
	 * up upon IRQ, ie IRQ lines are cut from GIC CPU IF
	 * to the CPU by disabling the GIC CPU IF to prevent wfi
	 * from completing execution behind power controller back
	 */
	if (!skip_wfi)
		gic_cpu_if_down();

	if (last_man && __mcpm_outbound_enter_critical(cpu, cluster)) {
		arch_spin_unlock(&tc2_pm_lock);

		if (read_cpuid_part_number() == ARM_CPU_PART_CORTEX_A15) {
			/*
			 * On the Cortex-A15 we need to disable
			 * L2 prefetching before flushing the cache.
			 */
			asm volatile(
			"mcr	p15, 1, %0, c15, c0, 3 \n\t"
			"isb	\n\t"
			"dsb	"
			: : "r" (0x400) );
		}

		/*
		 * We need to disable and flush the whole (L1 and L2) cache.
		 * Let's do it in the safest possible way i.e. with
		 * no memory access within the following sequence
		 * including the stack.
		 *
		 * Note: fp is preserved to the stack explicitly prior doing
		 * this since adding it to the clobber list is incompatible
		 * with having CONFIG_FRAME_POINTER=y.
		 */
		asm volatile(
		"str	fp, [sp, #-4]! \n\t"
		"mrc	p15, 0, r0, c1, c0, 0	@ get CR \n\t"
		"bic	r0, r0, #"__stringify(CR_C)" \n\t"
		"mcr	p15, 0, r0, c1, c0, 0	@ set CR \n\t"
		"isb	\n\t"
		"bl	v7_flush_dcache_all \n\t"
		"clrex	\n\t"
		"mrc	p15, 0, r0, c1, c0, 1	@ get AUXCR \n\t"
		"bic	r0, r0, #(1 << 6)	@ disable local coherency \n\t"
		"mcr	p15, 0, r0, c1, c0, 1	@ set AUXCR \n\t"
		"isb	\n\t"
		"dsb	\n\t"
		"ldr	fp, [sp], #4"
		: : : "r0","r1","r2","r3","r4","r5","r6","r7",
		      "r9","r10","lr","memory");

		cci_disable_port_by_cpu(mpidr);

		__mcpm_outbound_leave_critical(cluster, CLUSTER_DOWN);
	} else {
		/*
		 * If last man then undo any setup done previously.
		 */
		if (last_man) {
			ve_spc_powerdown(cluster, false);
			ve_spc_global_wakeup_irq(false);
		}

		arch_spin_unlock(&tc2_pm_lock);

		/*
		 * We need to disable and flush only the L1 cache.
		 * Let's do it in the safest possible way as above.
		 */
		asm volatile(
		"str	fp, [sp, #-4]! \n\t"
		"mrc	p15, 0, r0, c1, c0, 0	@ get CR \n\t"
		"bic	r0, r0, #"__stringify(CR_C)" \n\t"
		"mcr	p15, 0, r0, c1, c0, 0	@ set CR \n\t"
		"isb	\n\t"
		"bl	v7_flush_dcache_louis \n\t"
		"clrex	\n\t"
		"mrc	p15, 0, r0, c1, c0, 1	@ get AUXCR \n\t"
		"bic	r0, r0, #(1 << 6)	@ disable local coherency \n\t"
		"mcr	p15, 0, r0, c1, c0, 1	@ set AUXCR \n\t"
		"isb	\n\t"
		"dsb	\n\t"
		"ldr	fp, [sp], #4"
		: : : "r0","r1","r2","r3","r4","r5","r6","r7",
		      "r9","r10","lr","memory");
	}

	__mcpm_cpu_down(cpu, cluster);

	/* Now we are prepared for power-down, do it: */
	if (!skip_wfi)
		wfi();

	/* Not dead at this point?  Let our caller cope. */
}

static void tc2_pm_power_down(void)
{
	tc2_pm_down(0);
}

static void tc2_pm_suspend(u64 residency)
{
	unsigned int mpidr, cpu, cluster;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);
	ve_spc_set_resume_addr(cluster, cpu, virt_to_phys(mcpm_entry_point));
	tc2_pm_down(residency);
}

static void tc2_pm_powered_up(void)
{
	unsigned int mpidr, cpu, cluster;
	unsigned long flags;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	BUG_ON(cluster >= TC2_CLUSTERS || cpu >= TC2_MAX_CPUS_PER_CLUSTER);

	local_irq_save(flags);
	arch_spin_lock(&tc2_pm_lock);

	if (tc2_cluster_unused(cluster)) {
		ve_spc_powerdown(cluster, false);
		ve_spc_global_wakeup_irq(false);
	}

	if (!tc2_pm_use_count[cpu][cluster])
		tc2_pm_use_count[cpu][cluster] = 1;

	ve_spc_cpu_wakeup_irq(cluster, cpu, false);
	ve_spc_set_resume_addr(cluster, cpu, 0);

	arch_spin_unlock(&tc2_pm_lock);
	local_irq_restore(flags);
}

static const struct mcpm_platform_ops tc2_pm_power_ops = {
	.power_up	= tc2_pm_power_up,
	.power_down	= tc2_pm_power_down,
	.suspend	= tc2_pm_suspend,
	.powered_up	= tc2_pm_powered_up,
};

static bool __init tc2_pm_usage_count_init(void)
{
	unsigned int mpidr, cpu, cluster;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	if (cluster >= TC2_CLUSTERS || cpu >= tc2_nr_cpus[cluster]) {
		pr_err("%s: boot CPU is out of bound!\n", __func__);
		return false;
	}
	tc2_pm_use_count[cpu][cluster] = 1;
	return true;
}

/*
 * Enable cluster-level coherency, in preparation for turning on the MMU.
 */
static void __naked tc2_pm_power_up_setup(unsigned int affinity_level)
{
	asm volatile (" \n"
"	cmp	r0, #1 \n"
"	bxne	lr \n"
"	b	cci_enable_port_for_self ");
}

static int __init tc2_pm_init(void)
{
	int ret;
	void __iomem *scc;
	u32 a15_cluster_id, a7_cluster_id, sys_info;
	struct device_node *np;

	/*
	 * The power management-related features are hidden behind
	 * SCC registers. We need to extract runtime information like
	 * cluster ids and number of CPUs really available in clusters.
	 */
	np = of_find_compatible_node(NULL, NULL,
			"arm,vexpress-scc,v2p-ca15_a7");
	scc = of_iomap(np, 0);
	if (!scc)
		return -ENODEV;

	a15_cluster_id = readl_relaxed(scc + A15_CONF) & 0xf;
	a7_cluster_id = readl_relaxed(scc + A7_CONF) & 0xf;
	if (a15_cluster_id >= TC2_CLUSTERS || a7_cluster_id >= TC2_CLUSTERS)
		return -EINVAL;

	sys_info = readl_relaxed(scc + SYS_INFO);
	tc2_nr_cpus[a15_cluster_id] = (sys_info >> 16) & 0xf;
	tc2_nr_cpus[a7_cluster_id] = (sys_info >> 20) & 0xf;

	/*
	 * A subset of the SCC registers is also used to communicate
	 * with the SPC (power controller). We need to be able to
	 * drive it very early in the boot process to power up
	 * processors, so we initialize the SPC driver here.
	 */
	ret = ve_spc_init(scc + SPC_BASE, a15_cluster_id);
	if (ret)
		return ret;

	if (!cci_probed())
		return -ENODEV;

	if (!tc2_pm_usage_count_init())
		return -EINVAL;

	ret = mcpm_platform_register(&tc2_pm_power_ops);
	if (!ret) {
		mcpm_sync_init(tc2_pm_power_up_setup);
		pr_info("TC2 power management initialized\n");
	}
	return ret;
}

early_initcall(tc2_pm_init);
