/*
 * Copyright (C) 2009 Valentin Longchamp, EPFL Mobots group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <fikus/delay.h>
#include <fikus/gpio.h>
#include <fikus/init.h>
#include <fikus/interrupt.h>
#include <fikus/i2c.h>
#include <fikus/spi/spi.h>
#include <fikus/slab.h>
#include <fikus/platform_device.h>
#include <fikus/types.h>

#include <fikus/usb/otg.h>

#include <media/soc_camera.h>

#include "common.h"
#include "devices-imx31.h"
#include "hardware.h"
#include "iomux-mx3.h"
#include "ulpi.h"

static unsigned int marxbot_pins[] = {
	/* SDHC2 */
	MX31_PIN_PC_PWRON__SD2_DATA3, MX31_PIN_PC_VS1__SD2_DATA2,
	MX31_PIN_PC_READY__SD2_DATA1, MX31_PIN_PC_WAIT_B__SD2_DATA0,
	MX31_PIN_PC_CD2_B__SD2_CLK, MX31_PIN_PC_CD1_B__SD2_CMD,
	MX31_PIN_ATA_DIOR__GPIO3_28, MX31_PIN_ATA_DIOW__GPIO3_29,
	/* CSI */
	MX31_PIN_CSI_D6__CSI_D6, MX31_PIN_CSI_D7__CSI_D7,
	MX31_PIN_CSI_D8__CSI_D8, MX31_PIN_CSI_D9__CSI_D9,
	MX31_PIN_CSI_D10__CSI_D10, MX31_PIN_CSI_D11__CSI_D11,
	MX31_PIN_CSI_D12__CSI_D12, MX31_PIN_CSI_D13__CSI_D13,
	MX31_PIN_CSI_D14__CSI_D14, MX31_PIN_CSI_D15__CSI_D15,
	MX31_PIN_CSI_HSYNC__CSI_HSYNC, MX31_PIN_CSI_MCLK__CSI_MCLK,
	MX31_PIN_CSI_PIXCLK__CSI_PIXCLK, MX31_PIN_CSI_VSYNC__CSI_VSYNC,
	MX31_PIN_CSI_D4__GPIO3_4, MX31_PIN_CSI_D5__GPIO3_5,
	MX31_PIN_GPIO3_0__GPIO3_0, MX31_PIN_GPIO3_1__GPIO3_1,
	MX31_PIN_TXD2__GPIO1_28,
	/* dsPIC resets */
	MX31_PIN_STXD5__GPIO1_21, MX31_PIN_SRXD5__GPIO1_22,
	/*battery detection */
	MX31_PIN_LCS0__GPIO3_23,
	/* USB H1 */
	MX31_PIN_CSPI1_MISO__USBH1_RXDP, MX31_PIN_CSPI1_MOSI__USBH1_RXDM,
	MX31_PIN_CSPI1_SS0__USBH1_TXDM, MX31_PIN_CSPI1_SS1__USBH1_TXDP,
	MX31_PIN_CSPI1_SS2__USBH1_RCV, MX31_PIN_CSPI1_SCLK__USBH1_OEB,
	MX31_PIN_CSPI1_SPI_RDY__USBH1_FS, MX31_PIN_SFS6__USBH1_SUSPEND,
	MX31_PIN_NFRE_B__GPIO1_11, MX31_PIN_NFALE__GPIO1_12,
	/* SEL */
	MX31_PIN_DTR_DCE1__GPIO2_8, MX31_PIN_DSR_DCE1__GPIO2_9,
	MX31_PIN_RI_DCE1__GPIO2_10, MX31_PIN_DCD_DCE1__GPIO2_11,
};

#define SDHC2_CD IOMUX_TO_GPIO(MX31_PIN_ATA_DIOR)
#define SDHC2_WP IOMUX_TO_GPIO(MX31_PIN_ATA_DIOW)

static int marxbot_sdhc2_get_ro(struct device *dev)
{
	return !gpio_get_value(SDHC2_WP);
}

static int marxbot_sdhc2_init(struct device *dev, irq_handler_t detect_irq,
		void *data)
{
	int ret;

	ret = gpio_request(SDHC2_CD, "sdhc-detect");
	if (ret)
		return ret;

	gpio_direction_input(SDHC2_CD);

	ret = gpio_request(SDHC2_WP, "sdhc-wp");
	if (ret)
		goto err_gpio_free;
	gpio_direction_input(SDHC2_WP);

	ret = request_irq(gpio_to_irq(SDHC2_CD), detect_irq,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		"sdhc2-card-detect", data);
	if (ret)
		goto err_gpio_free_2;

	return 0;

err_gpio_free_2:
	gpio_free(SDHC2_WP);
err_gpio_free:
	gpio_free(SDHC2_CD);

	return ret;
}

static void marxbot_sdhc2_exit(struct device *dev, void *data)
{
	free_irq(gpio_to_irq(SDHC2_CD), data);
	gpio_free(SDHC2_WP);
	gpio_free(SDHC2_CD);
}

static const struct imxmmc_platform_data sdhc2_pdata __initconst = {
	.get_ro	= marxbot_sdhc2_get_ro,
	.init	= marxbot_sdhc2_init,
	.exit	= marxbot_sdhc2_exit,
};

#define TRSLAT_RST_B	IOMUX_TO_GPIO(MX31_PIN_STXD5)
#define DSPICS_RST_B	IOMUX_TO_GPIO(MX31_PIN_SRXD5)

static void dspics_resets_init(void)
{
	if (!gpio_request(TRSLAT_RST_B, "translator-rst")) {
		gpio_direction_output(TRSLAT_RST_B, 0);
		gpio_export(TRSLAT_RST_B, false);
	}

	if (!gpio_request(DSPICS_RST_B, "dspics-rst")) {
		gpio_direction_output(DSPICS_RST_B, 0);
		gpio_export(DSPICS_RST_B, false);
	}
}

static struct spi_board_info marxbot_spi_board_info[] __initdata = {
	{
		.modalias = "spidev",
		.max_speed_hz = 300000,
		.bus_num = 1,
		.chip_select = 1, /* according spi1_cs[] ! */
	},
};

#define TURRETCAM_POWER	IOMUX_TO_GPIO(MX31_PIN_GPIO3_1)
#define BASECAM_POWER	IOMUX_TO_GPIO(MX31_PIN_CSI_D5)
#define TURRETCAM_RST_B	IOMUX_TO_GPIO(MX31_PIN_GPIO3_0)
#define BASECAM_RST_B	IOMUX_TO_GPIO(MX31_PIN_CSI_D4)
#define CAM_CHOICE	IOMUX_TO_GPIO(MX31_PIN_TXD2)

static int marxbot_basecam_power(struct device *dev, int on)
{
	gpio_set_value(BASECAM_POWER, !on);
	return 0;
}

static int marxbot_basecam_reset(struct device *dev)
{
	gpio_set_value(BASECAM_RST_B, 0);
	udelay(100);
	gpio_set_value(BASECAM_RST_B, 1);
	return 0;
}

static struct i2c_board_info marxbot_i2c_devices[] = {
	{
		I2C_BOARD_INFO("mt9t031", 0x5d),
	},
};

static struct soc_camera_link base_iclink = {
	.bus_id		= 0,		/* Must match with the camera ID */
	.power		= marxbot_basecam_power,
	.reset		= marxbot_basecam_reset,
	.board_info	= &marxbot_i2c_devices[0],
	.i2c_adapter_id	= 0,
};

static struct platform_device marxbot_camera[] = {
	{
		.name	= "soc-camera-pdrv",
		.id	= 0,
		.dev	= {
			.platform_data = &base_iclink,
		},
	},
};

static struct platform_device *marxbot_cameras[] __initdata = {
	&marxbot_camera[0],
};

static int __init marxbot_cam_init(void)
{
	int ret = gpio_request(CAM_CHOICE, "cam-choice");
	if (ret)
		return ret;
	gpio_direction_output(CAM_CHOICE, 0);

	ret = gpio_request(BASECAM_RST_B, "basecam-reset");
	if (ret)
		return ret;
	gpio_direction_output(BASECAM_RST_B, 1);
	ret = gpio_request(BASECAM_POWER, "basecam-standby");
	if (ret)
		return ret;
	gpio_direction_output(BASECAM_POWER, 0);

	ret = gpio_request(TURRETCAM_RST_B, "turretcam-reset");
	if (ret)
		return ret;
	gpio_direction_output(TURRETCAM_RST_B, 1);
	ret = gpio_request(TURRETCAM_POWER, "turretcam-standby");
	if (ret)
		return ret;
	gpio_direction_output(TURRETCAM_POWER, 0);

	return 0;
}

#define SEL0 IOMUX_TO_GPIO(MX31_PIN_DTR_DCE1)
#define SEL1 IOMUX_TO_GPIO(MX31_PIN_DSR_DCE1)
#define SEL2 IOMUX_TO_GPIO(MX31_PIN_RI_DCE1)
#define SEL3 IOMUX_TO_GPIO(MX31_PIN_DCD_DCE1)

static void marxbot_init_sel_gpios(void)
{
	if (!gpio_request(SEL0, "sel0")) {
		gpio_direction_input(SEL0);
		gpio_export(SEL0, true);
	}

	if (!gpio_request(SEL1, "sel1")) {
		gpio_direction_input(SEL1);
		gpio_export(SEL1, true);
	}

	if (!gpio_request(SEL2, "sel2")) {
		gpio_direction_input(SEL2);
		gpio_export(SEL2, true);
	}

	if (!gpio_request(SEL3, "sel3")) {
		gpio_direction_input(SEL3);
		gpio_export(SEL3, true);
	}
}

#define USB_PAD_CFG (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST | PAD_CTL_HYS_CMOS | \
			PAD_CTL_ODE_CMOS | PAD_CTL_100K_PU)

static int marxbot_usbh1_hw_init(struct platform_device *pdev)
{
	mxc_iomux_set_gpr(MUX_PGP_USB_SUSPEND, true);

	mxc_iomux_set_pad(MX31_PIN_CSPI1_MISO, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_CSPI1_MOSI, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS0, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS1, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SS2, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SCLK, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_CSPI1_SPI_RDY, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_SFS6, USB_PAD_CFG);

	mdelay(10);

	return mx31_initialize_usb_hw(pdev->id, MXC_EHCI_POWER_PINS_ENABLED |
			MXC_EHCI_INTERFACE_SINGLE_UNI);
}

#define USBH1_VBUSEN_B	IOMUX_TO_GPIO(MX31_PIN_NFRE_B)
#define USBH1_MODE	IOMUX_TO_GPIO(MX31_PIN_NFALE)

static int marxbot_isp1105_init(struct usb_phy *otg)
{
	int ret = gpio_request(USBH1_MODE, "usbh1-mode");
	if (ret)
		return ret;
	/* single ended */
	gpio_direction_output(USBH1_MODE, 0);

	ret = gpio_request(USBH1_VBUSEN_B, "usbh1-vbusen");
	if (ret) {
		gpio_free(USBH1_MODE);
		return ret;
	}
	gpio_direction_output(USBH1_VBUSEN_B, 1);

	return 0;
}


static int marxbot_isp1105_set_vbus(struct usb_otg *otg, bool on)
{
	if (on)
		gpio_set_value(USBH1_VBUSEN_B, 0);
	else
		gpio_set_value(USBH1_VBUSEN_B, 1);

	return 0;
}

static struct mxc_usbh_platform_data usbh1_pdata __initdata = {
	.init	= marxbot_usbh1_hw_init,
	.portsc	= MXC_EHCI_MODE_UTMI | MXC_EHCI_SERIAL,
};

static int __init marxbot_usbh1_init(void)
{
	struct usb_phy *phy;
	struct platform_device *pdev;

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	phy->otg = kzalloc(sizeof(struct usb_otg), GFP_KERNEL);
	if (!phy->otg) {
		kfree(phy);
		return -ENOMEM;
	}

	phy->label	= "ISP1105";
	phy->init	= marxbot_isp1105_init;
	phy->otg->set_vbus	= marxbot_isp1105_set_vbus;

	usbh1_pdata.otg = phy;

	pdev = imx31_add_mxc_ehci_hs(1, &usbh1_pdata);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);

	return 0;
}

static const struct fsl_usb2_platform_data usb_pdata __initconst = {
	.operating_mode	= FSL_USB2_DR_DEVICE,
	.phy_mode	= FSL_USB2_PHY_ULPI,
};

/*
 * system init for baseboard usage. Will be called by mx31moboard init.
 */
void __init mx31moboard_marxbot_init(void)
{
	printk(KERN_INFO "Initializing mx31marxbot peripherals\n");

	mxc_iomux_setup_multiple_pins(marxbot_pins, ARRAY_SIZE(marxbot_pins),
		"marxbot");

	marxbot_init_sel_gpios();

	dspics_resets_init();

	imx31_add_mxc_mmc(1, &sdhc2_pdata);

	spi_register_board_info(marxbot_spi_board_info,
		ARRAY_SIZE(marxbot_spi_board_info));

	marxbot_cam_init();
	platform_add_devices(marxbot_cameras, ARRAY_SIZE(marxbot_cameras));

	/* battery present pin */
	gpio_request(IOMUX_TO_GPIO(MX31_PIN_LCS0), "bat-present");
	gpio_direction_input(IOMUX_TO_GPIO(MX31_PIN_LCS0));
	gpio_export(IOMUX_TO_GPIO(MX31_PIN_LCS0), false);

	imx31_add_fsl_usb2_udc(&usb_pdata);

	marxbot_usbh1_init();
}
