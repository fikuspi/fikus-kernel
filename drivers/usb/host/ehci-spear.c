/*
* Driver for EHCI HCD on SPEAr SOC
*
* Copyright (C) 2010 ST Micro Electronics,
* Deepak Sikri <deepak.sikri@st.com>
*
* Based on various ehci-*.c drivers
*
* This file is subject to the terms and conditions of the GNU General Public
* License. See the file COPYING in the main directory of this archive for
* more details.
*/

#include <fikus/clk.h>
#include <fikus/dma-mapping.h>
#include <fikus/io.h>
#include <fikus/jiffies.h>
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/of.h>
#include <fikus/platform_device.h>
#include <fikus/pm.h>
#include <fikus/usb.h>
#include <fikus/usb/hcd.h>

#include "ehci.h"

#define DRIVER_DESC "EHCI SPEAr driver"

static const char hcd_name[] = "SPEAr-ehci";

struct spear_ehci {
	struct clk *clk;
};

#define to_spear_ehci(hcd)	(struct spear_ehci *)(hcd_to_ehci(hcd)->priv)

static struct hc_driver __read_mostly ehci_spear_hc_driver;

#ifdef CONFIG_PM_SLEEP
static int ehci_spear_drv_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	bool do_wakeup = device_may_wakeup(dev);

	return ehci_suspend(hcd, do_wakeup);
}

static int ehci_spear_drv_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	ehci_resume(hcd, false);
	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(ehci_spear_pm_ops, ehci_spear_drv_suspend,
		ehci_spear_drv_resume);

static int spear_ehci_hcd_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd ;
	struct spear_ehci *sehci;
	struct resource *res;
	struct clk *usbh_clk;
	const struct hc_driver *driver = &ehci_spear_hc_driver;
	int irq, retval;

	if (usb_disabled())
		return -ENODEV;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		retval = irq;
		goto fail;
	}

	/*
	 * Right now device-tree probed devices don't get dma_mask set.
	 * Since shared usb code relies on it, set it here for now.
	 * Once we have dma capability bindings this can go away.
	 */
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	if (!pdev->dev.coherent_dma_mask)
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	usbh_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(usbh_clk)) {
		dev_err(&pdev->dev, "Error getting interface clock\n");
		retval = PTR_ERR(usbh_clk);
		goto fail;
	}

	hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd) {
		retval = -ENOMEM;
		goto fail;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		retval = -ENODEV;
		goto err_put_hcd;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	if (!devm_request_mem_region(&pdev->dev, hcd->rsrc_start, hcd->rsrc_len,
				driver->description)) {
		retval = -EBUSY;
		goto err_put_hcd;
	}

	hcd->regs = devm_ioremap(&pdev->dev, hcd->rsrc_start, hcd->rsrc_len);
	if (hcd->regs == NULL) {
		dev_dbg(&pdev->dev, "error mapping memory\n");
		retval = -ENOMEM;
		goto err_put_hcd;
	}

	sehci = to_spear_ehci(hcd);
	sehci->clk = usbh_clk;

	/* registers start at offset 0x0 */
	hcd_to_ehci(hcd)->caps = hcd->regs;

	clk_prepare_enable(sehci->clk);
	retval = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (retval)
		goto err_stop_ehci;

	return retval;

err_stop_ehci:
	clk_disable_unprepare(sehci->clk);
err_put_hcd:
	usb_put_hcd(hcd);
fail:
	dev_err(&pdev->dev, "init fail, %d\n", retval);

	return retval ;
}

static int spear_ehci_hcd_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct spear_ehci *sehci = to_spear_ehci(hcd);

	usb_remove_hcd(hcd);

	if (sehci->clk)
		clk_disable_unprepare(sehci->clk);
	usb_put_hcd(hcd);

	return 0;
}

static struct of_device_id spear_ehci_id_table[] = {
	{ .compatible = "st,spear600-ehci", },
	{ },
};

static struct platform_driver spear_ehci_hcd_driver = {
	.probe		= spear_ehci_hcd_drv_probe,
	.remove		= spear_ehci_hcd_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver		= {
		.name = "spear-ehci",
		.bus = &platform_bus_type,
		.pm = &ehci_spear_pm_ops,
		.of_match_table = spear_ehci_id_table,
	}
};

static const struct ehci_driver_overrides spear_overrides __initdata = {
	.extra_priv_size = sizeof(struct spear_ehci),
};

static int __init ehci_spear_init(void)
{
	if (usb_disabled())
		return -ENODEV;

	pr_info("%s: " DRIVER_DESC "\n", hcd_name);

	ehci_init_driver(&ehci_spear_hc_driver, &spear_overrides);
	return platform_driver_register(&spear_ehci_hcd_driver);
}
module_init(ehci_spear_init);

static void __exit ehci_spear_cleanup(void)
{
	platform_driver_unregister(&spear_ehci_hcd_driver);
}
module_exit(ehci_spear_cleanup);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_ALIAS("platform:spear-ehci");
MODULE_AUTHOR("Deepak Sikri");
MODULE_LICENSE("GPL");
