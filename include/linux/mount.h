/*
 *
 * Definitions for mount interface. This describes the in the kernel build 
 * linkedlist with mounted filesystems.
 *
 * Author:  Marco van Wieringen <mvw@planets.elm.net>
 *
 */
#ifndef _FIKUS_MOUNT_H
#define _FIKUS_MOUNT_H

#include <fikus/types.h>
#include <fikus/list.h>
#include <fikus/nodemask.h>
#include <fikus/spinlock.h>
#include <fikus/seqlock.h>
#include <fikus/atomic.h>

struct super_block;
struct vfsmount;
struct dentry;
struct mnt_namespace;

#define MNT_NOSUID	0x01
#define MNT_NODEV	0x02
#define MNT_NOEXEC	0x04
#define MNT_NOATIME	0x08
#define MNT_NODIRATIME	0x10
#define MNT_RELATIME	0x20
#define MNT_READONLY	0x40	/* does the user want this to be r/o? */

#define MNT_SHRINKABLE	0x100
#define MNT_WRITE_HOLD	0x200

#define MNT_SHARED	0x1000	/* if the vfsmount is a shared mount */
#define MNT_UNBINDABLE	0x2000	/* if the vfsmount is a unbindable mount */
/*
 * MNT_SHARED_MASK is the set of flags that should be cleared when a
 * mount becomes shared.  Currently, this is only the flag that says a
 * mount cannot be bind mounted, since this is how we create a mount
 * that shares events with another mount.  If you add a new MNT_*
 * flag, consider how it interacts with shared mounts.
 */
#define MNT_SHARED_MASK	(MNT_UNBINDABLE)
#define MNT_PROPAGATION_MASK	(MNT_SHARED | MNT_UNBINDABLE)


#define MNT_INTERNAL	0x4000

#define MNT_LOCK_READONLY	0x400000
#define MNT_LOCKED		0x800000

struct vfsmount {
	struct dentry *mnt_root;	/* root of the mounted tree */
	struct super_block *mnt_sb;	/* pointer to superblock */
	int mnt_flags;
};

struct file; /* forward dec */

extern int mnt_want_write(struct vfsmount *mnt);
extern int mnt_want_write_file(struct file *file);
extern int mnt_clone_write(struct vfsmount *mnt);
extern void mnt_drop_write(struct vfsmount *mnt);
extern void mnt_drop_write_file(struct file *file);
extern void mntput(struct vfsmount *mnt);
extern struct vfsmount *mntget(struct vfsmount *mnt);
extern void mnt_pin(struct vfsmount *mnt);
extern void mnt_unpin(struct vfsmount *mnt);
extern int __mnt_is_readonly(struct vfsmount *mnt);

struct file_system_type;
extern struct vfsmount *vfs_kern_mount(struct file_system_type *type,
				      int flags, const char *name,
				      void *data);

extern void mnt_set_expiry(struct vfsmount *mnt, struct list_head *expiry_list);
extern void mark_mounts_for_expiry(struct list_head *mounts);

extern dev_t name_to_dev_t(char *name);

#endif /* _FIKUS_MOUNT_H */
