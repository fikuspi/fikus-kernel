/*
 * SHmedia irqflags support
 *
 * Copyright (C) 2006 - 2009 Paul Mundt
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <fikus/irqflags.h>
#include <fikus/module.h>
#include <cpu/registers.h>

void notrace arch_local_irq_restore(unsigned long flags)
{
	unsigned long long __dummy;

	if (flags == ARCH_IRQ_DISABLED) {
		__asm__ __volatile__ (
			"getcon	" __SR ", %0\n\t"
			"or	%0, %1, %0\n\t"
			"putcon	%0, " __SR "\n\t"
			: "=&r" (__dummy)
			: "r" (ARCH_IRQ_DISABLED)
		);
	} else {
		__asm__ __volatile__ (
			"getcon	" __SR ", %0\n\t"
			"and	%0, %1, %0\n\t"
			"putcon	%0, " __SR "\n\t"
			: "=&r" (__dummy)
			: "r" (~ARCH_IRQ_DISABLED)
		);
	}
}
EXPORT_SYMBOL(arch_local_irq_restore);

unsigned long notrace arch_local_save_flags(void)
{
	unsigned long flags;

	__asm__ __volatile__ (
		"getcon	" __SR ", %0\n\t"
		"and	%0, %1, %0"
		: "=&r" (flags)
		: "r" (ARCH_IRQ_DISABLED)
	);

	return flags;
}
EXPORT_SYMBOL(arch_local_save_flags);
