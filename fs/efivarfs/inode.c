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
#include <fikus/ctype.h>
#include <fikus/slab.h>

#include "internal.h"

struct inode *efivarfs_get_inode(struct super_block *sb,
				const struct inode *dir, int mode, dev_t dev)
{
	struct inode *inode = new_inode(sb);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode->i_mode = mode;
		inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
		switch (mode & S_IFMT) {
		case S_IFREG:
			inode->i_fop = &efivarfs_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &efivarfs_dir_inode_operations;
			inode->i_fop = &simple_dir_operations;
			inc_nlink(inode);
			break;
		}
	}
	return inode;
}

/*
 * Return true if 'str' is a valid efivarfs filename of the form,
 *
 *	VariableName-12345678-1234-1234-1234-1234567891bc
 */
bool efivarfs_valid_name(const char *str, int len)
{
	static const char dashes[EFI_VARIABLE_GUID_LEN] = {
		[8] = 1, [13] = 1, [18] = 1, [23] = 1
	};
	const char *s = str + len - EFI_VARIABLE_GUID_LEN;
	int i;

	/*
	 * We need a GUID, plus at least one letter for the variable name,
	 * plus the '-' separator
	 */
	if (len < EFI_VARIABLE_GUID_LEN + 2)
		return false;

	/* GUID must be preceded by a '-' */
	if (*(s - 1) != '-')
		return false;

	/*
	 * Validate that 's' is of the correct format, e.g.
	 *
	 *	12345678-1234-1234-1234-123456789abc
	 */
	for (i = 0; i < EFI_VARIABLE_GUID_LEN; i++) {
		if (dashes[i]) {
			if (*s++ != '-')
				return false;
		} else {
			if (!isxdigit(*s++))
				return false;
		}
	}

	return true;
}

static void efivarfs_hex_to_guid(const char *str, efi_guid_t *guid)
{
	guid->b[0] = hex_to_bin(str[6]) << 4 | hex_to_bin(str[7]);
	guid->b[1] = hex_to_bin(str[4]) << 4 | hex_to_bin(str[5]);
	guid->b[2] = hex_to_bin(str[2]) << 4 | hex_to_bin(str[3]);
	guid->b[3] = hex_to_bin(str[0]) << 4 | hex_to_bin(str[1]);
	guid->b[4] = hex_to_bin(str[11]) << 4 | hex_to_bin(str[12]);
	guid->b[5] = hex_to_bin(str[9]) << 4 | hex_to_bin(str[10]);
	guid->b[6] = hex_to_bin(str[16]) << 4 | hex_to_bin(str[17]);
	guid->b[7] = hex_to_bin(str[14]) << 4 | hex_to_bin(str[15]);
	guid->b[8] = hex_to_bin(str[19]) << 4 | hex_to_bin(str[20]);
	guid->b[9] = hex_to_bin(str[21]) << 4 | hex_to_bin(str[22]);
	guid->b[10] = hex_to_bin(str[24]) << 4 | hex_to_bin(str[25]);
	guid->b[11] = hex_to_bin(str[26]) << 4 | hex_to_bin(str[27]);
	guid->b[12] = hex_to_bin(str[28]) << 4 | hex_to_bin(str[29]);
	guid->b[13] = hex_to_bin(str[30]) << 4 | hex_to_bin(str[31]);
	guid->b[14] = hex_to_bin(str[32]) << 4 | hex_to_bin(str[33]);
	guid->b[15] = hex_to_bin(str[34]) << 4 | hex_to_bin(str[35]);
}

static int efivarfs_create(struct inode *dir, struct dentry *dentry,
			  umode_t mode, bool excl)
{
	struct inode *inode;
	struct efivar_entry *var;
	int namelen, i = 0, err = 0;

	if (!efivarfs_valid_name(dentry->d_name.name, dentry->d_name.len))
		return -EINVAL;

	inode = efivarfs_get_inode(dir->i_sb, dir, mode, 0);
	if (!inode)
		return -ENOMEM;

	var = kzalloc(sizeof(struct efivar_entry), GFP_KERNEL);
	if (!var) {
		err = -ENOMEM;
		goto out;
	}

	/* length of the variable name itself: remove GUID and separator */
	namelen = dentry->d_name.len - EFI_VARIABLE_GUID_LEN - 1;

	efivarfs_hex_to_guid(dentry->d_name.name + namelen + 1,
			&var->var.VendorGuid);

	for (i = 0; i < namelen; i++)
		var->var.VariableName[i] = dentry->d_name.name[i];

	var->var.VariableName[i] = '\0';

	inode->i_private = var;

	efivar_entry_add(var, &efivarfs_list);
	d_instantiate(dentry, inode);
	dget(dentry);
out:
	if (err) {
		kfree(var);
		iput(inode);
	}
	return err;
}

static int efivarfs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct efivar_entry *var = dentry->d_inode->i_private;

	if (efivar_entry_delete(var))
		return -EINVAL;

	drop_nlink(dentry->d_inode);
	dput(dentry);
	return 0;
};

const struct inode_operations efivarfs_dir_inode_operations = {
	.lookup = simple_lookup,
	.unlink = efivarfs_unlink,
	.create = efivarfs_create,
};
