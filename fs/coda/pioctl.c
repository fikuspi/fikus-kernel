/*
 * Pioctl operations for Coda.
 * Original version: (C) 1996 Peter Braam
 * Rewritten for Fikus 2.1: (C) 1997 Carnegie Mellon University
 *
 * Carnegie Mellon encourages users of this code to contribute improvements
 * to the Coda project. Contact Peter Braam <coda@cs.cmu.edu>.
 */

#include <fikus/types.h>
#include <fikus/kernel.h>
#include <fikus/time.h>
#include <fikus/fs.h>
#include <fikus/stat.h>
#include <fikus/errno.h>
#include <fikus/string.h>
#include <fikus/namei.h>
#include <fikus/module.h>
#include <asm/uaccess.h>

#include <fikus/coda.h>
#include <fikus/coda_psdev.h>

#include "coda_fikus.h"

/* pioctl ops */
static int coda_ioctl_permission(struct inode *inode, int mask);
static long coda_pioctl(struct file *filp, unsigned int cmd,
			unsigned long user_data);

/* exported from this file */
const struct inode_operations coda_ioctl_inode_operations = {
	.permission	= coda_ioctl_permission,
	.setattr	= coda_setattr,
};

const struct file_operations coda_ioctl_operations = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= coda_pioctl,
	.llseek		= noop_llseek,
};

/* the coda pioctl inode ops */
static int coda_ioctl_permission(struct inode *inode, int mask)
{
	return (mask & MAY_EXEC) ? -EACCES : 0;
}

static long coda_pioctl(struct file *filp, unsigned int cmd,
			unsigned long user_data)
{
	struct path path;
	int error;
	struct PioctlData data;
	struct inode *inode = file_inode(filp);
	struct inode *target_inode = NULL;
	struct coda_inode_info *cnp;

	/* get the Pioctl data arguments from user space */
	if (copy_from_user(&data, (void __user *)user_data, sizeof(data)))
		return -EINVAL;

	/*
	 * Look up the pathname. Note that the pathname is in
	 * user memory, and namei takes care of this
	 */
	if (data.follow)
		error = user_path(data.path, &path);
	else
		error = user_lpath(data.path, &path);

	if (error)
		return error;

	target_inode = path.dentry->d_inode;

	/* return if it is not a Coda inode */
	if (target_inode->i_sb != inode->i_sb) {
		error = -EINVAL;
		goto out;
	}

	/* now proceed to make the upcall */
	cnp = ITOC(target_inode);

	error = venus_pioctl(inode->i_sb, &(cnp->c_fid), cmd, &data);
out:
	path_put(&path);
	return error;
}
