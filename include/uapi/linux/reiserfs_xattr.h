/*
  File: fikus/reiserfs_xattr.h
*/

#ifndef _FIKUS_REISERFS_XATTR_H
#define _FIKUS_REISERFS_XATTR_H

#include <fikus/types.h>

/* Magic value in header */
#define REISERFS_XATTR_MAGIC 0x52465841	/* "RFXA" */

struct reiserfs_xattr_header {
	__le32 h_magic;		/* magic number for identification */
	__le32 h_hash;		/* hash of the value */
};

struct reiserfs_security_handle {
	const char *name;
	void *value;
	size_t length;
};

#endif  /*  _FIKUS_REISERFS_XATTR_H  */
