/**
 * Copyright (c) ????		Jochen Schäuble <psionic@psionic.de>
 * Copyright (c) 2003-2004	Joern Engel <joern@wh.fh-wedel.de>
 *
 * Usage:
 *
 * one commend line parameter per device, each in the form:
 *   phram=<name>,<start>,<len>
 * <name> may be up to 63 characters.
 * <start> and <len> can be octal, decimal or hexadecimal.  If followed
 * by "ki", "Mi" or "Gi", the numbers will be interpreted as kilo, mega or
 * gigabytes.
 *
 * Example:
 *	phram=swap,64Mi,128Mi phram=test,900Mi,1Mi
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <asm/io.h>
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/list.h>
#include <fikus/module.h>
#include <fikus/moduleparam.h>
#include <fikus/slab.h>
#include <fikus/mtd/mtd.h>

struct phram_mtd_list {
	struct mtd_info mtd;
	struct list_head list;
};

static LIST_HEAD(phram_list);

static int phram_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	u_char *start = mtd->priv;

	memset(start + instr->addr, 0xff, instr->len);

	/*
	 * This'll catch a few races. Free the thing before returning :)
	 * I don't feel at all ashamed. This kind of thing is possible anyway
	 * with flash, but unlikely.
	 */
	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);
	return 0;
}

static int phram_point(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, void **virt, resource_size_t *phys)
{
	*virt = mtd->priv + from;
	*retlen = len;
	return 0;
}

static int phram_unpoint(struct mtd_info *mtd, loff_t from, size_t len)
{
	return 0;
}

static int phram_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
	u_char *start = mtd->priv;

	memcpy(buf, start + from, len);
	*retlen = len;
	return 0;
}

static int phram_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	u_char *start = mtd->priv;

	memcpy(start + to, buf, len);
	*retlen = len;
	return 0;
}

static void unregister_devices(void)
{
	struct phram_mtd_list *this, *safe;

	list_for_each_entry_safe(this, safe, &phram_list, list) {
		mtd_device_unregister(&this->mtd);
		iounmap(this->mtd.priv);
		kfree(this->mtd.name);
		kfree(this);
	}
}

static int register_device(char *name, unsigned long start, unsigned long len)
{
	struct phram_mtd_list *new;
	int ret = -ENOMEM;

	new = kzalloc(sizeof(*new), GFP_KERNEL);
	if (!new)
		goto out0;

	ret = -EIO;
	new->mtd.priv = ioremap(start, len);
	if (!new->mtd.priv) {
		pr_err("ioremap failed\n");
		goto out1;
	}


	new->mtd.name = name;
	new->mtd.size = len;
	new->mtd.flags = MTD_CAP_RAM;
	new->mtd._erase = phram_erase;
	new->mtd._point = phram_point;
	new->mtd._unpoint = phram_unpoint;
	new->mtd._read = phram_read;
	new->mtd._write = phram_write;
	new->mtd.owner = THIS_MODULE;
	new->mtd.type = MTD_RAM;
	new->mtd.erasesize = PAGE_SIZE;
	new->mtd.writesize = 1;

	ret = -EAGAIN;
	if (mtd_device_register(&new->mtd, NULL, 0)) {
		pr_err("Failed to register new device\n");
		goto out2;
	}

	list_add_tail(&new->list, &phram_list);
	return 0;

out2:
	iounmap(new->mtd.priv);
out1:
	kfree(new);
out0:
	return ret;
}

static int ustrtoul(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = simple_strtoul(cp, endp, base);

	switch (**endp) {
	case 'G':
		result *= 1024;
	case 'M':
		result *= 1024;
	case 'k':
		result *= 1024;
	/* By dwmw2 editorial decree, "ki", "Mi" or "Gi" are to be used. */
		if ((*endp)[1] == 'i')
			(*endp) += 2;
	}
	return result;
}

static int parse_num32(uint32_t *num32, const char *token)
{
	char *endp;
	unsigned long n;

	n = ustrtoul(token, &endp, 0);
	if (*endp)
		return -EINVAL;

	*num32 = n;
	return 0;
}

static int parse_name(char **pname, const char *token)
{
	size_t len;
	char *name;

	len = strlen(token) + 1;
	if (len > 64)
		return -ENOSPC;

	name = kmalloc(len, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	strcpy(name, token);

	*pname = name;
	return 0;
}


static inline void kill_final_newline(char *str)
{
	char *newline = strrchr(str, '\n');
	if (newline && !newline[1])
		*newline = 0;
}


#define parse_err(fmt, args...) do {	\
	pr_err(fmt , ## args);	\
	return 1;		\
} while (0)

/*
 * This shall contain the module parameter if any. It is of the form:
 * - phram=<device>,<address>,<size> for module case
 * - phram.phram=<device>,<address>,<size> for built-in case
 * We leave 64 bytes for the device name, 12 for the address and 12 for the
 * size.
 * Example: phram.phram=rootfs,0xa0000000,512Mi
 */
static __initdata char phram_paramline[64+12+12];

static int __init phram_setup(const char *val)
{
	char buf[64+12+12], *str = buf;
	char *token[3];
	char *name;
	uint32_t start;
	uint32_t len;
	int i, ret;

	if (strnlen(val, sizeof(buf)) >= sizeof(buf))
		parse_err("parameter too long\n");

	strcpy(str, val);
	kill_final_newline(str);

	for (i=0; i<3; i++)
		token[i] = strsep(&str, ",");

	if (str)
		parse_err("too many arguments\n");

	if (!token[2])
		parse_err("not enough arguments\n");

	ret = parse_name(&name, token[0]);
	if (ret)
		return ret;

	ret = parse_num32(&start, token[1]);
	if (ret) {
		kfree(name);
		parse_err("illegal start address\n");
	}

	ret = parse_num32(&len, token[2]);
	if (ret) {
		kfree(name);
		parse_err("illegal device length\n");
	}

	ret = register_device(name, start, len);
	if (!ret)
		pr_info("%s device: %#x at %#x\n", name, len, start);
	else
		kfree(name);

	return ret;
}

static int __init phram_param_call(const char *val, struct kernel_param *kp)
{
	/*
	 * This function is always called before 'init_phram()', whether
	 * built-in or module.
	 */
	if (strlen(val) >= sizeof(phram_paramline))
		return -ENOSPC;
	strcpy(phram_paramline, val);

	return 0;
}

module_param_call(phram, phram_param_call, NULL, NULL, 000);
MODULE_PARM_DESC(phram, "Memory region to map. \"phram=<name>,<start>,<length>\"");


static int __init init_phram(void)
{
	if (phram_paramline[0])
		return phram_setup(phram_paramline);

	return 0;
}

static void __exit cleanup_phram(void)
{
	unregister_devices();
}

module_init(init_phram);
module_exit(cleanup_phram);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joern Engel <joern@wh.fh-wedel.de>");
MODULE_DESCRIPTION("MTD driver for physical RAM");
