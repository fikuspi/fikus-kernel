/*
 *  Copyright IBM Corp. 2001, 2009
 *  Author(s): Ulrich Weigand <Ulrich.Weigand@de.ibm.com>,
 *	       Martin Schwidefsky <schwidefsky@de.ibm.com>,
 */

#include <fikus/kernel.h>
#include <fikus/mm.h>
#include <fikus/proc_fs.h>
#include <fikus/seq_file.h>
#include <fikus/init.h>
#include <fikus/delay.h>
#include <fikus/module.h>
#include <fikus/slab.h>
#include <asm/ebcdic.h>
#include <asm/sysinfo.h>
#include <asm/cpcmd.h>
#include <asm/topology.h>

/* Sigh, math-emu. Don't ask. */
#include <asm/sfp-util.h>
#include <math-emu/soft-fp.h>
#include <math-emu/single.h>

int topology_max_mnest;

/*
 * stsi - store system information
 *
 * Returns the current configuration level if function code 0 was specified.
 * Otherwise returns 0 on success or a negative value on error.
 */
int stsi(void *sysinfo, int fc, int sel1, int sel2)
{
	register int r0 asm("0") = (fc << 28) | sel1;
	register int r1 asm("1") = sel2;
	int rc = 0;

	asm volatile(
		"	stsi	0(%3)\n"
		"0:	jz	2f\n"
		"1:	lhi	%1,%4\n"
		"2:\n"
		EX_TABLE(0b, 1b)
		: "+d" (r0), "+d" (rc)
		: "d" (r1), "a" (sysinfo), "K" (-EOPNOTSUPP)
		: "cc", "memory");
	if (rc)
		return rc;
	return fc ? 0 : ((unsigned int) r0) >> 28;
}
EXPORT_SYMBOL(stsi);

static void stsi_1_1_1(struct seq_file *m, struct sysinfo_1_1_1 *info)
{
	int i;

	if (stsi(info, 1, 1, 1))
		return;
	EBCASC(info->manufacturer, sizeof(info->manufacturer));
	EBCASC(info->type, sizeof(info->type));
	EBCASC(info->model, sizeof(info->model));
	EBCASC(info->sequence, sizeof(info->sequence));
	EBCASC(info->plant, sizeof(info->plant));
	EBCASC(info->model_capacity, sizeof(info->model_capacity));
	EBCASC(info->model_perm_cap, sizeof(info->model_perm_cap));
	EBCASC(info->model_temp_cap, sizeof(info->model_temp_cap));
	seq_printf(m, "Manufacturer:         %-16.16s\n", info->manufacturer);
	seq_printf(m, "Type:                 %-4.4s\n", info->type);
	/*
	 * Sigh: the model field has been renamed with System z9
	 * to model_capacity and a new model field has been added
	 * after the plant field. To avoid confusing older programs
	 * the "Model:" prints "model_capacity model" or just
	 * "model_capacity" if the model string is empty .
	 */
	seq_printf(m, "Model:                %-16.16s", info->model_capacity);
	if (info->model[0] != '\0')
		seq_printf(m, " %-16.16s", info->model);
	seq_putc(m, '\n');
	seq_printf(m, "Sequence Code:        %-16.16s\n", info->sequence);
	seq_printf(m, "Plant:                %-4.4s\n", info->plant);
	seq_printf(m, "Model Capacity:       %-16.16s %08u\n",
		   info->model_capacity, info->model_cap_rating);
	if (info->model_perm_cap_rating)
		seq_printf(m, "Model Perm. Capacity: %-16.16s %08u\n",
			   info->model_perm_cap,
			   info->model_perm_cap_rating);
	if (info->model_temp_cap_rating)
		seq_printf(m, "Model Temp. Capacity: %-16.16s %08u\n",
			   info->model_temp_cap,
			   info->model_temp_cap_rating);
	if (info->ncr)
		seq_printf(m, "Nominal Cap. Rating:  %08u\n", info->ncr);
	if (info->npr)
		seq_printf(m, "Nominal Perm. Rating: %08u\n", info->npr);
	if (info->ntr)
		seq_printf(m, "Nominal Temp. Rating: %08u\n", info->ntr);
	if (info->cai) {
		seq_printf(m, "Capacity Adj. Ind.:   %d\n", info->cai);
		seq_printf(m, "Capacity Ch. Reason:  %d\n", info->ccr);
		seq_printf(m, "Capacity Transient:   %d\n", info->t);
	}
	if (info->p) {
		for (i = 1; i <= ARRAY_SIZE(info->typepct); i++) {
			seq_printf(m, "Type %d Percentage:    %d\n",
				   i, info->typepct[i - 1]);
		}
	}
}

static void stsi_15_1_x(struct seq_file *m, struct sysinfo_15_1_x *info)
{
	static int max_mnest;
	int i, rc;

	seq_putc(m, '\n');
	if (!MACHINE_HAS_TOPOLOGY)
		return;
	if (stsi(info, 15, 1, topology_max_mnest))
		return;
	seq_printf(m, "CPU Topology HW:     ");
	for (i = 0; i < TOPOLOGY_NR_MAG; i++)
		seq_printf(m, " %d", info->mag[i]);
	seq_putc(m, '\n');
#ifdef CONFIG_SCHED_MC
	store_topology(info);
	seq_printf(m, "CPU Topology SW:     ");
	for (i = 0; i < TOPOLOGY_NR_MAG; i++)
		seq_printf(m, " %d", info->mag[i]);
	seq_putc(m, '\n');
#endif
}

static void stsi_1_2_2(struct seq_file *m, struct sysinfo_1_2_2 *info)
{
	struct sysinfo_1_2_2_extension *ext;
	int i;

	if (stsi(info, 1, 2, 2))
		return;
	ext = (struct sysinfo_1_2_2_extension *)
		((unsigned long) info + info->acc_offset);
	seq_printf(m, "CPUs Total:           %d\n", info->cpus_total);
	seq_printf(m, "CPUs Configured:      %d\n", info->cpus_configured);
	seq_printf(m, "CPUs Standby:         %d\n", info->cpus_standby);
	seq_printf(m, "CPUs Reserved:        %d\n", info->cpus_reserved);
	/*
	 * Sigh 2. According to the specification the alternate
	 * capability field is a 32 bit floating point number
	 * if the higher order 8 bits are not zero. Printing
	 * a floating point number in the kernel is a no-no,
	 * always print the number as 32 bit unsigned integer.
	 * The user-space needs to know about the strange
	 * encoding of the alternate cpu capability.
	 */
	seq_printf(m, "Capability:           %u", info->capability);
	if (info->format == 1)
		seq_printf(m, " %u", ext->alt_capability);
	seq_putc(m, '\n');
	if (info->nominal_cap)
		seq_printf(m, "Nominal Capability:   %d\n", info->nominal_cap);
	if (info->secondary_cap)
		seq_printf(m, "Secondary Capability: %d\n", info->secondary_cap);
	for (i = 2; i <= info->cpus_total; i++) {
		seq_printf(m, "Adjustment %02d-way:    %u",
			   i, info->adjustment[i-2]);
		if (info->format == 1)
			seq_printf(m, " %u", ext->alt_adjustment[i-2]);
		seq_putc(m, '\n');
	}
}

static void stsi_2_2_2(struct seq_file *m, struct sysinfo_2_2_2 *info)
{
	if (stsi(info, 2, 2, 2))
		return;
	EBCASC(info->name, sizeof(info->name));
	seq_putc(m, '\n');
	seq_printf(m, "LPAR Number:          %d\n", info->lpar_number);
	seq_printf(m, "LPAR Characteristics: ");
	if (info->characteristics & LPAR_CHAR_DEDICATED)
		seq_printf(m, "Dedicated ");
	if (info->characteristics & LPAR_CHAR_SHARED)
		seq_printf(m, "Shared ");
	if (info->characteristics & LPAR_CHAR_LIMITED)
		seq_printf(m, "Limited ");
	seq_putc(m, '\n');
	seq_printf(m, "LPAR Name:            %-8.8s\n", info->name);
	seq_printf(m, "LPAR Adjustment:      %d\n", info->caf);
	seq_printf(m, "LPAR CPUs Total:      %d\n", info->cpus_total);
	seq_printf(m, "LPAR CPUs Configured: %d\n", info->cpus_configured);
	seq_printf(m, "LPAR CPUs Standby:    %d\n", info->cpus_standby);
	seq_printf(m, "LPAR CPUs Reserved:   %d\n", info->cpus_reserved);
	seq_printf(m, "LPAR CPUs Dedicated:  %d\n", info->cpus_dedicated);
	seq_printf(m, "LPAR CPUs Shared:     %d\n", info->cpus_shared);
}

static void stsi_3_2_2(struct seq_file *m, struct sysinfo_3_2_2 *info)
{
	int i;

	if (stsi(info, 3, 2, 2))
		return;
	for (i = 0; i < info->count; i++) {
		EBCASC(info->vm[i].name, sizeof(info->vm[i].name));
		EBCASC(info->vm[i].cpi, sizeof(info->vm[i].cpi));
		seq_putc(m, '\n');
		seq_printf(m, "VM%02d Name:            %-8.8s\n", i, info->vm[i].name);
		seq_printf(m, "VM%02d Control Program: %-16.16s\n", i, info->vm[i].cpi);
		seq_printf(m, "VM%02d Adjustment:      %d\n", i, info->vm[i].caf);
		seq_printf(m, "VM%02d CPUs Total:      %d\n", i, info->vm[i].cpus_total);
		seq_printf(m, "VM%02d CPUs Configured: %d\n", i, info->vm[i].cpus_configured);
		seq_printf(m, "VM%02d CPUs Standby:    %d\n", i, info->vm[i].cpus_standby);
		seq_printf(m, "VM%02d CPUs Reserved:   %d\n", i, info->vm[i].cpus_reserved);
	}
}

static int sysinfo_show(struct seq_file *m, void *v)
{
	void *info = (void *)get_zeroed_page(GFP_KERNEL);
	int level;

	if (!info)
		return 0;
	level = stsi(NULL, 0, 0, 0);
	if (level >= 1)
		stsi_1_1_1(m, info);
	if (level >= 1)
		stsi_15_1_x(m, info);
	if (level >= 1)
		stsi_1_2_2(m, info);
	if (level >= 2)
		stsi_2_2_2(m, info);
	if (level >= 3)
		stsi_3_2_2(m, info);
	free_page((unsigned long)info);
	return 0;
}

static int sysinfo_open(struct inode *inode, struct file *file)
{
	return single_open(file, sysinfo_show, NULL);
}

static const struct file_operations sysinfo_fops = {
	.open		= sysinfo_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init sysinfo_create_proc(void)
{
	proc_create("sysinfo", 0444, NULL, &sysinfo_fops);
	return 0;
}
device_initcall(sysinfo_create_proc);

/*
 * Service levels interface.
 */

static DECLARE_RWSEM(service_level_sem);
static LIST_HEAD(service_level_list);

int register_service_level(struct service_level *slr)
{
	struct service_level *ptr;

	down_write(&service_level_sem);
	list_for_each_entry(ptr, &service_level_list, list)
		if (ptr == slr) {
			up_write(&service_level_sem);
			return -EEXIST;
		}
	list_add_tail(&slr->list, &service_level_list);
	up_write(&service_level_sem);
	return 0;
}
EXPORT_SYMBOL(register_service_level);

int unregister_service_level(struct service_level *slr)
{
	struct service_level *ptr, *next;
	int rc = -ENOENT;

	down_write(&service_level_sem);
	list_for_each_entry_safe(ptr, next, &service_level_list, list) {
		if (ptr != slr)
			continue;
		list_del(&ptr->list);
		rc = 0;
		break;
	}
	up_write(&service_level_sem);
	return rc;
}
EXPORT_SYMBOL(unregister_service_level);

static void *service_level_start(struct seq_file *m, loff_t *pos)
{
	down_read(&service_level_sem);
	return seq_list_start(&service_level_list, *pos);
}

static void *service_level_next(struct seq_file *m, void *p, loff_t *pos)
{
	return seq_list_next(p, &service_level_list, pos);
}

static void service_level_stop(struct seq_file *m, void *p)
{
	up_read(&service_level_sem);
}

static int service_level_show(struct seq_file *m, void *p)
{
	struct service_level *slr;

	slr = list_entry(p, struct service_level, list);
	slr->seq_print(m, slr);
	return 0;
}

static const struct seq_operations service_level_seq_ops = {
	.start		= service_level_start,
	.next		= service_level_next,
	.stop		= service_level_stop,
	.show		= service_level_show
};

static int service_level_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &service_level_seq_ops);
}

static const struct file_operations service_level_ops = {
	.open		= service_level_open,
	.read		= seq_read,
	.llseek 	= seq_lseek,
	.release	= seq_release
};

static void service_level_vm_print(struct seq_file *m,
				   struct service_level *slr)
{
	char *query_buffer, *str;

	query_buffer = kmalloc(1024, GFP_KERNEL | GFP_DMA);
	if (!query_buffer)
		return;
	cpcmd("QUERY CPLEVEL", query_buffer, 1024, NULL);
	str = strchr(query_buffer, '\n');
	if (str)
		*str = 0;
	seq_printf(m, "VM: %s\n", query_buffer);
	kfree(query_buffer);
}

static struct service_level service_level_vm = {
	.seq_print = service_level_vm_print
};

static __init int create_proc_service_level(void)
{
	proc_create("service_levels", 0, NULL, &service_level_ops);
	if (MACHINE_IS_VM)
		register_service_level(&service_level_vm);
	return 0;
}
subsys_initcall(create_proc_service_level);

/*
 * CPU capability might have changed. Therefore recalculate loops_per_jiffy.
 */
void s390_adjust_jiffies(void)
{
	struct sysinfo_1_2_2 *info;
	const unsigned int fmil = 0x4b189680;	/* 1e7 as 32-bit float. */
	FP_DECL_S(SA); FP_DECL_S(SB); FP_DECL_S(SR);
	FP_DECL_EX;
	unsigned int capability;

	info = (void *) get_zeroed_page(GFP_KERNEL);
	if (!info)
		return;

	if (stsi(info, 1, 2, 2) == 0) {
		/*
		 * Major sigh. The cpu capability encoding is "special".
		 * If the first 9 bits of info->capability are 0 then it
		 * is a 32 bit unsigned integer in the range 0 .. 2^23.
		 * If the first 9 bits are != 0 then it is a 32 bit float.
		 * In addition a lower value indicates a proportionally
		 * higher cpu capacity. Bogomips are the other way round.
		 * To get to a halfway suitable number we divide 1e7
		 * by the cpu capability number. Yes, that means a floating
		 * point division .. math-emu here we come :-)
		 */
		FP_UNPACK_SP(SA, &fmil);
		if ((info->capability >> 23) == 0)
			FP_FROM_INT_S(SB, (long) info->capability, 64, long);
		else
			FP_UNPACK_SP(SB, &info->capability);
		FP_DIV_S(SR, SA, SB);
		FP_TO_INT_S(capability, SR, 32, 0);
	} else
		/*
		 * Really old machine without stsi block for basic
		 * cpu information. Report 42.0 bogomips.
		 */
		capability = 42;
	loops_per_jiffy = capability * (500000/HZ);
	free_page((unsigned long) info);
}

/*
 * calibrate the delay loop
 */
void calibrate_delay(void)
{
	s390_adjust_jiffies();
	/* Print the good old Bogomips line .. */
	printk(KERN_DEBUG "Calibrating delay loop (skipped)... "
	       "%lu.%02lu BogoMIPS preset\n", loops_per_jiffy/(500000/HZ),
	       (loops_per_jiffy/(5000/HZ)) % 100);
}
