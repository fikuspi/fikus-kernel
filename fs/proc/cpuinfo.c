#include <fikus/fs.h>
#include <fikus/init.h>
#include <fikus/proc_fs.h>
#include <fikus/seq_file.h>

extern const struct seq_operations cpuinfo_op;
static int cpuinfo_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &cpuinfo_op);
}

static const struct file_operations proc_cpuinfo_operations = {
	.open		= cpuinfo_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init proc_cpuinfo_init(void)
{
	proc_create("cpuinfo", 0, NULL, &proc_cpuinfo_operations);
	return 0;
}
module_init(proc_cpuinfo_init);
