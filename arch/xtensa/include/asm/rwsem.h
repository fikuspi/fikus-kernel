/*
 * include/asm-xtensa/rwsem.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Largely copied from include/asm-ppc/rwsem.h
 *
 * Copyright (C) 2001 - 2005 Tensilica Inc.
 */

#ifndef _XTENSA_RWSEM_H
#define _XTENSA_RWSEM_H

#ifndef _FIKUS_RWSEM_H
#error "Please don't include <asm/rwsem.h> directly, use <fikus/rwsem.h> instead."
#endif

#define RWSEM_UNLOCKED_VALUE		0x00000000
#define RWSEM_ACTIVE_BIAS		0x00000001
#define RWSEM_ACTIVE_MASK		0x0000ffff
#define RWSEM_WAITING_BIAS		(-0x00010000)
#define RWSEM_ACTIVE_READ_BIAS		RWSEM_ACTIVE_BIAS
#define RWSEM_ACTIVE_WRITE_BIAS		(RWSEM_WAITING_BIAS + RWSEM_ACTIVE_BIAS)

/*
 * lock for reading
 */
static inline void __down_read(struct rw_semaphore *sem)
{
	if (atomic_add_return(1,(atomic_t *)(&sem->count)) > 0)
		smp_wmb();
	else
		rwsem_down_read_failed(sem);
}

static inline int __down_read_trylock(struct rw_semaphore *sem)
{
	int tmp;

	while ((tmp = sem->count) >= 0) {
		if (tmp == cmpxchg(&sem->count, tmp,
				   tmp + RWSEM_ACTIVE_READ_BIAS)) {
			smp_wmb();
			return 1;
		}
	}
	return 0;
}

/*
 * lock for writing
 */
static inline void __down_write(struct rw_semaphore *sem)
{
	int tmp;

	tmp = atomic_add_return(RWSEM_ACTIVE_WRITE_BIAS,
				(atomic_t *)(&sem->count));
	if (tmp == RWSEM_ACTIVE_WRITE_BIAS)
		smp_wmb();
	else
		rwsem_down_write_failed(sem);
}

static inline int __down_write_trylock(struct rw_semaphore *sem)
{
	int tmp;

	tmp = cmpxchg(&sem->count, RWSEM_UNLOCKED_VALUE,
		      RWSEM_ACTIVE_WRITE_BIAS);
	smp_wmb();
	return tmp == RWSEM_UNLOCKED_VALUE;
}

/*
 * unlock after reading
 */
static inline void __up_read(struct rw_semaphore *sem)
{
	int tmp;

	smp_wmb();
	tmp = atomic_sub_return(1,(atomic_t *)(&sem->count));
	if (tmp < -1 && (tmp & RWSEM_ACTIVE_MASK) == 0)
		rwsem_wake(sem);
}

/*
 * unlock after writing
 */
static inline void __up_write(struct rw_semaphore *sem)
{
	smp_wmb();
	if (atomic_sub_return(RWSEM_ACTIVE_WRITE_BIAS,
			      (atomic_t *)(&sem->count)) < 0)
		rwsem_wake(sem);
}

/*
 * implement atomic add functionality
 */
static inline void rwsem_atomic_add(int delta, struct rw_semaphore *sem)
{
	atomic_add(delta, (atomic_t *)(&sem->count));
}

/*
 * downgrade write lock to read lock
 */
static inline void __downgrade_write(struct rw_semaphore *sem)
{
	int tmp;

	smp_wmb();
	tmp = atomic_add_return(-RWSEM_WAITING_BIAS, (atomic_t *)(&sem->count));
	if (tmp < 0)
		rwsem_downgrade_wake(sem);
}

/*
 * implement exchange and add functionality
 */
static inline int rwsem_atomic_update(int delta, struct rw_semaphore *sem)
{
	smp_mb();
	return atomic_add_return(delta, (atomic_t *)(&sem->count));
}

#endif	/* _XTENSA_RWSEM_H */
