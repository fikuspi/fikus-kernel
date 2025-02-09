/*
 * AXI clkgen driver
 *
 * Copyright 2012-2013 Analog Devices Inc.
 *  Author: Lars-Peter Clausen <lars@metafoo.de>
 *
 * Licensed under the GPL-2.
 *
 */

#include <fikus/platform_device.h>
#include <fikus/clk-provider.h>
#include <fikus/clk.h>
#include <fikus/slab.h>
#include <fikus/io.h>
#include <fikus/of.h>
#include <fikus/module.h>
#include <fikus/err.h>

#define AXI_CLKGEN_REG_UPDATE_ENABLE	0x04
#define AXI_CLKGEN_REG_CLK_OUT1		0x08
#define AXI_CLKGEN_REG_CLK_OUT2		0x0c
#define AXI_CLKGEN_REG_CLK_DIV		0x10
#define AXI_CLKGEN_REG_CLK_FB1		0x14
#define AXI_CLKGEN_REG_CLK_FB2		0x18
#define AXI_CLKGEN_REG_LOCK1		0x1c
#define AXI_CLKGEN_REG_LOCK2		0x20
#define AXI_CLKGEN_REG_LOCK3		0x24
#define AXI_CLKGEN_REG_FILTER1		0x28
#define AXI_CLKGEN_REG_FILTER2		0x2c

struct axi_clkgen {
	void __iomem *base;
	struct clk_hw clk_hw;
};

static uint32_t axi_clkgen_lookup_filter(unsigned int m)
{
	switch (m) {
	case 0:
		return 0x01001990;
	case 1:
		return 0x01001190;
	case 2:
		return 0x01009890;
	case 3:
		return 0x01001890;
	case 4:
		return 0x01008890;
	case 5 ... 8:
		return 0x01009090;
	case 9 ... 11:
		return 0x01000890;
	case 12:
		return 0x08009090;
	case 13 ... 22:
		return 0x01001090;
	case 23 ... 36:
		return 0x01008090;
	case 37 ... 46:
		return 0x08001090;
	default:
		return 0x08008090;
	}
}

static const uint32_t axi_clkgen_lock_table[] = {
	0x060603e8, 0x060603e8, 0x080803e8, 0x0b0b03e8,
	0x0e0e03e8, 0x111103e8, 0x131303e8, 0x161603e8,
	0x191903e8, 0x1c1c03e8, 0x1f1f0384, 0x1f1f0339,
	0x1f1f02ee, 0x1f1f02bc, 0x1f1f028a, 0x1f1f0271,
	0x1f1f023f, 0x1f1f0226, 0x1f1f020d, 0x1f1f01f4,
	0x1f1f01db, 0x1f1f01c2, 0x1f1f01a9, 0x1f1f0190,
	0x1f1f0190, 0x1f1f0177, 0x1f1f015e, 0x1f1f015e,
	0x1f1f0145, 0x1f1f0145, 0x1f1f012c, 0x1f1f012c,
	0x1f1f012c, 0x1f1f0113, 0x1f1f0113, 0x1f1f0113,
};

static uint32_t axi_clkgen_lookup_lock(unsigned int m)
{
	if (m < ARRAY_SIZE(axi_clkgen_lock_table))
		return axi_clkgen_lock_table[m];
	return 0x1f1f00fa;
}

static const unsigned int fpfd_min = 10000;
static const unsigned int fpfd_max = 300000;
static const unsigned int fvco_min = 600000;
static const unsigned int fvco_max = 1200000;

static void axi_clkgen_calc_params(unsigned long fin, unsigned long fout,
	unsigned int *best_d, unsigned int *best_m, unsigned int *best_dout)
{
	unsigned long d, d_min, d_max, _d_min, _d_max;
	unsigned long m, m_min, m_max;
	unsigned long f, dout, best_f, fvco;

	fin /= 1000;
	fout /= 1000;

	best_f = ULONG_MAX;
	*best_d = 0;
	*best_m = 0;
	*best_dout = 0;

	d_min = max_t(unsigned long, DIV_ROUND_UP(fin, fpfd_max), 1);
	d_max = min_t(unsigned long, fin / fpfd_min, 80);

	m_min = max_t(unsigned long, DIV_ROUND_UP(fvco_min, fin) * d_min, 1);
	m_max = min_t(unsigned long, fvco_max * d_max / fin, 64);

	for (m = m_min; m <= m_max; m++) {
		_d_min = max(d_min, DIV_ROUND_UP(fin * m, fvco_max));
		_d_max = min(d_max, fin * m / fvco_min);

		for (d = _d_min; d <= _d_max; d++) {
			fvco = fin * m / d;

			dout = DIV_ROUND_CLOSEST(fvco, fout);
			dout = clamp_t(unsigned long, dout, 1, 128);
			f = fvco / dout;
			if (abs(f - fout) < abs(best_f - fout)) {
				best_f = f;
				*best_d = d;
				*best_m = m;
				*best_dout = dout;
				if (best_f == fout)
					return;
			}
		}
	}
}

static void axi_clkgen_calc_clk_params(unsigned int divider, unsigned int *low,
	unsigned int *high, unsigned int *edge, unsigned int *nocount)
{
	if (divider == 1)
		*nocount = 1;
	else
		*nocount = 0;

	*high = divider / 2;
	*edge = divider % 2;
	*low = divider - *high;
}

static void axi_clkgen_write(struct axi_clkgen *axi_clkgen,
	unsigned int reg, unsigned int val)
{
	writel(val, axi_clkgen->base + reg);
}

static void axi_clkgen_read(struct axi_clkgen *axi_clkgen,
	unsigned int reg, unsigned int *val)
{
	*val = readl(axi_clkgen->base + reg);
}

static struct axi_clkgen *clk_hw_to_axi_clkgen(struct clk_hw *clk_hw)
{
	return container_of(clk_hw, struct axi_clkgen, clk_hw);
}

static int axi_clkgen_set_rate(struct clk_hw *clk_hw,
	unsigned long rate, unsigned long parent_rate)
{
	struct axi_clkgen *axi_clkgen = clk_hw_to_axi_clkgen(clk_hw);
	unsigned int d, m, dout;
	unsigned int nocount;
	unsigned int high;
	unsigned int edge;
	unsigned int low;
	uint32_t filter;
	uint32_t lock;

	if (parent_rate == 0 || rate == 0)
		return -EINVAL;

	axi_clkgen_calc_params(parent_rate, rate, &d, &m, &dout);

	if (d == 0 || dout == 0 || m == 0)
		return -EINVAL;

	filter = axi_clkgen_lookup_filter(m - 1);
	lock = axi_clkgen_lookup_lock(m - 1);

	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_UPDATE_ENABLE, 0);

	axi_clkgen_calc_clk_params(dout, &low, &high, &edge, &nocount);
	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_CLK_OUT1,
		(high << 6) | low);
	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_CLK_OUT2,
		(edge << 7) | (nocount << 6));

	axi_clkgen_calc_clk_params(d, &low, &high, &edge, &nocount);
	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_CLK_DIV,
		(edge << 13) | (nocount << 12) | (high << 6) | low);

	axi_clkgen_calc_clk_params(m, &low, &high, &edge, &nocount);
	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_CLK_FB1,
		(high << 6) | low);
	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_CLK_FB2,
		(edge << 7) | (nocount << 6));

	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_LOCK1, lock & 0x3ff);
	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_LOCK2,
		(((lock >> 16) & 0x1f) << 10) | 0x1);
	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_LOCK3,
		(((lock >> 24) & 0x1f) << 10) | 0x3e9);
	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_FILTER1, filter >> 16);
	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_FILTER2, filter);

	axi_clkgen_write(axi_clkgen, AXI_CLKGEN_REG_UPDATE_ENABLE, 1);

	return 0;
}

static long axi_clkgen_round_rate(struct clk_hw *hw, unsigned long rate,
	unsigned long *parent_rate)
{
	unsigned int d, m, dout;

	axi_clkgen_calc_params(*parent_rate, rate, &d, &m, &dout);

	if (d == 0 || dout == 0 || m == 0)
		return -EINVAL;

	return *parent_rate / d * m / dout;
}

static unsigned long axi_clkgen_recalc_rate(struct clk_hw *clk_hw,
	unsigned long parent_rate)
{
	struct axi_clkgen *axi_clkgen = clk_hw_to_axi_clkgen(clk_hw);
	unsigned int d, m, dout;
	unsigned int reg;
	unsigned long long tmp;

	axi_clkgen_read(axi_clkgen, AXI_CLKGEN_REG_CLK_OUT1, &reg);
	dout = (reg & 0x3f) + ((reg >> 6) & 0x3f);
	axi_clkgen_read(axi_clkgen, AXI_CLKGEN_REG_CLK_DIV, &reg);
	d = (reg & 0x3f) + ((reg >> 6) & 0x3f);
	axi_clkgen_read(axi_clkgen, AXI_CLKGEN_REG_CLK_FB1, &reg);
	m = (reg & 0x3f) + ((reg >> 6) & 0x3f);

	if (d == 0 || dout == 0)
		return 0;

	tmp = (unsigned long long)(parent_rate / d) * m;
	do_div(tmp, dout);

	if (tmp > ULONG_MAX)
		return ULONG_MAX;

	return tmp;
}

static const struct clk_ops axi_clkgen_ops = {
	.recalc_rate = axi_clkgen_recalc_rate,
	.round_rate = axi_clkgen_round_rate,
	.set_rate = axi_clkgen_set_rate,
};

static int axi_clkgen_probe(struct platform_device *pdev)
{
	struct axi_clkgen *axi_clkgen;
	struct clk_init_data init;
	const char *parent_name;
	const char *clk_name;
	struct resource *mem;
	struct clk *clk;

	axi_clkgen = devm_kzalloc(&pdev->dev, sizeof(*axi_clkgen), GFP_KERNEL);
	if (!axi_clkgen)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	axi_clkgen->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(axi_clkgen->base))
		return PTR_ERR(axi_clkgen->base);

	parent_name = of_clk_get_parent_name(pdev->dev.of_node, 0);
	if (!parent_name)
		return -EINVAL;

	clk_name = pdev->dev.of_node->name;
	of_property_read_string(pdev->dev.of_node, "clock-output-names",
		&clk_name);

	init.name = clk_name;
	init.ops = &axi_clkgen_ops;
	init.flags = 0;
	init.parent_names = &parent_name;
	init.num_parents = 1;

	axi_clkgen->clk_hw.init = &init;
	clk = devm_clk_register(&pdev->dev, &axi_clkgen->clk_hw);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	return of_clk_add_provider(pdev->dev.of_node, of_clk_src_simple_get,
				    clk);
}

static int axi_clkgen_remove(struct platform_device *pdev)
{
	of_clk_del_provider(pdev->dev.of_node);

	return 0;
}

static const struct of_device_id axi_clkgen_ids[] = {
	{ .compatible = "adi,axi-clkgen-1.00.a" },
	{ },
};
MODULE_DEVICE_TABLE(of, axi_clkgen_ids);

static struct platform_driver axi_clkgen_driver = {
	.driver = {
		.name = "adi-axi-clkgen",
		.owner = THIS_MODULE,
		.of_match_table = axi_clkgen_ids,
	},
	.probe = axi_clkgen_probe,
	.remove = axi_clkgen_remove,
};
module_platform_driver(axi_clkgen_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_DESCRIPTION("Driver for the Analog Devices' AXI clkgen pcore clock generator");
