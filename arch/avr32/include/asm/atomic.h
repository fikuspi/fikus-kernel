/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc.
 *
 * But use these as seldom as possible since they are slower than
 * regular operations.
 *
 * Copyright (C) 2004-2006 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_AVR32_ATOMIC_H
#define __ASM_AVR32_ATOMIC_H

#include <fikus/types.h>
#include <asm/cmpxchg.h>

#define ATOMIC_INIT(i)  { (i) }

#define atomic_read(v)		(*(volatile int *)&(v)->counter)
#define atomic_set(v, i)	(((v)->counter) = i)

/*
 * atomic_sub_return - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v. Returns the resulting value.
 */
static inline int atomic_sub_return(int i, atomic_t *v)
{
	int result;

	asm volatile(
		"/* atomic_sub_return */\n"
		"1:	ssrf	5\n"
		"	ld.w	%0, %2\n"
		"	sub	%0, %3\n"
		"	stcond	%1, %0\n"
		"	brne	1b"
		: "=&r"(result), "=o"(v->counter)
		: "m"(v->counter), "rKs21"(i)
		: "cc");

	return result;
}

/*
 * atomic_add_return - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v. Returns the resulting value.
 */
static inline int atomic_add_return(int i, atomic_t *v)
{
	int result;

	if (__builtin_constant_p(i) && (i >= -1048575) && (i <= 1048576))
		result = atomic_sub_return(-i, v);
	else
		asm volatile(
			"/* atomic_add_return */\n"
			"1:	ssrf	5\n"
			"	ld.w	%0, %1\n"
			"	add	%0, %3\n"
			"	stcond	%2, %0\n"
			"	brne	1b"
			: "=&r"(result), "=o"(v->counter)
			: "m"(v->counter), "r"(i)
			: "cc", "memory");

	return result;
}

/*
 * atomic_sub_unless - sub unless the number is a given value
 * @v: pointer of type atomic_t
 * @a: the amount to subtract from v...
 * @u: ...unless v is equal to u.
 *
 * Atomically subtract @a from @v, so long as it was not @u.
 * Returns the old value of @v.
*/
static inline void atomic_sub_unless(atomic_t *v, int a, int u)
{
	int tmp;

	asm volatile(
		"/* atomic_sub_unless */\n"
		"1:	ssrf	5\n"
		"	ld.w	%0, %2\n"
		"	cp.w	%0, %4\n"
		"	breq	1f\n"
		"	sub	%0, %3\n"
		"	stcond	%1, %0\n"
		"	brne	1b\n"
		"1:"
		: "=&r"(tmp), "=o"(v->counter)
		: "m"(v->counter), "rKs21"(a), "rKs21"(u)
		: "cc", "memory");
}

/*
 * __atomic_add_unless - add unless the number is a given value
 * @v: pointer of type atomic_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as it was not @u.
 * Returns the old value of @v.
*/
static inline int __atomic_add_unless(atomic_t *v, int a, int u)
{
	int tmp, old = atomic_read(v);

	if (__builtin_constant_p(a) && (a >= -1048575) && (a <= 1048576))
		atomic_sub_unless(v, -a, u);
	else {
		asm volatile(
			"/* __atomic_add_unless */\n"
			"1:	ssrf	5\n"
			"	ld.w	%0, %2\n"
			"	cp.w	%0, %4\n"
			"	breq	1f\n"
			"	add	%0, %3\n"
			"	stcond	%1, %0\n"
			"	brne	1b\n"
			"1:"
			: "=&r"(tmp), "=o"(v->counter)
			: "m"(v->counter), "r"(a), "ir"(u)
			: "cc", "memory");
	}

	return old;
}

/*
 * atomic_sub_if_positive - conditionally subtract integer from atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically test @v and subtract @i if @v is greater or equal than @i.
 * The function returns the old value of @v minus @i.
 */
static inline int atomic_sub_if_positive(int i, atomic_t *v)
{
	int result;

	asm volatile(
		"/* atomic_sub_if_positive */\n"
		"1:	ssrf	5\n"
		"	ld.w	%0, %2\n"
		"	sub	%0, %3\n"
		"	brlt	1f\n"
		"	stcond	%1, %0\n"
		"	brne	1b\n"
		"1:"
		: "=&r"(result), "=o"(v->counter)
		: "m"(v->counter), "ir"(i)
		: "cc", "memory");

	return result;
}

#define atomic_xchg(v, new)	(xchg(&((v)->counter), new))
#define atomic_cmpxchg(v, o, n)	(cmpxchg(&((v)->counter), (o), (n)))

#define atomic_sub(i, v)	(void)atomic_sub_return(i, v)
#define atomic_add(i, v)	(void)atomic_add_return(i, v)
#define atomic_dec(v)		atomic_sub(1, (v))
#define atomic_inc(v)		atomic_add(1, (v))

#define atomic_dec_return(v)	atomic_sub_return(1, v)
#define atomic_inc_return(v)	atomic_add_return(1, v)

#define atomic_sub_and_test(i, v) (atomic_sub_return(i, v) == 0)
#define atomic_inc_and_test(v) (atomic_add_return(1, v) == 0)
#define atomic_dec_and_test(v) (atomic_sub_return(1, v) == 0)
#define atomic_add_negative(i, v) (atomic_add_return(i, v) < 0)

#define atomic_dec_if_positive(v) atomic_sub_if_positive(1, v)

#define smp_mb__before_atomic_dec()	barrier()
#define smp_mb__after_atomic_dec()	barrier()
#define smp_mb__before_atomic_inc()	barrier()
#define smp_mb__after_atomic_inc()	barrier()

#endif /*  __ASM_AVR32_ATOMIC_H */
