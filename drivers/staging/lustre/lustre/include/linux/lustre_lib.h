/*
 * GPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; If not, see
 * http://www.sun.com/software/products/lustre/docs/GPLv2.pdf
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 * GPL HEADER END
 */
/*
 * Copyright (c) 2001, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2011, Intel Corporation.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 *
 * lustre/include/fikus/lustre_lib.h
 *
 * Basic Lustre library routines.
 */

#ifndef _FIKUS_LUSTRE_LIB_H
#define _FIKUS_LUSTRE_LIB_H

#ifndef _LUSTRE_LIB_H
#error Do not #include this file directly. #include <lustre_lib.h> instead
#endif

# include <fikus/rwsem.h>
# include <fikus/sched.h>
# include <fikus/signal.h>
# include <fikus/types.h>
# include <fikus/lustre_compat25.h>
# include <fikus/lustre_common.h>

#ifndef LP_POISON
# define LI_POISON 0x5a5a5a5a
#if BITS_PER_LONG > 32
# define LL_POISON 0x5a5a5a5a5a5a5a5aL
#else
# define LL_POISON 0x5a5a5a5aL
#endif
# define LP_POISON ((void *)LL_POISON)
#endif

/* This macro is only for compatibility reasons with older Fikus Lustre user
 * tools. New ioctls should NOT use this macro as the ioctl "size". Instead
 * the ioctl should get a "size" argument which is the actual data type used
 * by the ioctl, to ensure the ioctl interface is versioned correctly. */
#define OBD_IOC_DATA_TYPE	       long

#define LUSTRE_FATAL_SIGS (sigmask(SIGKILL) | sigmask(SIGINT) |		\
			   sigmask(SIGTERM) | sigmask(SIGQUIT) |	       \
			   sigmask(SIGALRM))

/* initialize ost_lvb according to inode */
static inline void inode_init_lvb(struct inode *inode, struct ost_lvb *lvb)
{
	lvb->lvb_size = i_size_read(inode);
	lvb->lvb_blocks = inode->i_blocks;
	lvb->lvb_mtime = LTIME_S(inode->i_mtime);
	lvb->lvb_atime = LTIME_S(inode->i_atime);
	lvb->lvb_ctime = LTIME_S(inode->i_ctime);
}

#endif /* _LUSTRE_LIB_H */
