/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Copyright (C) 2000, 2001 Kanoj Sarcar
 * Copyright (C) 2000, 2001 Ralf Baechle
 * Copyright (C) 2000, 2001 Silicon Graphics, Inc.
 * Copyright (C) 2000, 2001, 2003 Broadcom Corporation
 */
#include <fikus/cache.h>
#include <fikus/delay.h>
#include <fikus/init.h>
#include <fikus/interrupt.h>
#include <fikus/smp.h>
#include <fikus/spinlock.h>
#include <fikus/threads.h>
#include <fikus/module.h>
#include <fikus/time.h>
#include <fikus/timex.h>
#include <fikus/sched.h>
#include <fikus/cpumask.h>
#include <fikus/cpu.h>
#include <fikus/err.h>
#include <fikus/ftrace.h>

#include <fikus/atomic.h>
#include <asm/cpu.h>
#include <asm/processor.h>
#include <asm/idle.h>
#include <asm/r4k-timer.h>
#include <asm/mmu_context.h>
#include <asm/time.h>
#include <asm/setup.h>

#ifdef CONFIG_MIPS_MT_SMTC
#include <asm/mipsmtregs.h>
#endif /* CONFIG_MIPS_MT_SMTC */

volatile cpumask_t cpu_callin_map;	/* Bitmask of started secondaries */

int __cpu_number_map[NR_CPUS];		/* Map physical to logical */
EXPORT_SYMBOL(__cpu_number_map);

int __cpu_logical_map[NR_CPUS];		/* Map logical to physical */
EXPORT_SYMBOL(__cpu_logical_map);

/* Number of TCs (or siblings in Intel speak) per CPU core */
int smp_num_siblings = 1;
EXPORT_SYMBOL(smp_num_siblings);

/* representing the TCs (or siblings in Intel speak) of each logical CPU */
cpumask_t cpu_sibling_map[NR_CPUS] __read_mostly;
EXPORT_SYMBOL(cpu_sibling_map);

/* representing cpus for which sibling maps can be computed */
static cpumask_t cpu_sibling_setup_map;

static inline void set_cpu_sibling_map(int cpu)
{
	int i;

	cpu_set(cpu, cpu_sibling_setup_map);

	if (smp_num_siblings > 1) {
		for_each_cpu_mask(i, cpu_sibling_setup_map) {
			if (cpu_data[cpu].core == cpu_data[i].core) {
				cpu_set(i, cpu_sibling_map[cpu]);
				cpu_set(cpu, cpu_sibling_map[i]);
			}
		}
	} else
		cpu_set(cpu, cpu_sibling_map[cpu]);
}

struct plat_smp_ops *mp_ops;
EXPORT_SYMBOL(mp_ops);

void register_smp_ops(struct plat_smp_ops *ops)
{
	if (mp_ops)
		printk(KERN_WARNING "Overriding previously set SMP ops\n");

	mp_ops = ops;
}

/*
 * First C code run on the secondary CPUs after being started up by
 * the master.
 */
asmlinkage void start_secondary(void)
{
	unsigned int cpu;

#ifdef CONFIG_MIPS_MT_SMTC
	/* Only do cpu_probe for first TC of CPU */
	if ((read_c0_tcbind() & TCBIND_CURTC) != 0)
		__cpu_name[smp_processor_id()] = __cpu_name[0];
	else
#endif /* CONFIG_MIPS_MT_SMTC */
	cpu_probe();
	cpu_report();
	per_cpu_trap_init(false);
	mips_clockevent_init();
	mp_ops->init_secondary();

	/*
	 * XXX parity protection should be folded in here when it's converted
	 * to an option instead of something based on .cputype
	 */

	calibrate_delay();
	preempt_disable();
	cpu = smp_processor_id();
	cpu_data[cpu].udelay_val = loops_per_jiffy;

	notify_cpu_starting(cpu);

	set_cpu_online(cpu, true);

	set_cpu_sibling_map(cpu);

	cpu_set(cpu, cpu_callin_map);

	synchronise_count_slave(cpu);

	/*
	 * irq will be enabled in ->smp_finish(), enabling it too early
	 * is dangerous.
	 */
	WARN_ON_ONCE(!irqs_disabled());
	mp_ops->smp_finish();

	cpu_startup_entry(CPUHP_ONLINE);
}

/*
 * Call into both interrupt handlers, as we share the IPI for them
 */
void __irq_entry smp_call_function_interrupt(void)
{
	irq_enter();
	generic_smp_call_function_single_interrupt();
	generic_smp_call_function_interrupt();
	irq_exit();
}

static void stop_this_cpu(void *dummy)
{
	/*
	 * Remove this CPU:
	 */
	set_cpu_online(smp_processor_id(), false);
	for (;;) {
		if (cpu_wait)
			(*cpu_wait)();		/* Wait if available. */
	}
}

void smp_send_stop(void)
{
	smp_call_function(stop_this_cpu, NULL, 0);
}

void __init smp_cpus_done(unsigned int max_cpus)
{
	mp_ops->cpus_done();
}

/* called from main before smp_init() */
void __init smp_prepare_cpus(unsigned int max_cpus)
{
	init_new_context(current, &init_mm);
	current_thread_info()->cpu = 0;
	mp_ops->prepare_cpus(max_cpus);
	set_cpu_sibling_map(0);
#ifndef CONFIG_HOTPLUG_CPU
	init_cpu_present(cpu_possible_mask);
#endif
}

/* preload SMP state for boot cpu */
void smp_prepare_boot_cpu(void)
{
	set_cpu_possible(0, true);
	set_cpu_online(0, true);
	cpu_set(0, cpu_callin_map);
}

int __cpu_up(unsigned int cpu, struct task_struct *tidle)
{
	mp_ops->boot_secondary(cpu, tidle);

	/*
	 * Trust is futile.  We should really have timeouts ...
	 */
	while (!cpu_isset(cpu, cpu_callin_map))
		udelay(100);

	synchronise_count_master(cpu);
	return 0;
}

/* Not really SMP stuff ... */
int setup_profiling_timer(unsigned int multiplier)
{
	return 0;
}

static void flush_tlb_all_ipi(void *info)
{
	local_flush_tlb_all();
}

void flush_tlb_all(void)
{
	on_each_cpu(flush_tlb_all_ipi, NULL, 1);
}

static void flush_tlb_mm_ipi(void *mm)
{
	local_flush_tlb_mm((struct mm_struct *)mm);
}

/*
 * Special Variant of smp_call_function for use by TLB functions:
 *
 *  o No return value
 *  o collapses to normal function call on UP kernels
 *  o collapses to normal function call on systems with a single shared
 *    primary cache.
 *  o CONFIG_MIPS_MT_SMTC currently implies there is only one physical core.
 */
static inline void smp_on_other_tlbs(void (*func) (void *info), void *info)
{
#ifndef CONFIG_MIPS_MT_SMTC
	smp_call_function(func, info, 1);
#endif
}

static inline void smp_on_each_tlb(void (*func) (void *info), void *info)
{
	preempt_disable();

	smp_on_other_tlbs(func, info);
	func(info);

	preempt_enable();
}

/*
 * The following tlb flush calls are invoked when old translations are
 * being torn down, or pte attributes are changing. For single threaded
 * address spaces, a new context is obtained on the current cpu, and tlb
 * context on other cpus are invalidated to force a new context allocation
 * at switch_mm time, should the mm ever be used on other cpus. For
 * multithreaded address spaces, intercpu interrupts have to be sent.
 * Another case where intercpu interrupts are required is when the target
 * mm might be active on another cpu (eg debuggers doing the flushes on
 * behalf of debugees, kswapd stealing pages from another process etc).
 * Kanoj 07/00.
 */

void flush_tlb_mm(struct mm_struct *mm)
{
	preempt_disable();

	if ((atomic_read(&mm->mm_users) != 1) || (current->mm != mm)) {
		smp_on_other_tlbs(flush_tlb_mm_ipi, mm);
	} else {
		unsigned int cpu;

		for_each_online_cpu(cpu) {
			if (cpu != smp_processor_id() && cpu_context(cpu, mm))
				cpu_context(cpu, mm) = 0;
		}
	}
	local_flush_tlb_mm(mm);

	preempt_enable();
}

struct flush_tlb_data {
	struct vm_area_struct *vma;
	unsigned long addr1;
	unsigned long addr2;
};

static void flush_tlb_range_ipi(void *info)
{
	struct flush_tlb_data *fd = info;

	local_flush_tlb_range(fd->vma, fd->addr1, fd->addr2);
}

void flush_tlb_range(struct vm_area_struct *vma, unsigned long start, unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;

	preempt_disable();
	if ((atomic_read(&mm->mm_users) != 1) || (current->mm != mm)) {
		struct flush_tlb_data fd = {
			.vma = vma,
			.addr1 = start,
			.addr2 = end,
		};

		smp_on_other_tlbs(flush_tlb_range_ipi, &fd);
	} else {
		unsigned int cpu;

		for_each_online_cpu(cpu) {
			if (cpu != smp_processor_id() && cpu_context(cpu, mm))
				cpu_context(cpu, mm) = 0;
		}
	}
	local_flush_tlb_range(vma, start, end);
	preempt_enable();
}

static void flush_tlb_kernel_range_ipi(void *info)
{
	struct flush_tlb_data *fd = info;

	local_flush_tlb_kernel_range(fd->addr1, fd->addr2);
}

void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	struct flush_tlb_data fd = {
		.addr1 = start,
		.addr2 = end,
	};

	on_each_cpu(flush_tlb_kernel_range_ipi, &fd, 1);
}

static void flush_tlb_page_ipi(void *info)
{
	struct flush_tlb_data *fd = info;

	local_flush_tlb_page(fd->vma, fd->addr1);
}

void flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	preempt_disable();
	if ((atomic_read(&vma->vm_mm->mm_users) != 1) || (current->mm != vma->vm_mm)) {
		struct flush_tlb_data fd = {
			.vma = vma,
			.addr1 = page,
		};

		smp_on_other_tlbs(flush_tlb_page_ipi, &fd);
	} else {
		unsigned int cpu;

		for_each_online_cpu(cpu) {
			if (cpu != smp_processor_id() && cpu_context(cpu, vma->vm_mm))
				cpu_context(cpu, vma->vm_mm) = 0;
		}
	}
	local_flush_tlb_page(vma, page);
	preempt_enable();
}

static void flush_tlb_one_ipi(void *info)
{
	unsigned long vaddr = (unsigned long) info;

	local_flush_tlb_one(vaddr);
}

void flush_tlb_one(unsigned long vaddr)
{
	smp_on_each_tlb(flush_tlb_one_ipi, (void *) vaddr);
}

EXPORT_SYMBOL(flush_tlb_page);
EXPORT_SYMBOL(flush_tlb_one);

#if defined(CONFIG_KEXEC)
void (*dump_ipi_function_ptr)(void *) = NULL;
void dump_send_ipi(void (*dump_ipi_callback)(void *))
{
	int i;
	int cpu = smp_processor_id();

	dump_ipi_function_ptr = dump_ipi_callback;
	smp_mb();
	for_each_online_cpu(i)
		if (i != cpu)
			mp_ops->send_ipi_single(i, SMP_DUMP);

}
EXPORT_SYMBOL(dump_send_ipi);
#endif
