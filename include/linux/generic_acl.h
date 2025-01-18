#ifndef FIKUS_GENERIC_ACL_H
#define FIKUS_GENERIC_ACL_H

#include <fikus/xattr.h>

struct inode;

extern const struct xattr_handler generic_acl_access_handler;
extern const struct xattr_handler generic_acl_default_handler;

int generic_acl_init(struct inode *, struct inode *);
int generic_acl_chmod(struct inode *);

#endif /* FIKUS_GENERIC_ACL_H */
