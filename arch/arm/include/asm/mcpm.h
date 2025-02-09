/*
 * arch/arm/include/asm/mcpm.h
 *
 * Created by:  Nicolas Pitre, April 2012
 * Copyright:   (C) 2012-2013  Linaro Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef MCPM_H
#define MCPM_H

/*
 * Maximum number of possible clusters / CPUs per cluster.
 *
 * This should be sufficient for quite a while, while keeping the
 * (assembly) code simpler.  When this starts to grow then we'll have
 * to consider dynamic allocation.
 */
#define MAX_CPUS_PER_CLUSTER	4
#define MAX_NR_CLUSTERS		2

#ifndef __ASSEMBLY__

#include <fikus/types.h>
#include <asm/cacheflush.h>

/*
 * Platform specific code should use this symbol to set up secondary
 * entry location for processors to use when released from reset.
 */
extern void mcpm_entry_point(void);

/*
 * This is used to indicate where the given CPU from given cluster should
 * branch once it is ready to re-enter the kernel using ptr, or NULL if it
 * should be gated.  A gated CPU is held in a WFE loop until its vector
 * becomes non NULL.
 */
void mcpm_set_entry_vector(unsigned cpu, unsigned cluster, void *ptr);

/*
 * CPU/cluster power operations API for higher subsystems to use.
 */

/**
 * mcpm_cpu_power_up - make given CPU in given cluster runable
 *
 * @cpu: CPU number within given cluster
 * @cluster: cluster number for the CPU
 *
 * The identified CPU is brought out of reset.  If the cluster was powered
 * down then it is brought up as well, taking care not to let the other CPUs
 * in the cluster run, and ensuring appropriate cluster setup.
 *
 * Caller must ensure the appropriate entry vector is initialized with
 * mcpm_set_entry_vector() prior to calling this.
 *
 * This must be called in a sleepable context.  However, the implementation
 * is strongly encouraged to return early and let the operation happen
 * asynchronously, especially when significant delays are expected.
 *
 * If the operation cannot be performed then an error code is returned.
 */
int mcpm_cpu_power_up(unsigned int cpu, unsigned int cluster);

/**
 * mcpm_cpu_power_down - power the calling CPU down
 *
 * The calling CPU is powered down.
 *
 * If this CPU is found to be the "last man standing" in the cluster
 * then the cluster is prepared for power-down too.
 *
 * This must be called with interrupts disabled.
 *
 * On success this does not return.  Re-entry in the kernel is expected
 * via mcpm_entry_point.
 *
 * This will return if mcpm_platform_register() has not been called
 * previously in which case the caller should take appropriate action.
 */
void mcpm_cpu_power_down(void);

/**
 * mcpm_cpu_suspend - bring the calling CPU in a suspended state
 *
 * @expected_residency: duration in microseconds the CPU is expected
 *			to remain suspended, or 0 if unknown/infinity.
 *
 * The calling CPU is suspended.  The expected residency argument is used
 * as a hint by the platform specific backend to implement the appropriate
 * sleep state level according to the knowledge it has on wake-up latency
 * for the given hardware.
 *
 * If this CPU is found to be the "last man standing" in the cluster
 * then the cluster may be prepared for power-down too, if the expected
 * residency makes it worthwhile.
 *
 * This must be called with interrupts disabled.
 *
 * On success this does not return.  Re-entry in the kernel is expected
 * via mcpm_entry_point.
 *
 * This will return if mcpm_platform_register() has not been called
 * previously in which case the caller should take appropriate action.
 */
void mcpm_cpu_suspend(u64 expected_residency);

/**
 * mcpm_cpu_powered_up - housekeeping workafter a CPU has been powered up
 *
 * This lets the platform specific backend code perform needed housekeeping
 * work.  This must be called by the newly activated CPU as soon as it is
 * fully operational in kernel space, before it enables interrupts.
 *
 * If the operation cannot be performed then an error code is returned.
 */
int mcpm_cpu_powered_up(void);

/*
 * Platform specific methods used in the implementation of the above API.
 */
struct mcpm_platform_ops {
	int (*power_up)(unsigned int cpu, unsigned int cluster);
	void (*power_down)(void);
	void (*suspend)(u64);
	void (*powered_up)(void);
};

/**
 * mcpm_platform_register - register platform specific power methods
 *
 * @ops: mcpm_platform_ops structure to register
 *
 * An error is returned if the registration has been done previously.
 */
int __init mcpm_platform_register(const struct mcpm_platform_ops *ops);

/* Synchronisation structures for coordinating safe cluster setup/teardown: */

/*
 * When modifying this structure, make sure you update the MCPM_SYNC_ defines
 * to match.
 */
struct mcpm_sync_struct {
	/* individual CPU states */
	struct {
		s8 cpu __aligned(__CACHE_WRITEBACK_GRANULE);
	} cpus[MAX_CPUS_PER_CLUSTER];

	/* cluster state */
	s8 cluster __aligned(__CACHE_WRITEBACK_GRANULE);

	/* inbound-side state */
	s8 inbound __aligned(__CACHE_WRITEBACK_GRANULE);
};

struct sync_struct {
	struct mcpm_sync_struct clusters[MAX_NR_CLUSTERS];
};

extern unsigned long sync_phys;	/* physical address of *mcpm_sync */

void __mcpm_cpu_going_down(unsigned int cpu, unsigned int cluster);
void __mcpm_cpu_down(unsigned int cpu, unsigned int cluster);
void __mcpm_outbound_leave_critical(unsigned int cluster, int state);
bool __mcpm_outbound_enter_critical(unsigned int this_cpu, unsigned int cluster);
int __mcpm_cluster_state(unsigned int cluster);

int __init mcpm_sync_init(
	void (*power_up_setup)(unsigned int affinity_level));

void __init mcpm_smp_set_ops(void);

#else

/* 
 * asm-offsets.h causes trouble when included in .c files, and cacheflush.h
 * cannot be included in asm files.  Let's work around the conflict like this.
 */
#include <asm/asm-offsets.h>
#define __CACHE_WRITEBACK_GRANULE CACHE_WRITEBACK_GRANULE

#endif /* ! __ASSEMBLY__ */

/* Definitions for mcpm_sync_struct */
#define CPU_DOWN		0x11
#define CPU_COMING_UP		0x12
#define CPU_UP			0x13
#define CPU_GOING_DOWN		0x14

#define CLUSTER_DOWN		0x21
#define CLUSTER_UP		0x22
#define CLUSTER_GOING_DOWN	0x23

#define INBOUND_NOT_COMING_UP	0x31
#define INBOUND_COMING_UP	0x32

/*
 * Offsets for the mcpm_sync_struct members, for use in asm.
 * We don't want to make them global to the kernel via asm-offsets.c.
 */
#define MCPM_SYNC_CLUSTER_CPUS	0
#define MCPM_SYNC_CPU_SIZE	__CACHE_WRITEBACK_GRANULE
#define MCPM_SYNC_CLUSTER_CLUSTER \
	(MCPM_SYNC_CLUSTER_CPUS + MCPM_SYNC_CPU_SIZE * MAX_CPUS_PER_CLUSTER)
#define MCPM_SYNC_CLUSTER_INBOUND \
	(MCPM_SYNC_CLUSTER_CLUSTER + __CACHE_WRITEBACK_GRANULE)
#define MCPM_SYNC_CLUSTER_SIZE \
	(MCPM_SYNC_CLUSTER_INBOUND + __CACHE_WRITEBACK_GRANULE)

#endif
