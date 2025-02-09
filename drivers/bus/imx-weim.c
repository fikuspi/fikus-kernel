/*
 * EIM driver for Freescale's i.MX chips
 *
 * Copyright (C) 2013 Freescale Semiconductor, Inc.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <fikus/module.h>
#include <fikus/clk.h>
#include <fikus/io.h>
#include <fikus/of_device.h>

struct imx_weim_devtype {
	unsigned int	cs_count;
	unsigned int	cs_regs_count;
	unsigned int	cs_stride;
};

static const struct imx_weim_devtype imx1_weim_devtype = {
	.cs_count	= 6,
	.cs_regs_count	= 2,
	.cs_stride	= 0x08,
};

static const struct imx_weim_devtype imx27_weim_devtype = {
	.cs_count	= 6,
	.cs_regs_count	= 3,
	.cs_stride	= 0x10,
};

static const struct imx_weim_devtype imx50_weim_devtype = {
	.cs_count	= 4,
	.cs_regs_count	= 6,
	.cs_stride	= 0x18,
};

static const struct imx_weim_devtype imx51_weim_devtype = {
	.cs_count	= 6,
	.cs_regs_count	= 6,
	.cs_stride	= 0x18,
};

static const struct of_device_id weim_id_table[] = {
	/* i.MX1/21 */
	{ .compatible = "fsl,imx1-weim", .data = &imx1_weim_devtype, },
	/* i.MX25/27/31/35 */
	{ .compatible = "fsl,imx27-weim", .data = &imx27_weim_devtype, },
	/* i.MX50/53/6Q */
	{ .compatible = "fsl,imx50-weim", .data = &imx50_weim_devtype, },
	{ .compatible = "fsl,imx6q-weim", .data = &imx50_weim_devtype, },
	/* i.MX51 */
	{ .compatible = "fsl,imx51-weim", .data = &imx51_weim_devtype, },
	{ }
};
MODULE_DEVICE_TABLE(of, weim_id_table);

/* Parse and set the timing for this device. */
static int __init weim_timing_setup(struct device_node *np, void __iomem *base,
				    const struct imx_weim_devtype *devtype)
{
	u32 cs_idx, value[devtype->cs_regs_count];
	int i, ret;

	/* get the CS index from this child node's "reg" property. */
	ret = of_property_read_u32(np, "reg", &cs_idx);
	if (ret)
		return ret;

	if (cs_idx >= devtype->cs_count)
		return -EINVAL;

	ret = of_property_read_u32_array(np, "fsl,weim-cs-timing",
					 value, devtype->cs_regs_count);
	if (ret)
		return ret;

	/* set the timing for WEIM */
	for (i = 0; i < devtype->cs_regs_count; i++)
		writel(value[i], base + cs_idx * devtype->cs_stride + i * 4);

	return 0;
}

static int __init weim_parse_dt(struct platform_device *pdev,
				void __iomem *base)
{
	const struct of_device_id *of_id = of_match_device(weim_id_table,
							   &pdev->dev);
	const struct imx_weim_devtype *devtype = of_id->data;
	struct device_node *child;
	int ret;

	for_each_child_of_node(pdev->dev.of_node, child) {
		if (!child->name)
			continue;

		ret = weim_timing_setup(child, base, devtype);
		if (ret) {
			dev_err(&pdev->dev, "%s set timing failed.\n",
				child->full_name);
			return ret;
		}
	}

	ret = of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev);
	if (ret)
		dev_err(&pdev->dev, "%s fail to create devices.\n",
			pdev->dev.of_node->full_name);
	return ret;
}

static int __init weim_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct clk *clk;
	void __iomem *base;
	int ret;

	/* get the resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	/* get the clock */
	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_prepare_enable(clk);
	if (ret)
		return ret;

	/* parse the device node */
	ret = weim_parse_dt(pdev, base);
	if (ret)
		clk_disable_unprepare(clk);
	else
		dev_info(&pdev->dev, "Driver registered.\n");

	return ret;
}

static struct platform_driver weim_driver = {
	.driver = {
		.name		= "imx-weim",
		.owner		= THIS_MODULE,
		.of_match_table	= weim_id_table,
	},
};
module_platform_driver_probe(weim_driver, weim_probe);

MODULE_AUTHOR("Freescale Semiconductor Inc.");
MODULE_DESCRIPTION("i.MX EIM Controller Driver");
MODULE_LICENSE("GPL");
