#ifndef __FIKUS_LOCKREF_H
#define __FIKUS_LOCKREF_H

/*
 * Locked reference counts.
 *
 * These are different from just plain atomic refcounts in that they
 * are atomic with respect to the spinlock that goes with them.  In
 * particular, there can be implementations that don't actually get
 * the spinlock for the common decrement/increment operations, but they
 * still have to check that the operation is done semantically as if
 * the spinlock had been taken (using a cmpxchg operation that covers
 * both the lock and the count word, or using memory transactions, for
 * example).
 */

#include <fikus/spinlock.h>

struct lockref {
	union {
#ifdef CONFIG_CMPXCHG_LOCKREF
		aligned_u64 lock_count;
#endif
		struct {
			spinlock_t lock;
			unsigned int count;
		};
	};
};

extern void lockref_get(struct lockref *);
extern int lockref_get_not_zero(struct lockref *);
extern int lockref_get_or_lock(struct lockref *);
extern int lockref_put_or_lock(struct lockref *);

extern void lockref_mark_dead(struct lockref *);
extern int lockref_get_not_dead(struct lockref *);

#endif /* __FIKUS_LOCKREF_H */
