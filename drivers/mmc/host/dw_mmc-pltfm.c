/*
 * Synopsys DesignWare Multimedia Card Interface driver
 *
 * Copyright (C) 2009 NXP Semiconductors
 * Copyright (C) 2009, 2010 Imagination Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <fikus/err.h>
#include <fikus/interrupt.h>
#include <fikus/module.h>
#include <fikus/io.h>
#include <fikus/irq.h>
#include <fikus/platform_device.h>
#include <fikus/slab.h>
#include <fikus/mmc/host.h>
#include <fikus/mmc/mmc.h>
#include <fikus/mmc/dw_mmc.h>
#include <fikus/of.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"

static void dw_mci_rockchip_prepare_command(struct dw_mci *host, u32 *cmdr)
{
	*cmdr |= SDMMC_CMD_USE_HOLD_REG;
}

static const struct dw_mci_drv_data rockchip_drv_data = {
	.prepare_command	= dw_mci_rockchip_prepare_command,
};

int dw_mci_pltfm_register(struct platform_device *pdev,
			  const struct dw_mci_drv_data *drv_data)
{
	struct dw_mci *host;
	struct resource	*regs;
	int ret;

	host = devm_kzalloc(&pdev->dev, sizeof(struct dw_mci), GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	host->irq = platform_get_irq(pdev, 0);
	if (host->irq < 0)
		return host->irq;

	host->drv_data = drv_data;
	host->dev = &pdev->dev;
	host->irq_flags = 0;
	host->pdata = pdev->dev.platform_data;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->regs = devm_ioremap_resource(&pdev->dev, regs);
	if (IS_ERR(host->regs))
		return PTR_ERR(host->regs);

	if (drv_data && drv_data->init) {
		ret = drv_data->init(host);
		if (ret)
			return ret;
	}

	platform_set_drvdata(pdev, host);
	return dw_mci_probe(host);
}
EXPORT_SYMBOL_GPL(dw_mci_pltfm_register);

#ifdef CONFIG_PM_SLEEP
/*
 * TODO: we should probably disable the clock to the card in the suspend path.
 */
static int dw_mci_pltfm_suspend(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);

	return dw_mci_suspend(host);
}

static int dw_mci_pltfm_resume(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);

	return dw_mci_resume(host);
}
#else
#define dw_mci_pltfm_suspend	NULL
#define dw_mci_pltfm_resume	NULL
#endif /* CONFIG_PM_SLEEP */

SIMPLE_DEV_PM_OPS(dw_mci_pltfm_pmops, dw_mci_pltfm_suspend, dw_mci_pltfm_resume);
EXPORT_SYMBOL_GPL(dw_mci_pltfm_pmops);

static const struct of_device_id dw_mci_pltfm_match[] = {
	{ .compatible = "snps,dw-mshc", },
	{ .compatible = "rockchip,rk2928-dw-mshc",
		.data = &rockchip_drv_data },
	{},
};
MODULE_DEVICE_TABLE(of, dw_mci_pltfm_match);

static int dw_mci_pltfm_probe(struct platform_device *pdev)
{
	const struct dw_mci_drv_data *drv_data = NULL;
	const struct of_device_id *match;

	if (pdev->dev.of_node) {
		match = of_match_node(dw_mci_pltfm_match, pdev->dev.of_node);
		drv_data = match->data;
	}

	return dw_mci_pltfm_register(pdev, drv_data);
}

int dw_mci_pltfm_remove(struct platform_device *pdev)
{
	struct dw_mci *host = platform_get_drvdata(pdev);

	dw_mci_remove(host);
	return 0;
}
EXPORT_SYMBOL_GPL(dw_mci_pltfm_remove);

static struct platform_driver dw_mci_pltfm_driver = {
	.probe		= dw_mci_pltfm_probe,
	.remove		= dw_mci_pltfm_remove,
	.driver		= {
		.name		= "dw_mmc",
		.of_match_table	= of_match_ptr(dw_mci_pltfm_match),
		.pm		= &dw_mci_pltfm_pmops,
	},
};

module_platform_driver(dw_mci_pltfm_driver);

MODULE_DESCRIPTION("DW Multimedia Card Interface driver");
MODULE_AUTHOR("NXP Semiconductor VietNam");
MODULE_AUTHOR("Imagination Technologies Ltd");
MODULE_LICENSE("GPL v2");
