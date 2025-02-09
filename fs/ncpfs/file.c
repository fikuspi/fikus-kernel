/*
 *  file.c
 *
 *  Copyright (C) 1995, 1996 by Volker Lendecke
 *  Modified 1997 Peter Waltenberg, Bill Hawes, David Woodhouse for 2.1 dcache
 *
 */

#include <asm/uaccess.h>

#include <fikus/time.h>
#include <fikus/kernel.h>
#include <fikus/errno.h>
#include <fikus/fcntl.h>
#include <fikus/stat.h>
#include <fikus/mm.h>
#include <fikus/vmalloc.h>
#include <fikus/sched.h>

#include "ncp_fs.h"

static int ncp_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	return filemap_write_and_wait_range(file->f_mapping, start, end);
}

/*
 * Open a file with the specified read/write mode.
 */
int ncp_make_open(struct inode *inode, int right)
{
	int error;
	int access;

	error = -EINVAL;
	if (!inode) {
		printk(KERN_ERR "ncp_make_open: got NULL inode\n");
		goto out;
	}

	DPRINTK("ncp_make_open: opened=%d, volume # %u, dir entry # %u\n",
		atomic_read(&NCP_FINFO(inode)->opened), 
		NCP_FINFO(inode)->volNumber, 
		NCP_FINFO(inode)->dirEntNum);
	error = -EACCES;
	mutex_lock(&NCP_FINFO(inode)->open_mutex);
	if (!atomic_read(&NCP_FINFO(inode)->opened)) {
		struct ncp_entry_info finfo;
		int result;

		/* tries max. rights */
		finfo.access = O_RDWR;
		result = ncp_open_create_file_or_subdir(NCP_SERVER(inode),
					inode, NULL, OC_MODE_OPEN,
					0, AR_READ | AR_WRITE, &finfo);
		if (!result)
			goto update;
		/* RDWR did not succeeded, try readonly or writeonly as requested */
		switch (right) {
			case O_RDONLY:
				finfo.access = O_RDONLY;
				result = ncp_open_create_file_or_subdir(NCP_SERVER(inode),
					inode, NULL, OC_MODE_OPEN,
					0, AR_READ, &finfo);
				break;
			case O_WRONLY:
				finfo.access = O_WRONLY;
				result = ncp_open_create_file_or_subdir(NCP_SERVER(inode),
					inode, NULL, OC_MODE_OPEN,
					0, AR_WRITE, &finfo);
				break;
		}
		if (result) {
			PPRINTK("ncp_make_open: failed, result=%d\n", result);
			goto out_unlock;
		}
		/*
		 * Update the inode information.
		 */
	update:
		ncp_update_inode(inode, &finfo);
		atomic_set(&NCP_FINFO(inode)->opened, 1);
	}

	access = NCP_FINFO(inode)->access;
	PPRINTK("ncp_make_open: file open, access=%x\n", access);
	if (access == right || access == O_RDWR) {
		atomic_inc(&NCP_FINFO(inode)->opened);
		error = 0;
	}

out_unlock:
	mutex_unlock(&NCP_FINFO(inode)->open_mutex);
out:
	return error;
}

static ssize_t
ncp_file_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct dentry *dentry = file->f_path.dentry;
	struct inode *inode = dentry->d_inode;
	size_t already_read = 0;
	off_t pos;
	size_t bufsize;
	int error;
	void* freepage;
	size_t freelen;

	DPRINTK("ncp_file_read: enter %s/%s\n",
		dentry->d_parent->d_name.name, dentry->d_name.name);

	pos = *ppos;

	if ((ssize_t) count < 0) {
		return -EINVAL;
	}
	if (!count)
		return 0;
	if (pos > inode->i_sb->s_maxbytes)
		return 0;
	if (pos + count > inode->i_sb->s_maxbytes) {
		count = inode->i_sb->s_maxbytes - pos;
	}

	error = ncp_make_open(inode, O_RDONLY);
	if (error) {
		DPRINTK(KERN_ERR "ncp_file_read: open failed, error=%d\n", error);
		return error;
	}

	bufsize = NCP_SERVER(inode)->buffer_size;

	error = -EIO;
	freelen = ncp_read_bounce_size(bufsize);
	freepage = vmalloc(freelen);
	if (!freepage)
		goto outrel;
	error = 0;
	/* First read in as much as possible for each bufsize. */
	while (already_read < count) {
		int read_this_time;
		size_t to_read = min_t(unsigned int,
				     bufsize - (pos % bufsize),
				     count - already_read);

		error = ncp_read_bounce(NCP_SERVER(inode),
			 	NCP_FINFO(inode)->file_handle,
				pos, to_read, buf, &read_this_time, 
				freepage, freelen);
		if (error) {
			error = -EIO;	/* NW errno -> Fikus errno */
			break;
		}
		pos += read_this_time;
		buf += read_this_time;
		already_read += read_this_time;

		if (read_this_time != to_read) {
			break;
		}
	}
	vfree(freepage);

	*ppos = pos;

	file_accessed(file);

	DPRINTK("ncp_file_read: exit %s/%s\n",
		dentry->d_parent->d_name.name, dentry->d_name.name);
outrel:
	ncp_inode_close(inode);		
	return already_read ? already_read : error;
}

static ssize_t
ncp_file_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	struct dentry *dentry = file->f_path.dentry;
	struct inode *inode = dentry->d_inode;
	size_t already_written = 0;
	off_t pos;
	size_t bufsize;
	int errno;
	void* bouncebuffer;

	DPRINTK("ncp_file_write: enter %s/%s\n",
		dentry->d_parent->d_name.name, dentry->d_name.name);
	if ((ssize_t) count < 0)
		return -EINVAL;
	pos = *ppos;
	if (file->f_flags & O_APPEND) {
		pos = i_size_read(inode);
	}

	if (pos + count > MAX_NON_LFS && !(file->f_flags&O_LARGEFILE)) {
		if (pos >= MAX_NON_LFS) {
			return -EFBIG;
		}
		if (count > MAX_NON_LFS - (u32)pos) {
			count = MAX_NON_LFS - (u32)pos;
		}
	}
	if (pos >= inode->i_sb->s_maxbytes) {
		if (count || pos > inode->i_sb->s_maxbytes) {
			return -EFBIG;
		}
	}
	if (pos + count > inode->i_sb->s_maxbytes) {
		count = inode->i_sb->s_maxbytes - pos;
	}
	
	if (!count)
		return 0;
	errno = ncp_make_open(inode, O_WRONLY);
	if (errno) {
		DPRINTK(KERN_ERR "ncp_file_write: open failed, error=%d\n", errno);
		return errno;
	}
	bufsize = NCP_SERVER(inode)->buffer_size;

	already_written = 0;

	errno = file_update_time(file);
	if (errno)
		goto outrel;

	bouncebuffer = vmalloc(bufsize);
	if (!bouncebuffer) {
		errno = -EIO;	/* -ENOMEM */
		goto outrel;
	}
	while (already_written < count) {
		int written_this_time;
		size_t to_write = min_t(unsigned int,
				      bufsize - (pos % bufsize),
				      count - already_written);

		if (copy_from_user(bouncebuffer, buf, to_write)) {
			errno = -EFAULT;
			break;
		}
		if (ncp_write_kernel(NCP_SERVER(inode), 
		    NCP_FINFO(inode)->file_handle,
		    pos, to_write, bouncebuffer, &written_this_time) != 0) {
			errno = -EIO;
			break;
		}
		pos += written_this_time;
		buf += written_this_time;
		already_written += written_this_time;

		if (written_this_time != to_write) {
			break;
		}
	}
	vfree(bouncebuffer);

	*ppos = pos;

	if (pos > i_size_read(inode)) {
		mutex_lock(&inode->i_mutex);
		if (pos > i_size_read(inode))
			i_size_write(inode, pos);
		mutex_unlock(&inode->i_mutex);
	}
	DPRINTK("ncp_file_write: exit %s/%s\n",
		dentry->d_parent->d_name.name, dentry->d_name.name);
outrel:
	ncp_inode_close(inode);		
	return already_written ? already_written : errno;
}

static int ncp_release(struct inode *inode, struct file *file) {
	if (ncp_make_closed(inode)) {
		DPRINTK("ncp_release: failed to close\n");
	}
	return 0;
}

const struct file_operations ncp_file_operations =
{
	.llseek		= generic_file_llseek,
	.read		= ncp_file_read,
	.write		= ncp_file_write,
	.unlocked_ioctl	= ncp_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= ncp_compat_ioctl,
#endif
	.mmap		= ncp_mmap,
	.release	= ncp_release,
	.fsync		= ncp_fsync,
};

const struct inode_operations ncp_file_inode_operations =
{
	.setattr	= ncp_notify_change,
};
