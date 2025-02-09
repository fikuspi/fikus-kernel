#ifndef __PERCPU_IDA_H__
#define __PERCPU_IDA_H__

#include <fikus/types.h>
#include <fikus/bitops.h>
#include <fikus/init.h>
#include <fikus/spinlock_types.h>
#include <fikus/wait.h>
#include <fikus/cpumask.h>

struct percpu_ida_cpu;

struct percpu_ida {
	/*
	 * number of tags available to be allocated, as passed to
	 * percpu_ida_init()
	 */
	unsigned			nr_tags;

	struct percpu_ida_cpu __percpu	*tag_cpu;

	/*
	 * Bitmap of cpus that (may) have tags on their percpu freelists:
	 * steal_tags() uses this to decide when to steal tags, and which cpus
	 * to try stealing from.
	 *
	 * It's ok for a freelist to be empty when its bit is set - steal_tags()
	 * will just keep looking - but the bitmap _must_ be set whenever a
	 * percpu freelist does have tags.
	 */
	cpumask_t			cpus_have_tags;

	struct {
		spinlock_t		lock;
		/*
		 * When we go to steal tags from another cpu (see steal_tags()),
		 * we want to pick a cpu at random. Cycling through them every
		 * time we steal is a bit easier and more or less equivalent:
		 */
		unsigned		cpu_last_stolen;

		/* For sleeping on allocation failure */
		wait_queue_head_t	wait;

		/*
		 * Global freelist - it's a stack where nr_free points to the
		 * top
		 */
		unsigned		nr_free;
		unsigned		*freelist;
	} ____cacheline_aligned_in_smp;
};

int percpu_ida_alloc(struct percpu_ida *pool, gfp_t gfp);
void percpu_ida_free(struct percpu_ida *pool, unsigned tag);

void percpu_ida_destroy(struct percpu_ida *pool);
int percpu_ida_init(struct percpu_ida *pool, unsigned long nr_tags);

#endif /* __PERCPU_IDA_H__ */
