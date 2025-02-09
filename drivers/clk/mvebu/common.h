/*
 * Marvell EBU SoC common clock handling
 *
 * Copyright (C) 2012 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 * Andrew Lunn <andrew@lunn.ch>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __CLK_MVEBU_COMMON_H_
#define __CLK_MVEBU_COMMON_H_

#include <fikus/kernel.h>

struct device_node;

struct coreclk_ratio {
	int id;
	const char *name;
};

struct coreclk_soc_desc {
	u32 (*get_tclk_freq)(void __iomem *sar);
	u32 (*get_cpu_freq)(void __iomem *sar);
	void (*get_clk_ratio)(void __iomem *sar, int id, int *mult, int *div);
	const struct coreclk_ratio *ratios;
	int num_ratios;
};

struct clk_gating_soc_desc {
	const char *name;
	const char *parent;
	int bit_idx;
	unsigned long flags;
};

void __init mvebu_coreclk_setup(struct device_node *np,
				const struct coreclk_soc_desc *desc);

void __init mvebu_clk_gating_setup(struct device_node *np,
				   const struct clk_gating_soc_desc *desc);

#endif
