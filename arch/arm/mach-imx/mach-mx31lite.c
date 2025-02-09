/*
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *  Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *  Copyright (C) 2009 Daniel Mack <daniel@caiaq.de>
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

#include <fikus/types.h>
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/memory.h>
#include <fikus/platform_device.h>
#include <fikus/gpio.h>
#include <fikus/moduleparam.h>
#include <fikus/smsc911x.h>
#include <fikus/mfd/mc13783.h>
#include <fikus/spi/spi.h>
#include <fikus/usb/otg.h>
#include <fikus/usb/ulpi.h>
#include <fikus/mtd/physmap.h>
#include <fikus/delay.h>
#include <fikus/regulator/machine.h>
#include <fikus/regulator/fixed.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/page.h>
#include <asm/setup.h>

#include "board-mx31lite.h"
#include "common.h"
#include "devices-imx31.h"
#include "hardware.h"
#include "iomux-mx3.h"
#include "ulpi.h"

/*
 * This file contains the module-specific initialization routines.
 */

static unsigned int mx31lite_pins[] = {
	/* LAN9117 IRQ pin */
	IOMUX_MODE(MX31_PIN_SFS6, IOMUX_CONFIG_GPIO),
	/* SPI 1 */
	MX31_PIN_CSPI2_SCLK__SCLK,
	MX31_PIN_CSPI2_MOSI__MOSI,
	MX31_PIN_CSPI2_MISO__MISO,
	MX31_PIN_CSPI2_SPI_RDY__SPI_RDY,
	MX31_PIN_CSPI2_SS0__SS0,
	MX31_PIN_CSPI2_SS1__SS1,
	MX31_PIN_CSPI2_SS2__SS2,
};

static const struct mxc_nand_platform_data
mx31lite_nand_board_info __initconst  = {
	.width = 1,
	.hw_ecc = 1,
};

static struct smsc911x_platform_config smsc911x_config = {
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_16BIT,
};

static struct resource smsc911x_resources[] = {
	{
		.start		= MX31_CS4_BASE_ADDR,
		.end		= MX31_CS4_BASE_ADDR + 0x100,
		.flags		= IORESOURCE_MEM,
	}, {
		/* irq number is run-time assigned */
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device smsc911x_device = {
	.name		= "smsc911x",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smsc911x_resources),
	.resource	= smsc911x_resources,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
};

/*
 * SPI
 *
 * The MC13783 is the only hard-wired SPI device on the module.
 */

static int spi_internal_chipselect[] = {
	MXC_SPI_CS(0),
};

static const struct spi_imx_master spi1_pdata __initconst = {
	.chipselect	= spi_internal_chipselect,
	.num_chipselect	= ARRAY_SIZE(spi_internal_chipselect),
};

static struct mc13xxx_platform_data mc13783_pdata __initdata = {
	.flags = MC13XXX_USE_RTC,
};

static struct spi_board_info mc13783_spi_dev __initdata = {
	.modalias       = "mc13783",
	.max_speed_hz   = 1000000,
	.bus_num	= 1,
	.chip_select    = 0,
	.platform_data  = &mc13783_pdata,
	/* irq number is run-time assigned */
};

/*
 * USB
 */

#define USB_PAD_CFG (PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST | PAD_CTL_HYS_CMOS | \
			PAD_CTL_ODE_CMOS | PAD_CTL_100K_PU)

static int usbh2_init(struct platform_device *pdev)
{
	int pins[] = {
		MX31_PIN_USBH2_DATA0__USBH2_DATA0,
		MX31_PIN_USBH2_DATA1__USBH2_DATA1,
		MX31_PIN_USBH2_CLK__USBH2_CLK,
		MX31_PIN_USBH2_DIR__USBH2_DIR,
		MX31_PIN_USBH2_NXT__USBH2_NXT,
		MX31_PIN_USBH2_STP__USBH2_STP,
	};

	mxc_iomux_setup_multiple_pins(pins, ARRAY_SIZE(pins), "USB H2");

	mxc_iomux_set_pad(MX31_PIN_USBH2_CLK, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_DIR, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_NXT, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_STP, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_DATA0, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_USBH2_DATA1, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_SRXD6, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_STXD6, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_SFS3, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_SCK3, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_SRXD3, USB_PAD_CFG);
	mxc_iomux_set_pad(MX31_PIN_STXD3, USB_PAD_CFG);

	mxc_iomux_set_gpr(MUX_PGP_UH2, true);

	/* chip select */
	mxc_iomux_alloc_pin(IOMUX_MODE(MX31_PIN_DTR_DCE1, IOMUX_CONFIG_GPIO),
				"USBH2_CS");
	gpio_request(IOMUX_TO_GPIO(MX31_PIN_DTR_DCE1), "USBH2 CS");
	gpio_direction_output(IOMUX_TO_GPIO(MX31_PIN_DTR_DCE1), 0);

	mdelay(10);

	return mx31_initialize_usb_hw(pdev->id, MXC_EHCI_POWER_PINS_ENABLED);
}

static struct mxc_usbh_platform_data usbh2_pdata __initdata = {
	.init   = usbh2_init,
	.portsc = MXC_EHCI_MODE_ULPI | MXC_EHCI_UTMI_8BIT,
};

/*
 * NOR flash
 */

static struct physmap_flash_data nor_flash_data = {
	.width  = 2,
};

static struct resource nor_flash_resource = {
	.start  = 0xa0000000,
	.end    = 0xa1ffffff,
	.flags  = IORESOURCE_MEM,
};

static struct platform_device physmap_flash_device = {
	.name   = "physmap-flash",
	.id     = 0,
	.dev    = {
		.platform_data  = &nor_flash_data,
	},
	.resource = &nor_flash_resource,
	.num_resources = 1,
};



/*
 * This structure defines the MX31 memory map.
 */
static struct map_desc mx31lite_io_desc[] __initdata = {
	{
		.virtual = (unsigned long)MX31_CS4_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(MX31_CS4_BASE_ADDR),
		.length = MX31_CS4_SIZE,
		.type = MT_DEVICE
	}
};

/*
 * Set up static virtual mappings.
 */
void __init mx31lite_map_io(void)
{
	mx31_map_io();
	iotable_init(mx31lite_io_desc, ARRAY_SIZE(mx31lite_io_desc));
}

static int mx31lite_baseboard;
core_param(mx31lite_baseboard, mx31lite_baseboard, int, 0444);

static struct regulator_consumer_supply dummy_supplies[] = {
	REGULATOR_SUPPLY("vdd33a", "smsc911x"),
	REGULATOR_SUPPLY("vddvario", "smsc911x"),
};

static void __init mx31lite_init(void)
{
	int ret;

	imx31_soc_init();

	switch (mx31lite_baseboard) {
	case MX31LITE_NOBOARD:
		break;
	case MX31LITE_DB:
		mx31lite_db_init();
		break;
	default:
		printk(KERN_ERR "Illegal mx31lite_baseboard type %d\n",
				mx31lite_baseboard);
	}

	mxc_iomux_setup_multiple_pins(mx31lite_pins, ARRAY_SIZE(mx31lite_pins),
				      "mx31lite");

	/* NOR and NAND flash */
	platform_device_register(&physmap_flash_device);
	imx31_add_mxc_nand(&mx31lite_nand_board_info);

	imx31_add_spi_imx1(&spi1_pdata);
	mc13783_spi_dev.irq = gpio_to_irq(IOMUX_TO_GPIO(MX31_PIN_GPIO1_3));
	spi_register_board_info(&mc13783_spi_dev, 1);

	/* USB */
	usbh2_pdata.otg = imx_otg_ulpi_create(ULPI_OTG_DRVVBUS |
			ULPI_OTG_DRVVBUS_EXT);
	if (usbh2_pdata.otg)
		imx31_add_mxc_ehci_hs(2, &usbh2_pdata);

	regulator_register_fixed(0, dummy_supplies, ARRAY_SIZE(dummy_supplies));

	/* SMSC9117 IRQ pin */
	ret = gpio_request(IOMUX_TO_GPIO(MX31_PIN_SFS6), "sms9117-irq");
	if (ret)
		pr_warning("could not get LAN irq gpio\n");
	else {
		gpio_direction_input(IOMUX_TO_GPIO(MX31_PIN_SFS6));
		smsc911x_resources[1].start =
			gpio_to_irq(IOMUX_TO_GPIO(MX31_PIN_SFS6));
		smsc911x_resources[1].end =
			gpio_to_irq(IOMUX_TO_GPIO(MX31_PIN_SFS6));
		platform_device_register(&smsc911x_device);
	}
}

static void __init mx31lite_timer_init(void)
{
	mx31_clocks_init(26000000);
}

MACHINE_START(MX31LITE, "LogicPD i.MX31 SOM")
	/* Maintainer: Freescale Semiconductor, Inc. */
	.atag_offset = 0x100,
	.map_io = mx31lite_map_io,
	.init_early = imx31_init_early,
	.init_irq = mx31_init_irq,
	.handle_irq = imx31_handle_irq,
	.init_time	= mx31lite_timer_init,
	.init_machine = mx31lite_init,
	.restart	= mxc_restart,
MACHINE_END
