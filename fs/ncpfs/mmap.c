/*
 *  mmap.c
 *
 *  Copyright (C) 1995, 1996 by Volker Lendecke
 *  Modified 1997 Peter Waltenberg, Bill Hawes, David Woodhouse for 2.1 dcache
 *
 */

#include <fikus/stat.h>
#include <fikus/time.h>
#include <fikus/kernel.h>
#include <fikus/gfp.h>
#include <fikus/mm.h>
#include <fikus/shm.h>
#include <fikus/errno.h>
#include <fikus/mman.h>
#include <fikus/string.h>
#include <fikus/fcntl.h>
#include <fikus/memcontrol.h>

#include <asm/uaccess.h>

#include "ncp_fs.h"

/*
 * Fill in the supplied page for mmap
 * XXX: how are we excluding truncate/invalidate here? Maybe need to lock
 * page?
 */
static int ncp_file_mmap_fault(struct vm_area_struct *area,
					struct vm_fault *vmf)
{
	struct file *file = area->vm_file;
	struct dentry *dentry = file->f_path.dentry;
	struct inode *inode = dentry->d_inode;
	char *pg_addr;
	unsigned int already_read;
	unsigned int count;
	int bufsize;
	int pos; /* XXX: loff_t ? */

	/*
	 * ncpfs has nothing against high pages as long
	 * as recvmsg and memset works on it
	 */
	vmf->page = alloc_page(GFP_HIGHUSER);
	if (!vmf->page)
		return VM_FAULT_OOM;
	pg_addr = kmap(vmf->page);
	pos = vmf->pgoff << PAGE_SHIFT;

	count = PAGE_SIZE;
	/* what we can read in one go */
	bufsize = NCP_SERVER(inode)->buffer_size;

	already_read = 0;
	if (ncp_make_open(inode, O_RDONLY) >= 0) {
		while (already_read < count) {
			int read_this_time;
			int to_read;

			to_read = bufsize - (pos % bufsize);

			to_read = min_t(unsigned int, to_read, count - already_read);

			if (ncp_read_kernel(NCP_SERVER(inode),
				     NCP_FINFO(inode)->file_handle,
				     pos, to_read,
				     pg_addr + already_read,
				     &read_this_time) != 0) {
				read_this_time = 0;
			}
			pos += read_this_time;
			already_read += read_this_time;

			if (read_this_time < to_read) {
				break;
			}
		}
		ncp_inode_close(inode);

	}

	if (already_read < PAGE_SIZE)
		memset(pg_addr + already_read, 0, PAGE_SIZE - already_read);
	flush_dcache_page(vmf->page);
	kunmap(vmf->page);

	/*
	 * If I understand ncp_read_kernel() properly, the above always
	 * fetches from the network, here the analogue of disk.
	 * -- nyc
	 */
	count_vm_event(PGMAJFAULT);
	mem_cgroup_count_vm_event(area->vm_mm, PGMAJFAULT);
	return VM_FAULT_MAJOR;
}

static const struct vm_operations_struct ncp_file_mmap =
{
	.fault = ncp_file_mmap_fault,
};


/* This is used for a general mmap of a ncp file */
int ncp_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct inode *inode = file_inode(file);
	
	DPRINTK("ncp_mmap: called\n");

	if (!ncp_conn_valid(NCP_SERVER(inode)))
		return -EIO;

	/* only PAGE_COW or read-only supported now */
	if (vma->vm_flags & VM_SHARED)
		return -EINVAL;
	/* we do not support files bigger than 4GB... We eventually 
	   supports just 4GB... */
	if (vma_pages(vma) + vma->vm_pgoff
	   > (1U << (32 - PAGE_SHIFT)))
		return -EFBIG;

	vma->vm_ops = &ncp_file_mmap;
	file_accessed(file);
	return 0;
}
