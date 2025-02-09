/*
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P - Interrupt handling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <fikus/kernel.h>
#include <fikus/interrupt.h>
#include <fikus/irq.h>
#include <fikus/io.h>
#include <fikus/irqchip/arm-vic.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <plat/cpu.h>

void __init s5p_init_irq(u32 *vic, u32 num_vic)
{
#ifdef CONFIG_ARM_VIC
	int irq;

	/* initialize the VICs */
	for (irq = 0; irq < num_vic; irq++)
		vic_init(VA_VIC(irq), VIC_BASE(irq), vic[irq], 0);
#endif
}
