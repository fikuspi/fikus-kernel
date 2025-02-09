/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2011 by Kevin Cernekee (cernekee@gmail.com)
 *
 * SMP support for BMIPS
 */

#include <fikus/init.h>
#include <fikus/sched.h>
#include <fikus/mm.h>
#include <fikus/delay.h>
#include <fikus/smp.h>
#include <fikus/interrupt.h>
#include <fikus/spinlock.h>
#include <fikus/cpu.h>
#include <fikus/cpumask.h>
#include <fikus/reboot.h>
#include <fikus/io.h>
#include <fikus/compiler.h>
#include <fikus/linkage.h>
#include <fikus/bug.h>
#include <fikus/kernel.h>

#include <asm/time.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/bootinfo.h>
#include <asm/pmon.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/mipsregs.h>
#include <asm/bmips.h>
#include <asm/traps.h>
#include <asm/barrier.h>

static int __maybe_unused max_cpus = 1;

/* these may be configured by the platform code */
int bmips_smp_enabled = 1;
int bmips_cpu_offset;
cpumask_t bmips_booted_mask;

#ifdef CONFIG_SMP

/* initial $sp, $gp - used by arch/mips/kernel/bmips_vec.S */
unsigned long bmips_smp_boot_sp;
unsigned long bmips_smp_boot_gp;

static void bmips_send_ipi_single(int cpu, unsigned int action);
static irqreturn_t bmips_ipi_interrupt(int irq, void *dev_id);

/* SW interrupts 0,1 are used for interprocessor signaling */
#define IPI0_IRQ			(MIPS_CPU_IRQ_BASE + 0)
#define IPI1_IRQ			(MIPS_CPU_IRQ_BASE + 1)

#define CPUNUM(cpu, shift)		(((cpu) + bmips_cpu_offset) << (shift))
#define ACTION_CLR_IPI(cpu, ipi)	(0x2000 | CPUNUM(cpu, 9) | ((ipi) << 8))
#define ACTION_SET_IPI(cpu, ipi)	(0x3000 | CPUNUM(cpu, 9) | ((ipi) << 8))
#define ACTION_BOOT_THREAD(cpu)		(0x08 | CPUNUM(cpu, 0))

static void __init bmips_smp_setup(void)
{
	int i, cpu = 1, boot_cpu = 0;

#if defined(CONFIG_CPU_BMIPS4350) || defined(CONFIG_CPU_BMIPS4380)
	int cpu_hw_intr;

	/* arbitration priority */
	clear_c0_brcm_cmt_ctrl(0x30);

	/* NBK and weak order flags */
	set_c0_brcm_config_0(0x30000);

	/* Find out if we are running on TP0 or TP1 */
	boot_cpu = !!(read_c0_brcm_cmt_local() & (1 << 31));

	/*
	 * MIPS interrupts 0,1 (SW INT 0,1) cross over to the other thread
	 * MIPS interrupt 2 (HW INT 0) is the CPU0 L1 controller output
	 * MIPS interrupt 3 (HW INT 1) is the CPU1 L1 controller output
	 */
	if (boot_cpu == 0)
		cpu_hw_intr = 0x02;
	else
		cpu_hw_intr = 0x1d;

	change_c0_brcm_cmt_intr(0xf8018000, (cpu_hw_intr << 27) | (0x03 << 15));

	/* single core, 2 threads (2 pipelines) */
	max_cpus = 2;
#elif defined(CONFIG_CPU_BMIPS5000)
	/* enable raceless SW interrupts */
	set_c0_brcm_config(0x03 << 22);

	/* route HW interrupt 0 to CPU0, HW interrupt 1 to CPU1 */
	change_c0_brcm_mode(0x1f << 27, 0x02 << 27);

	/* N cores, 2 threads per core */
	max_cpus = (((read_c0_brcm_config() >> 6) & 0x03) + 1) << 1;

	/* clear any pending SW interrupts */
	for (i = 0; i < max_cpus; i++) {
		write_c0_brcm_action(ACTION_CLR_IPI(i, 0));
		write_c0_brcm_action(ACTION_CLR_IPI(i, 1));
	}
#endif

	if (!bmips_smp_enabled)
		max_cpus = 1;

	/* this can be overridden by the BSP */
	if (!board_ebase_setup)
		board_ebase_setup = &bmips_ebase_setup;

	__cpu_number_map[boot_cpu] = 0;
	__cpu_logical_map[0] = boot_cpu;

	for (i = 0; i < max_cpus; i++) {
		if (i != boot_cpu) {
			__cpu_number_map[i] = cpu;
			__cpu_logical_map[cpu] = i;
			cpu++;
		}
		set_cpu_possible(i, 1);
		set_cpu_present(i, 1);
	}
}

/*
 * IPI IRQ setup - runs on CPU0
 */
static void bmips_prepare_cpus(unsigned int max_cpus)
{
	if (request_irq(IPI0_IRQ, bmips_ipi_interrupt, IRQF_PERCPU,
			"smp_ipi0", NULL))
		panic("Can't request IPI0 interrupt\n");
	if (request_irq(IPI1_IRQ, bmips_ipi_interrupt, IRQF_PERCPU,
			"smp_ipi1", NULL))
		panic("Can't request IPI1 interrupt\n");
}

/*
 * Tell the hardware to boot CPUx - runs on CPU0
 */
static void bmips_boot_secondary(int cpu, struct task_struct *idle)
{
	bmips_smp_boot_sp = __KSTK_TOS(idle);
	bmips_smp_boot_gp = (unsigned long)task_thread_info(idle);
	mb();

	/*
	 * Initial boot sequence for secondary CPU:
	 *   bmips_reset_nmi_vec @ a000_0000 ->
	 *   bmips_smp_entry ->
	 *   plat_wired_tlb_setup (cached function call; optional) ->
	 *   start_secondary (cached jump)
	 *
	 * Warm restart sequence:
	 *   play_dead WAIT loop ->
	 *   bmips_smp_int_vec @ BMIPS_WARM_RESTART_VEC ->
	 *   eret to play_dead ->
	 *   bmips_secondary_reentry ->
	 *   start_secondary
	 */

	pr_info("SMP: Booting CPU%d...\n", cpu);

	if (cpumask_test_cpu(cpu, &bmips_booted_mask))
		bmips_send_ipi_single(cpu, 0);
	else {
#if defined(CONFIG_CPU_BMIPS4350) || defined(CONFIG_CPU_BMIPS4380)
		/* Reset slave TP1 if booting from TP0 */
		if (cpu_logical_map(cpu) == 1)
			set_c0_brcm_cmt_ctrl(0x01);
#elif defined(CONFIG_CPU_BMIPS5000)
		if (cpu & 0x01)
			write_c0_brcm_action(ACTION_BOOT_THREAD(cpu));
		else {
			/*
			 * core N thread 0 was already booted; just
			 * pulse the NMI line
			 */
			bmips_write_zscm_reg(0x210, 0xc0000000);
			udelay(10);
			bmips_write_zscm_reg(0x210, 0x00);
		}
#endif
		cpumask_set_cpu(cpu, &bmips_booted_mask);
	}
}

/*
 * Early setup - runs on secondary CPU after cache probe
 */
static void bmips_init_secondary(void)
{
	/* move NMI vector to kseg0, in case XKS01 is enabled */

#if defined(CONFIG_CPU_BMIPS4350) || defined(CONFIG_CPU_BMIPS4380)
	void __iomem *cbr = BMIPS_GET_CBR();
	unsigned long old_vec;
	unsigned long relo_vector;
	int boot_cpu;

	boot_cpu = !!(read_c0_brcm_cmt_local() & (1 << 31));
	relo_vector = boot_cpu ? BMIPS_RELO_VECTOR_CONTROL_0 :
			  BMIPS_RELO_VECTOR_CONTROL_1;

	old_vec = __raw_readl(cbr + relo_vector);
	__raw_writel(old_vec & ~0x20000000, cbr + relo_vector);

	clear_c0_cause(smp_processor_id() ? C_SW1 : C_SW0);
#elif defined(CONFIG_CPU_BMIPS5000)
	write_c0_brcm_bootvec(read_c0_brcm_bootvec() &
		(smp_processor_id() & 0x01 ? ~0x20000000 : ~0x2000));

	write_c0_brcm_action(ACTION_CLR_IPI(smp_processor_id(), 0));
#endif
}

/*
 * Late setup - runs on secondary CPU before entering the idle loop
 */
static void bmips_smp_finish(void)
{
	pr_info("SMP: CPU%d is running\n", smp_processor_id());

	/* make sure there won't be a timer interrupt for a little while */
	write_c0_compare(read_c0_count() + mips_hpt_frequency / HZ);

	irq_enable_hazard();
	set_c0_status(IE_SW0 | IE_SW1 | IE_IRQ1 | IE_IRQ5 | ST0_IE);
	irq_enable_hazard();
}

/*
 * Runs on CPU0 after all CPUs have been booted
 */
static void bmips_cpus_done(void)
{
}

#if defined(CONFIG_CPU_BMIPS5000)

/*
 * BMIPS5000 raceless IPIs
 *
 * Each CPU has two inbound SW IRQs which are independent of all other CPUs.
 * IPI0 is used for SMP_RESCHEDULE_YOURSELF
 * IPI1 is used for SMP_CALL_FUNCTION
 */

static void bmips_send_ipi_single(int cpu, unsigned int action)
{
	write_c0_brcm_action(ACTION_SET_IPI(cpu, action == SMP_CALL_FUNCTION));
}

static irqreturn_t bmips_ipi_interrupt(int irq, void *dev_id)
{
	int action = irq - IPI0_IRQ;

	write_c0_brcm_action(ACTION_CLR_IPI(smp_processor_id(), action));

	if (action == 0)
		scheduler_ipi();
	else
		smp_call_function_interrupt();

	return IRQ_HANDLED;
}

#else

/*
 * BMIPS43xx racey IPIs
 *
 * We use one inbound SW IRQ for each CPU.
 *
 * A spinlock must be held in order to keep CPUx from accidentally clearing
 * an incoming IPI when it writes CP0 CAUSE to raise an IPI on CPUy.  The
 * same spinlock is used to protect the action masks.
 */

static DEFINE_SPINLOCK(ipi_lock);
static DEFINE_PER_CPU(int, ipi_action_mask);

static void bmips_send_ipi_single(int cpu, unsigned int action)
{
	unsigned long flags;

	spin_lock_irqsave(&ipi_lock, flags);
	set_c0_cause(cpu ? C_SW1 : C_SW0);
	per_cpu(ipi_action_mask, cpu) |= action;
	irq_enable_hazard();
	spin_unlock_irqrestore(&ipi_lock, flags);
}

static irqreturn_t bmips_ipi_interrupt(int irq, void *dev_id)
{
	unsigned long flags;
	int action, cpu = irq - IPI0_IRQ;

	spin_lock_irqsave(&ipi_lock, flags);
	action = __get_cpu_var(ipi_action_mask);
	per_cpu(ipi_action_mask, cpu) = 0;
	clear_c0_cause(cpu ? C_SW1 : C_SW0);
	spin_unlock_irqrestore(&ipi_lock, flags);

	if (action & SMP_RESCHEDULE_YOURSELF)
		scheduler_ipi();
	if (action & SMP_CALL_FUNCTION)
		smp_call_function_interrupt();

	return IRQ_HANDLED;
}

#endif /* BMIPS type */

static void bmips_send_ipi_mask(const struct cpumask *mask,
	unsigned int action)
{
	unsigned int i;

	for_each_cpu(i, mask)
		bmips_send_ipi_single(i, action);
}

#ifdef CONFIG_HOTPLUG_CPU

static int bmips_cpu_disable(void)
{
	unsigned int cpu = smp_processor_id();

	if (cpu == 0)
		return -EBUSY;

	pr_info("SMP: CPU%d is offline\n", cpu);

	set_cpu_online(cpu, false);
	cpu_clear(cpu, cpu_callin_map);

	local_flush_tlb_all();
	local_flush_icache_range(0, ~0);

	return 0;
}

static void bmips_cpu_die(unsigned int cpu)
{
}

void __ref play_dead(void)
{
	idle_task_exit();

	/* flush data cache */
	_dma_cache_wback_inv(0, ~0);

	/*
	 * Wakeup is on SW0 or SW1; disable everything else
	 * Use BEV !IV (BMIPS_WARM_RESTART_VEC) to avoid the regular Fikus
	 * IRQ handlers; this clears ST0_IE and returns immediately.
	 */
	clear_c0_cause(CAUSEF_IV | C_SW0 | C_SW1);
	change_c0_status(IE_IRQ5 | IE_IRQ1 | IE_SW0 | IE_SW1 | ST0_IE | ST0_BEV,
		IE_SW0 | IE_SW1 | ST0_IE | ST0_BEV);
	irq_disable_hazard();

	/*
	 * wait for SW interrupt from bmips_boot_secondary(), then jump
	 * back to start_secondary()
	 */
	__asm__ __volatile__(
	"	wait\n"
	"	j	bmips_secondary_reentry\n"
	: : : "memory");
}

#endif /* CONFIG_HOTPLUG_CPU */

struct plat_smp_ops bmips_smp_ops = {
	.smp_setup		= bmips_smp_setup,
	.prepare_cpus		= bmips_prepare_cpus,
	.boot_secondary		= bmips_boot_secondary,
	.smp_finish		= bmips_smp_finish,
	.init_secondary		= bmips_init_secondary,
	.cpus_done		= bmips_cpus_done,
	.send_ipi_single	= bmips_send_ipi_single,
	.send_ipi_mask		= bmips_send_ipi_mask,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_disable		= bmips_cpu_disable,
	.cpu_die		= bmips_cpu_die,
#endif
};

#endif /* CONFIG_SMP */

/***********************************************************************
 * BMIPS vector relocation
 * This is primarily used for SMP boot, but it is applicable to some
 * UP BMIPS systems as well.
 ***********************************************************************/

static void bmips_wr_vec(unsigned long dst, char *start, char *end)
{
	memcpy((void *)dst, start, end - start);
	dma_cache_wback((unsigned long)start, end - start);
	local_flush_icache_range(dst, dst + (end - start));
	instruction_hazard();
}

static inline void bmips_nmi_handler_setup(void)
{
	bmips_wr_vec(BMIPS_NMI_RESET_VEC, &bmips_reset_nmi_vec,
		&bmips_reset_nmi_vec_end);
	bmips_wr_vec(BMIPS_WARM_RESTART_VEC, &bmips_smp_int_vec,
		&bmips_smp_int_vec_end);
}

void bmips_ebase_setup(void)
{
	unsigned long new_ebase = ebase;
	void __iomem __maybe_unused *cbr;

	BUG_ON(ebase != CKSEG0);

#if defined(CONFIG_CPU_BMIPS4350)
	/*
	 * BMIPS4350 cannot relocate the normal vectors, but it
	 * can relocate the BEV=1 vectors.  So CPU1 starts up at
	 * the relocated BEV=1, IV=0 general exception vector @
	 * 0xa000_0380.
	 *
	 * set_uncached_handler() is used here because:
	 *  - CPU1 will run this from uncached space
	 *  - None of the cacheflush functions are set up yet
	 */
	set_uncached_handler(BMIPS_WARM_RESTART_VEC - CKSEG0,
		&bmips_smp_int_vec, 0x80);
	__sync();
	return;
#elif defined(CONFIG_CPU_BMIPS4380)
	/*
	 * 0x8000_0000: reset/NMI (initially in kseg1)
	 * 0x8000_0400: normal vectors
	 */
	new_ebase = 0x80000400;
	cbr = BMIPS_GET_CBR();
	__raw_writel(0x80080800, cbr + BMIPS_RELO_VECTOR_CONTROL_0);
	__raw_writel(0xa0080800, cbr + BMIPS_RELO_VECTOR_CONTROL_1);
#elif defined(CONFIG_CPU_BMIPS5000)
	/*
	 * 0x8000_0000: reset/NMI (initially in kseg1)
	 * 0x8000_1000: normal vectors
	 */
	new_ebase = 0x80001000;
	write_c0_brcm_bootvec(0xa0088008);
	write_c0_ebase(new_ebase);
	if (max_cpus > 2)
		bmips_write_zscm_reg(0xa0, 0xa008a008);
#else
	return;
#endif
	board_nmi_handler_setup = &bmips_nmi_handler_setup;
	ebase = new_ebase;
}

asmlinkage void __weak plat_wired_tlb_setup(void)
{
	/*
	 * Called when starting/restarting a secondary CPU.
	 * Kernel stacks and other important data might only be accessible
	 * once the wired entries are present.
	 */
}
