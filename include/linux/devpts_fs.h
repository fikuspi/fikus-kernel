/* -*- fikus-c -*- --------------------------------------------------------- *
 *
 * fikus/include/fikus/devpts_fs.h
 *
 *  Copyright 1998-2004 H. Peter Anvin -- All Rights Reserved
 *
 * This file is part of the Fikus kernel and is made available under
 * the terms of the GNU General Public License, version 2, or at your
 * option, any later version, incorporated herein by reference.
 *
 * ------------------------------------------------------------------------- */

#ifndef _FIKUS_DEVPTS_FS_H
#define _FIKUS_DEVPTS_FS_H

#include <fikus/errno.h>

#ifdef CONFIG_UNIX98_PTYS

int devpts_new_index(struct inode *ptmx_inode);
void devpts_kill_index(struct inode *ptmx_inode, int idx);
/* mknod in devpts */
struct inode *devpts_pty_new(struct inode *ptmx_inode, dev_t device, int index,
		void *priv);
/* get private structure */
void *devpts_get_priv(struct inode *pts_inode);
/* unlink */
void devpts_pty_kill(struct inode *inode);

#else

/* Dummy stubs in the no-pty case */
static inline int devpts_new_index(struct inode *ptmx_inode) { return -EINVAL; }
static inline void devpts_kill_index(struct inode *ptmx_inode, int idx) { }
static inline struct inode *devpts_pty_new(struct inode *ptmx_inode,
		dev_t device, int index, void *priv)
{
	return ERR_PTR(-EINVAL);
}
static inline void *devpts_get_priv(struct inode *pts_inode)
{
	return NULL;
}
static inline void devpts_pty_kill(struct inode *inode) { }

#endif


#endif /* _FIKUS_DEVPTS_FS_H */
