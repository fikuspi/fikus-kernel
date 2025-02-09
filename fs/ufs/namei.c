/*
 * fikus/fs/ufs/namei.c
 *
 * Migration to usage of "page cache" on May 2006 by
 * Evgeniy Dushistov <dushistov@mail.ru> based on ext2 code base.
 *
 * Copyright (C) 1998
 * Daniel Pirkl <daniel.pirkl@email.cz>
 * Charles University, Faculty of Mathematics and Physics
 *
 *  from
 *
 *  fikus/fs/ext2/namei.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  fikus/fs/minix/namei.c
 *
 *  Copyright (C) 1991, 1992  John Torvalds
 *
 *  Big-endian to little-endian byte-swapping/bitmaps by
 *        David S. Miller (davem@caip.rutgers.edu), 1995
 */

#include <fikus/time.h>
#include <fikus/fs.h>

#include "ufs_fs.h"
#include "ufs.h"
#include "util.h"

static inline int ufs_add_nondir(struct dentry *dentry, struct inode *inode)
{
	int err = ufs_add_link(dentry, inode);
	if (!err) {
		d_instantiate(dentry, inode);
		return 0;
	}
	inode_dec_link_count(inode);
	iput(inode);
	return err;
}

static struct dentry *ufs_lookup(struct inode * dir, struct dentry *dentry, unsigned int flags)
{
	struct inode * inode = NULL;
	ino_t ino;
	
	if (dentry->d_name.len > UFS_MAXNAMLEN)
		return ERR_PTR(-ENAMETOOLONG);

	lock_ufs(dir->i_sb);
	ino = ufs_inode_by_name(dir, &dentry->d_name);
	if (ino)
		inode = ufs_iget(dir->i_sb, ino);
	unlock_ufs(dir->i_sb);
	return d_splice_alias(inode, dentry);
}

/*
 * By the time this is called, we already have created
 * the directory cache entry for the new file, but it
 * is so far negative - it has no inode.
 *
 * If the create succeeds, we fill in the inode information
 * with d_instantiate(). 
 */
static int ufs_create (struct inode * dir, struct dentry * dentry, umode_t mode,
		bool excl)
{
	struct inode *inode;
	int err;

	UFSD("BEGIN\n");

	inode = ufs_new_inode(dir, mode);
	err = PTR_ERR(inode);

	if (!IS_ERR(inode)) {
		inode->i_op = &ufs_file_inode_operations;
		inode->i_fop = &ufs_file_operations;
		inode->i_mapping->a_ops = &ufs_aops;
		mark_inode_dirty(inode);
		lock_ufs(dir->i_sb);
		err = ufs_add_nondir(dentry, inode);
		unlock_ufs(dir->i_sb);
	}
	UFSD("END: err=%d\n", err);
	return err;
}

static int ufs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t rdev)
{
	struct inode *inode;
	int err;

	if (!old_valid_dev(rdev))
		return -EINVAL;

	inode = ufs_new_inode(dir, mode);
	err = PTR_ERR(inode);
	if (!IS_ERR(inode)) {
		init_special_inode(inode, mode, rdev);
		ufs_set_inode_dev(inode->i_sb, UFS_I(inode), rdev);
		mark_inode_dirty(inode);
		lock_ufs(dir->i_sb);
		err = ufs_add_nondir(dentry, inode);
		unlock_ufs(dir->i_sb);
	}
	return err;
}

static int ufs_symlink (struct inode * dir, struct dentry * dentry,
	const char * symname)
{
	struct super_block * sb = dir->i_sb;
	int err = -ENAMETOOLONG;
	unsigned l = strlen(symname)+1;
	struct inode * inode;

	if (l > sb->s_blocksize)
		goto out_notlocked;

	lock_ufs(dir->i_sb);
	inode = ufs_new_inode(dir, S_IFLNK | S_IRWXUGO);
	err = PTR_ERR(inode);
	if (IS_ERR(inode))
		goto out;

	if (l > UFS_SB(sb)->s_uspi->s_maxsymlinklen) {
		/* slow symlink */
		inode->i_op = &ufs_symlink_inode_operations;
		inode->i_mapping->a_ops = &ufs_aops;
		err = page_symlink(inode, symname, l);
		if (err)
			goto out_fail;
	} else {
		/* fast symlink */
		inode->i_op = &ufs_fast_symlink_inode_operations;
		memcpy(UFS_I(inode)->i_u1.i_symlink, symname, l);
		inode->i_size = l-1;
	}
	mark_inode_dirty(inode);

	err = ufs_add_nondir(dentry, inode);
out:
	unlock_ufs(dir->i_sb);
out_notlocked:
	return err;

out_fail:
	inode_dec_link_count(inode);
	iput(inode);
	goto out;
}

static int ufs_link (struct dentry * old_dentry, struct inode * dir,
	struct dentry *dentry)
{
	struct inode *inode = old_dentry->d_inode;
	int error;

	lock_ufs(dir->i_sb);

	inode->i_ctime = CURRENT_TIME_SEC;
	inode_inc_link_count(inode);
	ihold(inode);

	error = ufs_add_nondir(dentry, inode);
	unlock_ufs(dir->i_sb);
	return error;
}

static int ufs_mkdir(struct inode * dir, struct dentry * dentry, umode_t mode)
{
	struct inode * inode;
	int err;

	lock_ufs(dir->i_sb);
	inode_inc_link_count(dir);

	inode = ufs_new_inode(dir, S_IFDIR|mode);
	err = PTR_ERR(inode);
	if (IS_ERR(inode))
		goto out_dir;

	inode->i_op = &ufs_dir_inode_operations;
	inode->i_fop = &ufs_dir_operations;
	inode->i_mapping->a_ops = &ufs_aops;

	inode_inc_link_count(inode);

	err = ufs_make_empty(inode, dir);
	if (err)
		goto out_fail;

	err = ufs_add_link(dentry, inode);
	if (err)
		goto out_fail;
	unlock_ufs(dir->i_sb);

	d_instantiate(dentry, inode);
out:
	return err;

out_fail:
	inode_dec_link_count(inode);
	inode_dec_link_count(inode);
	iput (inode);
out_dir:
	inode_dec_link_count(dir);
	unlock_ufs(dir->i_sb);
	goto out;
}

static int ufs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct inode * inode = dentry->d_inode;
	struct ufs_dir_entry *de;
	struct page *page;
	int err = -ENOENT;

	de = ufs_find_entry(dir, &dentry->d_name, &page);
	if (!de)
		goto out;

	err = ufs_delete_entry(dir, de, page);
	if (err)
		goto out;

	inode->i_ctime = dir->i_ctime;
	inode_dec_link_count(inode);
	err = 0;
out:
	return err;
}

static int ufs_rmdir (struct inode * dir, struct dentry *dentry)
{
	struct inode * inode = dentry->d_inode;
	int err= -ENOTEMPTY;

	lock_ufs(dir->i_sb);
	if (ufs_empty_dir (inode)) {
		err = ufs_unlink(dir, dentry);
		if (!err) {
			inode->i_size = 0;
			inode_dec_link_count(inode);
			inode_dec_link_count(dir);
		}
	}
	unlock_ufs(dir->i_sb);
	return err;
}

static int ufs_rename(struct inode *old_dir, struct dentry *old_dentry,
		      struct inode *new_dir, struct dentry *new_dentry)
{
	struct inode *old_inode = old_dentry->d_inode;
	struct inode *new_inode = new_dentry->d_inode;
	struct page *dir_page = NULL;
	struct ufs_dir_entry * dir_de = NULL;
	struct page *old_page;
	struct ufs_dir_entry *old_de;
	int err = -ENOENT;

	old_de = ufs_find_entry(old_dir, &old_dentry->d_name, &old_page);
	if (!old_de)
		goto out;

	if (S_ISDIR(old_inode->i_mode)) {
		err = -EIO;
		dir_de = ufs_dotdot(old_inode, &dir_page);
		if (!dir_de)
			goto out_old;
	}

	if (new_inode) {
		struct page *new_page;
		struct ufs_dir_entry *new_de;

		err = -ENOTEMPTY;
		if (dir_de && !ufs_empty_dir(new_inode))
			goto out_dir;

		err = -ENOENT;
		new_de = ufs_find_entry(new_dir, &new_dentry->d_name, &new_page);
		if (!new_de)
			goto out_dir;
		ufs_set_link(new_dir, new_de, new_page, old_inode);
		new_inode->i_ctime = CURRENT_TIME_SEC;
		if (dir_de)
			drop_nlink(new_inode);
		inode_dec_link_count(new_inode);
	} else {
		err = ufs_add_link(new_dentry, old_inode);
		if (err)
			goto out_dir;
		if (dir_de)
			inode_inc_link_count(new_dir);
	}

	/*
	 * Like most other Unix systems, set the ctime for inodes on a
 	 * rename.
	 */
	old_inode->i_ctime = CURRENT_TIME_SEC;

	ufs_delete_entry(old_dir, old_de, old_page);
	mark_inode_dirty(old_inode);

	if (dir_de) {
		ufs_set_link(old_inode, dir_de, dir_page, new_dir);
		inode_dec_link_count(old_dir);
	}
	return 0;


out_dir:
	if (dir_de) {
		kunmap(dir_page);
		page_cache_release(dir_page);
	}
out_old:
	kunmap(old_page);
	page_cache_release(old_page);
out:
	return err;
}

const struct inode_operations ufs_dir_inode_operations = {
	.create		= ufs_create,
	.lookup		= ufs_lookup,
	.link		= ufs_link,
	.unlink		= ufs_unlink,
	.symlink	= ufs_symlink,
	.mkdir		= ufs_mkdir,
	.rmdir		= ufs_rmdir,
	.mknod		= ufs_mknod,
	.rename		= ufs_rename,
};
