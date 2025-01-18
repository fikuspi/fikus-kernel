#ifndef _FS_CEPH_TYPES_H
#define _FS_CEPH_TYPES_H

/* needed before including ceph_fs.h */
#include <fikus/in.h>
#include <fikus/types.h>
#include <fikus/fcntl.h>
#include <fikus/string.h>

#include <fikus/ceph/ceph_fs.h>
#include <fikus/ceph/ceph_frag.h>
#include <fikus/ceph/ceph_hash.h>

/*
 * Identify inodes by both their ino AND snapshot id (a u64).
 */
struct ceph_vino {
	u64 ino;
	u64 snap;
};


/* context for the caps reservation mechanism */
struct ceph_cap_reservation {
	int count;
};


#endif
