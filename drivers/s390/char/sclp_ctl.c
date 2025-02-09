/*
 * IOCTL interface for SCLP
 *
 * Copyright IBM Corp. 2012
 *
 * Author: Michael Holzheu <holzheu@fikus.vnet.ibm.com>
 */

#include <fikus/compat.h>
#include <fikus/uaccess.h>
#include <fikus/miscdevice.h>
#include <fikus/gfp.h>
#include <fikus/module.h>
#include <fikus/ioctl.h>
#include <fikus/fs.h>
#include <asm/compat.h>
#include <asm/sclp_ctl.h>
#include <asm/sclp.h>

#include "sclp.h"

/*
 * Supported command words
 */
static unsigned int sclp_ctl_sccb_wlist[] = {
	0x00400002,
	0x00410002,
};

/*
 * Check if command word is supported
 */
static int sclp_ctl_cmdw_supported(unsigned int cmdw)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sclp_ctl_sccb_wlist); i++) {
		if (cmdw == sclp_ctl_sccb_wlist[i])
			return 1;
	}
	return 0;
}

static void __user *u64_to_uptr(u64 value)
{
	if (is_compat_task())
		return compat_ptr(value);
	else
		return (void __user *)(unsigned long)value;
}

/*
 * Start SCLP request
 */
static int sclp_ctl_ioctl_sccb(void __user *user_area)
{
	struct sclp_ctl_sccb ctl_sccb;
	struct sccb_header *sccb;
	int rc;

	if (copy_from_user(&ctl_sccb, user_area, sizeof(ctl_sccb)))
		return -EFAULT;
	if (!sclp_ctl_cmdw_supported(ctl_sccb.cmdw))
		return -EOPNOTSUPP;
	sccb = (void *) get_zeroed_page(GFP_KERNEL | GFP_DMA);
	if (!sccb)
		return -ENOMEM;
	if (copy_from_user(sccb, u64_to_uptr(ctl_sccb.sccb), sizeof(*sccb))) {
		rc = -EFAULT;
		goto out_free;
	}
	if (sccb->length > PAGE_SIZE || sccb->length < 8)
		return -EINVAL;
	if (copy_from_user(sccb, u64_to_uptr(ctl_sccb.sccb), sccb->length)) {
		rc = -EFAULT;
		goto out_free;
	}
	rc = sclp_sync_request(ctl_sccb.cmdw, sccb);
	if (rc)
		goto out_free;
	if (copy_to_user(u64_to_uptr(ctl_sccb.sccb), sccb, sccb->length))
		rc = -EFAULT;
out_free:
	free_page((unsigned long) sccb);
	return rc;
}

/*
 * SCLP SCCB ioctl function
 */
static long sclp_ctl_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	void __user *argp;

	if (is_compat_task())
		argp = compat_ptr(arg);
	else
		argp = (void __user *) arg;
	switch (cmd) {
	case SCLP_CTL_SCCB:
		return sclp_ctl_ioctl_sccb(argp);
	default: /* unknown ioctl number */
		return -ENOTTY;
	}
}

/*
 * File operations
 */
static const struct file_operations sclp_ctl_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = sclp_ctl_ioctl,
	.compat_ioctl = sclp_ctl_ioctl,
	.llseek = no_llseek,
};

/*
 * Misc device definition
 */
static struct miscdevice sclp_ctl_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sclp",
	.fops = &sclp_ctl_fops,
};

/*
 * Register sclp_ctl misc device
 */
static int __init sclp_ctl_init(void)
{
	return misc_register(&sclp_ctl_device);
}
module_init(sclp_ctl_init);

/*
 * Deregister sclp_ctl misc device
 */
static void __exit sclp_ctl_exit(void)
{
	misc_deregister(&sclp_ctl_device);
}
module_exit(sclp_ctl_exit);
