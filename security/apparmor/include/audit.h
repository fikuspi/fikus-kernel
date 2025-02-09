/*
 * AppArmor security module
 *
 * This file contains AppArmor auditing function definitions.
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#ifndef __AA_AUDIT_H
#define __AA_AUDIT_H

#include <fikus/audit.h>
#include <fikus/fs.h>
#include <fikus/lsm_audit.h>
#include <fikus/sched.h>
#include <fikus/slab.h>

#include "file.h"

struct aa_profile;

extern const char *const audit_mode_names[];
#define AUDIT_MAX_INDEX 5
enum audit_mode {
	AUDIT_NORMAL,		/* follow normal auditing of accesses */
	AUDIT_QUIET_DENIED,	/* quiet all denied access messages */
	AUDIT_QUIET,		/* quiet all messages */
	AUDIT_NOQUIET,		/* do not quiet audit messages */
	AUDIT_ALL		/* audit all accesses */
};

enum audit_type {
	AUDIT_APPARMOR_AUDIT,
	AUDIT_APPARMOR_ALLOWED,
	AUDIT_APPARMOR_DENIED,
	AUDIT_APPARMOR_HINT,
	AUDIT_APPARMOR_STATUS,
	AUDIT_APPARMOR_ERROR,
	AUDIT_APPARMOR_KILL,
	AUDIT_APPARMOR_AUTO
};

extern const char *const op_table[];
enum aa_ops {
	OP_NULL,

	OP_SYSCTL,
	OP_CAPABLE,

	OP_UNLINK,
	OP_MKDIR,
	OP_RMDIR,
	OP_MKNOD,
	OP_TRUNC,
	OP_LINK,
	OP_SYMLINK,
	OP_RENAME_SRC,
	OP_RENAME_DEST,
	OP_CHMOD,
	OP_CHOWN,
	OP_GETATTR,
	OP_OPEN,

	OP_FPERM,
	OP_FLOCK,
	OP_FMMAP,
	OP_FMPROT,

	OP_CREATE,
	OP_POST_CREATE,
	OP_BIND,
	OP_CONNECT,
	OP_LISTEN,
	OP_ACCEPT,
	OP_SENDMSG,
	OP_RECVMSG,
	OP_GETSOCKNAME,
	OP_GETPEERNAME,
	OP_GETSOCKOPT,
	OP_SETSOCKOPT,
	OP_SOCK_SHUTDOWN,

	OP_PTRACE,

	OP_EXEC,
	OP_CHANGE_HAT,
	OP_CHANGE_PROFILE,
	OP_CHANGE_ONEXEC,

	OP_SETPROCATTR,
	OP_SETRLIMIT,

	OP_PROF_REPL,
	OP_PROF_LOAD,
	OP_PROF_RM,
};


struct apparmor_audit_data {
	int error;
	int op;
	int type;
	void *profile;
	const char *name;
	const char *info;
	struct task_struct *tsk;
	union {
		void *target;
		struct {
			long pos;
			void *target;
		} iface;
		struct {
			int rlim;
			unsigned long max;
		} rlim;
		struct {
			const char *target;
			u32 request;
			u32 denied;
			kuid_t ouid;
		} fs;
	};
};

/* define a short hand for apparmor_audit_data structure */
#define aad apparmor_audit_data

void aa_audit_msg(int type, struct common_audit_data *sa,
		  void (*cb) (struct audit_buffer *, void *));
int aa_audit(int type, struct aa_profile *profile, gfp_t gfp,
	     struct common_audit_data *sa,
	     void (*cb) (struct audit_buffer *, void *));

static inline int complain_error(int error)
{
	if (error == -EPERM || error == -EACCES)
		return 0;
	return error;
}

#endif /* __AA_AUDIT_H */
