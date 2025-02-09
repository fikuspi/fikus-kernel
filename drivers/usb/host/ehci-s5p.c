/*
 * SAMSUNG S5P USB HOST EHCI Controller
 *
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Author: Jingoo Han <jg1.han@samsung.com>
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <fikus/clk.h>
#include <fikus/dma-mapping.h>
#include <fikus/io.h>
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/of.h>
#include <fikus/of_gpio.h>
#include <fikus/platform_device.h>
#include <fikus/platform_data/usb-ehci-s5p.h>
#include <fikus/usb/phy.h>
#include <fikus/usb/samsung_usb_phy.h>
#include <fikus/usb.h>
#include <fikus/usb/hcd.h>
#include <fikus/usb/otg.h>

#include "ehci.h"

#define DRIVER_DESC "EHCI s5p driver"

#define EHCI_INSNREG00(base)			(base + 0x90)
#define EHCI_INSNREG00_ENA_INCR16		(0x1 << 25)
#define EHCI_INSNREG00_ENA_INCR8		(0x1 << 24)
#define EHCI_INSNREG00_ENA_INCR4		(0x1 << 23)
#define EHCI_INSNREG00_ENA_INCRX_ALIGN		(0x1 << 22)
#define EHCI_INSNREG00_ENABLE_DMA_BURST	\
	(EHCI_INSNREG00_ENA_INCR16 | EHCI_INSNREG00_ENA_INCR8 |	\
	 EHCI_INSNREG00_ENA_INCR4 | EHCI_INSNREG00_ENA_INCRX_ALIGN)

static const char hcd_name[] = "ehci-s5p";
static struct hc_driver __read_mostly s5p_ehci_hc_driver;

struct s5p_ehci_hcd {
	struct clk *clk;
	struct usb_phy *phy;
	struct usb_otg *otg;
	struct s5p_ehci_platdata *pdata;
};

static struct s5p_ehci_platdata empty_platdata;

#define to_s5p_ehci(hcd)      (struct s5p_ehci_hcd *)(hcd_to_ehci(hcd)->priv)

static void s5p_setup_vbus_gpio(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int err;
	int gpio;

	if (!dev->of_node)
		return;

	gpio = of_get_named_gpio(dev->of_node, "samsung,vbus-gpio", 0);
	if (!gpio_is_valid(gpio))
		return;

	err = devm_gpio_request_one(dev, gpio, GPIOF_OUT_INIT_HIGH,
				    "ehci_vbus_gpio");
	if (err)
		dev_err(dev, "can't request ehci vbus gpio %d", gpio);
}

static int s5p_ehci_probe(struct platform_device *pdev)
{
	struct s5p_ehci_platdata *pdata = dev_get_platdata(&pdev->dev);
	struct s5p_ehci_hcd *s5p_ehci;
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	struct resource *res;
	struct usb_phy *phy;
	int irq;
	int err;

	/*
	 * Right now device-tree probed devices don't get dma_mask set.
	 * Since shared usb code relies on it, set it here for now.
	 * Once we move to full device tree support this will vanish off.
	 */
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	if (!pdev->dev.coherent_dma_mask)
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	s5p_setup_vbus_gpio(pdev);

	hcd = usb_create_hcd(&s5p_ehci_hc_driver,
			     &pdev->dev, dev_name(&pdev->dev));
	if (!hcd) {
		dev_err(&pdev->dev, "Unable to create HCD\n");
		return -ENOMEM;
	}
	s5p_ehci = to_s5p_ehci(hcd);

	if (of_device_is_compatible(pdev->dev.of_node,
					"samsung,exynos5440-ehci")) {
		s5p_ehci->pdata = &empty_platdata;
		goto skip_phy;
	}

	phy = devm_usb_get_phy(&pdev->dev, USB_PHY_TYPE_USB2);
	if (IS_ERR(phy)) {
		/* Fallback to pdata */
		if (!pdata) {
			usb_put_hcd(hcd);
			dev_warn(&pdev->dev, "no platform data or transceiver defined\n");
			return -EPROBE_DEFER;
		} else {
			s5p_ehci->pdata = pdata;
		}
	} else {
		s5p_ehci->phy = phy;
		s5p_ehci->otg = phy->otg;
	}

skip_phy:

	s5p_ehci->clk = devm_clk_get(&pdev->dev, "usbhost");

	if (IS_ERR(s5p_ehci->clk)) {
		dev_err(&pdev->dev, "Failed to get usbhost clock\n");
		err = PTR_ERR(s5p_ehci->clk);
		goto fail_clk;
	}

	err = clk_prepare_enable(s5p_ehci->clk);
	if (err)
		goto fail_clk;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get I/O memory\n");
		err = -ENXIO;
		goto fail_io;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	hcd->regs = devm_ioremap(&pdev->dev, res->start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "Failed to remap I/O memory\n");
		err = -ENOMEM;
		goto fail_io;
	}

	irq = platform_get_irq(pdev, 0);
	if (!irq) {
		dev_err(&pdev->dev, "Failed to get IRQ\n");
		err = -ENODEV;
		goto fail_io;
	}

	if (s5p_ehci->otg)
		s5p_ehci->otg->set_host(s5p_ehci->otg, &hcd->self);

	if (s5p_ehci->phy)
		usb_phy_init(s5p_ehci->phy);
	else if (s5p_ehci->pdata->phy_init)
		s5p_ehci->pdata->phy_init(pdev, USB_PHY_TYPE_HOST);

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;

	/* DMA burst Enable */
	writel(EHCI_INSNREG00_ENABLE_DMA_BURST, EHCI_INSNREG00(hcd->regs));

	err = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (err) {
		dev_err(&pdev->dev, "Failed to add USB HCD\n");
		goto fail_add_hcd;
	}

	platform_set_drvdata(pdev, hcd);

	return 0;

fail_add_hcd:
	if (s5p_ehci->phy)
		usb_phy_shutdown(s5p_ehci->phy);
	else if (s5p_ehci->pdata->phy_exit)
		s5p_ehci->pdata->phy_exit(pdev, USB_PHY_TYPE_HOST);
fail_io:
	clk_disable_unprepare(s5p_ehci->clk);
fail_clk:
	usb_put_hcd(hcd);
	return err;
}

static int s5p_ehci_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct s5p_ehci_hcd *s5p_ehci = to_s5p_ehci(hcd);

	usb_remove_hcd(hcd);

	if (s5p_ehci->otg)
		s5p_ehci->otg->set_host(s5p_ehci->otg, &hcd->self);

	if (s5p_ehci->phy)
		usb_phy_shutdown(s5p_ehci->phy);
	else if (s5p_ehci->pdata->phy_exit)
		s5p_ehci->pdata->phy_exit(pdev, USB_PHY_TYPE_HOST);

	clk_disable_unprepare(s5p_ehci->clk);

	usb_put_hcd(hcd);

	return 0;
}

#ifdef CONFIG_PM
static int s5p_ehci_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct s5p_ehci_hcd *s5p_ehci = to_s5p_ehci(hcd);
	struct platform_device *pdev = to_platform_device(dev);

	bool do_wakeup = device_may_wakeup(dev);
	int rc;

	rc = ehci_suspend(hcd, do_wakeup);

	if (s5p_ehci->otg)
		s5p_ehci->otg->set_host(s5p_ehci->otg, &hcd->self);

	if (s5p_ehci->phy)
		usb_phy_shutdown(s5p_ehci->phy);
	else if (s5p_ehci->pdata->phy_exit)
		s5p_ehci->pdata->phy_exit(pdev, USB_PHY_TYPE_HOST);

	clk_disable_unprepare(s5p_ehci->clk);

	return rc;
}

static int s5p_ehci_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct  s5p_ehci_hcd *s5p_ehci = to_s5p_ehci(hcd);
	struct platform_device *pdev = to_platform_device(dev);

	clk_prepare_enable(s5p_ehci->clk);

	if (s5p_ehci->otg)
		s5p_ehci->otg->set_host(s5p_ehci->otg, &hcd->self);

	if (s5p_ehci->phy)
		usb_phy_init(s5p_ehci->phy);
	else if (s5p_ehci->pdata->phy_init)
		s5p_ehci->pdata->phy_init(pdev, USB_PHY_TYPE_HOST);

	/* DMA burst Enable */
	writel(EHCI_INSNREG00_ENABLE_DMA_BURST, EHCI_INSNREG00(hcd->regs));

	ehci_resume(hcd, false);
	return 0;
}
#else
#define s5p_ehci_suspend	NULL
#define s5p_ehci_resume		NULL
#endif

static const struct dev_pm_ops s5p_ehci_pm_ops = {
	.suspend	= s5p_ehci_suspend,
	.resume		= s5p_ehci_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id exynos_ehci_match[] = {
	{ .compatible = "samsung,exynos4210-ehci" },
	{ .compatible = "samsung,exynos5440-ehci" },
	{},
};
MODULE_DEVICE_TABLE(of, exynos_ehci_match);
#endif

static struct platform_driver s5p_ehci_driver = {
	.probe		= s5p_ehci_probe,
	.remove		= s5p_ehci_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver = {
		.name	= "s5p-ehci",
		.owner	= THIS_MODULE,
		.pm	= &s5p_ehci_pm_ops,
		.of_match_table = of_match_ptr(exynos_ehci_match),
	}
};
static const struct ehci_driver_overrides s5p_overrides __initdata = {
	.extra_priv_size = sizeof(struct s5p_ehci_hcd),
};

static int __init ehci_s5p_init(void)
{
	if (usb_disabled())
		return -ENODEV;

	pr_info("%s: " DRIVER_DESC "\n", hcd_name);
	ehci_init_driver(&s5p_ehci_hc_driver, &s5p_overrides);
	return platform_driver_register(&s5p_ehci_driver);
}
module_init(ehci_s5p_init);

static void __exit ehci_s5p_cleanup(void)
{
	platform_driver_unregister(&s5p_ehci_driver);
}
module_exit(ehci_s5p_cleanup);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_ALIAS("platform:s5p-ehci");
MODULE_AUTHOR("Jingoo Han");
MODULE_AUTHOR("Joonyoung Shim");
MODULE_LICENSE("GPL v2");
