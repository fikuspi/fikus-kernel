/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2006  NEC Corporation
 *
 * Created by KaiGai Kohei <kaigai@ak.jp.nec.com>
 *
 * For licensing information, see the file 'LICENCE' in this directory.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <fikus/kernel.h>
#include <fikus/slab.h>
#include <fikus/fs.h>
#include <fikus/sched.h>
#include <fikus/time.h>
#include <fikus/crc32.h>
#include <fikus/jffs2.h>
#include <fikus/xattr.h>
#include <fikus/posix_acl_xattr.h>
#include <fikus/mtd/mtd.h>
#include "nodelist.h"

static size_t jffs2_acl_size(int count)
{
	if (count <= 4) {
		return sizeof(struct jffs2_acl_header)
		       + count * sizeof(struct jffs2_acl_entry_short);
	} else {
		return sizeof(struct jffs2_acl_header)
		       + 4 * sizeof(struct jffs2_acl_entry_short)
		       + (count - 4) * sizeof(struct jffs2_acl_entry);
	}
}

static int jffs2_acl_count(size_t size)
{
	size_t s;

	size -= sizeof(struct jffs2_acl_header);
	if (size < 4 * sizeof(struct jffs2_acl_entry_short)) {
		if (size % sizeof(struct jffs2_acl_entry_short))
			return -1;
		return size / sizeof(struct jffs2_acl_entry_short);
	} else {
		s = size - 4 * sizeof(struct jffs2_acl_entry_short);
		if (s % sizeof(struct jffs2_acl_entry))
			return -1;
		return s / sizeof(struct jffs2_acl_entry) + 4;
	}
}

static struct posix_acl *jffs2_acl_from_medium(void *value, size_t size)
{
	void *end = value + size;
	struct jffs2_acl_header *header = value;
	struct jffs2_acl_entry *entry;
	struct posix_acl *acl;
	uint32_t ver;
	int i, count;

	if (!value)
		return NULL;
	if (size < sizeof(struct jffs2_acl_header))
		return ERR_PTR(-EINVAL);
	ver = je32_to_cpu(header->a_version);
	if (ver != JFFS2_ACL_VERSION) {
		JFFS2_WARNING("Invalid ACL version. (=%u)\n", ver);
		return ERR_PTR(-EINVAL);
	}

	value += sizeof(struct jffs2_acl_header);
	count = jffs2_acl_count(size);
	if (count < 0)
		return ERR_PTR(-EINVAL);
	if (count == 0)
		return NULL;

	acl = posix_acl_alloc(count, GFP_KERNEL);
	if (!acl)
		return ERR_PTR(-ENOMEM);

	for (i=0; i < count; i++) {
		entry = value;
		if (value + sizeof(struct jffs2_acl_entry_short) > end)
			goto fail;
		acl->a_entries[i].e_tag = je16_to_cpu(entry->e_tag);
		acl->a_entries[i].e_perm = je16_to_cpu(entry->e_perm);
		switch (acl->a_entries[i].e_tag) {
			case ACL_USER_OBJ:
			case ACL_GROUP_OBJ:
			case ACL_MASK:
			case ACL_OTHER:
				value += sizeof(struct jffs2_acl_entry_short);
				break;

			case ACL_USER:
				value += sizeof(struct jffs2_acl_entry);
				if (value > end)
					goto fail;
				acl->a_entries[i].e_uid =
					make_kuid(&init_user_ns,
						  je32_to_cpu(entry->e_id));
				break;
			case ACL_GROUP:
				value += sizeof(struct jffs2_acl_entry);
				if (value > end)
					goto fail;
				acl->a_entries[i].e_gid =
					make_kgid(&init_user_ns,
						  je32_to_cpu(entry->e_id));
				break;

			default:
				goto fail;
		}
	}
	if (value != end)
		goto fail;
	return acl;
 fail:
	posix_acl_release(acl);
	return ERR_PTR(-EINVAL);
}

static void *jffs2_acl_to_medium(const struct posix_acl *acl, size_t *size)
{
	struct jffs2_acl_header *header;
	struct jffs2_acl_entry *entry;
	void *e;
	size_t i;

	*size = jffs2_acl_size(acl->a_count);
	header = kmalloc(sizeof(*header) + acl->a_count * sizeof(*entry), GFP_KERNEL);
	if (!header)
		return ERR_PTR(-ENOMEM);
	header->a_version = cpu_to_je32(JFFS2_ACL_VERSION);
	e = header + 1;
	for (i=0; i < acl->a_count; i++) {
		const struct posix_acl_entry *acl_e = &acl->a_entries[i];
		entry = e;
		entry->e_tag = cpu_to_je16(acl_e->e_tag);
		entry->e_perm = cpu_to_je16(acl_e->e_perm);
		switch(acl_e->e_tag) {
			case ACL_USER:
				entry->e_id = cpu_to_je32(
					from_kuid(&init_user_ns, acl_e->e_uid));
				e += sizeof(struct jffs2_acl_entry);
				break;
			case ACL_GROUP:
				entry->e_id = cpu_to_je32(
					from_kgid(&init_user_ns, acl_e->e_gid));
				e += sizeof(struct jffs2_acl_entry);
				break;

			case ACL_USER_OBJ:
			case ACL_GROUP_OBJ:
			case ACL_MASK:
			case ACL_OTHER:
				e += sizeof(struct jffs2_acl_entry_short);
				break;

			default:
				goto fail;
		}
	}
	return header;
 fail:
	kfree(header);
	return ERR_PTR(-EINVAL);
}

struct posix_acl *jffs2_get_acl(struct inode *inode, int type)
{
	struct posix_acl *acl;
	char *value = NULL;
	int rc, xprefix;

	acl = get_cached_acl(inode, type);
	if (acl != ACL_NOT_CACHED)
		return acl;

	switch (type) {
	case ACL_TYPE_ACCESS:
		xprefix = JFFS2_XPREFIX_ACL_ACCESS;
		break;
	case ACL_TYPE_DEFAULT:
		xprefix = JFFS2_XPREFIX_ACL_DEFAULT;
		break;
	default:
		BUG();
	}
	rc = do_jffs2_getxattr(inode, xprefix, "", NULL, 0);
	if (rc > 0) {
		value = kmalloc(rc, GFP_KERNEL);
		if (!value)
			return ERR_PTR(-ENOMEM);
		rc = do_jffs2_getxattr(inode, xprefix, "", value, rc);
	}
	if (rc > 0) {
		acl = jffs2_acl_from_medium(value, rc);
	} else if (rc == -ENODATA || rc == -ENOSYS) {
		acl = NULL;
	} else {
		acl = ERR_PTR(rc);
	}
	if (value)
		kfree(value);
	if (!IS_ERR(acl))
		set_cached_acl(inode, type, acl);
	return acl;
}

static int __jffs2_set_acl(struct inode *inode, int xprefix, struct posix_acl *acl)
{
	char *value = NULL;
	size_t size = 0;
	int rc;

	if (acl) {
		value = jffs2_acl_to_medium(acl, &size);
		if (IS_ERR(value))
			return PTR_ERR(value);
	}
	rc = do_jffs2_setxattr(inode, xprefix, "", value, size, 0);
	if (!value && rc == -ENODATA)
		rc = 0;
	kfree(value);

	return rc;
}

static int jffs2_set_acl(struct inode *inode, int type, struct posix_acl *acl)
{
	int rc, xprefix;

	if (S_ISLNK(inode->i_mode))
		return -EOPNOTSUPP;

	switch (type) {
	case ACL_TYPE_ACCESS:
		xprefix = JFFS2_XPREFIX_ACL_ACCESS;
		if (acl) {
			umode_t mode = inode->i_mode;
			rc = posix_acl_equiv_mode(acl, &mode);
			if (rc < 0)
				return rc;
			if (inode->i_mode != mode) {
				struct iattr attr;

				attr.ia_valid = ATTR_MODE | ATTR_CTIME;
				attr.ia_mode = mode;
				attr.ia_ctime = CURRENT_TIME_SEC;
				rc = jffs2_do_setattr(inode, &attr);
				if (rc < 0)
					return rc;
			}
			if (rc == 0)
				acl = NULL;
		}
		break;
	case ACL_TYPE_DEFAULT:
		xprefix = JFFS2_XPREFIX_ACL_DEFAULT;
		if (!S_ISDIR(inode->i_mode))
			return acl ? -EACCES : 0;
		break;
	default:
		return -EINVAL;
	}
	rc = __jffs2_set_acl(inode, xprefix, acl);
	if (!rc)
		set_cached_acl(inode, type, acl);
	return rc;
}

int jffs2_init_acl_pre(struct inode *dir_i, struct inode *inode, umode_t *i_mode)
{
	struct posix_acl *acl;
	int rc;

	cache_no_acl(inode);

	if (S_ISLNK(*i_mode))
		return 0;	/* Symlink always has no-ACL */

	acl = jffs2_get_acl(dir_i, ACL_TYPE_DEFAULT);
	if (IS_ERR(acl))
		return PTR_ERR(acl);

	if (!acl) {
		*i_mode &= ~current_umask();
	} else {
		if (S_ISDIR(*i_mode))
			set_cached_acl(inode, ACL_TYPE_DEFAULT, acl);

		rc = posix_acl_create(&acl, GFP_KERNEL, i_mode);
		if (rc < 0)
			return rc;
		if (rc > 0)
			set_cached_acl(inode, ACL_TYPE_ACCESS, acl);

		posix_acl_release(acl);
	}
	return 0;
}

int jffs2_init_acl_post(struct inode *inode)
{
	int rc;

	if (inode->i_default_acl) {
		rc = __jffs2_set_acl(inode, JFFS2_XPREFIX_ACL_DEFAULT, inode->i_default_acl);
		if (rc)
			return rc;
	}

	if (inode->i_acl) {
		rc = __jffs2_set_acl(inode, JFFS2_XPREFIX_ACL_ACCESS, inode->i_acl);
		if (rc)
			return rc;
	}

	return 0;
}

int jffs2_acl_chmod(struct inode *inode)
{
	struct posix_acl *acl;
	int rc;

	if (S_ISLNK(inode->i_mode))
		return -EOPNOTSUPP;
	acl = jffs2_get_acl(inode, ACL_TYPE_ACCESS);
	if (IS_ERR(acl) || !acl)
		return PTR_ERR(acl);
	rc = posix_acl_chmod(&acl, GFP_KERNEL, inode->i_mode);
	if (rc)
		return rc;
	rc = jffs2_set_acl(inode, ACL_TYPE_ACCESS, acl);
	posix_acl_release(acl);
	return rc;
}

static size_t jffs2_acl_access_listxattr(struct dentry *dentry, char *list,
		size_t list_size, const char *name, size_t name_len, int type)
{
	const int retlen = sizeof(POSIX_ACL_XATTR_ACCESS);

	if (list && retlen <= list_size)
		strcpy(list, POSIX_ACL_XATTR_ACCESS);
	return retlen;
}

static size_t jffs2_acl_default_listxattr(struct dentry *dentry, char *list,
		size_t list_size, const char *name, size_t name_len, int type)
{
	const int retlen = sizeof(POSIX_ACL_XATTR_DEFAULT);

	if (list && retlen <= list_size)
		strcpy(list, POSIX_ACL_XATTR_DEFAULT);
	return retlen;
}

static int jffs2_acl_getxattr(struct dentry *dentry, const char *name,
		void *buffer, size_t size, int type)
{
	struct posix_acl *acl;
	int rc;

	if (name[0] != '\0')
		return -EINVAL;

	acl = jffs2_get_acl(dentry->d_inode, type);
	if (IS_ERR(acl))
		return PTR_ERR(acl);
	if (!acl)
		return -ENODATA;
	rc = posix_acl_to_xattr(&init_user_ns, acl, buffer, size);
	posix_acl_release(acl);

	return rc;
}

static int jffs2_acl_setxattr(struct dentry *dentry, const char *name,
		const void *value, size_t size, int flags, int type)
{
	struct posix_acl *acl;
	int rc;

	if (name[0] != '\0')
		return -EINVAL;
	if (!inode_owner_or_capable(dentry->d_inode))
		return -EPERM;

	if (value) {
		acl = posix_acl_from_xattr(&init_user_ns, value, size);
		if (IS_ERR(acl))
			return PTR_ERR(acl);
		if (acl) {
			rc = posix_acl_valid(acl);
			if (rc)
				goto out;
		}
	} else {
		acl = NULL;
	}
	rc = jffs2_set_acl(dentry->d_inode, type, acl);
 out:
	posix_acl_release(acl);
	return rc;
}

const struct xattr_handler jffs2_acl_access_xattr_handler = {
	.prefix	= POSIX_ACL_XATTR_ACCESS,
	.flags	= ACL_TYPE_DEFAULT,
	.list	= jffs2_acl_access_listxattr,
	.get	= jffs2_acl_getxattr,
	.set	= jffs2_acl_setxattr,
};

const struct xattr_handler jffs2_acl_default_xattr_handler = {
	.prefix	= POSIX_ACL_XATTR_DEFAULT,
	.flags	= ACL_TYPE_DEFAULT,
	.list	= jffs2_acl_default_listxattr,
	.get	= jffs2_acl_getxattr,
	.set	= jffs2_acl_setxattr,
};
