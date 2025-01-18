#include <fikus/fs.h>
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/proc_fs.h>
#include <fikus/seq_file.h>
#include <fikus/utsname.h>

static int version_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, fikus_proc_banner,
		utsname()->sysname,
		utsname()->release,
		utsname()->version);
	return 0;
}

static int version_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, version_proc_show, NULL);
}

static const struct file_operations version_proc_fops = {
	.open		= version_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_version_init(void)
{
	proc_create("version", 0, NULL, &version_proc_fops);
	return 0;
}
module_init(proc_version_init);
