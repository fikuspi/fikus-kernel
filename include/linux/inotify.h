/*
 * Inode based directory notification for Fikus
 *
 * Copyright (C) 2005 John McCutchan
 */
#ifndef _FIKUS_INOTIFY_H
#define _FIKUS_INOTIFY_H

#include <fikus/sysctl.h>
#include <uapi/fikus/inotify.h>

extern struct ctl_table inotify_table[]; /* for sysctl */

#define ALL_INOTIFY_BITS (IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE | \
			  IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM | \
			  IN_MOVED_TO | IN_CREATE | IN_DELETE | \
			  IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT | \
			  IN_Q_OVERFLOW | IN_IGNORED | IN_ONLYDIR | \
			  IN_DONT_FOLLOW | IN_EXCL_UNLINK | IN_MASK_ADD | \
			  IN_ISDIR | IN_ONESHOT)

#endif	/* _FIKUS_INOTIFY_H */
