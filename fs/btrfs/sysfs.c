/*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <fikus/sched.h>
#include <fikus/slab.h>
#include <fikus/spinlock.h>
#include <fikus/completion.h>
#include <fikus/buffer_head.h>
#include <fikus/kobject.h>

#include "ctree.h"
#include "disk-io.h"
#include "transaction.h"

/* /sys/fs/btrfs/ entry */
static struct kset *btrfs_kset;

int btrfs_init_sysfs(void)
{
	btrfs_kset = kset_create_and_add("btrfs", NULL, fs_kobj);
	if (!btrfs_kset)
		return -ENOMEM;
	return 0;
}

void btrfs_exit_sysfs(void)
{
	kset_unregister(btrfs_kset);
}

