/*
 *    Copyright IBM Corp. 2004, 2011
 *    Author(s): Martin Schwidefsky <schwidefsky@de.ibm.com>,
 *		 Holger Smolinski <Holger.Smolinski@de.ibm.com>,
 *		 Thomas Spatzier <tspat@de.ibm.com>,
 *
 * This file contains interrupt related functions.
 */

#include <fikus/kernel_stat.h>
#include <fikus/interrupt.h>
#include <fikus/seq_file.h>
#include <fikus/proc_fs.h>
#include <fikus/profile.h>
#include <fikus/module.h>
#include <fikus/kernel.h>
#include <fikus/ftrace.h>
#include <fikus/errno.h>
#include <fikus/slab.h>
#include <fikus/cpu.h>
#include <asm/irq_regs.h>
#include <asm/cputime.h>
#include <asm/lowcore.h>
#include <asm/irq.h>
#include <asm/hw_irq.h>
#include "entry.h"

DEFINE_PER_CPU_SHARED_ALIGNED(struct irq_stat, irq_stat);
EXPORT_PER_CPU_SYMBOL_GPL(irq_stat);

struct irq_class {
	char *name;
	char *desc;
};

/*
 * The list of "main" irq classes on s390. This is the list of interrupts
 * that appear both in /proc/stat ("intr" line) and /proc/interrupts.
 * Historically only external and I/O interrupts have been part of /proc/stat.
 * We can't add the split external and I/O sub classes since the first field
 * in the "intr" line in /proc/stat is supposed to be the sum of all other
 * fields.
 * Since the external and I/O interrupt fields are already sums we would end
 * up with having a sum which accounts each interrupt twice.
 */
static const struct irq_class irqclass_main_desc[NR_IRQS_BASE] = {
	[EXT_INTERRUPT]  = {.name = "EXT"},
	[IO_INTERRUPT]	 = {.name = "I/O"},
	[THIN_INTERRUPT] = {.name = "AIO"},
};

/*
 * The list of split external and I/O interrupts that appear only in
 * /proc/interrupts.
 * In addition this list contains non external / I/O events like NMIs.
 */
static const struct irq_class irqclass_sub_desc[NR_ARCH_IRQS] = {
	[IRQEXT_CLK] = {.name = "CLK", .desc = "[EXT] Clock Comparator"},
	[IRQEXT_EXC] = {.name = "EXC", .desc = "[EXT] External Call"},
	[IRQEXT_EMS] = {.name = "EMS", .desc = "[EXT] Emergency Signal"},
	[IRQEXT_TMR] = {.name = "TMR", .desc = "[EXT] CPU Timer"},
	[IRQEXT_TLA] = {.name = "TAL", .desc = "[EXT] Timing Alert"},
	[IRQEXT_PFL] = {.name = "PFL", .desc = "[EXT] Pseudo Page Fault"},
	[IRQEXT_DSD] = {.name = "DSD", .desc = "[EXT] DASD Diag"},
	[IRQEXT_VRT] = {.name = "VRT", .desc = "[EXT] Virtio"},
	[IRQEXT_SCP] = {.name = "SCP", .desc = "[EXT] Service Call"},
	[IRQEXT_IUC] = {.name = "IUC", .desc = "[EXT] IUCV"},
	[IRQEXT_CMS] = {.name = "CMS", .desc = "[EXT] CPU-Measurement: Sampling"},
	[IRQEXT_CMC] = {.name = "CMC", .desc = "[EXT] CPU-Measurement: Counter"},
	[IRQEXT_CMR] = {.name = "CMR", .desc = "[EXT] CPU-Measurement: RI"},
	[IRQIO_CIO]  = {.name = "CIO", .desc = "[I/O] Common I/O Layer Interrupt"},
	[IRQIO_QAI]  = {.name = "QAI", .desc = "[I/O] QDIO Adapter Interrupt"},
	[IRQIO_DAS]  = {.name = "DAS", .desc = "[I/O] DASD"},
	[IRQIO_C15]  = {.name = "C15", .desc = "[I/O] 3215"},
	[IRQIO_C70]  = {.name = "C70", .desc = "[I/O] 3270"},
	[IRQIO_TAP]  = {.name = "TAP", .desc = "[I/O] Tape"},
	[IRQIO_VMR]  = {.name = "VMR", .desc = "[I/O] Unit Record Devices"},
	[IRQIO_LCS]  = {.name = "LCS", .desc = "[I/O] LCS"},
	[IRQIO_CLW]  = {.name = "CLW", .desc = "[I/O] CLAW"},
	[IRQIO_CTC]  = {.name = "CTC", .desc = "[I/O] CTC"},
	[IRQIO_APB]  = {.name = "APB", .desc = "[I/O] AP Bus"},
	[IRQIO_ADM]  = {.name = "ADM", .desc = "[I/O] EADM Subchannel"},
	[IRQIO_CSC]  = {.name = "CSC", .desc = "[I/O] CHSC Subchannel"},
	[IRQIO_PCI]  = {.name = "PCI", .desc = "[I/O] PCI Interrupt" },
	[IRQIO_MSI]  = {.name = "MSI", .desc = "[I/O] MSI Interrupt" },
	[IRQIO_VIR]  = {.name = "VIR", .desc = "[I/O] Virtual I/O Devices"},
	[NMI_NMI]    = {.name = "NMI", .desc = "[NMI] Machine Check"},
	[CPU_RST]    = {.name = "RST", .desc = "[CPU] CPU Restart"},
};

void __init init_IRQ(void)
{
	irq_reserve_irqs(0, THIN_INTERRUPT);
	init_cio_interrupts();
	init_airq_interrupts();
	init_ext_interrupts();
}

void do_IRQ(struct pt_regs *regs, int irq)
{
	struct pt_regs *old_regs;

	old_regs = set_irq_regs(regs);
	irq_enter();
	if (S390_lowcore.int_clock >= S390_lowcore.clock_comparator)
		/* Serve timer interrupts first. */
		clock_comparator_work();
	generic_handle_irq(irq);
	irq_exit();
	set_irq_regs(old_regs);
}

/*
 * show_interrupts is needed by /proc/interrupts.
 */
int show_interrupts(struct seq_file *p, void *v)
{
	int irq = *(loff_t *) v;
	int cpu;

	get_online_cpus();
	if (irq == 0) {
		seq_puts(p, "           ");
		for_each_online_cpu(cpu)
			seq_printf(p, "CPU%d       ", cpu);
		seq_putc(p, '\n');
		goto out;
	}
	if (irq < NR_IRQS) {
		if (irq >= NR_IRQS_BASE)
			goto out;
		seq_printf(p, "%s: ", irqclass_main_desc[irq].name);
		for_each_online_cpu(cpu)
			seq_printf(p, "%10u ", kstat_irqs_cpu(irq, cpu));
		seq_putc(p, '\n');
		goto out;
	}
	for (irq = 0; irq < NR_ARCH_IRQS; irq++) {
		seq_printf(p, "%s: ", irqclass_sub_desc[irq].name);
		for_each_online_cpu(cpu)
			seq_printf(p, "%10u ",
				   per_cpu(irq_stat, cpu).irqs[irq]);
		if (irqclass_sub_desc[irq].desc)
			seq_printf(p, "  %s", irqclass_sub_desc[irq].desc);
		seq_putc(p, '\n');
	}
out:
	put_online_cpus();
	return 0;
}

int arch_show_interrupts(struct seq_file *p, int prec)
{
	return 0;
}

/*
 * Switch to the asynchronous interrupt stack for softirq execution.
 */
asmlinkage void do_softirq(void)
{
	unsigned long flags, old, new;

	if (in_interrupt())
		return;

	local_irq_save(flags);

	if (local_softirq_pending()) {
		/* Get current stack pointer. */
		asm volatile("la %0,0(15)" : "=a" (old));
		/* Check against async. stack address range. */
		new = S390_lowcore.async_stack;
		if (((new - old) >> (PAGE_SHIFT + THREAD_ORDER)) != 0) {
			/* Need to switch to the async. stack. */
			new -= STACK_FRAME_OVERHEAD;
			((struct stack_frame *) new)->back_chain = old;

			asm volatile("   la    15,0(%0)\n"
				     "   basr  14,%2\n"
				     "   la    15,0(%1)\n"
				     : : "a" (new), "a" (old),
				         "a" (__do_softirq)
				     : "0", "1", "2", "3", "4", "5", "14",
				       "cc", "memory" );
		} else {
			/* We are already on the async stack. */
			__do_softirq();
		}
	}

	local_irq_restore(flags);
}

/*
 * ext_int_hash[index] is the list head for all external interrupts that hash
 * to this index.
 */
static struct hlist_head ext_int_hash[32] ____cacheline_aligned;

struct ext_int_info {
	ext_int_handler_t handler;
	struct hlist_node entry;
	struct rcu_head rcu;
	u16 code;
};

/* ext_int_hash_lock protects the handler lists for external interrupts */
static DEFINE_SPINLOCK(ext_int_hash_lock);

static inline int ext_hash(u16 code)
{
	BUILD_BUG_ON(!is_power_of_2(ARRAY_SIZE(ext_int_hash)));

	return (code + (code >> 9)) & (ARRAY_SIZE(ext_int_hash) - 1);
}

int register_external_interrupt(u16 code, ext_int_handler_t handler)
{
	struct ext_int_info *p;
	unsigned long flags;
	int index;

	p = kmalloc(sizeof(*p), GFP_ATOMIC);
	if (!p)
		return -ENOMEM;
	p->code = code;
	p->handler = handler;
	index = ext_hash(code);

	spin_lock_irqsave(&ext_int_hash_lock, flags);
	hlist_add_head_rcu(&p->entry, &ext_int_hash[index]);
	spin_unlock_irqrestore(&ext_int_hash_lock, flags);
	return 0;
}
EXPORT_SYMBOL(register_external_interrupt);

int unregister_external_interrupt(u16 code, ext_int_handler_t handler)
{
	struct ext_int_info *p;
	unsigned long flags;
	int index = ext_hash(code);

	spin_lock_irqsave(&ext_int_hash_lock, flags);
	hlist_for_each_entry_rcu(p, &ext_int_hash[index], entry) {
		if (p->code == code && p->handler == handler) {
			hlist_del_rcu(&p->entry);
			kfree_rcu(p, rcu);
		}
	}
	spin_unlock_irqrestore(&ext_int_hash_lock, flags);
	return 0;
}
EXPORT_SYMBOL(unregister_external_interrupt);

static irqreturn_t do_ext_interrupt(int irq, void *dummy)
{
	struct pt_regs *regs = get_irq_regs();
	struct ext_code ext_code;
	struct ext_int_info *p;
	int index;

	ext_code = *(struct ext_code *) &regs->int_code;
	if (ext_code.code != 0x1004)
		__get_cpu_var(s390_idle).nohz_delay = 1;

	index = ext_hash(ext_code.code);
	rcu_read_lock();
	hlist_for_each_entry_rcu(p, &ext_int_hash[index], entry) {
		if (unlikely(p->code != ext_code.code))
			continue;
		p->handler(ext_code, regs->int_parm, regs->int_parm_long);
	}
	rcu_read_unlock();
	return IRQ_HANDLED;
}

static struct irqaction external_interrupt = {
	.name	 = "EXT",
	.handler = do_ext_interrupt,
};

void __init init_ext_interrupts(void)
{
	int idx;

	for (idx = 0; idx < ARRAY_SIZE(ext_int_hash); idx++)
		INIT_HLIST_HEAD(&ext_int_hash[idx]);

	irq_set_chip_and_handler(EXT_INTERRUPT,
				 &dummy_irq_chip, handle_percpu_irq);
	setup_irq(EXT_INTERRUPT, &external_interrupt);
}

static DEFINE_SPINLOCK(irq_subclass_lock);
static unsigned char irq_subclass_refcount[64];

void irq_subclass_register(enum irq_subclass subclass)
{
	spin_lock(&irq_subclass_lock);
	if (!irq_subclass_refcount[subclass])
		ctl_set_bit(0, subclass);
	irq_subclass_refcount[subclass]++;
	spin_unlock(&irq_subclass_lock);
}
EXPORT_SYMBOL(irq_subclass_register);

void irq_subclass_unregister(enum irq_subclass subclass)
{
	spin_lock(&irq_subclass_lock);
	irq_subclass_refcount[subclass]--;
	if (!irq_subclass_refcount[subclass])
		ctl_clear_bit(0, subclass);
	spin_unlock(&irq_subclass_lock);
}
EXPORT_SYMBOL(irq_subclass_unregister);
