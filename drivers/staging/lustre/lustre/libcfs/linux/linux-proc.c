/*
 * GPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; If not, see
 * http://www.sun.com/software/products/lustre/docs/GPLv2.pdf
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 * GPL HEADER END
 */
/*
 * Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2011, 2012, Intel Corporation.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 *
 * libcfs/libcfs/fikus/fikus-proc.c
 *
 * Author: Zach Brown <zab@zabbo.net>
 * Author: Peter J. Braam <braam@clusterfs.com>
 * Author: Phil Schwan <phil@clusterfs.com>
 */

#include <fikus/module.h>
#include <fikus/kernel.h>
#include <fikus/mm.h>
#include <fikus/string.h>
#include <fikus/stat.h>
#include <fikus/errno.h>
#include <fikus/unistd.h>
#include <net/sock.h>
#include <fikus/uio.h>

#include <asm/uaccess.h>

#include <fikus/fs.h>
#include <fikus/file.h>
#include <fikus/list.h>

#include <fikus/proc_fs.h>
#include <fikus/sysctl.h>

# define DEBUG_SUBSYSTEM S_LNET

#include <fikus/libcfs/libcfs.h>
#include <asm/div64.h>
#include "tracefile.h"

#ifdef CONFIG_SYSCTL
static ctl_table_header_t *lnet_table_header = NULL;
#endif
extern char lnet_upcall[1024];
/**
 * The path of debug log dump upcall script.
 */
extern char lnet_debug_log_upcall[1024];

#define CTL_LNET	(0x100)
enum {
	PSDEV_DEBUG = 1,	  /* control debugging */
	PSDEV_SUBSYSTEM_DEBUG,    /* control debugging */
	PSDEV_PRINTK,	     /* force all messages to console */
	PSDEV_CONSOLE_RATELIMIT,  /* ratelimit console messages */
	PSDEV_CONSOLE_MAX_DELAY_CS, /* maximum delay over which we skip messages */
	PSDEV_CONSOLE_MIN_DELAY_CS, /* initial delay over which we skip messages */
	PSDEV_CONSOLE_BACKOFF,    /* delay increase factor */
	PSDEV_DEBUG_PATH,	 /* crashdump log location */
	PSDEV_DEBUG_DUMP_PATH,    /* crashdump tracelog location */
	PSDEV_CPT_TABLE,	  /* information about cpu partitions */
	PSDEV_LNET_UPCALL,	/* User mode upcall script  */
	PSDEV_LNET_MEMUSED,       /* bytes currently PORTAL_ALLOCated */
	PSDEV_LNET_CATASTROPHE,   /* if we have LBUGged or panic'd */
	PSDEV_LNET_PANIC_ON_LBUG, /* flag to panic on LBUG */
	PSDEV_LNET_DUMP_KERNEL,   /* snapshot kernel debug buffer to file */
	PSDEV_LNET_DAEMON_FILE,   /* spool kernel debug buffer to file */
	PSDEV_LNET_DEBUG_MB,      /* size of debug buffer */
	PSDEV_LNET_DEBUG_LOG_UPCALL, /* debug log upcall script */
	PSDEV_LNET_WATCHDOG_RATELIMIT,  /* ratelimit watchdog messages  */
	PSDEV_LNET_FORCE_LBUG,    /* hook to force an LBUG */
	PSDEV_LNET_FAIL_LOC,      /* control test failures instrumentation */
	PSDEV_LNET_FAIL_VAL,      /* userdata for fail loc */
};

int
proc_call_handler(void *data, int write,
		  loff_t *ppos, void *buffer, size_t *lenp,
		  int (*handler)(void *data, int write,
				 loff_t pos, void *buffer, int len))
{
	int rc = handler(data, write, *ppos, buffer, *lenp);

	if (rc < 0)
		return rc;

	if (write) {
		*ppos += *lenp;
	} else {
		*lenp = rc;
		*ppos += rc;
	}
	return 0;
}
EXPORT_SYMBOL(proc_call_handler);

static int __proc_dobitmasks(void *data, int write,
			     loff_t pos, void *buffer, int nob)
{
	const int     tmpstrlen = 512;
	char	 *tmpstr;
	int	   rc;
	unsigned int *mask = data;
	int	   is_subsys = (mask == &libcfs_subsystem_debug) ? 1 : 0;
	int	   is_printk = (mask == &libcfs_printk) ? 1 : 0;

	rc = cfs_trace_allocate_string_buffer(&tmpstr, tmpstrlen);
	if (rc < 0)
		return rc;

	if (!write) {
		libcfs_debug_mask2str(tmpstr, tmpstrlen, *mask, is_subsys);
		rc = strlen(tmpstr);

		if (pos >= rc) {
			rc = 0;
		} else {
			rc = cfs_trace_copyout_string(buffer, nob,
						      tmpstr + pos, "\n");
		}
	} else {
		rc = cfs_trace_copyin_string(tmpstr, tmpstrlen, buffer, nob);
		if (rc < 0) {
			cfs_trace_free_string_buffer(tmpstr, tmpstrlen);
			return rc;
		}

		rc = libcfs_debug_str2mask(mask, tmpstr, is_subsys);
		/* Always print LBUG/LASSERT to console, so keep this mask */
		if (is_printk)
			*mask |= D_EMERG;
	}

	cfs_trace_free_string_buffer(tmpstr, tmpstrlen);
	return rc;
}

DECLARE_PROC_HANDLER(proc_dobitmasks)

static int min_watchdog_ratelimit = 0;	  /* disable ratelimiting */
static int max_watchdog_ratelimit = (24*60*60); /* limit to once per day */

static int __proc_dump_kernel(void *data, int write,
			      loff_t pos, void *buffer, int nob)
{
	if (!write)
		return 0;

	return cfs_trace_dump_debug_buffer_usrstr(buffer, nob);
}

DECLARE_PROC_HANDLER(proc_dump_kernel)

static int __proc_daemon_file(void *data, int write,
			      loff_t pos, void *buffer, int nob)
{
	if (!write) {
		int len = strlen(cfs_tracefile);

		if (pos >= len)
			return 0;

		return cfs_trace_copyout_string(buffer, nob,
						cfs_tracefile + pos, "\n");
	}

	return cfs_trace_daemon_command_usrstr(buffer, nob);
}

DECLARE_PROC_HANDLER(proc_daemon_file)

static int __proc_debug_mb(void *data, int write,
			   loff_t pos, void *buffer, int nob)
{
	if (!write) {
		char tmpstr[32];
		int  len = snprintf(tmpstr, sizeof(tmpstr), "%d",
				    cfs_trace_get_debug_mb());

		if (pos >= len)
			return 0;

		return cfs_trace_copyout_string(buffer, nob, tmpstr + pos,
		       "\n");
	}

	return cfs_trace_set_debug_mb_usrstr(buffer, nob);
}

DECLARE_PROC_HANDLER(proc_debug_mb)

int LL_PROC_PROTO(proc_console_max_delay_cs)
{
	int rc, max_delay_cs;
	ctl_table_t dummy = *table;
	cfs_duration_t d;

	dummy.data = &max_delay_cs;
	dummy.proc_handler = &proc_dointvec;

	if (!write) { /* read */
		max_delay_cs = cfs_duration_sec(libcfs_console_max_delay * 100);
		rc = ll_proc_dointvec(&dummy, write, filp, buffer, lenp, ppos);
		return rc;
	}

	/* write */
	max_delay_cs = 0;
	rc = ll_proc_dointvec(&dummy, write, filp, buffer, lenp, ppos);
	if (rc < 0)
		return rc;
	if (max_delay_cs <= 0)
		return -EINVAL;

	d = cfs_time_seconds(max_delay_cs) / 100;
	if (d == 0 || d < libcfs_console_min_delay)
		return -EINVAL;
	libcfs_console_max_delay = d;

	return rc;
}

int LL_PROC_PROTO(proc_console_min_delay_cs)
{
	int rc, min_delay_cs;
	ctl_table_t dummy = *table;
	cfs_duration_t d;

	dummy.data = &min_delay_cs;
	dummy.proc_handler = &proc_dointvec;

	if (!write) { /* read */
		min_delay_cs = cfs_duration_sec(libcfs_console_min_delay * 100);
		rc = ll_proc_dointvec(&dummy, write, filp, buffer, lenp, ppos);
		return rc;
	}

	/* write */
	min_delay_cs = 0;
	rc = ll_proc_dointvec(&dummy, write, filp, buffer, lenp, ppos);
	if (rc < 0)
		return rc;
	if (min_delay_cs <= 0)
		return -EINVAL;

	d = cfs_time_seconds(min_delay_cs) / 100;
	if (d == 0 || d > libcfs_console_max_delay)
		return -EINVAL;
	libcfs_console_min_delay = d;

	return rc;
}

int LL_PROC_PROTO(proc_console_backoff)
{
	int rc, backoff;
	ctl_table_t dummy = *table;

	dummy.data = &backoff;
	dummy.proc_handler = &proc_dointvec;

	if (!write) { /* read */
		backoff= libcfs_console_backoff;
		rc = ll_proc_dointvec(&dummy, write, filp, buffer, lenp, ppos);
		return rc;
	}

	/* write */
	backoff = 0;
	rc = ll_proc_dointvec(&dummy, write, filp, buffer, lenp, ppos);
	if (rc < 0)
		return rc;
	if (backoff <= 0)
		return -EINVAL;

	libcfs_console_backoff = backoff;

	return rc;
}

int LL_PROC_PROTO(libcfs_force_lbug)
{
	if (write)
		LBUG();
	return 0;
}

int LL_PROC_PROTO(proc_fail_loc)
{
	int rc;
	long old_fail_loc = cfs_fail_loc;

	rc = ll_proc_dolongvec(table, write, filp, buffer, lenp, ppos);
	if (old_fail_loc != cfs_fail_loc)
		wake_up(&cfs_race_waitq);
	return rc;
}

static int __proc_cpt_table(void *data, int write,
			    loff_t pos, void *buffer, int nob)
{
	char *buf = NULL;
	int   len = 4096;
	int   rc  = 0;

	if (write)
		return -EPERM;

	LASSERT(cfs_cpt_table != NULL);

	while (1) {
		LIBCFS_ALLOC(buf, len);
		if (buf == NULL)
			return -ENOMEM;

		rc = cfs_cpt_table_print(cfs_cpt_table, buf, len);
		if (rc >= 0)
			break;

		LIBCFS_FREE(buf, len);
		if (rc == -EFBIG) {
			len <<= 1;
			continue;
		}
		goto out;
	}

	if (pos >= rc) {
		rc = 0;
		goto out;
	}

	rc = cfs_trace_copyout_string(buffer, nob, buf + pos, NULL);
 out:
	if (buf != NULL)
		LIBCFS_FREE(buf, len);
	return rc;
}
DECLARE_PROC_HANDLER(proc_cpt_table)

static ctl_table_t lnet_table[] = {
	/*
	 * NB No .strategy entries have been provided since sysctl(8) prefers
	 * to go via /proc for portability.
	 */
	{
		INIT_CTL_NAME(PSDEV_DEBUG)
		.procname = "debug",
		.data     = &libcfs_debug,
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_dobitmasks,
	},
	{
		INIT_CTL_NAME(PSDEV_SUBSYSTEM_DEBUG)
		.procname = "subsystem_debug",
		.data     = &libcfs_subsystem_debug,
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_dobitmasks,
	},
	{
		INIT_CTL_NAME(PSDEV_PRINTK)
		.procname = "printk",
		.data     = &libcfs_printk,
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_dobitmasks,
	},
	{
		INIT_CTL_NAME(PSDEV_CONSOLE_RATELIMIT)
		.procname = "console_ratelimit",
		.data     = &libcfs_console_ratelimit,
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_dointvec
	},
	{
		INIT_CTL_NAME(PSDEV_CONSOLE_MAX_DELAY_CS)
		.procname = "console_max_delay_centisecs",
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_console_max_delay_cs
	},
	{
		INIT_CTL_NAME(PSDEV_CONSOLE_MIN_DELAY_CS)
		.procname = "console_min_delay_centisecs",
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_console_min_delay_cs
	},
	{
		INIT_CTL_NAME(PSDEV_CONSOLE_BACKOFF)
		.procname = "console_backoff",
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_console_backoff
	},

	{
		INIT_CTL_NAME(PSDEV_DEBUG_PATH)
		.procname = "debug_path",
		.data     = libcfs_debug_file_path_arr,
		.maxlen   = sizeof(libcfs_debug_file_path_arr),
		.mode     = 0644,
		.proc_handler = &proc_dostring,
	},

	{
		INIT_CTL_NAME(PSDEV_CPT_TABLE)
		.procname = "cpu_partition_table",
		.maxlen   = 128,
		.mode     = 0444,
		.proc_handler = &proc_cpt_table,
	},

	{
		INIT_CTL_NAME(PSDEV_LNET_UPCALL)
		.procname = "upcall",
		.data     = lnet_upcall,
		.maxlen   = sizeof(lnet_upcall),
		.mode     = 0644,
		.proc_handler = &proc_dostring,
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_DEBUG_LOG_UPCALL)
		.procname = "debug_log_upcall",
		.data     = lnet_debug_log_upcall,
		.maxlen   = sizeof(lnet_debug_log_upcall),
		.mode     = 0644,
		.proc_handler = &proc_dostring,
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_MEMUSED)
		.procname = "lnet_memused",
		.data     = (int *)&libcfs_kmemory.counter,
		.maxlen   = sizeof(int),
		.mode     = 0444,
		.proc_handler = &proc_dointvec,
		INIT_STRATEGY(&sysctl_intvec)
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_CATASTROPHE)
		.procname = "catastrophe",
		.data     = &libcfs_catastrophe,
		.maxlen   = sizeof(int),
		.mode     = 0444,
		.proc_handler = &proc_dointvec,
		INIT_STRATEGY(&sysctl_intvec)
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_PANIC_ON_LBUG)
		.procname = "panic_on_lbug",
		.data     = &libcfs_panic_on_lbug,
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_dointvec,
		INIT_STRATEGY(&sysctl_intvec)
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_DUMP_KERNEL)
		.procname = "dump_kernel",
		.maxlen   = 256,
		.mode     = 0200,
		.proc_handler = &proc_dump_kernel,
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_DAEMON_FILE)
		.procname = "daemon_file",
		.mode     = 0644,
		.maxlen   = 256,
		.proc_handler = &proc_daemon_file,
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_DEBUG_MB)
		.procname = "debug_mb",
		.mode     = 0644,
		.proc_handler = &proc_debug_mb,
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_WATCHDOG_RATELIMIT)
		.procname = "watchdog_ratelimit",
		.data     = &libcfs_watchdog_ratelimit,
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_dointvec_minmax,
		.extra1   = &min_watchdog_ratelimit,
		.extra2   = &max_watchdog_ratelimit,
	},
	{       INIT_CTL_NAME(PSDEV_LNET_FORCE_LBUG)
		.procname = "force_lbug",
		.data     = NULL,
		.maxlen   = 0,
		.mode     = 0200,
		.proc_handler = &libcfs_force_lbug
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_FAIL_LOC)
		.procname = "fail_loc",
		.data     = &cfs_fail_loc,
		.maxlen   = sizeof(cfs_fail_loc),
		.mode     = 0644,
		.proc_handler = &proc_fail_loc
	},
	{
		INIT_CTL_NAME(PSDEV_LNET_FAIL_VAL)
		.procname = "fail_val",
		.data     = &cfs_fail_val,
		.maxlen   = sizeof(int),
		.mode     = 0644,
		.proc_handler = &proc_dointvec
	},
	{
		INIT_CTL_NAME(0)
	}
};

#ifdef CONFIG_SYSCTL
static ctl_table_t top_table[] = {
	{
		INIT_CTL_NAME(CTL_LNET)
		.procname = "lnet",
		.mode     = 0555,
		.data     = NULL,
		.maxlen   = 0,
		.child    = lnet_table,
	},
	{
		INIT_CTL_NAME(0)
	}
};
#endif

int insert_proc(void)
{
#ifdef CONFIG_SYSCTL
	if (lnet_table_header == NULL)
		lnet_table_header = register_sysctl_table(top_table);
#endif
	return 0;
}

void remove_proc(void)
{
#ifdef CONFIG_SYSCTL
	if (lnet_table_header != NULL)
		unregister_sysctl_table(lnet_table_header);

	lnet_table_header = NULL;
#endif
}
