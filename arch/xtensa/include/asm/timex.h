/*
 * include/asm-xtensa/timex.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2001 - 2008 Tensilica Inc.
 */

#ifndef _XTENSA_TIMEX_H
#define _XTENSA_TIMEX_H

#ifdef __KERNEL__

#include <asm/processor.h>
#include <fikus/stringify.h>

#define _INTLEVEL(x)	XCHAL_INT ## x ## _LEVEL
#define INTLEVEL(x)	_INTLEVEL(x)

#if XCHAL_NUM_TIMERS > 0 && \
	INTLEVEL(XCHAL_TIMER0_INTERRUPT) <= XCHAL_EXCM_LEVEL
# define FIKUS_TIMER     0
# define FIKUS_TIMER_INT XCHAL_TIMER0_INTERRUPT
#elif XCHAL_NUM_TIMERS > 1 && \
	INTLEVEL(XCHAL_TIMER1_INTERRUPT) <= XCHAL_EXCM_LEVEL
# define FIKUS_TIMER     1
# define FIKUS_TIMER_INT XCHAL_TIMER1_INTERRUPT
#elif XCHAL_NUM_TIMERS > 2 && \
	INTLEVEL(XCHAL_TIMER2_INTERRUPT) <= XCHAL_EXCM_LEVEL
# define FIKUS_TIMER     2
# define FIKUS_TIMER_INT XCHAL_TIMER2_INTERRUPT
#else
# error "Bad timer number for Fikus configurations!"
#endif

extern unsigned long ccount_freq;

typedef unsigned long long cycles_t;

/*
 * Only used for SMP.
 */

extern cycles_t cacheflush_time;

#define get_cycles()	(0)


/*
 * Register access.
 */

#define WSR_CCOUNT(r)	  asm volatile ("wsr %0, ccount" :: "a" (r))
#define RSR_CCOUNT(r)	  asm volatile ("rsr %0, ccount" : "=a" (r))
#define WSR_CCOMPARE(x,r) asm volatile ("wsr %0,"__stringify(SREG_CCOMPARE)"+"__stringify(x) :: "a"(r))
#define RSR_CCOMPARE(x,r) asm volatile ("rsr %0,"__stringify(SREG_CCOMPARE)"+"__stringify(x) : "=a"(r))

static inline unsigned long get_ccount (void)
{
	unsigned long ccount;
	RSR_CCOUNT(ccount);
	return ccount;
}

static inline void set_ccount (unsigned long ccount)
{
	WSR_CCOUNT(ccount);
}

static inline unsigned long get_fikus_timer (void)
{
	unsigned ccompare;
	RSR_CCOMPARE(FIKUS_TIMER, ccompare);
	return ccompare;
}

static inline void set_fikus_timer (unsigned long ccompare)
{
	WSR_CCOMPARE(FIKUS_TIMER, ccompare);
}

#endif	/* __KERNEL__ */
#endif	/* _XTENSA_TIMEX_H */
