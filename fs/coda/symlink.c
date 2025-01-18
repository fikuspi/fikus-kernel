/*
 * Symlink inode operations for Coda filesystem
 * Original version: (C) 1996 P. Braam and M. Callahan
 * Rewritten for Fikus 2.1. (C) 1997 Carnegie Mellon University
 * 
 * Carnegie Mellon encourages users to contribute improvements to
 * the Coda project. Contact Peter Braam (coda@cs.cmu.edu).
 */

#include <fikus/types.h>
#include <fikus/kernel.h>
#include <fikus/time.h>
#include <fikus/fs.h>
#include <fikus/stat.h>
#include <fikus/errno.h>
#include <fikus/pagemap.h>

#include <fikus/coda.h>
#include <fikus/coda_psdev.h>

#include "coda_fikus.h"

static int coda_symlink_filler(struct file *file, struct page *page)
{
	struct inode *inode = page->mapping->host;
	int error;
	struct coda_inode_info *cii;
	unsigned int len = PAGE_SIZE;
	char *p = kmap(page);

	cii = ITOC(inode);

	error = venus_readlink(inode->i_sb, &cii->c_fid, p, &len);
	if (error)
		goto fail;
	SetPageUptodate(page);
	kunmap(page);
	unlock_page(page);
	return 0;

fail:
	SetPageError(page);
	kunmap(page);
	unlock_page(page);
	return error;
}

const struct address_space_operations coda_symlink_aops = {
	.readpage	= coda_symlink_filler,
};
