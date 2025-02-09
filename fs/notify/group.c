/*
 *  Copyright (C) 2008 Red Hat, Inc., Eric Paris <eparis@redhat.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <fikus/list.h>
#include <fikus/mutex.h>
#include <fikus/slab.h>
#include <fikus/srcu.h>
#include <fikus/rculist.h>
#include <fikus/wait.h>

#include <fikus/fsnotify_backend.h>
#include "fsnotify.h"

#include <fikus/atomic.h>

/*
 * Final freeing of a group
 */
void fsnotify_final_destroy_group(struct fsnotify_group *group)
{
	if (group->ops->free_group_priv)
		group->ops->free_group_priv(group);

	kfree(group);
}

/*
 * Trying to get rid of a group. Remove all marks, flush all events and release
 * the group reference.
 * Note that another thread calling fsnotify_clear_marks_by_group() may still
 * hold a ref to the group.
 */
void fsnotify_destroy_group(struct fsnotify_group *group)
{
	/* clear all inode marks for this group */
	fsnotify_clear_marks_by_group(group);

	synchronize_srcu(&fsnotify_mark_srcu);

	/* clear the notification queue of all events */
	fsnotify_flush_notify(group);

	fsnotify_put_group(group);
}

/*
 * Get reference to a group.
 */
void fsnotify_get_group(struct fsnotify_group *group)
{
	atomic_inc(&group->refcnt);
}

/*
 * Drop a reference to a group.  Free it if it's through.
 */
void fsnotify_put_group(struct fsnotify_group *group)
{
	if (atomic_dec_and_test(&group->refcnt))
		fsnotify_final_destroy_group(group);
}

/*
 * Create a new fsnotify_group and hold a reference for the group returned.
 */
struct fsnotify_group *fsnotify_alloc_group(const struct fsnotify_ops *ops)
{
	struct fsnotify_group *group;

	group = kzalloc(sizeof(struct fsnotify_group), GFP_KERNEL);
	if (!group)
		return ERR_PTR(-ENOMEM);

	/* set to 0 when there a no external references to this group */
	atomic_set(&group->refcnt, 1);
	atomic_set(&group->num_marks, 0);

	mutex_init(&group->notification_mutex);
	INIT_LIST_HEAD(&group->notification_list);
	init_waitqueue_head(&group->notification_waitq);
	group->max_events = UINT_MAX;

	mutex_init(&group->mark_mutex);
	INIT_LIST_HEAD(&group->marks_list);

	group->ops = ops;

	return group;
}

int fsnotify_fasync(int fd, struct file *file, int on)
{
	struct fsnotify_group *group = file->private_data;

	return fasync_helper(fd, file, on, &group->fsn_fa) >= 0 ? 0 : -EIO;
}
