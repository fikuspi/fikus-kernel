/*
 * builtin-stat.c
 *
 * Builtin stat command: Give a precise performance counters summary
 * overview about any workload, CPU or specific PID.
 *
 * Sample output:

   $ perf stat ./hackbench 10

  Time: 0.118

  Performance counter stats for './hackbench 10':

       1708.761321 task-clock                #   11.037 CPUs utilized
            41,190 context-switches          #    0.024 M/sec
             6,735 CPU-migrations            #    0.004 M/sec
            17,318 page-faults               #    0.010 M/sec
     5,205,202,243 cycles                    #    3.046 GHz
     3,856,436,920 stalled-cycles-frontend   #   74.09% frontend cycles idle
     1,600,790,871 stalled-cycles-backend    #   30.75% backend  cycles idle
     2,603,501,247 instructions              #    0.50  insns per cycle
                                             #    1.48  stalled cycles per insn
       484,357,498 branches                  #  283.455 M/sec
         6,388,934 branch-misses             #    1.32% of all branches

        0.154822978  seconds time elapsed

 *
 * Copyright (C) 2008-2011, Red Hat Inc, Ingo Molnar <mingo@redhat.com>
 *
 * Improvements and fixes by:
 *
 *   Arjan van de Ven <arjan@fikus.intel.com>
 *   Yanmin Zhang <yanmin.zhang@intel.com>
 *   Wu Fengguang <fengguang.wu@intel.com>
 *   Mike Galbraith <efault@gmx.de>
 *   Paul Mackerras <paulus@samba.org>
 *   Jaswinder Singh Rajput <jaswinder@kernel.org>
 *
 * Released under the GPL v2. (and only v2, not any later version)
 */

#include "perf.h"
#include "builtin.h"
#include "util/util.h"
#include "util/parse-options.h"
#include "util/parse-events.h"
#include "util/event.h"
#include "util/evlist.h"
#include "util/evsel.h"
#include "util/debug.h"
#include "util/color.h"
#include "util/stat.h"
#include "util/header.h"
#include "util/cpumap.h"
#include "util/thread.h"
#include "util/thread_map.h"

#include <stdlib.h>
#include <sys/prctl.h>
#include <locale.h>

#define DEFAULT_SEPARATOR	" "
#define CNTR_NOT_SUPPORTED	"<not supported>"
#define CNTR_NOT_COUNTED	"<not counted>"

static void print_stat(int argc, const char **argv);
static void print_counter_aggr(struct perf_evsel *counter, char *prefix);
static void print_counter(struct perf_evsel *counter, char *prefix);
static void print_aggr(char *prefix);

static struct perf_evlist	*evsel_list;

static struct perf_target	target = {
	.uid	= UINT_MAX,
};

enum aggr_mode {
	AGGR_NONE,
	AGGR_GLOBAL,
	AGGR_SOCKET,
	AGGR_CORE,
};

static int			run_count			=  1;
static bool			no_inherit			= false;
static bool			scale				=  true;
static enum aggr_mode		aggr_mode			= AGGR_GLOBAL;
static volatile pid_t		child_pid			= -1;
static bool			null_run			=  false;
static int			detailed_run			=  0;
static bool			big_num				=  true;
static int			big_num_opt			=  -1;
static const char		*csv_sep			= NULL;
static bool			csv_output			= false;
static bool			group				= false;
static FILE			*output				= NULL;
static const char		*pre_cmd			= NULL;
static const char		*post_cmd			= NULL;
static bool			sync_run			= false;
static unsigned int		interval			= 0;
static unsigned int		initial_delay			= 0;
static bool			forever				= false;
static struct timespec		ref_time;
static struct cpu_map		*aggr_map;
static int			(*aggr_get_id)(struct cpu_map *m, int cpu);

static volatile int done = 0;

struct perf_stat {
	struct stats	  res_stats[3];
};

static inline void diff_timespec(struct timespec *r, struct timespec *a,
				 struct timespec *b)
{
	r->tv_sec = a->tv_sec - b->tv_sec;
	if (a->tv_nsec < b->tv_nsec) {
		r->tv_nsec = a->tv_nsec + 1000000000L - b->tv_nsec;
		r->tv_sec--;
	} else {
		r->tv_nsec = a->tv_nsec - b->tv_nsec ;
	}
}

static inline struct cpu_map *perf_evsel__cpus(struct perf_evsel *evsel)
{
	return (evsel->cpus && !target.cpu_list) ? evsel->cpus : evsel_list->cpus;
}

static inline int perf_evsel__nr_cpus(struct perf_evsel *evsel)
{
	return perf_evsel__cpus(evsel)->nr;
}

static void perf_evsel__reset_stat_priv(struct perf_evsel *evsel)
{
	memset(evsel->priv, 0, sizeof(struct perf_stat));
}

static int perf_evsel__alloc_stat_priv(struct perf_evsel *evsel)
{
	evsel->priv = zalloc(sizeof(struct perf_stat));
	return evsel->priv == NULL ? -ENOMEM : 0;
}

static void perf_evsel__free_stat_priv(struct perf_evsel *evsel)
{
	free(evsel->priv);
	evsel->priv = NULL;
}

static int perf_evsel__alloc_prev_raw_counts(struct perf_evsel *evsel)
{
	void *addr;
	size_t sz;

	sz = sizeof(*evsel->counts) +
	     (perf_evsel__nr_cpus(evsel) * sizeof(struct perf_counts_values));

	addr = zalloc(sz);
	if (!addr)
		return -ENOMEM;

	evsel->prev_raw_counts =  addr;

	return 0;
}

static void perf_evsel__free_prev_raw_counts(struct perf_evsel *evsel)
{
	free(evsel->prev_raw_counts);
	evsel->prev_raw_counts = NULL;
}

static void perf_evlist__free_stats(struct perf_evlist *evlist)
{
	struct perf_evsel *evsel;

	list_for_each_entry(evsel, &evlist->entries, node) {
		perf_evsel__free_stat_priv(evsel);
		perf_evsel__free_counts(evsel);
		perf_evsel__free_prev_raw_counts(evsel);
	}
}

static int perf_evlist__alloc_stats(struct perf_evlist *evlist, bool alloc_raw)
{
	struct perf_evsel *evsel;

	list_for_each_entry(evsel, &evlist->entries, node) {
		if (perf_evsel__alloc_stat_priv(evsel) < 0 ||
		    perf_evsel__alloc_counts(evsel, perf_evsel__nr_cpus(evsel)) < 0 ||
		    (alloc_raw && perf_evsel__alloc_prev_raw_counts(evsel) < 0))
			goto out_free;
	}

	return 0;

out_free:
	perf_evlist__free_stats(evlist);
	return -1;
}

static struct stats runtime_nsecs_stats[MAX_NR_CPUS];
static struct stats runtime_cycles_stats[MAX_NR_CPUS];
static struct stats runtime_stalled_cycles_front_stats[MAX_NR_CPUS];
static struct stats runtime_stalled_cycles_back_stats[MAX_NR_CPUS];
static struct stats runtime_branches_stats[MAX_NR_CPUS];
static struct stats runtime_cacherefs_stats[MAX_NR_CPUS];
static struct stats runtime_l1_dcache_stats[MAX_NR_CPUS];
static struct stats runtime_l1_icache_stats[MAX_NR_CPUS];
static struct stats runtime_ll_cache_stats[MAX_NR_CPUS];
static struct stats runtime_itlb_cache_stats[MAX_NR_CPUS];
static struct stats runtime_dtlb_cache_stats[MAX_NR_CPUS];
static struct stats walltime_nsecs_stats;

static void perf_stat__reset_stats(struct perf_evlist *evlist)
{
	struct perf_evsel *evsel;

	list_for_each_entry(evsel, &evlist->entries, node) {
		perf_evsel__reset_stat_priv(evsel);
		perf_evsel__reset_counts(evsel, perf_evsel__nr_cpus(evsel));
	}

	memset(runtime_nsecs_stats, 0, sizeof(runtime_nsecs_stats));
	memset(runtime_cycles_stats, 0, sizeof(runtime_cycles_stats));
	memset(runtime_stalled_cycles_front_stats, 0, sizeof(runtime_stalled_cycles_front_stats));
	memset(runtime_stalled_cycles_back_stats, 0, sizeof(runtime_stalled_cycles_back_stats));
	memset(runtime_branches_stats, 0, sizeof(runtime_branches_stats));
	memset(runtime_cacherefs_stats, 0, sizeof(runtime_cacherefs_stats));
	memset(runtime_l1_dcache_stats, 0, sizeof(runtime_l1_dcache_stats));
	memset(runtime_l1_icache_stats, 0, sizeof(runtime_l1_icache_stats));
	memset(runtime_ll_cache_stats, 0, sizeof(runtime_ll_cache_stats));
	memset(runtime_itlb_cache_stats, 0, sizeof(runtime_itlb_cache_stats));
	memset(runtime_dtlb_cache_stats, 0, sizeof(runtime_dtlb_cache_stats));
	memset(&walltime_nsecs_stats, 0, sizeof(walltime_nsecs_stats));
}

static int create_perf_stat_counter(struct perf_evsel *evsel)
{
	struct perf_event_attr *attr = &evsel->attr;

	if (scale)
		attr->read_format = PERF_FORMAT_TOTAL_TIME_ENABLED |
				    PERF_FORMAT_TOTAL_TIME_RUNNING;

	attr->inherit = !no_inherit;

	if (perf_target__has_cpu(&target))
		return perf_evsel__open_per_cpu(evsel, perf_evsel__cpus(evsel));

	if (!perf_target__has_task(&target) &&
	    perf_evsel__is_group_leader(evsel)) {
		attr->disabled = 1;
		if (!initial_delay)
			attr->enable_on_exec = 1;
	}

	return perf_evsel__open_per_thread(evsel, evsel_list->threads);
}

/*
 * Does the counter have nsecs as a unit?
 */
static inline int nsec_counter(struct perf_evsel *evsel)
{
	if (perf_evsel__match(evsel, SOFTWARE, SW_CPU_CLOCK) ||
	    perf_evsel__match(evsel, SOFTWARE, SW_TASK_CLOCK))
		return 1;

	return 0;
}

/*
 * Update various tracking values we maintain to print
 * more semantic information such as miss/hit ratios,
 * instruction rates, etc:
 */
static void update_shadow_stats(struct perf_evsel *counter, u64 *count)
{
	if (perf_evsel__match(counter, SOFTWARE, SW_TASK_CLOCK))
		update_stats(&runtime_nsecs_stats[0], count[0]);
	else if (perf_evsel__match(counter, HARDWARE, HW_CPU_CYCLES))
		update_stats(&runtime_cycles_stats[0], count[0]);
	else if (perf_evsel__match(counter, HARDWARE, HW_STALLED_CYCLES_FRONTEND))
		update_stats(&runtime_stalled_cycles_front_stats[0], count[0]);
	else if (perf_evsel__match(counter, HARDWARE, HW_STALLED_CYCLES_BACKEND))
		update_stats(&runtime_stalled_cycles_back_stats[0], count[0]);
	else if (perf_evsel__match(counter, HARDWARE, HW_BRANCH_INSTRUCTIONS))
		update_stats(&runtime_branches_stats[0], count[0]);
	else if (perf_evsel__match(counter, HARDWARE, HW_CACHE_REFERENCES))
		update_stats(&runtime_cacherefs_stats[0], count[0]);
	else if (perf_evsel__match(counter, HW_CACHE, HW_CACHE_L1D))
		update_stats(&runtime_l1_dcache_stats[0], count[0]);
	else if (perf_evsel__match(counter, HW_CACHE, HW_CACHE_L1I))
		update_stats(&runtime_l1_icache_stats[0], count[0]);
	else if (perf_evsel__match(counter, HW_CACHE, HW_CACHE_LL))
		update_stats(&runtime_ll_cache_stats[0], count[0]);
	else if (perf_evsel__match(counter, HW_CACHE, HW_CACHE_DTLB))
		update_stats(&runtime_dtlb_cache_stats[0], count[0]);
	else if (perf_evsel__match(counter, HW_CACHE, HW_CACHE_ITLB))
		update_stats(&runtime_itlb_cache_stats[0], count[0]);
}

/*
 * Read out the results of a single counter:
 * aggregate counts across CPUs in system-wide mode
 */
static int read_counter_aggr(struct perf_evsel *counter)
{
	struct perf_stat *ps = counter->priv;
	u64 *count = counter->counts->aggr.values;
	int i;

	if (__perf_evsel__read(counter, perf_evsel__nr_cpus(counter),
			       thread_map__nr(evsel_list->threads), scale) < 0)
		return -1;

	for (i = 0; i < 3; i++)
		update_stats(&ps->res_stats[i], count[i]);

	if (verbose) {
		fprintf(output, "%s: %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
			perf_evsel__name(counter), count[0], count[1], count[2]);
	}

	/*
	 * Save the full runtime - to allow normalization during printout:
	 */
	update_shadow_stats(counter, count);

	return 0;
}

/*
 * Read out the results of a single counter:
 * do not aggregate counts across CPUs in system-wide mode
 */
static int read_counter(struct perf_evsel *counter)
{
	u64 *count;
	int cpu;

	for (cpu = 0; cpu < perf_evsel__nr_cpus(counter); cpu++) {
		if (__perf_evsel__read_on_cpu(counter, cpu, 0, scale) < 0)
			return -1;

		count = counter->counts->cpu[cpu].values;

		update_shadow_stats(counter, count);
	}

	return 0;
}

static void print_interval(void)
{
	static int num_print_interval;
	struct perf_evsel *counter;
	struct perf_stat *ps;
	struct timespec ts, rs;
	char prefix[64];

	if (aggr_mode == AGGR_GLOBAL) {
		list_for_each_entry(counter, &evsel_list->entries, node) {
			ps = counter->priv;
			memset(ps->res_stats, 0, sizeof(ps->res_stats));
			read_counter_aggr(counter);
		}
	} else	{
		list_for_each_entry(counter, &evsel_list->entries, node) {
			ps = counter->priv;
			memset(ps->res_stats, 0, sizeof(ps->res_stats));
			read_counter(counter);
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &ts);
	diff_timespec(&rs, &ts, &ref_time);
	sprintf(prefix, "%6lu.%09lu%s", rs.tv_sec, rs.tv_nsec, csv_sep);

	if (num_print_interval == 0 && !csv_output) {
		switch (aggr_mode) {
		case AGGR_SOCKET:
			fprintf(output, "#           time socket cpus             counts events\n");
			break;
		case AGGR_CORE:
			fprintf(output, "#           time core         cpus             counts events\n");
			break;
		case AGGR_NONE:
			fprintf(output, "#           time CPU                 counts events\n");
			break;
		case AGGR_GLOBAL:
		default:
			fprintf(output, "#           time             counts events\n");
		}
	}

	if (++num_print_interval == 25)
		num_print_interval = 0;

	switch (aggr_mode) {
	case AGGR_CORE:
	case AGGR_SOCKET:
		print_aggr(prefix);
		break;
	case AGGR_NONE:
		list_for_each_entry(counter, &evsel_list->entries, node)
			print_counter(counter, prefix);
		break;
	case AGGR_GLOBAL:
	default:
		list_for_each_entry(counter, &evsel_list->entries, node)
			print_counter_aggr(counter, prefix);
	}

	fflush(output);
}

static void handle_initial_delay(void)
{
	struct perf_evsel *counter;

	if (initial_delay) {
		const int ncpus = cpu_map__nr(evsel_list->cpus),
			nthreads = thread_map__nr(evsel_list->threads);

		usleep(initial_delay * 1000);
		list_for_each_entry(counter, &evsel_list->entries, node)
			perf_evsel__enable(counter, ncpus, nthreads);
	}
}

static int __run_perf_stat(int argc, const char **argv)
{
	char msg[512];
	unsigned long long t0, t1;
	struct perf_evsel *counter;
	struct timespec ts;
	int status = 0;
	const bool forks = (argc > 0);

	if (interval) {
		ts.tv_sec  = interval / 1000;
		ts.tv_nsec = (interval % 1000) * 1000000;
	} else {
		ts.tv_sec  = 1;
		ts.tv_nsec = 0;
	}

	if (forks) {
		if (perf_evlist__prepare_workload(evsel_list, &target, argv,
						  false, false) < 0) {
			perror("failed to prepare workload");
			return -1;
		}
		child_pid = evsel_list->workload.pid;
	}

	if (group)
		perf_evlist__set_leader(evsel_list);

	list_for_each_entry(counter, &evsel_list->entries, node) {
		if (create_perf_stat_counter(counter) < 0) {
			/*
			 * PPC returns ENXIO for HW counters until 2.6.37
			 * (behavior changed with commit b0a873e).
			 */
			if (errno == EINVAL || errno == ENOSYS ||
			    errno == ENOENT || errno == EOPNOTSUPP ||
			    errno == ENXIO) {
				if (verbose)
					ui__warning("%s event is not supported by the kernel.\n",
						    perf_evsel__name(counter));
				counter->supported = false;
				continue;
			}

			perf_evsel__open_strerror(counter, &target,
						  errno, msg, sizeof(msg));
			ui__error("%s\n", msg);

			if (child_pid != -1)
				kill(child_pid, SIGTERM);

			return -1;
		}
		counter->supported = true;
	}

	if (perf_evlist__apply_filters(evsel_list)) {
		error("failed to set filter with %d (%s)\n", errno,
			strerror(errno));
		return -1;
	}

	/*
	 * Enable counters and exec the command:
	 */
	t0 = rdclock();
	clock_gettime(CLOCK_MONOTONIC, &ref_time);

	if (forks) {
		perf_evlist__start_workload(evsel_list);
		handle_initial_delay();

		if (interval) {
			while (!waitpid(child_pid, &status, WNOHANG)) {
				nanosleep(&ts, NULL);
				print_interval();
			}
		}
		wait(&status);
		if (WIFSIGNALED(status))
			psignal(WTERMSIG(status), argv[0]);
	} else {
		handle_initial_delay();
		while (!done) {
			nanosleep(&ts, NULL);
			if (interval)
				print_interval();
		}
	}

	t1 = rdclock();

	update_stats(&walltime_nsecs_stats, t1 - t0);

	if (aggr_mode == AGGR_GLOBAL) {
		list_for_each_entry(counter, &evsel_list->entries, node) {
			read_counter_aggr(counter);
			perf_evsel__close_fd(counter, perf_evsel__nr_cpus(counter),
					     thread_map__nr(evsel_list->threads));
		}
	} else {
		list_for_each_entry(counter, &evsel_list->entries, node) {
			read_counter(counter);
			perf_evsel__close_fd(counter, perf_evsel__nr_cpus(counter), 1);
		}
	}

	return WEXITSTATUS(status);
}

static int run_perf_stat(int argc __maybe_unused, const char **argv)
{
	int ret;

	if (pre_cmd) {
		ret = system(pre_cmd);
		if (ret)
			return ret;
	}

	if (sync_run)
		sync();

	ret = __run_perf_stat(argc, argv);
	if (ret)
		return ret;

	if (post_cmd) {
		ret = system(post_cmd);
		if (ret)
			return ret;
	}

	return ret;
}

static void print_noise_pct(double total, double avg)
{
	double pct = rel_stddev_stats(total, avg);

	if (csv_output)
		fprintf(output, "%s%.2f%%", csv_sep, pct);
	else if (pct)
		fprintf(output, "  ( +-%6.2f%% )", pct);
}

static void print_noise(struct perf_evsel *evsel, double avg)
{
	struct perf_stat *ps;

	if (run_count == 1)
		return;

	ps = evsel->priv;
	print_noise_pct(stddev_stats(&ps->res_stats[0]), avg);
}

static void aggr_printout(struct perf_evsel *evsel, int id, int nr)
{
	switch (aggr_mode) {
	case AGGR_CORE:
		fprintf(output, "S%d-C%*d%s%*d%s",
			cpu_map__id_to_socket(id),
			csv_output ? 0 : -8,
			cpu_map__id_to_cpu(id),
			csv_sep,
			csv_output ? 0 : 4,
			nr,
			csv_sep);
		break;
	case AGGR_SOCKET:
		fprintf(output, "S%*d%s%*d%s",
			csv_output ? 0 : -5,
			id,
			csv_sep,
			csv_output ? 0 : 4,
			nr,
			csv_sep);
			break;
	case AGGR_NONE:
		fprintf(output, "CPU%*d%s",
			csv_output ? 0 : -4,
			perf_evsel__cpus(evsel)->map[id], csv_sep);
		break;
	case AGGR_GLOBAL:
	default:
		break;
	}
}

static void nsec_printout(int cpu, int nr, struct perf_evsel *evsel, double avg)
{
	double msecs = avg / 1e6;
	const char *fmt = csv_output ? "%.6f%s%s" : "%18.6f%s%-25s";

	aggr_printout(evsel, cpu, nr);

	fprintf(output, fmt, msecs, csv_sep, perf_evsel__name(evsel));

	if (evsel->cgrp)
		fprintf(output, "%s%s", csv_sep, evsel->cgrp->name);

	if (csv_output || interval)
		return;

	if (perf_evsel__match(evsel, SOFTWARE, SW_TASK_CLOCK))
		fprintf(output, " # %8.3f CPUs utilized          ",
			avg / avg_stats(&walltime_nsecs_stats));
	else
		fprintf(output, "                                   ");
}

/* used for get_ratio_color() */
enum grc_type {
	GRC_STALLED_CYCLES_FE,
	GRC_STALLED_CYCLES_BE,
	GRC_CACHE_MISSES,
	GRC_MAX_NR
};

static const char *get_ratio_color(enum grc_type type, double ratio)
{
	static const double grc_table[GRC_MAX_NR][3] = {
		[GRC_STALLED_CYCLES_FE] = { 50.0, 30.0, 10.0 },
		[GRC_STALLED_CYCLES_BE] = { 75.0, 50.0, 20.0 },
		[GRC_CACHE_MISSES] 	= { 20.0, 10.0, 5.0 },
	};
	const char *color = PERF_COLOR_NORMAL;

	if (ratio > grc_table[type][0])
		color = PERF_COLOR_RED;
	else if (ratio > grc_table[type][1])
		color = PERF_COLOR_MAGENTA;
	else if (ratio > grc_table[type][2])
		color = PERF_COLOR_YELLOW;

	return color;
}

static void print_stalled_cycles_frontend(int cpu,
					  struct perf_evsel *evsel
					  __maybe_unused, double avg)
{
	double total, ratio = 0.0;
	const char *color;

	total = avg_stats(&runtime_cycles_stats[cpu]);

	if (total)
		ratio = avg / total * 100.0;

	color = get_ratio_color(GRC_STALLED_CYCLES_FE, ratio);

	fprintf(output, " #  ");
	color_fprintf(output, color, "%6.2f%%", ratio);
	fprintf(output, " frontend cycles idle   ");
}

static void print_stalled_cycles_backend(int cpu,
					 struct perf_evsel *evsel
					 __maybe_unused, double avg)
{
	double total, ratio = 0.0;
	const char *color;

	total = avg_stats(&runtime_cycles_stats[cpu]);

	if (total)
		ratio = avg / total * 100.0;

	color = get_ratio_color(GRC_STALLED_CYCLES_BE, ratio);

	fprintf(output, " #  ");
	color_fprintf(output, color, "%6.2f%%", ratio);
	fprintf(output, " backend  cycles idle   ");
}

static void print_branch_misses(int cpu,
				struct perf_evsel *evsel __maybe_unused,
				double avg)
{
	double total, ratio = 0.0;
	const char *color;

	total = avg_stats(&runtime_branches_stats[cpu]);

	if (total)
		ratio = avg / total * 100.0;

	color = get_ratio_color(GRC_CACHE_MISSES, ratio);

	fprintf(output, " #  ");
	color_fprintf(output, color, "%6.2f%%", ratio);
	fprintf(output, " of all branches        ");
}

static void print_l1_dcache_misses(int cpu,
				   struct perf_evsel *evsel __maybe_unused,
				   double avg)
{
	double total, ratio = 0.0;
	const char *color;

	total = avg_stats(&runtime_l1_dcache_stats[cpu]);

	if (total)
		ratio = avg / total * 100.0;

	color = get_ratio_color(GRC_CACHE_MISSES, ratio);

	fprintf(output, " #  ");
	color_fprintf(output, color, "%6.2f%%", ratio);
	fprintf(output, " of all L1-dcache hits  ");
}

static void print_l1_icache_misses(int cpu,
				   struct perf_evsel *evsel __maybe_unused,
				   double avg)
{
	double total, ratio = 0.0;
	const char *color;

	total = avg_stats(&runtime_l1_icache_stats[cpu]);

	if (total)
		ratio = avg / total * 100.0;

	color = get_ratio_color(GRC_CACHE_MISSES, ratio);

	fprintf(output, " #  ");
	color_fprintf(output, color, "%6.2f%%", ratio);
	fprintf(output, " of all L1-icache hits  ");
}

static void print_dtlb_cache_misses(int cpu,
				    struct perf_evsel *evsel __maybe_unused,
				    double avg)
{
	double total, ratio = 0.0;
	const char *color;

	total = avg_stats(&runtime_dtlb_cache_stats[cpu]);

	if (total)
		ratio = avg / total * 100.0;

	color = get_ratio_color(GRC_CACHE_MISSES, ratio);

	fprintf(output, " #  ");
	color_fprintf(output, color, "%6.2f%%", ratio);
	fprintf(output, " of all dTLB cache hits ");
}

static void print_itlb_cache_misses(int cpu,
				    struct perf_evsel *evsel __maybe_unused,
				    double avg)
{
	double total, ratio = 0.0;
	const char *color;

	total = avg_stats(&runtime_itlb_cache_stats[cpu]);

	if (total)
		ratio = avg / total * 100.0;

	color = get_ratio_color(GRC_CACHE_MISSES, ratio);

	fprintf(output, " #  ");
	color_fprintf(output, color, "%6.2f%%", ratio);
	fprintf(output, " of all iTLB cache hits ");
}

static void print_ll_cache_misses(int cpu,
				  struct perf_evsel *evsel __maybe_unused,
				  double avg)
{
	double total, ratio = 0.0;
	const char *color;

	total = avg_stats(&runtime_ll_cache_stats[cpu]);

	if (total)
		ratio = avg / total * 100.0;

	color = get_ratio_color(GRC_CACHE_MISSES, ratio);

	fprintf(output, " #  ");
	color_fprintf(output, color, "%6.2f%%", ratio);
	fprintf(output, " of all LL-cache hits   ");
}

static void abs_printout(int cpu, int nr, struct perf_evsel *evsel, double avg)
{
	double total, ratio = 0.0;
	const char *fmt;

	if (csv_output)
		fmt = "%.0f%s%s";
	else if (big_num)
		fmt = "%'18.0f%s%-25s";
	else
		fmt = "%18.0f%s%-25s";

	aggr_printout(evsel, cpu, nr);

	if (aggr_mode == AGGR_GLOBAL)
		cpu = 0;

	fprintf(output, fmt, avg, csv_sep, perf_evsel__name(evsel));

	if (evsel->cgrp)
		fprintf(output, "%s%s", csv_sep, evsel->cgrp->name);

	if (csv_output || interval)
		return;

	if (perf_evsel__match(evsel, HARDWARE, HW_INSTRUCTIONS)) {
		total = avg_stats(&runtime_cycles_stats[cpu]);
		if (total)
			ratio = avg / total;

		fprintf(output, " #   %5.2f  insns per cycle        ", ratio);

		total = avg_stats(&runtime_stalled_cycles_front_stats[cpu]);
		total = max(total, avg_stats(&runtime_stalled_cycles_back_stats[cpu]));

		if (total && avg) {
			ratio = total / avg;
			fprintf(output, "\n                                             #   %5.2f  stalled cycles per insn", ratio);
		}

	} else if (perf_evsel__match(evsel, HARDWARE, HW_BRANCH_MISSES) &&
			runtime_branches_stats[cpu].n != 0) {
		print_branch_misses(cpu, evsel, avg);
	} else if (
		evsel->attr.type == PERF_TYPE_HW_CACHE &&
		evsel->attr.config ==  ( PERF_COUNT_HW_CACHE_L1D |
					((PERF_COUNT_HW_CACHE_OP_READ) << 8) |
					((PERF_COUNT_HW_CACHE_RESULT_MISS) << 16)) &&
			runtime_l1_dcache_stats[cpu].n != 0) {
		print_l1_dcache_misses(cpu, evsel, avg);
	} else if (
		evsel->attr.type == PERF_TYPE_HW_CACHE &&
		evsel->attr.config ==  ( PERF_COUNT_HW_CACHE_L1I |
					((PERF_COUNT_HW_CACHE_OP_READ) << 8) |
					((PERF_COUNT_HW_CACHE_RESULT_MISS) << 16)) &&
			runtime_l1_icache_stats[cpu].n != 0) {
		print_l1_icache_misses(cpu, evsel, avg);
	} else if (
		evsel->attr.type == PERF_TYPE_HW_CACHE &&
		evsel->attr.config ==  ( PERF_COUNT_HW_CACHE_DTLB |
					((PERF_COUNT_HW_CACHE_OP_READ) << 8) |
					((PERF_COUNT_HW_CACHE_RESULT_MISS) << 16)) &&
			runtime_dtlb_cache_stats[cpu].n != 0) {
		print_dtlb_cache_misses(cpu, evsel, avg);
	} else if (
		evsel->attr.type == PERF_TYPE_HW_CACHE &&
		evsel->attr.config ==  ( PERF_COUNT_HW_CACHE_ITLB |
					((PERF_COUNT_HW_CACHE_OP_READ) << 8) |
					((PERF_COUNT_HW_CACHE_RESULT_MISS) << 16)) &&
			runtime_itlb_cache_stats[cpu].n != 0) {
		print_itlb_cache_misses(cpu, evsel, avg);
	} else if (
		evsel->attr.type == PERF_TYPE_HW_CACHE &&
		evsel->attr.config ==  ( PERF_COUNT_HW_CACHE_LL |
					((PERF_COUNT_HW_CACHE_OP_READ) << 8) |
					((PERF_COUNT_HW_CACHE_RESULT_MISS) << 16)) &&
			runtime_ll_cache_stats[cpu].n != 0) {
		print_ll_cache_misses(cpu, evsel, avg);
	} else if (perf_evsel__match(evsel, HARDWARE, HW_CACHE_MISSES) &&
			runtime_cacherefs_stats[cpu].n != 0) {
		total = avg_stats(&runtime_cacherefs_stats[cpu]);

		if (total)
			ratio = avg * 100 / total;

		fprintf(output, " # %8.3f %% of all cache refs    ", ratio);

	} else if (perf_evsel__match(evsel, HARDWARE, HW_STALLED_CYCLES_FRONTEND)) {
		print_stalled_cycles_frontend(cpu, evsel, avg);
	} else if (perf_evsel__match(evsel, HARDWARE, HW_STALLED_CYCLES_BACKEND)) {
		print_stalled_cycles_backend(cpu, evsel, avg);
	} else if (perf_evsel__match(evsel, HARDWARE, HW_CPU_CYCLES)) {
		total = avg_stats(&runtime_nsecs_stats[cpu]);

		if (total)
			ratio = 1.0 * avg / total;

		fprintf(output, " # %8.3f GHz                    ", ratio);
	} else if (runtime_nsecs_stats[cpu].n != 0) {
		char unit = 'M';

		total = avg_stats(&runtime_nsecs_stats[cpu]);

		if (total)
			ratio = 1000.0 * avg / total;
		if (ratio < 0.001) {
			ratio *= 1000;
			unit = 'K';
		}

		fprintf(output, " # %8.3f %c/sec                  ", ratio, unit);
	} else {
		fprintf(output, "                                   ");
	}
}

static void print_aggr(char *prefix)
{
	struct perf_evsel *counter;
	int cpu, cpu2, s, s2, id, nr;
	u64 ena, run, val;

	if (!(aggr_map || aggr_get_id))
		return;

	for (s = 0; s < aggr_map->nr; s++) {
		id = aggr_map->map[s];
		list_for_each_entry(counter, &evsel_list->entries, node) {
			val = ena = run = 0;
			nr = 0;
			for (cpu = 0; cpu < perf_evsel__nr_cpus(counter); cpu++) {
				cpu2 = perf_evsel__cpus(counter)->map[cpu];
				s2 = aggr_get_id(evsel_list->cpus, cpu2);
				if (s2 != id)
					continue;
				val += counter->counts->cpu[cpu].val;
				ena += counter->counts->cpu[cpu].ena;
				run += counter->counts->cpu[cpu].run;
				nr++;
			}
			if (prefix)
				fprintf(output, "%s", prefix);

			if (run == 0 || ena == 0) {
				aggr_printout(counter, id, nr);

				fprintf(output, "%*s%s%*s",
					csv_output ? 0 : 18,
					counter->supported ? CNTR_NOT_COUNTED : CNTR_NOT_SUPPORTED,
					csv_sep,
					csv_output ? 0 : -24,
					perf_evsel__name(counter));

				if (counter->cgrp)
					fprintf(output, "%s%s",
						csv_sep, counter->cgrp->name);

				fputc('\n', output);
				continue;
			}

			if (nsec_counter(counter))
				nsec_printout(id, nr, counter, val);
			else
				abs_printout(id, nr, counter, val);

			if (!csv_output) {
				print_noise(counter, 1.0);

				if (run != ena)
					fprintf(output, "  (%.2f%%)",
						100.0 * run / ena);
			}
			fputc('\n', output);
		}
	}
}

/*
 * Print out the results of a single counter:
 * aggregated counts in system-wide mode
 */
static void print_counter_aggr(struct perf_evsel *counter, char *prefix)
{
	struct perf_stat *ps = counter->priv;
	double avg = avg_stats(&ps->res_stats[0]);
	int scaled = counter->counts->scaled;

	if (prefix)
		fprintf(output, "%s", prefix);

	if (scaled == -1) {
		fprintf(output, "%*s%s%*s",
			csv_output ? 0 : 18,
			counter->supported ? CNTR_NOT_COUNTED : CNTR_NOT_SUPPORTED,
			csv_sep,
			csv_output ? 0 : -24,
			perf_evsel__name(counter));

		if (counter->cgrp)
			fprintf(output, "%s%s", csv_sep, counter->cgrp->name);

		fputc('\n', output);
		return;
	}

	if (nsec_counter(counter))
		nsec_printout(-1, 0, counter, avg);
	else
		abs_printout(-1, 0, counter, avg);

	print_noise(counter, avg);

	if (csv_output) {
		fputc('\n', output);
		return;
	}

	if (scaled) {
		double avg_enabled, avg_running;

		avg_enabled = avg_stats(&ps->res_stats[1]);
		avg_running = avg_stats(&ps->res_stats[2]);

		fprintf(output, " [%5.2f%%]", 100 * avg_running / avg_enabled);
	}
	fprintf(output, "\n");
}

/*
 * Print out the results of a single counter:
 * does not use aggregated count in system-wide
 */
static void print_counter(struct perf_evsel *counter, char *prefix)
{
	u64 ena, run, val;
	int cpu;

	for (cpu = 0; cpu < perf_evsel__nr_cpus(counter); cpu++) {
		val = counter->counts->cpu[cpu].val;
		ena = counter->counts->cpu[cpu].ena;
		run = counter->counts->cpu[cpu].run;

		if (prefix)
			fprintf(output, "%s", prefix);

		if (run == 0 || ena == 0) {
			fprintf(output, "CPU%*d%s%*s%s%*s",
				csv_output ? 0 : -4,
				perf_evsel__cpus(counter)->map[cpu], csv_sep,
				csv_output ? 0 : 18,
				counter->supported ? CNTR_NOT_COUNTED : CNTR_NOT_SUPPORTED,
				csv_sep,
				csv_output ? 0 : -24,
				perf_evsel__name(counter));

			if (counter->cgrp)
				fprintf(output, "%s%s",
					csv_sep, counter->cgrp->name);

			fputc('\n', output);
			continue;
		}

		if (nsec_counter(counter))
			nsec_printout(cpu, 0, counter, val);
		else
			abs_printout(cpu, 0, counter, val);

		if (!csv_output) {
			print_noise(counter, 1.0);

			if (run != ena)
				fprintf(output, "  (%.2f%%)",
					100.0 * run / ena);
		}
		fputc('\n', output);
	}
}

static void print_stat(int argc, const char **argv)
{
	struct perf_evsel *counter;
	int i;

	fflush(stdout);

	if (!csv_output) {
		fprintf(output, "\n");
		fprintf(output, " Performance counter stats for ");
		if (!perf_target__has_task(&target)) {
			fprintf(output, "\'%s", argv[0]);
			for (i = 1; i < argc; i++)
				fprintf(output, " %s", argv[i]);
		} else if (target.pid)
			fprintf(output, "process id \'%s", target.pid);
		else
			fprintf(output, "thread id \'%s", target.tid);

		fprintf(output, "\'");
		if (run_count > 1)
			fprintf(output, " (%d runs)", run_count);
		fprintf(output, ":\n\n");
	}

	switch (aggr_mode) {
	case AGGR_CORE:
	case AGGR_SOCKET:
		print_aggr(NULL);
		break;
	case AGGR_GLOBAL:
		list_for_each_entry(counter, &evsel_list->entries, node)
			print_counter_aggr(counter, NULL);
		break;
	case AGGR_NONE:
		list_for_each_entry(counter, &evsel_list->entries, node)
			print_counter(counter, NULL);
		break;
	default:
		break;
	}

	if (!csv_output) {
		if (!null_run)
			fprintf(output, "\n");
		fprintf(output, " %17.9f seconds time elapsed",
				avg_stats(&walltime_nsecs_stats)/1e9);
		if (run_count > 1) {
			fprintf(output, "                                        ");
			print_noise_pct(stddev_stats(&walltime_nsecs_stats),
					avg_stats(&walltime_nsecs_stats));
		}
		fprintf(output, "\n\n");
	}
}

static volatile int signr = -1;

static void skip_signal(int signo)
{
	if ((child_pid == -1) || interval)
		done = 1;

	signr = signo;
	/*
	 * render child_pid harmless
	 * won't send SIGTERM to a random
	 * process in case of race condition
	 * and fast PID recycling
	 */
	child_pid = -1;
}

static void sig_atexit(void)
{
	sigset_t set, oset;

	/*
	 * avoid race condition with SIGCHLD handler
	 * in skip_signal() which is modifying child_pid
	 * goal is to avoid send SIGTERM to a random
	 * process
	 */
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &set, &oset);

	if (child_pid != -1)
		kill(child_pid, SIGTERM);

	sigprocmask(SIG_SETMASK, &oset, NULL);

	if (signr == -1)
		return;

	signal(signr, SIG_DFL);
	kill(getpid(), signr);
}

static int stat__set_big_num(const struct option *opt __maybe_unused,
			     const char *s __maybe_unused, int unset)
{
	big_num_opt = unset ? 0 : 1;
	return 0;
}

static int perf_stat_init_aggr_mode(void)
{
	switch (aggr_mode) {
	case AGGR_SOCKET:
		if (cpu_map__build_socket_map(evsel_list->cpus, &aggr_map)) {
			perror("cannot build socket map");
			return -1;
		}
		aggr_get_id = cpu_map__get_socket;
		break;
	case AGGR_CORE:
		if (cpu_map__build_core_map(evsel_list->cpus, &aggr_map)) {
			perror("cannot build core map");
			return -1;
		}
		aggr_get_id = cpu_map__get_core;
		break;
	case AGGR_NONE:
	case AGGR_GLOBAL:
	default:
		break;
	}
	return 0;
}


/*
 * Add default attributes, if there were no attributes specified or
 * if -d/--detailed, -d -d or -d -d -d is used:
 */
static int add_default_attributes(void)
{
	struct perf_event_attr default_attrs[] = {

  { .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_TASK_CLOCK		},
  { .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_CONTEXT_SWITCHES	},
  { .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_CPU_MIGRATIONS		},
  { .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_PAGE_FAULTS		},

  { .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_CPU_CYCLES		},
  { .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND	},
  { .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_STALLED_CYCLES_BACKEND	},
  { .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_INSTRUCTIONS		},
  { .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS	},
  { .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_BRANCH_MISSES		},

};

/*
 * Detailed stats (-d), covering the L1 and last level data caches:
 */
	struct perf_event_attr detailed_attrs[] = {

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_L1D		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_ACCESS	<< 16)				},

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_L1D		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_MISS	<< 16)				},

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_LL			<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_ACCESS	<< 16)				},

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_LL			<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_MISS	<< 16)				},
};

/*
 * Very detailed stats (-d -d), covering the instruction cache and the TLB caches:
 */
	struct perf_event_attr very_detailed_attrs[] = {

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_L1I		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_ACCESS	<< 16)				},

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_L1I		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_MISS	<< 16)				},

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_DTLB		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_ACCESS	<< 16)				},

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_DTLB		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_MISS	<< 16)				},

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_ITLB		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_ACCESS	<< 16)				},

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_ITLB		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_READ		<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_MISS	<< 16)				},

};

/*
 * Very, very detailed stats (-d -d -d), adding prefetch events:
 */
	struct perf_event_attr very_very_detailed_attrs[] = {

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_L1D		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_PREFETCH	<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_ACCESS	<< 16)				},

  { .type = PERF_TYPE_HW_CACHE,
    .config =
	 PERF_COUNT_HW_CACHE_L1D		<<  0  |
	(PERF_COUNT_HW_CACHE_OP_PREFETCH	<<  8) |
	(PERF_COUNT_HW_CACHE_RESULT_MISS	<< 16)				},
};

	/* Set attrs if no event is selected and !null_run: */
	if (null_run)
		return 0;

	if (!evsel_list->nr_entries) {
		if (perf_evlist__add_default_attrs(evsel_list, default_attrs) < 0)
			return -1;
	}

	/* Detailed events get appended to the event list: */

	if (detailed_run <  1)
		return 0;

	/* Append detailed run extra attributes: */
	if (perf_evlist__add_default_attrs(evsel_list, detailed_attrs) < 0)
		return -1;

	if (detailed_run < 2)
		return 0;

	/* Append very detailed run extra attributes: */
	if (perf_evlist__add_default_attrs(evsel_list, very_detailed_attrs) < 0)
		return -1;

	if (detailed_run < 3)
		return 0;

	/* Append very, very detailed run extra attributes: */
	return perf_evlist__add_default_attrs(evsel_list, very_very_detailed_attrs);
}

int cmd_stat(int argc, const char **argv, const char *prefix __maybe_unused)
{
	bool append_file = false;
	int output_fd = 0;
	const char *output_name	= NULL;
	const struct option options[] = {
	OPT_CALLBACK('e', "event", &evsel_list, "event",
		     "event selector. use 'perf list' to list available events",
		     parse_events_option),
	OPT_CALLBACK(0, "filter", &evsel_list, "filter",
		     "event filter", parse_filter),
	OPT_BOOLEAN('i', "no-inherit", &no_inherit,
		    "child tasks do not inherit counters"),
	OPT_STRING('p', "pid", &target.pid, "pid",
		   "stat events on existing process id"),
	OPT_STRING('t', "tid", &target.tid, "tid",
		   "stat events on existing thread id"),
	OPT_BOOLEAN('a', "all-cpus", &target.system_wide,
		    "system-wide collection from all CPUs"),
	OPT_BOOLEAN('g', "group", &group,
		    "put the counters into a counter group"),
	OPT_BOOLEAN('c', "scale", &scale, "scale/normalize counters"),
	OPT_INCR('v', "verbose", &verbose,
		    "be more verbose (show counter open errors, etc)"),
	OPT_INTEGER('r', "repeat", &run_count,
		    "repeat command and print average + stddev (max: 100, forever: 0)"),
	OPT_BOOLEAN('n', "null", &null_run,
		    "null run - dont start any counters"),
	OPT_INCR('d', "detailed", &detailed_run,
		    "detailed run - start a lot of events"),
	OPT_BOOLEAN('S', "sync", &sync_run,
		    "call sync() before starting a run"),
	OPT_CALLBACK_NOOPT('B', "big-num", NULL, NULL, 
			   "print large numbers with thousands\' separators",
			   stat__set_big_num),
	OPT_STRING('C', "cpu", &target.cpu_list, "cpu",
		    "list of cpus to monitor in system-wide"),
	OPT_SET_UINT('A', "no-aggr", &aggr_mode,
		    "disable CPU count aggregation", AGGR_NONE),
	OPT_STRING('x', "field-separator", &csv_sep, "separator",
		   "print counts with custom separator"),
	OPT_CALLBACK('G', "cgroup", &evsel_list, "name",
		     "monitor event in cgroup name only", parse_cgroups),
	OPT_STRING('o', "output", &output_name, "file", "output file name"),
	OPT_BOOLEAN(0, "append", &append_file, "append to the output file"),
	OPT_INTEGER(0, "log-fd", &output_fd,
		    "log output to fd, instead of stderr"),
	OPT_STRING(0, "pre", &pre_cmd, "command",
			"command to run prior to the measured command"),
	OPT_STRING(0, "post", &post_cmd, "command",
			"command to run after to the measured command"),
	OPT_UINTEGER('I', "interval-print", &interval,
		    "print counts at regular interval in ms (>= 100)"),
	OPT_SET_UINT(0, "per-socket", &aggr_mode,
		     "aggregate counts per processor socket", AGGR_SOCKET),
	OPT_SET_UINT(0, "per-core", &aggr_mode,
		     "aggregate counts per physical processor core", AGGR_CORE),
	OPT_UINTEGER('D', "delay", &initial_delay,
		     "ms to wait before starting measurement after program start"),
	OPT_END()
	};
	const char * const stat_usage[] = {
		"perf stat [<options>] [<command>]",
		NULL
	};
	int status = -ENOMEM, run_idx;
	const char *mode;

	setlocale(LC_ALL, "");

	evsel_list = perf_evlist__new();
	if (evsel_list == NULL)
		return -ENOMEM;

	argc = parse_options(argc, argv, options, stat_usage,
		PARSE_OPT_STOP_AT_NON_OPTION);

	output = stderr;
	if (output_name && strcmp(output_name, "-"))
		output = NULL;

	if (output_name && output_fd) {
		fprintf(stderr, "cannot use both --output and --log-fd\n");
		usage_with_options(stat_usage, options);
	}

	if (output_fd < 0) {
		fprintf(stderr, "argument to --log-fd must be a > 0\n");
		usage_with_options(stat_usage, options);
	}

	if (!output) {
		struct timespec tm;
		mode = append_file ? "a" : "w";

		output = fopen(output_name, mode);
		if (!output) {
			perror("failed to create output file");
			return -1;
		}
		clock_gettime(CLOCK_REALTIME, &tm);
		fprintf(output, "# started on %s\n", ctime(&tm.tv_sec));
	} else if (output_fd > 0) {
		mode = append_file ? "a" : "w";
		output = fdopen(output_fd, mode);
		if (!output) {
			perror("Failed opening logfd");
			return -errno;
		}
	}

	if (csv_sep) {
		csv_output = true;
		if (!strcmp(csv_sep, "\\t"))
			csv_sep = "\t";
	} else
		csv_sep = DEFAULT_SEPARATOR;

	/*
	 * let the spreadsheet do the pretty-printing
	 */
	if (csv_output) {
		/* User explicitly passed -B? */
		if (big_num_opt == 1) {
			fprintf(stderr, "-B option not supported with -x\n");
			usage_with_options(stat_usage, options);
		} else /* Nope, so disable big number formatting */
			big_num = false;
	} else if (big_num_opt == 0) /* User passed --no-big-num */
		big_num = false;

	if (!argc && !perf_target__has_task(&target))
		usage_with_options(stat_usage, options);
	if (run_count < 0) {
		usage_with_options(stat_usage, options);
	} else if (run_count == 0) {
		forever = true;
		run_count = 1;
	}

	/* no_aggr, cgroup are for system-wide only */
	if ((aggr_mode != AGGR_GLOBAL || nr_cgroups)
	     && !perf_target__has_cpu(&target)) {
		fprintf(stderr, "both cgroup and no-aggregation "
			"modes only available in system-wide mode\n");

		usage_with_options(stat_usage, options);
		return -1;
	}

	if (add_default_attributes())
		goto out;

	perf_target__validate(&target);

	if (perf_evlist__create_maps(evsel_list, &target) < 0) {
		if (perf_target__has_task(&target))
			pr_err("Problems finding threads of monitor\n");
		if (perf_target__has_cpu(&target))
			perror("failed to parse CPUs map");

		usage_with_options(stat_usage, options);
		return -1;
	}
	if (interval && interval < 100) {
		pr_err("print interval must be >= 100ms\n");
		usage_with_options(stat_usage, options);
		return -1;
	}

	if (perf_evlist__alloc_stats(evsel_list, interval))
		goto out_free_maps;

	if (perf_stat_init_aggr_mode())
		goto out;

	/*
	 * We dont want to block the signals - that would cause
	 * child tasks to inherit that and Ctrl-C would not work.
	 * What we want is for Ctrl-C to work in the exec()-ed
	 * task, but being ignored by perf stat itself:
	 */
	atexit(sig_atexit);
	if (!forever)
		signal(SIGINT,  skip_signal);
	signal(SIGCHLD, skip_signal);
	signal(SIGALRM, skip_signal);
	signal(SIGABRT, skip_signal);

	status = 0;
	for (run_idx = 0; forever || run_idx < run_count; run_idx++) {
		if (run_count != 1 && verbose)
			fprintf(output, "[ perf stat: executing run #%d ... ]\n",
				run_idx + 1);

		status = run_perf_stat(argc, argv);
		if (forever && status != -1) {
			print_stat(argc, argv);
			perf_stat__reset_stats(evsel_list);
		}
	}

	if (!forever && status != -1 && !interval)
		print_stat(argc, argv);

	perf_evlist__free_stats(evsel_list);
out_free_maps:
	perf_evlist__delete_maps(evsel_list);
out:
	perf_evlist__delete(evsel_list);
	return status;
}
