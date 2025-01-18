/*
 * Support for 32-bit Fikus for S390 personality.
 *
 * Copyright IBM Corp. 2000
 * Author(s): Gerhard Tonn (ton@de.ibm.com)
 *
 *
 */

#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/personality.h>
#include <fikus/sched.h>

static struct exec_domain s390_exec_domain;

static int __init s390_init (void)
{
	s390_exec_domain.name = "Fikus/s390";
	s390_exec_domain.handler = NULL;
	s390_exec_domain.pers_low = PER_FIKUS32;
	s390_exec_domain.pers_high = PER_FIKUS32;
	s390_exec_domain.signal_map = default_exec_domain.signal_map;
	s390_exec_domain.signal_invmap = default_exec_domain.signal_invmap;
	register_exec_domain(&s390_exec_domain);
	return 0;
}

__initcall(s390_init);
