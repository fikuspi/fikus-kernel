/*
 * Copyright (C) 2012 Red Hat, Inc.
 * Copyright (C) 2012 Jeremy Kerr <jeremy.kerr@canonical.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/efi.h>
#include <fikus/fs.h>
#include <fikus/slab.h>

#include "internal.h"

static ssize_t efivarfs_file_write(struct file *file,
		const char __user *userbuf, size_t count, loff_t *ppos)
{
	struct efivar_entry *var = file->private_data;
	void *data;
	u32 attributes;
	struct inode *inode = file->f_mapping->host;
	unsigned long datasize = count - sizeof(attributes);
	ssize_t bytes = 0;
	bool set = false;

	if (count < sizeof(attributes))
		return -EINVAL;

	if (copy_from_user(&attributes, userbuf, sizeof(attributes)))
		return -EFAULT;

	if (attributes & ~(EFI_VARIABLE_MASK))
		return -EINVAL;

	data = kmalloc(datasize, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (copy_from_user(data, userbuf + sizeof(attributes), datasize)) {
		bytes = -EFAULT;
		goto out;
	}

	bytes = efivar_entry_set_get_size(var, attributes, &datasize,
					  data, &set);
	if (!set && bytes) {
		if (bytes == -ENOENT)
			bytes = -EIO;
		goto out;
	}

	if (bytes == -ENOENT) {
		drop_nlink(inode);
		d_delete(file->f_dentry);
		dput(file->f_dentry);
	} else {
		mutex_lock(&inode->i_mutex);
		i_size_write(inode, datasize + sizeof(attributes));
		mutex_unlock(&inode->i_mutex);
	}

	bytes = count;

out:
	kfree(data);

	return bytes;
}

static ssize_t efivarfs_file_read(struct file *file, char __user *userbuf,
		size_t count, loff_t *ppos)
{
	struct efivar_entry *var = file->private_data;
	unsigned long datasize = 0;
	u32 attributes;
	void *data;
	ssize_t size = 0;
	int err;

	err = efivar_entry_size(var, &datasize);

	/*
	 * efivarfs represents uncommitted variables with
	 * zero-length files. Reading them should return EOF.
	 */
	if (err == -ENOENT)
		return 0;
	else if (err)
		return err;

	data = kmalloc(datasize + sizeof(attributes), GFP_KERNEL);

	if (!data)
		return -ENOMEM;

	size = efivar_entry_get(var, &attributes, &datasize,
				data + sizeof(attributes));
	if (size)
		goto out_free;

	memcpy(data, &attributes, sizeof(attributes));
	size = simple_read_from_buffer(userbuf, count, ppos,
				       data, datasize + sizeof(attributes));
out_free:
	kfree(data);

	return size;
}

const struct file_operations efivarfs_file_operations = {
	.open	= simple_open,
	.read	= efivarfs_file_read,
	.write	= efivarfs_file_write,
	.llseek	= no_llseek,
};
