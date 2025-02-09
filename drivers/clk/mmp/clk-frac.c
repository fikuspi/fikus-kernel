/*
 * mmp factor clock operation source file
 *
 * Copyright (C) 2012 Marvell
 * Chao Xie <xiechao.mail@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <fikus/clk-provider.h>
#include <fikus/slab.h>
#include <fikus/io.h>
#include <fikus/err.h>

#include "clk.h"
/*
 * It is M/N clock
 *
 * Fout from synthesizer can be given from two equations:
 * numerator/denominator = Fin / (Fout * factor)
 */

#define to_clk_factor(hw) container_of(hw, struct clk_factor, hw)
struct clk_factor {
	struct clk_hw		hw;
	void __iomem		*base;
	struct clk_factor_masks	*masks;
	struct clk_factor_tbl	*ftbl;
	unsigned int		ftbl_cnt;
};

static long clk_factor_round_rate(struct clk_hw *hw, unsigned long drate,
		unsigned long *prate)
{
	struct clk_factor *factor = to_clk_factor(hw);
	unsigned long rate = 0, prev_rate;
	int i;

	for (i = 0; i < factor->ftbl_cnt; i++) {
		prev_rate = rate;
		rate = (((*prate / 10000) * factor->ftbl[i].num) /
			(factor->ftbl[i].den * factor->masks->factor)) * 10000;
		if (rate > drate)
			break;
	}
	if (i == 0)
		return rate;
	else
		return prev_rate;
}

static unsigned long clk_factor_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct clk_factor *factor = to_clk_factor(hw);
	struct clk_factor_masks *masks = factor->masks;
	unsigned int val, num, den;

	val = readl_relaxed(factor->base);

	/* calculate numerator */
	num = (val >> masks->num_shift) & masks->num_mask;

	/* calculate denominator */
	den = (val >> masks->den_shift) & masks->num_mask;

	if (!den)
		return 0;

	return (((parent_rate / 10000)  * den) /
			(num * factor->masks->factor)) * 10000;
}

/* Configures new clock rate*/
static int clk_factor_set_rate(struct clk_hw *hw, unsigned long drate,
				unsigned long prate)
{
	struct clk_factor *factor = to_clk_factor(hw);
	struct clk_factor_masks *masks = factor->masks;
	int i;
	unsigned long val;
	unsigned long prev_rate, rate = 0;

	for (i = 0; i < factor->ftbl_cnt; i++) {
		prev_rate = rate;
		rate = (((prate / 10000) * factor->ftbl[i].num) /
			(factor->ftbl[i].den * factor->masks->factor)) * 10000;
		if (rate > drate)
			break;
	}
	if (i > 0)
		i--;

	val = readl_relaxed(factor->base);

	val &= ~(masks->num_mask << masks->num_shift);
	val |= (factor->ftbl[i].num & masks->num_mask) << masks->num_shift;

	val &= ~(masks->den_mask << masks->den_shift);
	val |= (factor->ftbl[i].den & masks->den_mask) << masks->den_shift;

	writel_relaxed(val, factor->base);

	return 0;
}

static struct clk_ops clk_factor_ops = {
	.recalc_rate = clk_factor_recalc_rate,
	.round_rate = clk_factor_round_rate,
	.set_rate = clk_factor_set_rate,
};

struct clk *mmp_clk_register_factor(const char *name, const char *parent_name,
		unsigned long flags, void __iomem *base,
		struct clk_factor_masks *masks, struct clk_factor_tbl *ftbl,
		unsigned int ftbl_cnt)
{
	struct clk_factor *factor;
	struct clk_init_data init;
	struct clk *clk;

	if (!masks) {
		pr_err("%s: must pass a clk_factor_mask\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	factor = kzalloc(sizeof(*factor), GFP_KERNEL);
	if (!factor) {
		pr_err("%s: could not allocate factor  clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	/* struct clk_aux assignments */
	factor->base = base;
	factor->masks = masks;
	factor->ftbl = ftbl;
	factor->ftbl_cnt = ftbl_cnt;
	factor->hw.init = &init;

	init.name = name;
	init.ops = &clk_factor_ops;
	init.flags = flags;
	init.parent_names = &parent_name;
	init.num_parents = 1;

	clk = clk_register(NULL, &factor->hw);
	if (IS_ERR_OR_NULL(clk))
		kfree(factor);

	return clk;
}
