/*
 * Broadcom BCM63xx Random Number Generator support
 *
 * Copyright (C) 2011, Florian Fainelli <florian@openwrt.org>
 * Copyright (C) 2009, Broadcom Corporation
 *
 */
#include <fikus/module.h>
#include <fikus/slab.h>
#include <fikus/io.h>
#include <fikus/err.h>
#include <fikus/clk.h>
#include <fikus/platform_device.h>
#include <fikus/hw_random.h>

#include <bcm63xx_io.h>
#include <bcm63xx_regs.h>

struct bcm63xx_rng_priv {
	struct clk *clk;
	void __iomem *regs;
};

#define to_rng_priv(rng)	((struct bcm63xx_rng_priv *)rng->priv)

static int bcm63xx_rng_init(struct hwrng *rng)
{
	struct bcm63xx_rng_priv *priv = to_rng_priv(rng);
	u32 val;

	val = bcm_readl(priv->regs + RNG_CTRL);
	val |= RNG_EN;
	bcm_writel(val, priv->regs + RNG_CTRL);

	return 0;
}

static void bcm63xx_rng_cleanup(struct hwrng *rng)
{
	struct bcm63xx_rng_priv *priv = to_rng_priv(rng);
	u32 val;

	val = bcm_readl(priv->regs + RNG_CTRL);
	val &= ~RNG_EN;
	bcm_writel(val, priv->regs + RNG_CTRL);
}

static int bcm63xx_rng_data_present(struct hwrng *rng, int wait)
{
	struct bcm63xx_rng_priv *priv = to_rng_priv(rng);

	return bcm_readl(priv->regs + RNG_STAT) & RNG_AVAIL_MASK;
}

static int bcm63xx_rng_data_read(struct hwrng *rng, u32 *data)
{
	struct bcm63xx_rng_priv *priv = to_rng_priv(rng);

	*data = bcm_readl(priv->regs + RNG_DATA);

	return 4;
}

static int bcm63xx_rng_probe(struct platform_device *pdev)
{
	struct resource *r;
	struct clk *clk;
	int ret;
	struct bcm63xx_rng_priv *priv;
	struct hwrng *rng;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		dev_err(&pdev->dev, "no iomem resource\n");
		ret = -ENXIO;
		goto out;
	}

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "no memory for private structure\n");
		ret = -ENOMEM;
		goto out;
	}

	rng = kzalloc(sizeof(*rng), GFP_KERNEL);
	if (!rng) {
		dev_err(&pdev->dev, "no memory for rng structure\n");
		ret = -ENOMEM;
		goto out_free_priv;
	}

	platform_set_drvdata(pdev, rng);
	rng->priv = (unsigned long)priv;
	rng->name = pdev->name;
	rng->init = bcm63xx_rng_init;
	rng->cleanup = bcm63xx_rng_cleanup;
	rng->data_present = bcm63xx_rng_data_present;
	rng->data_read = bcm63xx_rng_data_read;

	clk = clk_get(&pdev->dev, "ipsec");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "no clock for device\n");
		ret = PTR_ERR(clk);
		goto out_free_rng;
	}

	priv->clk = clk;

	if (!devm_request_mem_region(&pdev->dev, r->start,
					resource_size(r), pdev->name)) {
		dev_err(&pdev->dev, "request mem failed");
		ret = -ENOMEM;
		goto out_free_rng;
	}

	priv->regs = devm_ioremap_nocache(&pdev->dev, r->start,
					resource_size(r));
	if (!priv->regs) {
		dev_err(&pdev->dev, "ioremap failed");
		ret = -ENOMEM;
		goto out_free_rng;
	}

	clk_enable(clk);

	ret = hwrng_register(rng);
	if (ret) {
		dev_err(&pdev->dev, "failed to register rng device\n");
		goto out_clk_disable;
	}

	dev_info(&pdev->dev, "registered RNG driver\n");

	return 0;

out_clk_disable:
	clk_disable(clk);
out_free_rng:
	kfree(rng);
out_free_priv:
	kfree(priv);
out:
	return ret;
}

static int bcm63xx_rng_remove(struct platform_device *pdev)
{
	struct hwrng *rng = platform_get_drvdata(pdev);
	struct bcm63xx_rng_priv *priv = to_rng_priv(rng);

	hwrng_unregister(rng);
	clk_disable(priv->clk);
	kfree(priv);
	kfree(rng);

	return 0;
}

static struct platform_driver bcm63xx_rng_driver = {
	.probe		= bcm63xx_rng_probe,
	.remove		= bcm63xx_rng_remove,
	.driver		= {
		.name	= "bcm63xx-rng",
		.owner	= THIS_MODULE,
	},
};

module_platform_driver(bcm63xx_rng_driver);

MODULE_AUTHOR("Florian Fainelli <florian@openwrt.org>");
MODULE_DESCRIPTION("Broadcom BCM63xx RNG driver");
MODULE_LICENSE("GPL");
