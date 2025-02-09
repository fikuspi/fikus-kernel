/*
 * Copyright (C) 2010 Eric Benard - eric@eukrea.com
 * Copyright (C) 2009 Sascha Hauer, Pengutronix
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fikus/types.h>
#include <fikus/init.h>

#include <fikus/platform_device.h>
#include <fikus/mtd/physmap.h>
#include <fikus/memory.h>
#include <fikus/gpio.h>
#include <fikus/interrupt.h>
#include <fikus/delay.h>
#include <fikus/i2c.h>
#include <fikus/i2c/tsc2007.h>
#include <fikus/usb/otg.h>
#include <fikus/usb/ulpi.h>
#include <fikus/i2c-gpio.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>

#include "common.h"
#include "devices-imx35.h"
#include "eukrea-baseboards.h"
#include "hardware.h"
#include "iomux-mx35.h"

static const struct imxuart_platform_data uart_pdata __initconst = {
	.flags = IMXUART_HAVE_RTSCTS,
};

static const struct imxi2c_platform_data
		eukrea_cpuimx35_i2c0_data __initconst = {
	.bitrate =		100000,
};

#define TSC2007_IRQGPIO		IMX_GPIO_NR(3, 2)
static int tsc2007_get_pendown_state(void)
{
	return !gpio_get_value(TSC2007_IRQGPIO);
}

static struct tsc2007_platform_data tsc2007_info = {
	.model			= 2007,
	.x_plate_ohms		= 180,
	.get_pendown_state = tsc2007_get_pendown_state,
};

static struct i2c_board_info eukrea_cpuimx35_i2c_devices[] = {
	{
		I2C_BOARD_INFO("pcf8563", 0x51),
	}, {
		I2C_BOARD_INFO("tsc2007", 0x48),
		.platform_data	= &tsc2007_info,
		/* irq number is run-time assigned */
	},
};

static iomux_v3_cfg_t eukrea_cpuimx35_pads[] = {
	/* UART1 */
	MX35_PAD_CTS1__UART1_CTS,
	MX35_PAD_RTS1__UART1_RTS,
	MX35_PAD_TXD1__UART1_TXD_MUX,
	MX35_PAD_RXD1__UART1_RXD_MUX,
	/* FEC */
	MX35_PAD_FEC_TX_CLK__FEC_TX_CLK,
	MX35_PAD_FEC_RX_CLK__FEC_RX_CLK,
	MX35_PAD_FEC_RX_DV__FEC_RX_DV,
	MX35_PAD_FEC_COL__FEC_COL,
	MX35_PAD_FEC_RDATA0__FEC_RDATA_0,
	MX35_PAD_FEC_TDATA0__FEC_TDATA_0,
	MX35_PAD_FEC_TX_EN__FEC_TX_EN,
	MX35_PAD_FEC_MDC__FEC_MDC,
	MX35_PAD_FEC_MDIO__FEC_MDIO,
	MX35_PAD_FEC_TX_ERR__FEC_TX_ERR,
	MX35_PAD_FEC_RX_ERR__FEC_RX_ERR,
	MX35_PAD_FEC_CRS__FEC_CRS,
	MX35_PAD_FEC_RDATA1__FEC_RDATA_1,
	MX35_PAD_FEC_TDATA1__FEC_TDATA_1,
	MX35_PAD_FEC_RDATA2__FEC_RDATA_2,
	MX35_PAD_FEC_TDATA2__FEC_TDATA_2,
	MX35_PAD_FEC_RDATA3__FEC_RDATA_3,
	MX35_PAD_FEC_TDATA3__FEC_TDATA_3,
	/* I2C1 */
	MX35_PAD_I2C1_CLK__I2C1_SCL,
	MX35_PAD_I2C1_DAT__I2C1_SDA,
	/* TSC2007 IRQ */
	MX35_PAD_ATA_DA2__GPIO3_2,
};

static const struct mxc_nand_platform_data
		eukrea_cpuimx35_nand_board_info __initconst = {
	.width		= 1,
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static int eukrea_cpuimx35_otg_init(struct platform_device *pdev)
{
	return mx35_initialize_usb_hw(pdev->id, MXC_EHCI_INTERFACE_DIFF_UNI);
}

static const struct mxc_usbh_platform_data otg_pdata __initconst = {
	.init	= eukrea_cpuimx35_otg_init,
	.portsc	= MXC_EHCI_MODE_UTMI,
};

static int eukrea_cpuimx35_usbh1_init(struct platform_device *pdev)
{
	return mx35_initialize_usb_hw(pdev->id, MXC_EHCI_INTERFACE_SINGLE_UNI |
			MXC_EHCI_INTERNAL_PHY | MXC_EHCI_IPPUE_DOWN);
}

static const struct mxc_usbh_platform_data usbh1_pdata __initconst = {
	.init	= eukrea_cpuimx35_usbh1_init,
	.portsc	= MXC_EHCI_MODE_SERIAL,
};

static const struct fsl_usb2_platform_data otg_device_pdata __initconst = {
	.operating_mode	= FSL_USB2_DR_DEVICE,
	.phy_mode	= FSL_USB2_PHY_UTMI,
	.workaround	= FLS_USB2_WORKAROUND_ENGCM09152,
};

static bool otg_mode_host __initdata;

static int __init eukrea_cpuimx35_otg_mode(char *options)
{
	if (!strcmp(options, "host"))
		otg_mode_host = true;
	else if (!strcmp(options, "device"))
		otg_mode_host = false;
	else
		pr_info("otg_mode neither \"host\" nor \"device\". "
			"Defaulting to device\n");
	return 1;
}
__setup("otg_mode=", eukrea_cpuimx35_otg_mode);

/*
 * Board specific initialization.
 */
static void __init eukrea_cpuimx35_init(void)
{
	imx35_soc_init();

	mxc_iomux_v3_setup_multiple_pads(eukrea_cpuimx35_pads,
			ARRAY_SIZE(eukrea_cpuimx35_pads));

	imx35_add_fec(NULL);
	imx35_add_imx2_wdt();

	imx35_add_imx_uart0(&uart_pdata);
	imx35_add_mxc_nand(&eukrea_cpuimx35_nand_board_info);

	eukrea_cpuimx35_i2c_devices[1].irq = gpio_to_irq(TSC2007_IRQGPIO);
	i2c_register_board_info(0, eukrea_cpuimx35_i2c_devices,
			ARRAY_SIZE(eukrea_cpuimx35_i2c_devices));
	imx35_add_imx_i2c0(&eukrea_cpuimx35_i2c0_data);

	if (otg_mode_host)
		imx35_add_mxc_ehci_otg(&otg_pdata);
	else
		imx35_add_fsl_usb2_udc(&otg_device_pdata);

	imx35_add_mxc_ehci_hs(&usbh1_pdata);

#ifdef CONFIG_MACH_EUKREA_MBIMXSD35_BASEBOARD
	eukrea_mbimxsd35_baseboard_init();
#endif
}

static void __init eukrea_cpuimx35_timer_init(void)
{
	mx35_clocks_init();
}

MACHINE_START(EUKREA_CPUIMX35SD, "Eukrea CPUIMX35")
	/* Maintainer: Eukrea Electromatique */
	.atag_offset = 0x100,
	.map_io = mx35_map_io,
	.init_early = imx35_init_early,
	.init_irq = mx35_init_irq,
	.handle_irq = imx35_handle_irq,
	.init_time	= eukrea_cpuimx35_timer_init,
	.init_machine = eukrea_cpuimx35_init,
	.restart	= mxc_restart,
MACHINE_END
