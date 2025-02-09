/**
 * @file oprofile_stats.c
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon
 */

#include <fikus/oprofile.h>
#include <fikus/smp.h>
#include <fikus/cpumask.h>
#include <fikus/threads.h>

#include "oprofile_stats.h"
#include "cpu_buffer.h"

struct oprofile_stat_struct oprofile_stats;

void oprofile_reset_stats(void)
{
	struct oprofile_cpu_buffer *cpu_buf;
	int i;

	for_each_possible_cpu(i) {
		cpu_buf = &per_cpu(op_cpu_buffer, i);
		cpu_buf->sample_received = 0;
		cpu_buf->sample_lost_overflow = 0;
		cpu_buf->backtrace_aborted = 0;
		cpu_buf->sample_invalid_eip = 0;
	}

	atomic_set(&oprofile_stats.sample_lost_no_mm, 0);
	atomic_set(&oprofile_stats.sample_lost_no_mapping, 0);
	atomic_set(&oprofile_stats.event_lost_overflow, 0);
	atomic_set(&oprofile_stats.bt_lost_no_mapping, 0);
	atomic_set(&oprofile_stats.multiplex_counter, 0);
}


void oprofile_create_stats_files(struct dentry *root)
{
	struct oprofile_cpu_buffer *cpu_buf;
	struct dentry *cpudir;
	struct dentry *dir;
	char buf[10];
	int i;

	dir = oprofilefs_mkdir(root, "stats");
	if (!dir)
		return;

	for_each_possible_cpu(i) {
		cpu_buf = &per_cpu(op_cpu_buffer, i);
		snprintf(buf, 10, "cpu%d", i);
		cpudir = oprofilefs_mkdir(dir, buf);

		/* Strictly speaking access to these ulongs is racy,
		 * but we can't simply lock them, and they are
		 * informational only.
		 */
		oprofilefs_create_ro_ulong(cpudir, "sample_received",
			&cpu_buf->sample_received);
		oprofilefs_create_ro_ulong(cpudir, "sample_lost_overflow",
			&cpu_buf->sample_lost_overflow);
		oprofilefs_create_ro_ulong(cpudir, "backtrace_aborted",
			&cpu_buf->backtrace_aborted);
		oprofilefs_create_ro_ulong(cpudir, "sample_invalid_eip",
			&cpu_buf->sample_invalid_eip);
	}

	oprofilefs_create_ro_atomic(dir, "sample_lost_no_mm",
		&oprofile_stats.sample_lost_no_mm);
	oprofilefs_create_ro_atomic(dir, "sample_lost_no_mapping",
		&oprofile_stats.sample_lost_no_mapping);
	oprofilefs_create_ro_atomic(dir, "event_lost_overflow",
		&oprofile_stats.event_lost_overflow);
	oprofilefs_create_ro_atomic(dir, "bt_lost_no_mapping",
		&oprofile_stats.bt_lost_no_mapping);
#ifdef CONFIG_OPROFILE_EVENT_MULTIPLEX
	oprofilefs_create_ro_atomic(dir, "multiplex_counter",
		&oprofile_stats.multiplex_counter);
#endif
}
