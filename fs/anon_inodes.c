/*
 *  fs/anon_inodes.c
 *
 *  Copyright (C) 2007  Davide Libenzi <davidel@xmailserver.org>
 *
 *  Thanks to Arnd Bergmann for code review and suggestions.
 *  More changes for Thomas Gleixner suggestions.
 *
 */

#include <fikus/cred.h>
#include <fikus/file.h>
#include <fikus/poll.h>
#include <fikus/sched.h>
#include <fikus/init.h>
#include <fikus/fs.h>
#include <fikus/mount.h>
#include <fikus/module.h>
#include <fikus/kernel.h>
#include <fikus/magic.h>
#include <fikus/anon_inodes.h>

#include <asm/uaccess.h>

static struct vfsmount *anon_inode_mnt __read_mostly;
static struct inode *anon_inode_inode;

/*
 * anon_inodefs_dname() is called from d_path().
 */
static char *anon_inodefs_dname(struct dentry *dentry, char *buffer, int buflen)
{
	return dynamic_dname(dentry, buffer, buflen, "anon_inode:%s",
				dentry->d_name.name);
}

static const struct dentry_operations anon_inodefs_dentry_operations = {
	.d_dname	= anon_inodefs_dname,
};

static struct dentry *anon_inodefs_mount(struct file_system_type *fs_type,
				int flags, const char *dev_name, void *data)
{
	struct dentry *root;
	root = mount_pseudo(fs_type, "anon_inode:", NULL,
			&anon_inodefs_dentry_operations, ANON_INODE_FS_MAGIC);
	if (!IS_ERR(root)) {
		struct super_block *s = root->d_sb;
		anon_inode_inode = alloc_anon_inode(s);
		if (IS_ERR(anon_inode_inode)) {
			dput(root);
			deactivate_locked_super(s);
			root = ERR_CAST(anon_inode_inode);
		}
	}
	return root;
}

static struct file_system_type anon_inode_fs_type = {
	.name		= "anon_inodefs",
	.mount		= anon_inodefs_mount,
	.kill_sb	= kill_anon_super,
};

/**
 * anon_inode_getfile_private - creates a new file instance by hooking it up to an
 *                      anonymous inode, and a dentry that describe the "class"
 *                      of the file
 *
 * @name:    [in]    name of the "class" of the new file
 * @fops:    [in]    file operations for the new file
 * @priv:    [in]    private data for the new file (will be file's private_data)
 * @flags:   [in]    flags
 *
 *
 * Similar to anon_inode_getfile, but each file holds a single inode.
 *
 */
struct file *anon_inode_getfile_private(const char *name,
					const struct file_operations *fops,
					void *priv, int flags)
{
	struct qstr this;
	struct path path;
	struct file *file;
	struct inode *inode;

	if (fops->owner && !try_module_get(fops->owner))
		return ERR_PTR(-ENOENT);

	inode = alloc_anon_inode(anon_inode_mnt->mnt_sb);
	if (IS_ERR(inode)) {
		file = ERR_PTR(-ENOMEM);
		goto err_module;
	}

	/*
	 * Link the inode to a directory entry by creating a unique name
	 * using the inode sequence number.
	 */
	file = ERR_PTR(-ENOMEM);
	this.name = name;
	this.len = strlen(name);
	this.hash = 0;
	path.dentry = d_alloc_pseudo(anon_inode_mnt->mnt_sb, &this);
	if (!path.dentry)
		goto err_module;

	path.mnt = mntget(anon_inode_mnt);

	d_instantiate(path.dentry, inode);

	file = alloc_file(&path, OPEN_FMODE(flags), fops);
	if (IS_ERR(file))
		goto err_dput;

	file->f_mapping = inode->i_mapping;
	file->f_flags = flags & (O_ACCMODE | O_NONBLOCK);
	file->private_data = priv;

	return file;

err_dput:
	path_put(&path);
err_module:
	module_put(fops->owner);
	return file;
}
EXPORT_SYMBOL_GPL(anon_inode_getfile_private);

/**
 * anon_inode_getfile - creates a new file instance by hooking it up to an
 *                      anonymous inode, and a dentry that describe the "class"
 *                      of the file
 *
 * @name:    [in]    name of the "class" of the new file
 * @fops:    [in]    file operations for the new file
 * @priv:    [in]    private data for the new file (will be file's private_data)
 * @flags:   [in]    flags
 *
 * Creates a new file by hooking it on a single inode. This is useful for files
 * that do not need to have a full-fledged inode in order to operate correctly.
 * All the files created with anon_inode_getfile() will share a single inode,
 * hence saving memory and avoiding code duplication for the file/inode/dentry
 * setup.  Returns the newly created file* or an error pointer.
 */
struct file *anon_inode_getfile(const char *name,
				const struct file_operations *fops,
				void *priv, int flags)
{
	struct qstr this;
	struct path path;
	struct file *file;

	if (IS_ERR(anon_inode_inode))
		return ERR_PTR(-ENODEV);

	if (fops->owner && !try_module_get(fops->owner))
		return ERR_PTR(-ENOENT);

	/*
	 * Link the inode to a directory entry by creating a unique name
	 * using the inode sequence number.
	 */
	file = ERR_PTR(-ENOMEM);
	this.name = name;
	this.len = strlen(name);
	this.hash = 0;
	path.dentry = d_alloc_pseudo(anon_inode_mnt->mnt_sb, &this);
	if (!path.dentry)
		goto err_module;

	path.mnt = mntget(anon_inode_mnt);
	/*
	 * We know the anon_inode inode count is always greater than zero,
	 * so ihold() is safe.
	 */
	ihold(anon_inode_inode);

	d_instantiate(path.dentry, anon_inode_inode);

	file = alloc_file(&path, OPEN_FMODE(flags), fops);
	if (IS_ERR(file))
		goto err_dput;
	file->f_mapping = anon_inode_inode->i_mapping;

	file->f_flags = flags & (O_ACCMODE | O_NONBLOCK);
	file->private_data = priv;

	return file;

err_dput:
	path_put(&path);
err_module:
	module_put(fops->owner);
	return file;
}
EXPORT_SYMBOL_GPL(anon_inode_getfile);

/**
 * anon_inode_getfd - creates a new file instance by hooking it up to an
 *                    anonymous inode, and a dentry that describe the "class"
 *                    of the file
 *
 * @name:    [in]    name of the "class" of the new file
 * @fops:    [in]    file operations for the new file
 * @priv:    [in]    private data for the new file (will be file's private_data)
 * @flags:   [in]    flags
 *
 * Creates a new file by hooking it on a single inode. This is useful for files
 * that do not need to have a full-fledged inode in order to operate correctly.
 * All the files created with anon_inode_getfd() will share a single inode,
 * hence saving memory and avoiding code duplication for the file/inode/dentry
 * setup.  Returns new descriptor or an error code.
 */
int anon_inode_getfd(const char *name, const struct file_operations *fops,
		     void *priv, int flags)
{
	int error, fd;
	struct file *file;

	error = get_unused_fd_flags(flags);
	if (error < 0)
		return error;
	fd = error;

	file = anon_inode_getfile(name, fops, priv, flags);
	if (IS_ERR(file)) {
		error = PTR_ERR(file);
		goto err_put_unused_fd;
	}
	fd_install(fd, file);

	return fd;

err_put_unused_fd:
	put_unused_fd(fd);
	return error;
}
EXPORT_SYMBOL_GPL(anon_inode_getfd);

static int __init anon_inode_init(void)
{
	int error;

	error = register_filesystem(&anon_inode_fs_type);
	if (error)
		goto err_exit;
	anon_inode_mnt = kern_mount(&anon_inode_fs_type);
	if (IS_ERR(anon_inode_mnt)) {
		error = PTR_ERR(anon_inode_mnt);
		goto err_unregister_filesystem;
	}
	return 0;

err_unregister_filesystem:
	unregister_filesystem(&anon_inode_fs_type);
err_exit:
	panic(KERN_ERR "anon_inode_init() failed (%d)\n", error);
}

fs_initcall(anon_inode_init);

