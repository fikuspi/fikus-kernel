/* MN10300 Short delay interpolation routines
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */
#include <fikus/module.h>
#include <fikus/sched.h>
#include <fikus/delay.h>
#include <asm/div64.h>

/*
 * basic delay loop
 */
void __delay(unsigned long loops)
{
	int d0;

	asm volatile(
		"	bra	1f	\n"
		"	.align	4	\n"
		"1:	bra	2f	\n"
		"	.align	4	\n"
		"2:	add	-1,%0	\n"
		"	bne	2b	\n"
		: "=&d" (d0)
		: "0" (loops)
		: "cc");
}
EXPORT_SYMBOL(__delay);

/*
 * handle a delay specified in terms of microseconds
 */
void __udelay(unsigned long usecs)
{
	unsigned long start, stop, cnt;

	/* usecs * CLK / 1E6 */
	stop = __muldiv64u(usecs, MN10300_TSCCLK, 1000000);
	start = TMTSCBC;

	do {
		cnt = start - TMTSCBC;
	} while (cnt < stop);
}
EXPORT_SYMBOL(__udelay);
