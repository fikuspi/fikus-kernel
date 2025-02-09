#ifndef __FIKUS_MUTEX_DEBUG_H
#define __FIKUS_MUTEX_DEBUG_H

#include <fikus/linkage.h>
#include <fikus/lockdep.h>
#include <fikus/debug_locks.h>

/*
 * Mutexes - debugging helpers:
 */

#define __DEBUG_MUTEX_INITIALIZER(lockname)				\
	, .magic = &lockname

#define mutex_init(mutex)						\
do {									\
	static struct lock_class_key __key;				\
									\
	__mutex_init((mutex), #mutex, &__key);				\
} while (0)

extern void mutex_destroy(struct mutex *lock);

#endif
