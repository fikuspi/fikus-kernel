#include <fikus/module.h>
#include <fikus/platform_device.h>
#include <fikus/dma-mapping.h>
#include <fikus/usb/otg.h>
#include <fikus/usb/usb_phy_gen_xceiv.h>
#include <fikus/slab.h>
#include <fikus/clk.h>
#include <fikus/regulator/consumer.h>
#include <fikus/of.h>
#include <fikus/of_address.h>

#include "am35x-phy-control.h"
#include "phy-generic.h"

struct am335x_phy {
	struct usb_phy_gen_xceiv usb_phy_gen;
	struct phy_control *phy_ctrl;
	int id;
};

static int am335x_init(struct usb_phy *phy)
{
	struct am335x_phy *am_phy = dev_get_drvdata(phy->dev);

	phy_ctrl_power(am_phy->phy_ctrl, am_phy->id, true);
	return 0;
}

static void am335x_shutdown(struct usb_phy *phy)
{
	struct am335x_phy *am_phy = dev_get_drvdata(phy->dev);

	phy_ctrl_power(am_phy->phy_ctrl, am_phy->id, false);
}

static int am335x_phy_probe(struct platform_device *pdev)
{
	struct am335x_phy *am_phy;
	struct device *dev = &pdev->dev;
	int ret;

	am_phy = devm_kzalloc(dev, sizeof(*am_phy), GFP_KERNEL);
	if (!am_phy)
		return -ENOMEM;

	am_phy->phy_ctrl = am335x_get_phy_control(dev);
	if (!am_phy->phy_ctrl)
		return -EPROBE_DEFER;
	am_phy->id = of_alias_get_id(pdev->dev.of_node, "phy");
	if (am_phy->id < 0) {
		dev_err(&pdev->dev, "Missing PHY id: %d\n", am_phy->id);
		return am_phy->id;
	}

	ret = usb_phy_gen_create_phy(dev, &am_phy->usb_phy_gen,
			USB_PHY_TYPE_USB2, 0, false, false);
	if (ret)
		return ret;

	ret = usb_add_phy_dev(&am_phy->usb_phy_gen.phy);
	if (ret)
		goto err_add;
	am_phy->usb_phy_gen.phy.init = am335x_init;
	am_phy->usb_phy_gen.phy.shutdown = am335x_shutdown;

	platform_set_drvdata(pdev, am_phy);
	return 0;

err_add:
	usb_phy_gen_cleanup_phy(&am_phy->usb_phy_gen);
	return ret;
}

static int am335x_phy_remove(struct platform_device *pdev)
{
	struct am335x_phy *am_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&am_phy->usb_phy_gen.phy);
	return 0;
}

static const struct of_device_id am335x_phy_ids[] = {
	{ .compatible = "ti,am335x-usb-phy" },
	{ }
};
MODULE_DEVICE_TABLE(of, am335x_phy_ids);

static struct platform_driver am335x_phy_driver = {
	.probe          = am335x_phy_probe,
	.remove         = am335x_phy_remove,
	.driver         = {
		.name   = "am335x-phy-driver",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(am335x_phy_ids),
	},
};

module_platform_driver(am335x_phy_driver);
MODULE_LICENSE("GPL v2");
