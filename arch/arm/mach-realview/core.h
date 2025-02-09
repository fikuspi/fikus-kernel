/*
 *  fikus/arch/arm/mach-realview/core.h
 *
 *  Copyright (C) 2004 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ASM_ARCH_REALVIEW_H
#define __ASM_ARCH_REALVIEW_H

#include <fikus/amba/bus.h>
#include <fikus/io.h>

#include <asm/setup.h>

#define APB_DEVICE(name, busid, base, plat)			\
static AMBA_APB_DEVICE(name, busid, 0, REALVIEW_##base##_BASE, base##_IRQ, plat)

#define AHB_DEVICE(name, busid, base, plat)			\
static AMBA_AHB_DEVICE(name, busid, 0, REALVIEW_##base##_BASE, base##_IRQ, plat)

struct machine_desc;

extern struct platform_device realview_flash_device;
extern struct platform_device realview_cf_device;
extern struct platform_device realview_i2c_device;
extern struct mmci_platform_data realview_mmc0_plat_data;
extern struct mmci_platform_data realview_mmc1_plat_data;
extern struct clcd_board clcd_plat_data;
extern void __iomem *timer0_va_base;
extern void __iomem *timer1_va_base;
extern void __iomem *timer2_va_base;
extern void __iomem *timer3_va_base;

extern void realview_timer_init(unsigned int timer_irq);
extern int realview_flash_register(struct resource *res, u32 num);
extern int realview_eth_register(const char *name, struct resource *res);
extern int realview_usb_register(struct resource *res);
extern void realview_init_early(void);
extern void realview_fixup(struct tag *tags, char **from,
			   struct meminfo *meminfo);

extern struct smp_operations realview_smp_ops;
extern void realview_cpu_die(unsigned int cpu);

#endif
