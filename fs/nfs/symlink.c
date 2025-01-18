/*
 *  fikus/fs/nfs/symlink.c
 *
 *  Copyright (C) 1992  Rick Sladkey
 *
 *  Optimization changes Copyright (C) 1994 Florian La Roche
 *
 *  Jun 7 1999, cache symlink lookups in the page cache.  -DaveM
 *
 *  nfs symlink handling code
 */

#include <fikus/time.h>
#include <fikus/errno.h>
#include <fikus/sunrpc/clnt.h>
#include <fikus/nfs.h>
#include <fikus/nfs2.h>
#include <fikus/nfs_fs.h>
#include <fikus/pagemap.h>
#include <fikus/stat.h>
#include <fikus/mm.h>
#include <fikus/string.h>
#include <fikus/namei.h>

/* Symlink caching in the page cache is even more simplistic
 * and straight-forward than readdir caching.
 */

static int nfs_symlink_filler(struct inode *inode, struct page *page)
{
	int error;

	error = NFS_PROTO(inode)->readlink(inode, page, 0, PAGE_SIZE);
	if (error < 0)
		goto error;
	SetPageUptodate(page);
	unlock_page(page);
	return 0;

error:
	SetPageError(page);
	unlock_page(page);
	return -EIO;
}

static void *nfs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	struct inode *inode = dentry->d_inode;
	struct page *page;
	void *err;

	err = ERR_PTR(nfs_revalidate_mapping(inode, inode->i_mapping));
	if (err)
		goto read_failed;
	page = read_cache_page(&inode->i_data, 0,
				(filler_t *)nfs_symlink_filler, inode);
	if (IS_ERR(page)) {
		err = page;
		goto read_failed;
	}
	nd_set_link(nd, kmap(page));
	return page;

read_failed:
	nd_set_link(nd, err);
	return NULL;
}

/*
 * symlinks can't do much...
 */
const struct inode_operations nfs_symlink_inode_operations = {
	.readlink	= generic_readlink,
	.follow_link	= nfs_follow_link,
	.put_link	= page_put_link,
	.getattr	= nfs_getattr,
	.setattr	= nfs_setattr,
};
