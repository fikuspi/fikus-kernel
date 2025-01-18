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
 * Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2012, Intel Corporation.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 *
 * libcfs/include/libcfs/fikus/fikus-prim.h
 *
 * Basic library routines.
 */

#ifndef __LIBCFS_FIKUS_CFS_PRIM_H__
#define __LIBCFS_FIKUS_CFS_PRIM_H__

#ifndef __LIBCFS_LIBCFS_H__
#error Do not #include this file directly. #include <fikus/libcfs/libcfs.h> instead
#endif


#include <fikus/module.h>
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/proc_fs.h>
#include <fikus/mm.h>
#include <fikus/timer.h>
#include <fikus/signal.h>
#include <fikus/sched.h>
#include <fikus/kthread.h>
#include <fikus/random.h>

#include <fikus/miscdevice.h>
#include <fikus/libcfs/fikus/portals_compat25.h>
#include <asm/div64.h>

#include <fikus/libcfs/fikus/fikus-time.h>

/*
 * Sysctl register
 */
typedef struct ctl_table		ctl_table_t;
typedef struct ctl_table_header		ctl_table_header_t;

#define DECLARE_PROC_HANDLER(name)		      \
static int					      \
LL_PROC_PROTO(name)				     \
{						       \
	DECLARE_LL_PROC_PPOS_DECL;		      \
							\
	return proc_call_handler(table->data, write,    \
				 ppos, buffer, lenp,    \
				 __##name);	     \
}

#endif
