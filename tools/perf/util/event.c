#include <fikus/types.h>
#include "event.h"
#include "debug.h"
#include "machine.h"
#include "sort.h"
#include "string.h"
#include "strlist.h"
#include "thread.h"
#include "thread_map.h"

static const char *perf_event__names[] = {
	[0]					= "TOTAL",
	[PERF_RECORD_MMAP]			= "MMAP",
	[PERF_RECORD_MMAP2]			= "MMAP2",
	[PERF_RECORD_LOST]			= "LOST",
	[PERF_RECORD_COMM]			= "COMM",
	[PERF_RECORD_EXIT]			= "EXIT",
	[PERF_RECORD_THROTTLE]			= "THROTTLE",
	[PERF_RECORD_UNTHROTTLE]		= "UNTHROTTLE",
	[PERF_RECORD_FORK]			= "FORK",
	[PERF_RECORD_READ]			= "READ",
	[PERF_RECORD_SAMPLE]			= "SAMPLE",
	[PERF_RECORD_HEADER_ATTR]		= "ATTR",
	[PERF_RECORD_HEADER_EVENT_TYPE]		= "EVENT_TYPE",
	[PERF_RECORD_HEADER_TRACING_DATA]	= "TRACING_DATA",
	[PERF_RECORD_HEADER_BUILD_ID]		= "BUILD_ID",
	[PERF_RECORD_FINISHED_ROUND]		= "FINISHED_ROUND",
};

const char *perf_event__name(unsigned int id)
{
	if (id >= ARRAY_SIZE(perf_event__names))
		return "INVALID";
	if (!perf_event__names[id])
		return "UNKNOWN";
	return perf_event__names[id];
}

static struct perf_sample synth_sample = {
	.pid	   = -1,
	.tid	   = -1,
	.time	   = -1,
	.stream_id = -1,
	.cpu	   = -1,
	.period	   = 1,
};

static pid_t perf_event__get_comm_tgid(pid_t pid, char *comm, size_t len)
{
	char filename[PATH_MAX];
	char bf[BUFSIZ];
	FILE *fp;
	size_t size = 0;
	pid_t tgid = -1;

	snprintf(filename, sizeof(filename), "/proc/%d/status", pid);

	fp = fopen(filename, "r");
	if (fp == NULL) {
		pr_debug("couldn't open %s\n", filename);
		return 0;
	}

	while (!comm[0] || (tgid < 0)) {
		if (fgets(bf, sizeof(bf), fp) == NULL) {
			pr_warning("couldn't get COMM and pgid, malformed %s\n",
				   filename);
			break;
		}

		if (memcmp(bf, "Name:", 5) == 0) {
			char *name = bf + 5;
			while (*name && isspace(*name))
				++name;
			size = strlen(name) - 1;
			if (size >= len)
				size = len - 1;
			memcpy(comm, name, size);
			comm[size] = '\0';

		} else if (memcmp(bf, "Tgid:", 5) == 0) {
			char *tgids = bf + 5;
			while (*tgids && isspace(*tgids))
				++tgids;
			tgid = atoi(tgids);
		}
	}

	fclose(fp);

	return tgid;
}

static pid_t perf_event__synthesize_comm(struct perf_tool *tool,
					 union perf_event *event, pid_t pid,
					 int full,
					 perf_event__handler_t process,
					 struct machine *machine)
{
	char filename[PATH_MAX];
	size_t size;
	DIR *tasks;
	struct dirent dirent, *next;
	pid_t tgid;

	memset(&event->comm, 0, sizeof(event->comm));

	tgid = perf_event__get_comm_tgid(pid, event->comm.comm,
					 sizeof(event->comm.comm));
	if (tgid < 0)
		goto out;

	event->comm.pid = tgid;
	event->comm.header.type = PERF_RECORD_COMM;

	size = strlen(event->comm.comm) + 1;
	size = PERF_ALIGN(size, sizeof(u64));
	memset(event->comm.comm + size, 0, machine->id_hdr_size);
	event->comm.header.size = (sizeof(event->comm) -
				(sizeof(event->comm.comm) - size) +
				machine->id_hdr_size);
	if (!full) {
		event->comm.tid = pid;

		if (process(tool, event, &synth_sample, machine) != 0)
			return -1;

		goto out;
	}

	snprintf(filename, sizeof(filename), "/proc/%d/task", pid);

	tasks = opendir(filename);
	if (tasks == NULL) {
		pr_debug("couldn't open %s\n", filename);
		return 0;
	}

	while (!readdir_r(tasks, &dirent, &next) && next) {
		char *end;
		pid = strtol(dirent.d_name, &end, 10);
		if (*end)
			continue;

		/* already have tgid; jut want to update the comm */
		(void) perf_event__get_comm_tgid(pid, event->comm.comm,
					 sizeof(event->comm.comm));

		size = strlen(event->comm.comm) + 1;
		size = PERF_ALIGN(size, sizeof(u64));
		memset(event->comm.comm + size, 0, machine->id_hdr_size);
		event->comm.header.size = (sizeof(event->comm) -
					  (sizeof(event->comm.comm) - size) +
					  machine->id_hdr_size);

		event->comm.tid = pid;

		if (process(tool, event, &synth_sample, machine) != 0) {
			tgid = -1;
			break;
		}
	}

	closedir(tasks);
out:
	return tgid;
}

static int perf_event__synthesize_mmap_events(struct perf_tool *tool,
					      union perf_event *event,
					      pid_t pid, pid_t tgid,
					      perf_event__handler_t process,
					      struct machine *machine)
{
	char filename[PATH_MAX];
	FILE *fp;
	int rc = 0;

	snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);

	fp = fopen(filename, "r");
	if (fp == NULL) {
		/*
		 * We raced with a task exiting - just return:
		 */
		pr_debug("couldn't open %s\n", filename);
		return -1;
	}

	event->header.type = PERF_RECORD_MMAP;
	/*
	 * Just like the kernel, see __perf_event_mmap in kernel/perf_event.c
	 */
	event->header.misc = PERF_RECORD_MISC_USER;

	while (1) {
		char bf[BUFSIZ];
		char prot[5];
		char execname[PATH_MAX];
		char anonstr[] = "//anon";
		size_t size;
		ssize_t n;

		if (fgets(bf, sizeof(bf), fp) == NULL)
			break;

		/* ensure null termination since stack will be reused. */
		strcpy(execname, "");

		/* 00400000-0040c000 r-xp 00000000 fd:01 41038  /bin/cat */
		n = sscanf(bf, "%"PRIx64"-%"PRIx64" %s %"PRIx64" %*x:%*x %*u %s\n",
		       &event->mmap.start, &event->mmap.len, prot,
		       &event->mmap.pgoff,
		       execname);
		/*
 		 * Anon maps don't have the execname.
 		 */
		if (n < 4)
			continue;

		if (prot[2] != 'x')
			continue;

		if (!strcmp(execname, ""))
			strcpy(execname, anonstr);

		size = strlen(execname) + 1;
		memcpy(event->mmap.filename, execname, size);
		size = PERF_ALIGN(size, sizeof(u64));
		event->mmap.len -= event->mmap.start;
		event->mmap.header.size = (sizeof(event->mmap) -
					(sizeof(event->mmap.filename) - size));
		memset(event->mmap.filename + size, 0, machine->id_hdr_size);
		event->mmap.header.size += machine->id_hdr_size;
		event->mmap.pid = tgid;
		event->mmap.tid = pid;

		if (process(tool, event, &synth_sample, machine) != 0) {
			rc = -1;
			break;
		}
	}

	fclose(fp);
	return rc;
}

int perf_event__synthesize_modules(struct perf_tool *tool,
				   perf_event__handler_t process,
				   struct machine *machine)
{
	int rc = 0;
	struct rb_node *nd;
	struct map_groups *kmaps = &machine->kmaps;
	union perf_event *event = zalloc((sizeof(event->mmap) +
					  machine->id_hdr_size));
	if (event == NULL) {
		pr_debug("Not enough memory synthesizing mmap event "
			 "for kernel modules\n");
		return -1;
	}

	event->header.type = PERF_RECORD_MMAP;

	/*
	 * kernel uses 0 for user space maps, see kernel/perf_event.c
	 * __perf_event_mmap
	 */
	if (machine__is_host(machine))
		event->header.misc = PERF_RECORD_MISC_KERNEL;
	else
		event->header.misc = PERF_RECORD_MISC_GUEST_KERNEL;

	for (nd = rb_first(&kmaps->maps[MAP__FUNCTION]);
	     nd; nd = rb_next(nd)) {
		size_t size;
		struct map *pos = rb_entry(nd, struct map, rb_node);

		if (pos->dso->kernel)
			continue;

		size = PERF_ALIGN(pos->dso->long_name_len + 1, sizeof(u64));
		event->mmap.header.type = PERF_RECORD_MMAP;
		event->mmap.header.size = (sizeof(event->mmap) -
				        (sizeof(event->mmap.filename) - size));
		memset(event->mmap.filename + size, 0, machine->id_hdr_size);
		event->mmap.header.size += machine->id_hdr_size;
		event->mmap.start = pos->start;
		event->mmap.len   = pos->end - pos->start;
		event->mmap.pid   = machine->pid;

		memcpy(event->mmap.filename, pos->dso->long_name,
		       pos->dso->long_name_len + 1);
		if (process(tool, event, &synth_sample, machine) != 0) {
			rc = -1;
			break;
		}
	}

	free(event);
	return rc;
}

static int __event__synthesize_thread(union perf_event *comm_event,
				      union perf_event *mmap_event,
				      pid_t pid, int full,
					  perf_event__handler_t process,
				      struct perf_tool *tool,
				      struct machine *machine)
{
	pid_t tgid = perf_event__synthesize_comm(tool, comm_event, pid, full,
						 process, machine);
	if (tgid == -1)
		return -1;
	return perf_event__synthesize_mmap_events(tool, mmap_event, pid, tgid,
						  process, machine);
}

int perf_event__synthesize_thread_map(struct perf_tool *tool,
				      struct thread_map *threads,
				      perf_event__handler_t process,
				      struct machine *machine)
{
	union perf_event *comm_event, *mmap_event;
	int err = -1, thread, j;

	comm_event = malloc(sizeof(comm_event->comm) + machine->id_hdr_size);
	if (comm_event == NULL)
		goto out;

	mmap_event = malloc(sizeof(mmap_event->mmap) + machine->id_hdr_size);
	if (mmap_event == NULL)
		goto out_free_comm;

	err = 0;
	for (thread = 0; thread < threads->nr; ++thread) {
		if (__event__synthesize_thread(comm_event, mmap_event,
					       threads->map[thread], 0,
					       process, tool, machine)) {
			err = -1;
			break;
		}

		/*
		 * comm.pid is set to thread group id by
		 * perf_event__synthesize_comm
		 */
		if ((int) comm_event->comm.pid != threads->map[thread]) {
			bool need_leader = true;

			/* is thread group leader in thread_map? */
			for (j = 0; j < threads->nr; ++j) {
				if ((int) comm_event->comm.pid == threads->map[j]) {
					need_leader = false;
					break;
				}
			}

			/* if not, generate events for it */
			if (need_leader &&
			    __event__synthesize_thread(comm_event,
						      mmap_event,
						      comm_event->comm.pid, 0,
						      process, tool, machine)) {
				err = -1;
				break;
			}
		}
	}
	free(mmap_event);
out_free_comm:
	free(comm_event);
out:
	return err;
}

int perf_event__synthesize_threads(struct perf_tool *tool,
				   perf_event__handler_t process,
				   struct machine *machine)
{
	DIR *proc;
	struct dirent dirent, *next;
	union perf_event *comm_event, *mmap_event;
	int err = -1;

	comm_event = malloc(sizeof(comm_event->comm) + machine->id_hdr_size);
	if (comm_event == NULL)
		goto out;

	mmap_event = malloc(sizeof(mmap_event->mmap) + machine->id_hdr_size);
	if (mmap_event == NULL)
		goto out_free_comm;

	proc = opendir("/proc");
	if (proc == NULL)
		goto out_free_mmap;

	while (!readdir_r(proc, &dirent, &next) && next) {
		char *end;
		pid_t pid = strtol(dirent.d_name, &end, 10);

		if (*end) /* only interested in proper numerical dirents */
			continue;
		/*
 		 * We may race with exiting thread, so don't stop just because
 		 * one thread couldn't be synthesized.
 		 */
		__event__synthesize_thread(comm_event, mmap_event, pid, 1,
					   process, tool, machine);
	}

	err = 0;
	closedir(proc);
out_free_mmap:
	free(mmap_event);
out_free_comm:
	free(comm_event);
out:
	return err;
}

struct process_symbol_args {
	const char *name;
	u64	   start;
};

static int find_symbol_cb(void *arg, const char *name, char type,
			  u64 start)
{
	struct process_symbol_args *args = arg;

	/*
	 * Must be a function or at least an alias, as in PARISC64, where "_text" is
	 * an 'A' to the same address as "_stext".
	 */
	if (!(symbol_type__is_a(type, MAP__FUNCTION) ||
	      type == 'A') || strcmp(name, args->name))
		return 0;

	args->start = start;
	return 1;
}

int perf_event__synthesize_kernel_mmap(struct perf_tool *tool,
				       perf_event__handler_t process,
				       struct machine *machine,
				       const char *symbol_name)
{
	size_t size;
	const char *filename, *mmap_name;
	char path[PATH_MAX];
	char name_buff[PATH_MAX];
	struct map *map;
	int err;
	/*
	 * We should get this from /sys/kernel/sections/.text, but till that is
	 * available use this, and after it is use this as a fallback for older
	 * kernels.
	 */
	struct process_symbol_args args = { .name = symbol_name, };
	union perf_event *event = zalloc((sizeof(event->mmap) +
					  machine->id_hdr_size));
	if (event == NULL) {
		pr_debug("Not enough memory synthesizing mmap event "
			 "for kernel modules\n");
		return -1;
	}

	mmap_name = machine__mmap_name(machine, name_buff, sizeof(name_buff));
	if (machine__is_host(machine)) {
		/*
		 * kernel uses PERF_RECORD_MISC_USER for user space maps,
		 * see kernel/perf_event.c __perf_event_mmap
		 */
		event->header.misc = PERF_RECORD_MISC_KERNEL;
		filename = "/proc/kallsyms";
	} else {
		event->header.misc = PERF_RECORD_MISC_GUEST_KERNEL;
		if (machine__is_default_guest(machine))
			filename = (char *) symbol_conf.default_guest_kallsyms;
		else {
			sprintf(path, "%s/proc/kallsyms", machine->root_dir);
			filename = path;
		}
	}

	if (kallsyms__parse(filename, &args, find_symbol_cb) <= 0) {
		free(event);
		return -ENOENT;
	}

	map = machine->vmfikus_maps[MAP__FUNCTION];
	size = snprintf(event->mmap.filename, sizeof(event->mmap.filename),
			"%s%s", mmap_name, symbol_name) + 1;
	size = PERF_ALIGN(size, sizeof(u64));
	event->mmap.header.type = PERF_RECORD_MMAP;
	event->mmap.header.size = (sizeof(event->mmap) -
			(sizeof(event->mmap.filename) - size) + machine->id_hdr_size);
	event->mmap.pgoff = args.start;
	event->mmap.start = map->start;
	event->mmap.len   = map->end - event->mmap.start;
	event->mmap.pid   = machine->pid;

	err = process(tool, event, &synth_sample, machine);
	free(event);

	return err;
}

size_t perf_event__fprintf_comm(union perf_event *event, FILE *fp)
{
	return fprintf(fp, ": %s:%d\n", event->comm.comm, event->comm.tid);
}

int perf_event__process_comm(struct perf_tool *tool __maybe_unused,
			     union perf_event *event,
			     struct perf_sample *sample __maybe_unused,
			     struct machine *machine)
{
	return machine__process_comm_event(machine, event);
}

int perf_event__process_lost(struct perf_tool *tool __maybe_unused,
			     union perf_event *event,
			     struct perf_sample *sample __maybe_unused,
			     struct machine *machine)
{
	return machine__process_lost_event(machine, event);
}

size_t perf_event__fprintf_mmap(union perf_event *event, FILE *fp)
{
	return fprintf(fp, " %d/%d: [%#" PRIx64 "(%#" PRIx64 ") @ %#" PRIx64 "]: %s\n",
		       event->mmap.pid, event->mmap.tid, event->mmap.start,
		       event->mmap.len, event->mmap.pgoff, event->mmap.filename);
}

size_t perf_event__fprintf_mmap2(union perf_event *event, FILE *fp)
{
	return fprintf(fp, " %d/%d: [%#" PRIx64 "(%#" PRIx64 ") @ %#" PRIx64
			   " %02x:%02x %"PRIu64" %"PRIu64"]: %s\n",
		       event->mmap2.pid, event->mmap2.tid, event->mmap2.start,
		       event->mmap2.len, event->mmap2.pgoff, event->mmap2.maj,
		       event->mmap2.min, event->mmap2.ino,
		       event->mmap2.ino_generation,
		       event->mmap2.filename);
}

int perf_event__process_mmap(struct perf_tool *tool __maybe_unused,
			     union perf_event *event,
			     struct perf_sample *sample __maybe_unused,
			     struct machine *machine)
{
	return machine__process_mmap_event(machine, event);
}

int perf_event__process_mmap2(struct perf_tool *tool __maybe_unused,
			     union perf_event *event,
			     struct perf_sample *sample __maybe_unused,
			     struct machine *machine)
{
	return machine__process_mmap2_event(machine, event);
}

size_t perf_event__fprintf_task(union perf_event *event, FILE *fp)
{
	return fprintf(fp, "(%d:%d):(%d:%d)\n",
		       event->fork.pid, event->fork.tid,
		       event->fork.ppid, event->fork.ptid);
}

int perf_event__process_fork(struct perf_tool *tool __maybe_unused,
			     union perf_event *event,
			     struct perf_sample *sample __maybe_unused,
			     struct machine *machine)
{
	return machine__process_fork_event(machine, event);
}

int perf_event__process_exit(struct perf_tool *tool __maybe_unused,
			     union perf_event *event,
			     struct perf_sample *sample __maybe_unused,
			     struct machine *machine)
{
	return machine__process_exit_event(machine, event);
}

size_t perf_event__fprintf(union perf_event *event, FILE *fp)
{
	size_t ret = fprintf(fp, "PERF_RECORD_%s",
			     perf_event__name(event->header.type));

	switch (event->header.type) {
	case PERF_RECORD_COMM:
		ret += perf_event__fprintf_comm(event, fp);
		break;
	case PERF_RECORD_FORK:
	case PERF_RECORD_EXIT:
		ret += perf_event__fprintf_task(event, fp);
		break;
	case PERF_RECORD_MMAP:
		ret += perf_event__fprintf_mmap(event, fp);
		break;
	case PERF_RECORD_MMAP2:
		ret += perf_event__fprintf_mmap2(event, fp);
		break;
	default:
		ret += fprintf(fp, "\n");
	}

	return ret;
}

int perf_event__process(struct perf_tool *tool __maybe_unused,
			union perf_event *event,
			struct perf_sample *sample __maybe_unused,
			struct machine *machine)
{
	return machine__process_event(machine, event);
}

void thread__find_addr_map(struct thread *self,
			   struct machine *machine, u8 cpumode,
			   enum map_type type, u64 addr,
			   struct addr_location *al)
{
	struct map_groups *mg = &self->mg;
	bool load_map = false;

	al->thread = self;
	al->addr = addr;
	al->cpumode = cpumode;
	al->filtered = false;

	if (machine == NULL) {
		al->map = NULL;
		return;
	}

	if (cpumode == PERF_RECORD_MISC_KERNEL && perf_host) {
		al->level = 'k';
		mg = &machine->kmaps;
		load_map = true;
	} else if (cpumode == PERF_RECORD_MISC_USER && perf_host) {
		al->level = '.';
	} else if (cpumode == PERF_RECORD_MISC_GUEST_KERNEL && perf_guest) {
		al->level = 'g';
		mg = &machine->kmaps;
		load_map = true;
	} else {
		/*
		 * 'u' means guest os user space.
		 * TODO: We don't support guest user space. Might support late.
		 */
		if (cpumode == PERF_RECORD_MISC_GUEST_USER && perf_guest)
			al->level = 'u';
		else
			al->level = 'H';
		al->map = NULL;

		if ((cpumode == PERF_RECORD_MISC_GUEST_USER ||
			cpumode == PERF_RECORD_MISC_GUEST_KERNEL) &&
			!perf_guest)
			al->filtered = true;
		if ((cpumode == PERF_RECORD_MISC_USER ||
			cpumode == PERF_RECORD_MISC_KERNEL) &&
			!perf_host)
			al->filtered = true;

		return;
	}
try_again:
	al->map = map_groups__find(mg, type, al->addr);
	if (al->map == NULL) {
		/*
		 * If this is outside of all known maps, and is a negative
		 * address, try to look it up in the kernel dso, as it might be
		 * a vsyscall or vdso (which executes in user-mode).
		 *
		 * XXX This is nasty, we should have a symbol list in the
		 * "[vdso]" dso, but for now lets use the old trick of looking
		 * in the whole kernel symbol list.
		 */
		if ((long long)al->addr < 0 &&
		    cpumode == PERF_RECORD_MISC_USER &&
		    machine && mg != &machine->kmaps) {
			mg = &machine->kmaps;
			goto try_again;
		}
	} else {
		/*
		 * Kernel maps might be changed when loading symbols so loading
		 * must be done prior to using kernel maps.
		 */
		if (load_map)
			map__load(al->map, machine->symbol_filter);
		al->addr = al->map->map_ip(al->map, al->addr);
	}
}

void thread__find_addr_location(struct thread *thread, struct machine *machine,
				u8 cpumode, enum map_type type, u64 addr,
				struct addr_location *al)
{
	thread__find_addr_map(thread, machine, cpumode, type, addr, al);
	if (al->map != NULL)
		al->sym = map__find_symbol(al->map, al->addr,
					   machine->symbol_filter);
	else
		al->sym = NULL;
}

int perf_event__preprocess_sample(const union perf_event *event,
				  struct machine *machine,
				  struct addr_location *al,
				  struct perf_sample *sample)
{
	u8 cpumode = event->header.misc & PERF_RECORD_MISC_CPUMODE_MASK;
	struct thread *thread = machine__findnew_thread(machine, sample->pid,
							sample->pid);

	if (thread == NULL)
		return -1;

	if (symbol_conf.comm_list &&
	    !strlist__has_entry(symbol_conf.comm_list, thread->comm))
		goto out_filtered;

	dump_printf(" ... thread: %s:%d\n", thread->comm, thread->tid);
	/*
	 * Have we already created the kernel maps for this machine?
	 *
	 * This should have happened earlier, when we processed the kernel MMAP
	 * events, but for older perf.data files there was no such thing, so do
	 * it now.
	 */
	if (cpumode == PERF_RECORD_MISC_KERNEL &&
	    machine->vmfikus_maps[MAP__FUNCTION] == NULL)
		machine__create_kernel_maps(machine);

	thread__find_addr_map(thread, machine, cpumode, MAP__FUNCTION,
			      sample->ip, al);
	dump_printf(" ...... dso: %s\n",
		    al->map ? al->map->dso->long_name :
			al->level == 'H' ? "[hypervisor]" : "<not found>");
	al->sym = NULL;
	al->cpu = sample->cpu;

	if (al->map) {
		struct dso *dso = al->map->dso;

		if (symbol_conf.dso_list &&
		    (!dso || !(strlist__has_entry(symbol_conf.dso_list,
						  dso->short_name) ||
			       (dso->short_name != dso->long_name &&
				strlist__has_entry(symbol_conf.dso_list,
						   dso->long_name)))))
			goto out_filtered;

		al->sym = map__find_symbol(al->map, al->addr,
					   machine->symbol_filter);
	}

	if (symbol_conf.sym_list &&
		(!al->sym || !strlist__has_entry(symbol_conf.sym_list,
						al->sym->name)))
		goto out_filtered;

	return 0;

out_filtered:
	al->filtered = true;
	return 0;
}
