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
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2012, Intel Corporation.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 */

#define DEBUG_PORTAL_ALLOC

#ifndef __FIKUS_SOCKNAL_LIB_H__
#define __FIKUS_SOCKNAL_LIB_H__

#include <fikus/module.h>
#include <fikus/kernel.h>
#include <fikus/mm.h>
#include <fikus/string.h>
#include <fikus/stat.h>
#include <fikus/errno.h>
#include <fikus/unistd.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <fikus/uio.h>
#include <fikus/if.h>

#include <asm/uaccess.h>
#include <asm/irq.h>

#include <fikus/init.h>
#include <fikus/fs.h>
#include <fikus/file.h>
#include <fikus/list.h>
#include <fikus/kmod.h>
#include <fikus/sysctl.h>
#include <asm/div64.h>
#include <fikus/syscalls.h>

#include <fikus/libcfs/libcfs.h>
#include <fikus/libcfs/fikus/portals_compat25.h>

#include <fikus/crc32.h>
static inline __u32 ksocknal_csum(__u32 crc, unsigned char const *p, size_t len)
{
#if 1
	return crc32_le(crc, p, len);
#else
	while (len-- > 0)
		crc = ((crc + 0x100) & ~0xff) | ((crc + *p++) & 0xff) ;
	return crc;
#endif
}

#define SOCKNAL_WSPACE(sk)       sk_stream_wspace(sk)
#define SOCKNAL_MIN_WSPACE(sk)   sk_stream_min_wspace(sk)

/* assume one thread for each connection type */
#define SOCKNAL_NSCHEDS		3
#define SOCKNAL_NSCHEDS_HIGH	(SOCKNAL_NSCHEDS << 1)

#endif
