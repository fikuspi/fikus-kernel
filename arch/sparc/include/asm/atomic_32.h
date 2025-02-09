/* atomic.h: These still suck, but the I-cache hit rate is higher.
 *
 * Copyright (C) 1996 David S. Miller (davem@davemloft.net)
 * Copyright (C) 2000 Anton Blanchard (anton@fikuscare.com.au)
 * Copyright (C) 2007 Kyle McMartin (kyle@parisc-fikus.org)
 *
 * Additions by Keith M Wesolowski (wesolows@foobazco.org) based
 * on asm-parisc/atomic.h Copyright (C) 2000 Philipp Rumpf <prumpf@tux.org>.
 */

#ifndef __ARCH_SPARC_ATOMIC__
#define __ARCH_SPARC_ATOMIC__

#include <fikus/types.h>

#include <asm/cmpxchg.h>
#include <asm-generic/atomic64.h>


#define ATOMIC_INIT(i)  { (i) }

extern int __atomic_add_return(int, atomic_t *);
extern int atomic_cmpxchg(atomic_t *, int, int);
#define atomic_xchg(v, new) (xchg(&((v)->counter), new))
extern int __atomic_add_unless(atomic_t *, int, int);
extern void atomic_set(atomic_t *, int);

#define atomic_read(v)          (*(volatile int *)&(v)->counter)

#define atomic_add(i, v)	((void)__atomic_add_return( (int)(i), (v)))
#define atomic_sub(i, v)	((void)__atomic_add_return(-(int)(i), (v)))
#define atomic_inc(v)		((void)__atomic_add_return(        1, (v)))
#define atomic_dec(v)		((void)__atomic_add_return(       -1, (v)))

#define atomic_add_return(i, v)	(__atomic_add_return( (int)(i), (v)))
#define atomic_sub_return(i, v)	(__atomic_add_return(-(int)(i), (v)))
#define atomic_inc_return(v)	(__atomic_add_return(        1, (v)))
#define atomic_dec_return(v)	(__atomic_add_return(       -1, (v)))

#define atomic_add_negative(a, v)	(atomic_add_return((a), (v)) < 0)

/*
 * atomic_inc_and_test - increment and test
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
#define atomic_inc_and_test(v) (atomic_inc_return(v) == 0)

#define atomic_dec_and_test(v) (atomic_dec_return(v) == 0)
#define atomic_sub_and_test(i, v) (atomic_sub_return(i, v) == 0)

/* Atomic operations are already serializing */
#define smp_mb__before_atomic_dec()	barrier()
#define smp_mb__after_atomic_dec()	barrier()
#define smp_mb__before_atomic_inc()	barrier()
#define smp_mb__after_atomic_inc()	barrier()

#endif /* !(__ARCH_SPARC_ATOMIC__) */
