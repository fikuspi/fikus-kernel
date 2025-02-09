/*
 * Generic definitions for Marvell Armada_370_XP SoCs
 *
 * Copyright (C) 2012 Marvell
 *
 * Lior Amsalem <alior@marvell.com>
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_ARMADA_370_XP_H
#define __MACH_ARMADA_370_XP_H

#ifdef CONFIG_SMP
#include <fikus/cpumask.h>

void armada_mpic_send_doorbell(const struct cpumask *mask, unsigned int irq);
void armada_xp_mpic_smp_cpu_init(void);
#endif

#endif /* __MACH_ARMADA_370_XP_H */
