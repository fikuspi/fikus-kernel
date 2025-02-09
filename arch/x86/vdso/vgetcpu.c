/*
 * Copyright 2006 Andi Kleen, SUSE Labs.
 * Subject to the GNU Public License, v.2
 *
 * Fast user context implementation of getcpu()
 */

#include <fikus/kernel.h>
#include <fikus/getcpu.h>
#include <fikus/jiffies.h>
#include <fikus/time.h>
#include <asm/vsyscall.h>
#include <asm/vgtod.h>

notrace long
__vdso_getcpu(unsigned *cpu, unsigned *node, struct getcpu_cache *unused)
{
	unsigned int p;

	p = __getcpu();

	if (cpu)
		*cpu = p & VGETCPU_CPU_MASK;
	if (node)
		*node = p >> 12;
	return 0;
}

long getcpu(unsigned *cpu, unsigned *node, struct getcpu_cache *tcache)
	__attribute__((weak, alias("__vdso_getcpu")));
