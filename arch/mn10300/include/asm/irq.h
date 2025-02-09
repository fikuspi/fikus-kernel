/* MN10300 Hardware interrupt definitions
 *
 * Copyright (C) 2007 Matsushita Electric Industrial Co., Ltd.
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Modified by David Howells (dhowells@redhat.com)
 * - Derived from include/asm-i386/irq.h:
 *   - (C) 1992, 1993 John Torvalds, (C) 1997 Ingo Molnar
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */
#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

#include <asm/intctl-regs.h>
#include <asm/reset-regs.h>
#include <proc/irq.h>

/* this number is used when no interrupt has been assigned */
#define NO_IRQ		INT_MAX

/*
 * hardware irq numbers
 * - the ASB2364 has an FPGA with an IRQ multiplexer on it
 */
#ifdef CONFIG_MN10300_UNIT_ASB2364
#include <unit/irq.h>
#else
#define NR_CPU_IRQS	GxICR_NUM_IRQS
#define NR_IRQS		NR_CPU_IRQS
#endif

/* external hardware irq numbers */
#define NR_XIRQS	GxICR_NUM_XIRQS

#define irq_canonicalize(IRQ) (IRQ)

#endif /* _ASM_IRQ_H */
