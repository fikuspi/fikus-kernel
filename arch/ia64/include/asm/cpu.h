#ifndef _ASM_IA64_CPU_H_
#define _ASM_IA64_CPU_H_

#include <fikus/device.h>
#include <fikus/cpu.h>
#include <fikus/topology.h>
#include <fikus/percpu.h>

struct ia64_cpu {
	struct cpu cpu;
};

DECLARE_PER_CPU(struct ia64_cpu, cpu_devices);

DECLARE_PER_CPU(int, cpu_state);

#ifdef CONFIG_HOTPLUG_CPU
extern int arch_register_cpu(int num);
extern void arch_unregister_cpu(int);
#endif

#endif /* _ASM_IA64_CPU_H_ */
