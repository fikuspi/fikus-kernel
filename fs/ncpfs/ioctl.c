/*
 *  ioctl.c
 *
 *  Copyright (C) 1995, 1996 by Volker Lendecke
 *  Modified 1997 Peter Waltenberg, Bill Hawes, David Woodhouse for 2.1 dcache
 *  Modified 1998, 1999 Wolfram Pienkoss for NLS
 *
 */

#include <fikus/capability.h>
#include <fikus/compat.h>
#include <fikus/errno.h>
#include <fikus/fs.h>
#include <fikus/ioctl.h>
#include <fikus/time.h>
#include <fikus/mm.h>
#include <fikus/mount.h>
#include <fikus/slab.h>
#include <fikus/highuid.h>
#include <fikus/vmalloc.h>
#include <fikus/sched.h>

#include <asm/uaccess.h>

#include "ncp_fs.h"

/* maximum limit for ncp_objectname_ioctl */
#define NCP_OBJECT_NAME_MAX_LEN	4096
/* maximum limit for ncp_privatedata_ioctl */
#define NCP_PRIVATE_DATA_MAX_LEN 8192
/* maximum negotiable packet size */
#define NCP_PACKET_SIZE_INTERNAL 65536

static int
ncp_get_fs_info(struct ncp_server * server, struct inode *inode,
		struct ncp_fs_info __user *arg)
{
	struct ncp_fs_info info;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	if (info.version != NCP_GET_FS_INFO_VERSION) {
		DPRINTK("info.version invalid: %d\n", info.version);
		return -EINVAL;
	}
	/* TODO: info.addr = server->m.serv_addr; */
	SET_UID(info.mounted_uid, from_kuid_munged(current_user_ns(), server->m.mounted_uid));
	info.connection		= server->connection;
	info.buffer_size	= server->buffer_size;
	info.volume_number	= NCP_FINFO(inode)->volNumber;
	info.directory_id	= NCP_FINFO(inode)->DosDirNum;

	if (copy_to_user(arg, &info, sizeof(info)))
		return -EFAULT;
	return 0;
}

static int
ncp_get_fs_info_v2(struct ncp_server * server, struct inode *inode,
		   struct ncp_fs_info_v2 __user * arg)
{
	struct ncp_fs_info_v2 info2;

	if (copy_from_user(&info2, arg, sizeof(info2)))
		return -EFAULT;

	if (info2.version != NCP_GET_FS_INFO_VERSION_V2) {
		DPRINTK("info.version invalid: %d\n", info2.version);
		return -EINVAL;
	}
	info2.mounted_uid   = from_kuid_munged(current_user_ns(), server->m.mounted_uid);
	info2.connection    = server->connection;
	info2.buffer_size   = server->buffer_size;
	info2.volume_number = NCP_FINFO(inode)->volNumber;
	info2.directory_id  = NCP_FINFO(inode)->DosDirNum;
	info2.dummy1 = info2.dummy2 = info2.dummy3 = 0;

	if (copy_to_user(arg, &info2, sizeof(info2)))
		return -EFAULT;
	return 0;
}

#ifdef CONFIG_COMPAT
struct compat_ncp_objectname_ioctl
{
	s32		auth_type;
	u32		object_name_len;
	compat_caddr_t	object_name;	/* a userspace data, in most cases user name */
};

struct compat_ncp_fs_info_v2 {
	s32 version;
	u32 mounted_uid;
	u32 connection;
	u32 buffer_size;

	u32 volume_number;
	u32 directory_id;

	u32 dummy1;
	u32 dummy2;
	u32 dummy3;
};

struct compat_ncp_ioctl_request {
	u32 function;
	u32 size;
	compat_caddr_t data;
};

struct compat_ncp_privatedata_ioctl
{
	u32		len;
	compat_caddr_t	data;		/* ~1000 for NDS */
};

#define NCP_IOC_GET_FS_INFO_V2_32	_IOWR('n', 4, struct compat_ncp_fs_info_v2)
#define NCP_IOC_NCPREQUEST_32		_IOR('n', 1, struct compat_ncp_ioctl_request)
#define NCP_IOC_GETOBJECTNAME_32	_IOWR('n', 9, struct compat_ncp_objectname_ioctl)
#define NCP_IOC_SETOBJECTNAME_32	_IOR('n', 9, struct compat_ncp_objectname_ioctl)
#define NCP_IOC_GETPRIVATEDATA_32	_IOWR('n', 10, struct compat_ncp_privatedata_ioctl)
#define NCP_IOC_SETPRIVATEDATA_32	_IOR('n', 10, struct compat_ncp_privatedata_ioctl)

static int
ncp_get_compat_fs_info_v2(struct ncp_server * server, struct inode *inode,
		   struct compat_ncp_fs_info_v2 __user * arg)
{
	struct compat_ncp_fs_info_v2 info2;

	if (copy_from_user(&info2, arg, sizeof(info2)))
		return -EFAULT;

	if (info2.version != NCP_GET_FS_INFO_VERSION_V2) {
		DPRINTK("info.version invalid: %d\n", info2.version);
		return -EINVAL;
	}
	info2.mounted_uid   = from_kuid_munged(current_user_ns(), server->m.mounted_uid);
	info2.connection    = server->connection;
	info2.buffer_size   = server->buffer_size;
	info2.volume_number = NCP_FINFO(inode)->volNumber;
	info2.directory_id  = NCP_FINFO(inode)->DosDirNum;
	info2.dummy1 = info2.dummy2 = info2.dummy3 = 0;

	if (copy_to_user(arg, &info2, sizeof(info2)))
		return -EFAULT;
	return 0;
}
#endif

#define NCP_IOC_GETMOUNTUID16		_IOW('n', 2, u16)
#define NCP_IOC_GETMOUNTUID32		_IOW('n', 2, u32)
#define NCP_IOC_GETMOUNTUID64		_IOW('n', 2, u64)

#ifdef CONFIG_NCPFS_NLS
/* Here we are select the iocharset and the codepage for NLS.
 * Thanks Petr Vandrovec for idea and many hints.
 */
static int
ncp_set_charsets(struct ncp_server* server, struct ncp_nls_ioctl __user *arg)
{
	struct ncp_nls_ioctl user;
	struct nls_table *codepage;
	struct nls_table *iocharset;
	struct nls_table *oldset_io;
	struct nls_table *oldset_cp;
	int utf8;
	int err;

	if (copy_from_user(&user, arg, sizeof(user)))
		return -EFAULT;

	codepage = NULL;
	user.codepage[NCP_IOCSNAME_LEN] = 0;
	if (!user.codepage[0] || !strcmp(user.codepage, "default"))
		codepage = load_nls_default();
	else {
		codepage = load_nls(user.codepage);
		if (!codepage) {
			return -EBADRQC;
		}
	}

	iocharset = NULL;
	user.iocharset[NCP_IOCSNAME_LEN] = 0;
	if (!user.iocharset[0] || !strcmp(user.iocharset, "default")) {
		iocharset = load_nls_default();
		utf8 = 0;
	} else if (!strcmp(user.iocharset, "utf8")) {
		iocharset = load_nls_default();
		utf8 = 1;
	} else {
		iocharset = load_nls(user.iocharset);
		if (!iocharset) {
			unload_nls(codepage);
			return -EBADRQC;
		}
		utf8 = 0;
	}

	mutex_lock(&server->root_setup_lock);
	if (server->root_setuped) {
		oldset_cp = codepage;
		oldset_io = iocharset;
		err = -EBUSY;
	} else {
		if (utf8)
			NCP_SET_FLAG(server, NCP_FLAG_UTF8);
		else
			NCP_CLR_FLAG(server, NCP_FLAG_UTF8);
		oldset_cp = server->nls_vol;
		server->nls_vol = codepage;
		oldset_io = server->nls_io;
		server->nls_io = iocharset;
		err = 0;
	}
	mutex_unlock(&server->root_setup_lock);
	unload_nls(oldset_cp);
	unload_nls(oldset_io);

	return err;
}

static int
ncp_get_charsets(struct ncp_server* server, struct ncp_nls_ioctl __user *arg)
{
	struct ncp_nls_ioctl user;
	int len;

	memset(&user, 0, sizeof(user));
	mutex_lock(&server->root_setup_lock);
	if (server->nls_vol && server->nls_vol->charset) {
		len = strlen(server->nls_vol->charset);
		if (len > NCP_IOCSNAME_LEN)
			len = NCP_IOCSNAME_LEN;
		strncpy(user.codepage, server->nls_vol->charset, len);
		user.codepage[len] = 0;
	}

	if (NCP_IS_FLAG(server, NCP_FLAG_UTF8))
		strcpy(user.iocharset, "utf8");
	else if (server->nls_io && server->nls_io->charset) {
		len = strlen(server->nls_io->charset);
		if (len > NCP_IOCSNAME_LEN)
			len = NCP_IOCSNAME_LEN;
		strncpy(user.iocharset,	server->nls_io->charset, len);
		user.iocharset[len] = 0;
	}
	mutex_unlock(&server->root_setup_lock);

	if (copy_to_user(arg, &user, sizeof(user)))
		return -EFAULT;
	return 0;
}
#endif /* CONFIG_NCPFS_NLS */

static long __ncp_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg)
{
	struct ncp_server *server = NCP_SERVER(inode);
	int result;
	struct ncp_ioctl_request request;
	char* bouncebuffer;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
#ifdef CONFIG_COMPAT
	case NCP_IOC_NCPREQUEST_32:
#endif
	case NCP_IOC_NCPREQUEST:
#ifdef CONFIG_COMPAT
		if (cmd == NCP_IOC_NCPREQUEST_32) {
			struct compat_ncp_ioctl_request request32;
			if (copy_from_user(&request32, argp, sizeof(request32)))
				return -EFAULT;
			request.function = request32.function;
			request.size = request32.size;
			request.data = compat_ptr(request32.data);
		} else
#endif
		if (copy_from_user(&request, argp, sizeof(request)))
			return -EFAULT;

		if ((request.function > 255)
		    || (request.size >
		  NCP_PACKET_SIZE - sizeof(struct ncp_request_header))) {
			return -EINVAL;
		}
		bouncebuffer = vmalloc(NCP_PACKET_SIZE_INTERNAL);
		if (!bouncebuffer)
			return -ENOMEM;
		if (copy_from_user(bouncebuffer, request.data, request.size)) {
			vfree(bouncebuffer);
			return -EFAULT;
		}
		ncp_lock_server(server);

		/* FIXME: We hack around in the server's structures
		   here to be able to use ncp_request */

		server->has_subfunction = 0;
		server->current_size = request.size;
		memcpy(server->packet, bouncebuffer, request.size);

		result = ncp_request2(server, request.function,
			bouncebuffer, NCP_PACKET_SIZE_INTERNAL);
		if (result < 0)
			result = -EIO;
		else
			result = server->reply_size;
		ncp_unlock_server(server);
		DPRINTK("ncp_ioctl: copy %d bytes\n",
			result);
		if (result >= 0)
			if (copy_to_user(request.data, bouncebuffer, result))
				result = -EFAULT;
		vfree(bouncebuffer);
		return result;

	case NCP_IOC_CONN_LOGGED_IN:

		if (!(server->m.int_flags & NCP_IMOUNT_LOGGEDIN_POSSIBLE))
			return -EINVAL;
		mutex_lock(&server->root_setup_lock);
		if (server->root_setuped)
			result = -EBUSY;
		else {
			result = ncp_conn_logged_in(inode->i_sb);
			if (result == 0)
				server->root_setuped = 1;
		}
		mutex_unlock(&server->root_setup_lock);
		return result;

	case NCP_IOC_GET_FS_INFO:
		return ncp_get_fs_info(server, inode, argp);

	case NCP_IOC_GET_FS_INFO_V2:
		return ncp_get_fs_info_v2(server, inode, argp);

#ifdef CONFIG_COMPAT
	case NCP_IOC_GET_FS_INFO_V2_32:
		return ncp_get_compat_fs_info_v2(server, inode, argp);
#endif
	/* we have too many combinations of CONFIG_COMPAT,
	 * CONFIG_64BIT and CONFIG_UID16, so just handle
	 * any of the possible ioctls */
	case NCP_IOC_GETMOUNTUID16:
		{
			u16 uid;

			SET_UID(uid, from_kuid_munged(current_user_ns(), server->m.mounted_uid));
			if (put_user(uid, (u16 __user *)argp))
				return -EFAULT;
			return 0;
		}
	case NCP_IOC_GETMOUNTUID32:
	{
		uid_t uid = from_kuid_munged(current_user_ns(), server->m.mounted_uid);
		if (put_user(uid, (u32 __user *)argp))
			return -EFAULT;
		return 0;
	}
	case NCP_IOC_GETMOUNTUID64:
	{
		uid_t uid = from_kuid_munged(current_user_ns(), server->m.mounted_uid);
		if (put_user(uid, (u64 __user *)argp))
			return -EFAULT;
		return 0;
	}
	case NCP_IOC_GETROOT:
		{
			struct ncp_setroot_ioctl sr;

			result = -EACCES;
			mutex_lock(&server->root_setup_lock);
			if (server->m.mounted_vol[0]) {
				struct dentry* dentry = inode->i_sb->s_root;

				if (dentry) {
					struct inode* s_inode = dentry->d_inode;

					if (s_inode) {
						sr.volNumber = NCP_FINFO(s_inode)->volNumber;
						sr.dirEntNum = NCP_FINFO(s_inode)->dirEntNum;
						sr.namespace = server->name_space[sr.volNumber];
						result = 0;
					} else
						DPRINTK("ncpfs: s_root->d_inode==NULL\n");
				} else
					DPRINTK("ncpfs: s_root==NULL\n");
			} else {
				sr.volNumber = -1;
				sr.namespace = 0;
				sr.dirEntNum = 0;
				result = 0;
			}
			mutex_unlock(&server->root_setup_lock);
			if (!result && copy_to_user(argp, &sr, sizeof(sr)))
				result = -EFAULT;
			return result;
		}

	case NCP_IOC_SETROOT:
		{
			struct ncp_setroot_ioctl sr;
			__u32 vnum;
			__le32 de;
			__le32 dosde;
			struct dentry* dentry;

			if (copy_from_user(&sr, argp, sizeof(sr)))
				return -EFAULT;
			mutex_lock(&server->root_setup_lock);
			if (server->root_setuped)
				result = -EBUSY;
			else {
				if (sr.volNumber < 0) {
					server->m.mounted_vol[0] = 0;
					vnum = NCP_NUMBER_OF_VOLUMES;
					de = 0;
					dosde = 0;
					result = 0;
				} else if (sr.volNumber >= NCP_NUMBER_OF_VOLUMES) {
					result = -EINVAL;
				} else if (ncp_mount_subdir(server, sr.volNumber,
							sr.namespace, sr.dirEntNum,
							&vnum, &de, &dosde)) {
					result = -ENOENT;
				} else
					result = 0;

				if (result == 0) {
					dentry = inode->i_sb->s_root;
					if (dentry) {
						struct inode* s_inode = dentry->d_inode;

						if (s_inode) {
							NCP_FINFO(s_inode)->volNumber = vnum;
							NCP_FINFO(s_inode)->dirEntNum = de;
							NCP_FINFO(s_inode)->DosDirNum = dosde;
							server->root_setuped = 1;
						} else {
							DPRINTK("ncpfs: s_root->d_inode==NULL\n");
							result = -EIO;
						}
					} else {
						DPRINTK("ncpfs: s_root==NULL\n");
						result = -EIO;
					}
				}
				result = 0;
			}
			mutex_unlock(&server->root_setup_lock);

			return result;
		}

#ifdef CONFIG_NCPFS_PACKET_SIGNING
	case NCP_IOC_SIGN_INIT:
		{
			struct ncp_sign_init sign;

			if (argp)
				if (copy_from_user(&sign, argp, sizeof(sign)))
					return -EFAULT;
			ncp_lock_server(server);
			mutex_lock(&server->rcv.creq_mutex);
			if (argp) {
				if (server->sign_wanted) {
					memcpy(server->sign_root,sign.sign_root,8);
					memcpy(server->sign_last,sign.sign_last,16);
					server->sign_active = 1;
				}
				/* ignore when signatures not wanted */
			} else {
				server->sign_active = 0;
			}
			mutex_unlock(&server->rcv.creq_mutex);
			ncp_unlock_server(server);
			return 0;
		}

        case NCP_IOC_SIGN_WANTED:
		{
			int state;

			ncp_lock_server(server);
			state = server->sign_wanted;
			ncp_unlock_server(server);
			if (put_user(state, (int __user *)argp))
				return -EFAULT;
			return 0;
		}

	case NCP_IOC_SET_SIGN_WANTED:
		{
			int newstate;

			/* get only low 8 bits... */
			if (get_user(newstate, (unsigned char __user *)argp))
				return -EFAULT;
			result = 0;
			ncp_lock_server(server);
			if (server->sign_active) {
				/* cannot turn signatures OFF when active */
				if (!newstate)
					result = -EINVAL;
			} else {
				server->sign_wanted = newstate != 0;
			}
			ncp_unlock_server(server);
			return result;
		}

#endif /* CONFIG_NCPFS_PACKET_SIGNING */

#ifdef CONFIG_NCPFS_IOCTL_LOCKING
	case NCP_IOC_LOCKUNLOCK:
		{
			struct ncp_lock_ioctl	 rqdata;

			if (copy_from_user(&rqdata, argp, sizeof(rqdata)))
				return -EFAULT;
			if (rqdata.origin != 0)
				return -EINVAL;
			/* check for cmd */
			switch (rqdata.cmd) {
				case NCP_LOCK_EX:
				case NCP_LOCK_SH:
						if (rqdata.timeout == 0)
							rqdata.timeout = NCP_LOCK_DEFAULT_TIMEOUT;
						else if (rqdata.timeout > NCP_LOCK_MAX_TIMEOUT)
							rqdata.timeout = NCP_LOCK_MAX_TIMEOUT;
						break;
				case NCP_LOCK_LOG:
						rqdata.timeout = NCP_LOCK_DEFAULT_TIMEOUT;	/* has no effect */
				case NCP_LOCK_CLEAR:
						break;
				default:
						return -EINVAL;
			}
			/* locking needs both read and write access */
			if ((result = ncp_make_open(inode, O_RDWR)) != 0)
			{
				return result;
			}
			result = -EISDIR;
			if (!S_ISREG(inode->i_mode))
				goto outrel;
			if (rqdata.cmd == NCP_LOCK_CLEAR)
			{
				result = ncp_ClearPhysicalRecord(NCP_SERVER(inode),
							NCP_FINFO(inode)->file_handle,
							rqdata.offset,
							rqdata.length);
				if (result > 0) result = 0;	/* no such lock */
			}
			else
			{
				int lockcmd;

				switch (rqdata.cmd)
				{
					case NCP_LOCK_EX:  lockcmd=1; break;
					case NCP_LOCK_SH:  lockcmd=3; break;
					default:	   lockcmd=0; break;
				}
				result = ncp_LogPhysicalRecord(NCP_SERVER(inode),
							NCP_FINFO(inode)->file_handle,
							lockcmd,
							rqdata.offset,
							rqdata.length,
							rqdata.timeout);
				if (result > 0) result = -EAGAIN;
			}
outrel:
			ncp_inode_close(inode);
			return result;
		}
#endif	/* CONFIG_NCPFS_IOCTL_LOCKING */

#ifdef CONFIG_COMPAT
	case NCP_IOC_GETOBJECTNAME_32:
		{
			struct compat_ncp_objectname_ioctl user;
			size_t outl;

			if (copy_from_user(&user, argp, sizeof(user)))
				return -EFAULT;
			down_read(&server->auth_rwsem);
			user.auth_type = server->auth.auth_type;
			outl = user.object_name_len;
			user.object_name_len = server->auth.object_name_len;
			if (outl > user.object_name_len)
				outl = user.object_name_len;
			result = 0;
			if (outl) {
				if (copy_to_user(compat_ptr(user.object_name),
						 server->auth.object_name,
						 outl))
					result = -EFAULT;
			}
			up_read(&server->auth_rwsem);
			if (!result && copy_to_user(argp, &user, sizeof(user)))
				result = -EFAULT;
			return result;
		}
#endif

	case NCP_IOC_GETOBJECTNAME:
		{
			struct ncp_objectname_ioctl user;
			size_t outl;

			if (copy_from_user(&user, argp, sizeof(user)))
				return -EFAULT;
			down_read(&server->auth_rwsem);
			user.auth_type = server->auth.auth_type;
			outl = user.object_name_len;
			user.object_name_len = server->auth.object_name_len;
			if (outl > user.object_name_len)
				outl = user.object_name_len;
			result = 0;
			if (outl) {
				if (copy_to_user(user.object_name,
						 server->auth.object_name,
						 outl))
					result = -EFAULT;
			}
			up_read(&server->auth_rwsem);
			if (!result && copy_to_user(argp, &user, sizeof(user)))
				result = -EFAULT;
			return result;
		}

#ifdef CONFIG_COMPAT
	case NCP_IOC_SETOBJECTNAME_32:
#endif
	case NCP_IOC_SETOBJECTNAME:
		{
			struct ncp_objectname_ioctl user;
			void* newname;
			void* oldname;
			size_t oldnamelen;
			void* oldprivate;
			size_t oldprivatelen;

#ifdef CONFIG_COMPAT
			if (cmd == NCP_IOC_SETOBJECTNAME_32) {
				struct compat_ncp_objectname_ioctl user32;
				if (copy_from_user(&user32, argp, sizeof(user32)))
					return -EFAULT;
				user.auth_type = user32.auth_type;
				user.object_name_len = user32.object_name_len;
				user.object_name = compat_ptr(user32.object_name);
			} else
#endif
			if (copy_from_user(&user, argp, sizeof(user)))
				return -EFAULT;

			if (user.object_name_len > NCP_OBJECT_NAME_MAX_LEN)
				return -ENOMEM;
			if (user.object_name_len) {
				newname = memdup_user(user.object_name,
						      user.object_name_len);
				if (IS_ERR(newname))
					return PTR_ERR(newname);
			} else {
				newname = NULL;
			}
			down_write(&server->auth_rwsem);
			oldname = server->auth.object_name;
			oldnamelen = server->auth.object_name_len;
			oldprivate = server->priv.data;
			oldprivatelen = server->priv.len;
			server->auth.auth_type = user.auth_type;
			server->auth.object_name_len = user.object_name_len;
			server->auth.object_name = newname;
			server->priv.len = 0;
			server->priv.data = NULL;
			up_write(&server->auth_rwsem);
			kfree(oldprivate);
			kfree(oldname);
			return 0;
		}

#ifdef CONFIG_COMPAT
	case NCP_IOC_GETPRIVATEDATA_32:
#endif
	case NCP_IOC_GETPRIVATEDATA:
		{
			struct ncp_privatedata_ioctl user;
			size_t outl;

#ifdef CONFIG_COMPAT
			if (cmd == NCP_IOC_GETPRIVATEDATA_32) {
				struct compat_ncp_privatedata_ioctl user32;
				if (copy_from_user(&user32, argp, sizeof(user32)))
					return -EFAULT;
				user.len = user32.len;
				user.data = compat_ptr(user32.data);
			} else
#endif
			if (copy_from_user(&user, argp, sizeof(user)))
				return -EFAULT;

			down_read(&server->auth_rwsem);
			outl = user.len;
			user.len = server->priv.len;
			if (outl > user.len) outl = user.len;
			result = 0;
			if (outl) {
				if (copy_to_user(user.data,
						 server->priv.data,
						 outl))
					result = -EFAULT;
			}
			up_read(&server->auth_rwsem);
			if (result)
				return result;
#ifdef CONFIG_COMPAT
			if (cmd == NCP_IOC_GETPRIVATEDATA_32) {
				struct compat_ncp_privatedata_ioctl user32;
				user32.len = user.len;
				user32.data = (unsigned long) user.data;
				if (copy_to_user(argp, &user32, sizeof(user32)))
					return -EFAULT;
			} else
#endif
			if (copy_to_user(argp, &user, sizeof(user)))
				return -EFAULT;

			return 0;
		}

#ifdef CONFIG_COMPAT
	case NCP_IOC_SETPRIVATEDATA_32:
#endif
	case NCP_IOC_SETPRIVATEDATA:
		{
			struct ncp_privatedata_ioctl user;
			void* new;
			void* old;
			size_t oldlen;

#ifdef CONFIG_COMPAT
			if (cmd == NCP_IOC_SETPRIVATEDATA_32) {
				struct compat_ncp_privatedata_ioctl user32;
				if (copy_from_user(&user32, argp, sizeof(user32)))
					return -EFAULT;
				user.len = user32.len;
				user.data = compat_ptr(user32.data);
			} else
#endif
			if (copy_from_user(&user, argp, sizeof(user)))
				return -EFAULT;

			if (user.len > NCP_PRIVATE_DATA_MAX_LEN)
				return -ENOMEM;
			if (user.len) {
				new = memdup_user(user.data, user.len);
				if (IS_ERR(new))
					return PTR_ERR(new);
			} else {
				new = NULL;
			}
			down_write(&server->auth_rwsem);
			old = server->priv.data;
			oldlen = server->priv.len;
			server->priv.len = user.len;
			server->priv.data = new;
			up_write(&server->auth_rwsem);
			kfree(old);
			return 0;
		}

#ifdef CONFIG_NCPFS_NLS
	case NCP_IOC_SETCHARSETS:
		return ncp_set_charsets(server, argp);

	case NCP_IOC_GETCHARSETS:
		return ncp_get_charsets(server, argp);

#endif /* CONFIG_NCPFS_NLS */

	case NCP_IOC_SETDENTRYTTL:
		{
			u_int32_t user;

			if (copy_from_user(&user, argp, sizeof(user)))
				return -EFAULT;
			/* 20 secs at most... */
			if (user > 20000)
				return -EINVAL;
			user = (user * HZ) / 1000;
			atomic_set(&server->dentry_ttl, user);
			return 0;
		}

	case NCP_IOC_GETDENTRYTTL:
		{
			u_int32_t user = (atomic_read(&server->dentry_ttl) * 1000) / HZ;
			if (copy_to_user(argp, &user, sizeof(user)))
				return -EFAULT;
			return 0;
		}

	}
	return -EINVAL;
}

long ncp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct ncp_server *server = NCP_SERVER(inode);
	kuid_t uid = current_uid();
	int need_drop_write = 0;
	long ret;

	switch (cmd) {
	case NCP_IOC_SETCHARSETS:
	case NCP_IOC_CONN_LOGGED_IN:
	case NCP_IOC_SETROOT:
		if (!capable(CAP_SYS_ADMIN)) {
			ret = -EPERM;
			goto out;
		}
		break;
	}
	if (!uid_eq(server->m.mounted_uid, uid)) {
		switch (cmd) {
		/*
		 * Only mount owner can issue these ioctls.  Information
		 * necessary to authenticate to other NDS servers are
		 * stored here.
		 */
		case NCP_IOC_GETOBJECTNAME:
		case NCP_IOC_SETOBJECTNAME:
		case NCP_IOC_GETPRIVATEDATA:
		case NCP_IOC_SETPRIVATEDATA:
#ifdef CONFIG_COMPAT
		case NCP_IOC_GETOBJECTNAME_32:
		case NCP_IOC_SETOBJECTNAME_32:
		case NCP_IOC_GETPRIVATEDATA_32:
		case NCP_IOC_SETPRIVATEDATA_32:
#endif
			ret = -EACCES;
			goto out;
		/*
		 * These require write access on the inode if user id
		 * does not match.  Note that they do not write to the
		 * file...  But old code did mnt_want_write, so I keep
		 * it as is.  Of course not for mountpoint owner, as
		 * that breaks read-only mounts altogether as ncpmount
		 * needs working NCP_IOC_NCPREQUEST and
		 * NCP_IOC_GET_FS_INFO.  Some of these codes (setdentryttl,
		 * signinit, setsignwanted) should be probably restricted
		 * to owner only, or even more to CAP_SYS_ADMIN).
		 */
		case NCP_IOC_GET_FS_INFO:
		case NCP_IOC_GET_FS_INFO_V2:
		case NCP_IOC_NCPREQUEST:
		case NCP_IOC_SETDENTRYTTL:
		case NCP_IOC_SIGN_INIT:
		case NCP_IOC_LOCKUNLOCK:
		case NCP_IOC_SET_SIGN_WANTED:
#ifdef CONFIG_COMPAT
		case NCP_IOC_GET_FS_INFO_V2_32:
		case NCP_IOC_NCPREQUEST_32:
#endif
			ret = mnt_want_write_file(filp);
			if (ret)
				goto out;
			need_drop_write = 1;
			ret = inode_permission(inode, MAY_WRITE);
			if (ret)
				goto outDropWrite;
			break;
		/*
		 * Read access required.
		 */
		case NCP_IOC_GETMOUNTUID16:
		case NCP_IOC_GETMOUNTUID32:
		case NCP_IOC_GETMOUNTUID64:
		case NCP_IOC_GETROOT:
		case NCP_IOC_SIGN_WANTED:
			ret = inode_permission(inode, MAY_READ);
			if (ret)
				goto out;
			break;
		/*
		 * Anybody can read these.
		 */
		case NCP_IOC_GETCHARSETS:
		case NCP_IOC_GETDENTRYTTL:
		default:
		/* Three codes below are protected by CAP_SYS_ADMIN above. */
		case NCP_IOC_SETCHARSETS:
		case NCP_IOC_CONN_LOGGED_IN:
		case NCP_IOC_SETROOT:
			break;
		}
	}
	ret = __ncp_ioctl(inode, cmd, arg);
outDropWrite:
	if (need_drop_write)
		mnt_drop_write_file(filp);
out:
	return ret;
}

#ifdef CONFIG_COMPAT
long ncp_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret;

	arg = (unsigned long) compat_ptr(arg);
	ret = ncp_ioctl(file, cmd, arg);
	return ret;
}
#endif
