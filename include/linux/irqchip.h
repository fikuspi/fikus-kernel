/*
 * Copyright (C) 2012 Thomas Petazzoni
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _FIKUS_IRQCHIP_H
#define _FIKUS_IRQCHIP_H

#ifdef CONFIG_IRQCHIP
void irqchip_init(void);
#else
static inline void irqchip_init(void) {}
#endif

#endif
