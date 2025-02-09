/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2008 by Sascha Hauer <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#ifndef __MACH_IOMUX_MX3_H__
#define __MACH_IOMUX_MX3_H__

#include <fikus/types.h>
/*
 * various IOMUX output functions
 */

#define	IOMUX_OCONFIG_GPIO (0 << 4)	/* used as GPIO */
#define	IOMUX_OCONFIG_FUNC (1 << 4)	/* used as function */
#define	IOMUX_OCONFIG_ALT1 (2 << 4)	/* used as alternate function 1 */
#define	IOMUX_OCONFIG_ALT2 (3 << 4)	/* used as alternate function 2 */
#define	IOMUX_OCONFIG_ALT3 (4 << 4)	/* used as alternate function 3 */
#define	IOMUX_OCONFIG_ALT4 (5 << 4)	/* used as alternate function 4 */
#define	IOMUX_OCONFIG_ALT5 (6 << 4)	/* used as alternate function 5 */
#define	IOMUX_OCONFIG_ALT6 (7 << 4)	/* used as alternate function 6 */
#define	IOMUX_ICONFIG_NONE  0		/* not configured for input */
#define	IOMUX_ICONFIG_GPIO  1		/* used as GPIO */
#define	IOMUX_ICONFIG_FUNC  2		/* used as function */
#define	IOMUX_ICONFIG_ALT1  4		/* used as alternate function 1 */
#define	IOMUX_ICONFIG_ALT2  8		/* used as alternate function 2 */

#define IOMUX_CONFIG_GPIO (IOMUX_OCONFIG_GPIO | IOMUX_ICONFIG_GPIO)
#define IOMUX_CONFIG_FUNC (IOMUX_OCONFIG_FUNC | IOMUX_ICONFIG_FUNC)
#define IOMUX_CONFIG_ALT1 (IOMUX_OCONFIG_ALT1 | IOMUX_ICONFIG_ALT1)
#define IOMUX_CONFIG_ALT2 (IOMUX_OCONFIG_ALT2 | IOMUX_ICONFIG_ALT2)

/*
 * various IOMUX pad functions
 */
enum iomux_pad_config {
	PAD_CTL_NOLOOPBACK	= 0x0 << 9,
	PAD_CTL_LOOPBACK	= 0x1 << 9,
	PAD_CTL_PKE_NONE	= 0x0 << 8,
	PAD_CTL_PKE_ENABLE	= 0x1 << 8,
	PAD_CTL_PUE_KEEPER	= 0x0 << 7,
	PAD_CTL_PUE_PUD		= 0x1 << 7,
	PAD_CTL_100K_PD		= 0x0 << 5,
	PAD_CTL_100K_PU		= 0x1 << 5,
	PAD_CTL_47K_PU		= 0x2 << 5,
	PAD_CTL_22K_PU		= 0x3 << 5,
	PAD_CTL_HYS_CMOS	= 0x0 << 4,
	PAD_CTL_HYS_SCHMITZ	= 0x1 << 4,
	PAD_CTL_ODE_CMOS	= 0x0 << 3,
	PAD_CTL_ODE_OpenDrain	= 0x1 << 3,
	PAD_CTL_DRV_NORMAL	= 0x0 << 1,
	PAD_CTL_DRV_HIGH	= 0x1 << 1,
	PAD_CTL_DRV_MAX		= 0x2 << 1,
	PAD_CTL_SRE_SLOW	= 0x0 << 0,
	PAD_CTL_SRE_FAST	= 0x1 << 0
};

/*
 * various IOMUX general purpose functions
 */
enum iomux_gp_func {
	MUX_PGP_FIRI			= 1 << 0,
	MUX_DDR_MODE			= 1 << 1,
	MUX_PGP_CSPI_BB			= 1 << 2,
	MUX_PGP_ATA_1			= 1 << 3,
	MUX_PGP_ATA_2			= 1 << 4,
	MUX_PGP_ATA_3			= 1 << 5,
	MUX_PGP_ATA_4			= 1 << 6,
	MUX_PGP_ATA_5			= 1 << 7,
	MUX_PGP_ATA_6			= 1 << 8,
	MUX_PGP_ATA_7			= 1 << 9,
	MUX_PGP_ATA_8			= 1 << 10,
	MUX_PGP_UH2			= 1 << 11,
	MUX_SDCTL_CSD0_SEL		= 1 << 12,
	MUX_SDCTL_CSD1_SEL		= 1 << 13,
	MUX_CSPI1_UART3			= 1 << 14,
	MUX_EXTDMAREQ2_MBX_SEL		= 1 << 15,
	MUX_TAMPER_DETECT_EN		= 1 << 16,
	MUX_PGP_USB_4WIRE		= 1 << 17,
	MUX_PGP_USB_COMMON		= 1 << 18,
	MUX_SDHC_MEMSTICK1		= 1 << 19,
	MUX_SDHC_MEMSTICK2		= 1 << 20,
	MUX_PGP_SPLL_BYP		= 1 << 21,
	MUX_PGP_UPLL_BYP		= 1 << 22,
	MUX_PGP_MSHC1_CLK_SEL		= 1 << 23,
	MUX_PGP_MSHC2_CLK_SEL		= 1 << 24,
	MUX_CSPI3_UART5_SEL		= 1 << 25,
	MUX_PGP_ATA_9			= 1 << 26,
	MUX_PGP_USB_SUSPEND		= 1 << 27,
	MUX_PGP_USB_OTG_LOOPBACK	= 1 << 28,
	MUX_PGP_USB_HS1_LOOPBACK	= 1 << 29,
	MUX_PGP_USB_HS2_LOOPBACK	= 1 << 30,
	MUX_CLKO_DDR_MODE		= 1 << 31,
};

/*
 * setups a single pin:
 * 	- reserves the pin so that it is not claimed by another driver
 * 	- setups the iomux according to the configuration
 * 	- if the pin is configured as a GPIO, we claim it through kernel gpiolib
 */
int mxc_iomux_alloc_pin(unsigned int pin, const char *label);
/*
 * setups mutliple pins
 * convenient way to call the above function with tables
 */
int mxc_iomux_setup_multiple_pins(const unsigned int *pin_list, unsigned count,
		const char *label);

/*
 * releases a single pin:
 * 	- make it available for a future use by another driver
 * 	- frees the GPIO if the pin was configured as GPIO
 * 	- DOES NOT reconfigure the IOMUX in its reset state
 */
void mxc_iomux_release_pin(unsigned int pin);
/*
 * releases multiple pins
 * convenvient way to call the above function with tables
 */
void mxc_iomux_release_multiple_pins(const unsigned int *pin_list, int count);

/*
 * This function enables/disables the general purpose function for a particular
 * signal.
 */
void mxc_iomux_set_gpr(enum iomux_gp_func, bool en);

/*
 * This function only configures the iomux hardware.
 * It is called by the setup functions and should not be called directly anymore.
 * It is here visible for backward compatibility
 */
int mxc_iomux_mode(unsigned int pin_mode);

#define IOMUX_PADNUM_MASK	0x1ff
#define IOMUX_GPIONUM_SHIFT	9
#define IOMUX_GPIONUM_MASK	(0xff << IOMUX_GPIONUM_SHIFT)
#define IOMUX_MODE_SHIFT	17
#define IOMUX_MODE_MASK	(0xff << IOMUX_MODE_SHIFT)

#define IOMUX_PIN(gpionum, padnum) \
	(((gpionum << IOMUX_GPIONUM_SHIFT) & IOMUX_GPIONUM_MASK) | \
	 (padnum & IOMUX_PADNUM_MASK))

#define IOMUX_MODE(pin, mode) (pin | mode << IOMUX_MODE_SHIFT)

#define IOMUX_TO_GPIO(iomux_pin) \
	((iomux_pin & IOMUX_GPIONUM_MASK) >> IOMUX_GPIONUM_SHIFT)

/*
 * This enumeration is constructed based on the Section
 * "sw_pad_ctl & sw_mux_ctl details" of the MX31 IC Spec. Each enumerated
 * value is constructed based on the rules described above.
 */

enum iomux_pins {
	MX31_PIN_TTM_PAD	= IOMUX_PIN(0xff,   0),
	MX31_PIN_CSPI3_SPI_RDY	= IOMUX_PIN(0xff,   1),
	MX31_PIN_CSPI3_SCLK	= IOMUX_PIN(0xff,   2),
	MX31_PIN_CSPI3_MISO	= IOMUX_PIN(0xff,   3),
	MX31_PIN_CSPI3_MOSI	= IOMUX_PIN(0xff,   4),
	MX31_PIN_CLKSS		= IOMUX_PIN(0xff,   5),
	MX31_PIN_CE_CONTROL	= IOMUX_PIN(0xff,   6),
	MX31_PIN_ATA_RESET_B	= IOMUX_PIN(95,     7),
	MX31_PIN_ATA_DMACK	= IOMUX_PIN(94,     8),
	MX31_PIN_ATA_DIOW	= IOMUX_PIN(93,     9),
	MX31_PIN_ATA_DIOR	= IOMUX_PIN(92,    10),
	MX31_PIN_ATA_CS1	= IOMUX_PIN(91,    11),
	MX31_PIN_ATA_CS0	= IOMUX_PIN(90,    12),
	MX31_PIN_SD1_DATA3	= IOMUX_PIN(63,    13),
	MX31_PIN_SD1_DATA2	= IOMUX_PIN(62,    14),
	MX31_PIN_SD1_DATA1	= IOMUX_PIN(61,    15),
	MX31_PIN_SD1_DATA0	= IOMUX_PIN(60,    16),
	MX31_PIN_SD1_CLK	= IOMUX_PIN(59,    17),
	MX31_PIN_SD1_CMD	= IOMUX_PIN(58,    18),
	MX31_PIN_D3_SPL		= IOMUX_PIN(0xff,  19),
	MX31_PIN_D3_CLS		= IOMUX_PIN(0xff,  20),
	MX31_PIN_D3_REV		= IOMUX_PIN(0xff,  21),
	MX31_PIN_CONTRAST	= IOMUX_PIN(0xff,  22),
	MX31_PIN_VSYNC3		= IOMUX_PIN(0xff,  23),
	MX31_PIN_READ		= IOMUX_PIN(0xff,  24),
	MX31_PIN_WRITE		= IOMUX_PIN(0xff,  25),
	MX31_PIN_PAR_RS		= IOMUX_PIN(0xff,  26),
	MX31_PIN_SER_RS		= IOMUX_PIN(89,    27),
	MX31_PIN_LCS1		= IOMUX_PIN(88,    28),
	MX31_PIN_LCS0		= IOMUX_PIN(87,    29),
	MX31_PIN_SD_D_CLK	= IOMUX_PIN(86,    30),
	MX31_PIN_SD_D_IO	= IOMUX_PIN(85,    31),
	MX31_PIN_SD_D_I		= IOMUX_PIN(84,    32),
	MX31_PIN_DRDY0		= IOMUX_PIN(0xff,  33),
	MX31_PIN_FPSHIFT	= IOMUX_PIN(0xff,  34),
	MX31_PIN_HSYNC		= IOMUX_PIN(0xff,  35),
	MX31_PIN_VSYNC0		= IOMUX_PIN(0xff,  36),
	MX31_PIN_LD17		= IOMUX_PIN(0xff,  37),
	MX31_PIN_LD16		= IOMUX_PIN(0xff,  38),
	MX31_PIN_LD15		= IOMUX_PIN(0xff,  39),
	MX31_PIN_LD14		= IOMUX_PIN(0xff,  40),
	MX31_PIN_LD13		= IOMUX_PIN(0xff,  41),
	MX31_PIN_LD12		= IOMUX_PIN(0xff,  42),
	MX31_PIN_LD11		= IOMUX_PIN(0xff,  43),
	MX31_PIN_LD10		= IOMUX_PIN(0xff,  44),
	MX31_PIN_LD9		= IOMUX_PIN(0xff,  45),
	MX31_PIN_LD8		= IOMUX_PIN(0xff,  46),
	MX31_PIN_LD7		= IOMUX_PIN(0xff,  47),
	MX31_PIN_LD6		= IOMUX_PIN(0xff,  48),
	MX31_PIN_LD5		= IOMUX_PIN(0xff,  49),
	MX31_PIN_LD4		= IOMUX_PIN(0xff,  50),
	MX31_PIN_LD3		= IOMUX_PIN(0xff,  51),
	MX31_PIN_LD2		= IOMUX_PIN(0xff,  52),
	MX31_PIN_LD1		= IOMUX_PIN(0xff,  53),
	MX31_PIN_LD0		= IOMUX_PIN(0xff,  54),
	MX31_PIN_USBH2_DATA1	= IOMUX_PIN(0xff,  55),
	MX31_PIN_USBH2_DATA0	= IOMUX_PIN(0xff,  56),
	MX31_PIN_USBH2_NXT	= IOMUX_PIN(0xff,  57),
	MX31_PIN_USBH2_STP	= IOMUX_PIN(0xff,  58),
	MX31_PIN_USBH2_DIR	= IOMUX_PIN(0xff,  59),
	MX31_PIN_USBH2_CLK	= IOMUX_PIN(0xff,  60),
	MX31_PIN_USBOTG_DATA7	= IOMUX_PIN(0xff,  61),
	MX31_PIN_USBOTG_DATA6	= IOMUX_PIN(0xff,  62),
	MX31_PIN_USBOTG_DATA5	= IOMUX_PIN(0xff,  63),
	MX31_PIN_USBOTG_DATA4	= IOMUX_PIN(0xff,  64),
	MX31_PIN_USBOTG_DATA3	= IOMUX_PIN(0xff,  65),
	MX31_PIN_USBOTG_DATA2	= IOMUX_PIN(0xff,  66),
	MX31_PIN_USBOTG_DATA1	= IOMUX_PIN(0xff,  67),
	MX31_PIN_USBOTG_DATA0	= IOMUX_PIN(0xff,  68),
	MX31_PIN_USBOTG_NXT	= IOMUX_PIN(0xff,  69),
	MX31_PIN_USBOTG_STP	= IOMUX_PIN(0xff,  70),
	MX31_PIN_USBOTG_DIR	= IOMUX_PIN(0xff,  71),
	MX31_PIN_USBOTG_CLK	= IOMUX_PIN(0xff,  72),
	MX31_PIN_USB_BYP	= IOMUX_PIN(31,    73),
	MX31_PIN_USB_OC		= IOMUX_PIN(30,    74),
	MX31_PIN_USB_PWR	= IOMUX_PIN(29,    75),
	MX31_PIN_SJC_MOD	= IOMUX_PIN(0xff,  76),
	MX31_PIN_DE_B		= IOMUX_PIN(0xff,  77),
	MX31_PIN_TRSTB		= IOMUX_PIN(0xff,  78),
	MX31_PIN_TDO		= IOMUX_PIN(0xff,  79),
	MX31_PIN_TDI		= IOMUX_PIN(0xff,  80),
	MX31_PIN_TMS		= IOMUX_PIN(0xff,  81),
	MX31_PIN_TCK		= IOMUX_PIN(0xff,  82),
	MX31_PIN_RTCK		= IOMUX_PIN(0xff,  83),
	MX31_PIN_KEY_COL7	= IOMUX_PIN(57,    84),
	MX31_PIN_KEY_COL6	= IOMUX_PIN(56,    85),
	MX31_PIN_KEY_COL5	= IOMUX_PIN(55,    86),
	MX31_PIN_KEY_COL4	= IOMUX_PIN(54,    87),
	MX31_PIN_KEY_COL3	= IOMUX_PIN(0xff,  88),
	MX31_PIN_KEY_COL2	= IOMUX_PIN(0xff,  89),
	MX31_PIN_KEY_COL1	= IOMUX_PIN(0xff,  90),
	MX31_PIN_KEY_COL0	= IOMUX_PIN(0xff,  91),
	MX31_PIN_KEY_ROW7	= IOMUX_PIN(53,    92),
	MX31_PIN_KEY_ROW6	= IOMUX_PIN(52,    93),
	MX31_PIN_KEY_ROW5	= IOMUX_PIN(51,    94),
	MX31_PIN_KEY_ROW4	= IOMUX_PIN(50,    95),
	MX31_PIN_KEY_ROW3	= IOMUX_PIN(0xff,  96),
	MX31_PIN_KEY_ROW2	= IOMUX_PIN(0xff,  97),
	MX31_PIN_KEY_ROW1	= IOMUX_PIN(0xff,  98),
	MX31_PIN_KEY_ROW0	= IOMUX_PIN(0xff,  99),
	MX31_PIN_BATT_LINE	= IOMUX_PIN(49,   100),
	MX31_PIN_CTS2		= IOMUX_PIN(0xff, 101),
	MX31_PIN_RTS2		= IOMUX_PIN(0xff, 102),
	MX31_PIN_TXD2		= IOMUX_PIN(28,   103),
	MX31_PIN_RXD2		= IOMUX_PIN(27,   104),
	MX31_PIN_DTR_DCE2	= IOMUX_PIN(48,   105),
	MX31_PIN_DCD_DTE1	= IOMUX_PIN(47,   106),
	MX31_PIN_RI_DTE1	= IOMUX_PIN(46,   107),
	MX31_PIN_DSR_DTE1	= IOMUX_PIN(45,   108),
	MX31_PIN_DTR_DTE1	= IOMUX_PIN(44,   109),
	MX31_PIN_DCD_DCE1	= IOMUX_PIN(43,   110),
	MX31_PIN_RI_DCE1	= IOMUX_PIN(42,   111),
	MX31_PIN_DSR_DCE1	= IOMUX_PIN(41,   112),
	MX31_PIN_DTR_DCE1	= IOMUX_PIN(40,   113),
	MX31_PIN_CTS1		= IOMUX_PIN(39,   114),
	MX31_PIN_RTS1		= IOMUX_PIN(38,   115),
	MX31_PIN_TXD1		= IOMUX_PIN(37,   116),
	MX31_PIN_RXD1		= IOMUX_PIN(36,   117),
	MX31_PIN_CSPI2_SPI_RDY	= IOMUX_PIN(0xff, 118),
	MX31_PIN_CSPI2_SCLK	= IOMUX_PIN(0xff, 119),
	MX31_PIN_CSPI2_SS2	= IOMUX_PIN(0xff, 120),
	MX31_PIN_CSPI2_SS1	= IOMUX_PIN(0xff, 121),
	MX31_PIN_CSPI2_SS0	= IOMUX_PIN(0xff, 122),
	MX31_PIN_CSPI2_MISO	= IOMUX_PIN(0xff, 123),
	MX31_PIN_CSPI2_MOSI	= IOMUX_PIN(0xff, 124),
	MX31_PIN_CSPI1_SPI_RDY	= IOMUX_PIN(0xff, 125),
	MX31_PIN_CSPI1_SCLK	= IOMUX_PIN(0xff, 126),
	MX31_PIN_CSPI1_SS2	= IOMUX_PIN(0xff, 127),
	MX31_PIN_CSPI1_SS1	= IOMUX_PIN(0xff, 128),
	MX31_PIN_CSPI1_SS0	= IOMUX_PIN(0xff, 129),
	MX31_PIN_CSPI1_MISO	= IOMUX_PIN(0xff, 130),
	MX31_PIN_CSPI1_MOSI	= IOMUX_PIN(0xff, 131),
	MX31_PIN_SFS6		= IOMUX_PIN(26,   132),
	MX31_PIN_SCK6		= IOMUX_PIN(25,   133),
	MX31_PIN_SRXD6		= IOMUX_PIN(24,   134),
	MX31_PIN_STXD6		= IOMUX_PIN(23,   135),
	MX31_PIN_SFS5		= IOMUX_PIN(0xff, 136),
	MX31_PIN_SCK5		= IOMUX_PIN(0xff, 137),
	MX31_PIN_SRXD5		= IOMUX_PIN(22,   138),
	MX31_PIN_STXD5		= IOMUX_PIN(21,   139),
	MX31_PIN_SFS4		= IOMUX_PIN(0xff, 140),
	MX31_PIN_SCK4		= IOMUX_PIN(0xff, 141),
	MX31_PIN_SRXD4		= IOMUX_PIN(20,   142),
	MX31_PIN_STXD4		= IOMUX_PIN(19,   143),
	MX31_PIN_SFS3		= IOMUX_PIN(0xff, 144),
	MX31_PIN_SCK3		= IOMUX_PIN(0xff, 145),
	MX31_PIN_SRXD3		= IOMUX_PIN(18,   146),
	MX31_PIN_STXD3		= IOMUX_PIN(17,   147),
	MX31_PIN_I2C_DAT	= IOMUX_PIN(0xff, 148),
	MX31_PIN_I2C_CLK	= IOMUX_PIN(0xff, 149),
	MX31_PIN_CSI_PIXCLK	= IOMUX_PIN(83,   150),
	MX31_PIN_CSI_HSYNC	= IOMUX_PIN(82,   151),
	MX31_PIN_CSI_VSYNC	= IOMUX_PIN(81,   152),
	MX31_PIN_CSI_MCLK	= IOMUX_PIN(80,   153),
	MX31_PIN_CSI_D15	= IOMUX_PIN(79,   154),
	MX31_PIN_CSI_D14	= IOMUX_PIN(78,   155),
	MX31_PIN_CSI_D13	= IOMUX_PIN(77,   156),
	MX31_PIN_CSI_D12	= IOMUX_PIN(76,   157),
	MX31_PIN_CSI_D11	= IOMUX_PIN(75,   158),
	MX31_PIN_CSI_D10	= IOMUX_PIN(74,   159),
	MX31_PIN_CSI_D9		= IOMUX_PIN(73,   160),
	MX31_PIN_CSI_D8		= IOMUX_PIN(72,   161),
	MX31_PIN_CSI_D7		= IOMUX_PIN(71,   162),
	MX31_PIN_CSI_D6		= IOMUX_PIN(70,   163),
	MX31_PIN_CSI_D5		= IOMUX_PIN(69,   164),
	MX31_PIN_CSI_D4		= IOMUX_PIN(68,   165),
	MX31_PIN_M_GRANT	= IOMUX_PIN(0xff, 166),
	MX31_PIN_M_REQUEST	= IOMUX_PIN(0xff, 167),
	MX31_PIN_PC_POE		= IOMUX_PIN(0xff, 168),
	MX31_PIN_PC_RW_B	= IOMUX_PIN(0xff, 169),
	MX31_PIN_IOIS16		= IOMUX_PIN(0xff, 170),
	MX31_PIN_PC_RST		= IOMUX_PIN(0xff, 171),
	MX31_PIN_PC_BVD2	= IOMUX_PIN(0xff, 172),
	MX31_PIN_PC_BVD1	= IOMUX_PIN(0xff, 173),
	MX31_PIN_PC_VS2		= IOMUX_PIN(0xff, 174),
	MX31_PIN_PC_VS1		= IOMUX_PIN(0xff, 175),
	MX31_PIN_PC_PWRON	= IOMUX_PIN(0xff, 176),
	MX31_PIN_PC_READY	= IOMUX_PIN(0xff, 177),
	MX31_PIN_PC_WAIT_B	= IOMUX_PIN(0xff, 178),
	MX31_PIN_PC_CD2_B	= IOMUX_PIN(0xff, 179),
	MX31_PIN_PC_CD1_B	= IOMUX_PIN(0xff, 180),
	MX31_PIN_D0		= IOMUX_PIN(0xff, 181),
	MX31_PIN_D1		= IOMUX_PIN(0xff, 182),
	MX31_PIN_D2		= IOMUX_PIN(0xff, 183),
	MX31_PIN_D3		= IOMUX_PIN(0xff, 184),
	MX31_PIN_D4		= IOMUX_PIN(0xff, 185),
	MX31_PIN_D5		= IOMUX_PIN(0xff, 186),
	MX31_PIN_D6		= IOMUX_PIN(0xff, 187),
	MX31_PIN_D7		= IOMUX_PIN(0xff, 188),
	MX31_PIN_D8		= IOMUX_PIN(0xff, 189),
	MX31_PIN_D9		= IOMUX_PIN(0xff, 190),
	MX31_PIN_D10		= IOMUX_PIN(0xff, 191),
	MX31_PIN_D11		= IOMUX_PIN(0xff, 192),
	MX31_PIN_D12		= IOMUX_PIN(0xff, 193),
	MX31_PIN_D13		= IOMUX_PIN(0xff, 194),
	MX31_PIN_D14		= IOMUX_PIN(0xff, 195),
	MX31_PIN_D15		= IOMUX_PIN(0xff, 196),
	MX31_PIN_NFRB		= IOMUX_PIN(16,   197),
	MX31_PIN_NFCE_B		= IOMUX_PIN(15,   198),
	MX31_PIN_NFWP_B		= IOMUX_PIN(14,   199),
	MX31_PIN_NFCLE		= IOMUX_PIN(13,   200),
	MX31_PIN_NFALE		= IOMUX_PIN(12,   201),
	MX31_PIN_NFRE_B		= IOMUX_PIN(11,   202),
	MX31_PIN_NFWE_B		= IOMUX_PIN(10,   203),
	MX31_PIN_SDQS3		= IOMUX_PIN(0xff, 204),
	MX31_PIN_SDQS2		= IOMUX_PIN(0xff, 205),
	MX31_PIN_SDQS1		= IOMUX_PIN(0xff, 206),
	MX31_PIN_SDQS0		= IOMUX_PIN(0xff, 207),
	MX31_PIN_SDCLK_B	= IOMUX_PIN(0xff, 208),
	MX31_PIN_SDCLK		= IOMUX_PIN(0xff, 209),
	MX31_PIN_SDCKE1		= IOMUX_PIN(0xff, 210),
	MX31_PIN_SDCKE0		= IOMUX_PIN(0xff, 211),
	MX31_PIN_SDWE		= IOMUX_PIN(0xff, 212),
	MX31_PIN_CAS		= IOMUX_PIN(0xff, 213),
	MX31_PIN_RAS		= IOMUX_PIN(0xff, 214),
	MX31_PIN_RW		= IOMUX_PIN(0xff, 215),
	MX31_PIN_BCLK		= IOMUX_PIN(0xff, 216),
	MX31_PIN_LBA		= IOMUX_PIN(0xff, 217),
	MX31_PIN_ECB		= IOMUX_PIN(0xff, 218),
	MX31_PIN_CS5		= IOMUX_PIN(0xff, 219),
	MX31_PIN_CS4		= IOMUX_PIN(0xff, 220),
	MX31_PIN_CS3		= IOMUX_PIN(0xff, 221),
	MX31_PIN_CS2		= IOMUX_PIN(0xff, 222),
	MX31_PIN_CS1		= IOMUX_PIN(0xff, 223),
	MX31_PIN_CS0		= IOMUX_PIN(0xff, 224),
	MX31_PIN_OE		= IOMUX_PIN(0xff, 225),
	MX31_PIN_EB1		= IOMUX_PIN(0xff, 226),
	MX31_PIN_EB0		= IOMUX_PIN(0xff, 227),
	MX31_PIN_DQM3		= IOMUX_PIN(0xff, 228),
	MX31_PIN_DQM2		= IOMUX_PIN(0xff, 229),
	MX31_PIN_DQM1		= IOMUX_PIN(0xff, 230),
	MX31_PIN_DQM0		= IOMUX_PIN(0xff, 231),
	MX31_PIN_SD31		= IOMUX_PIN(0xff, 232),
	MX31_PIN_SD30		= IOMUX_PIN(0xff, 233),
	MX31_PIN_SD29		= IOMUX_PIN(0xff, 234),
	MX31_PIN_SD28		= IOMUX_PIN(0xff, 235),
	MX31_PIN_SD27		= IOMUX_PIN(0xff, 236),
	MX31_PIN_SD26		= IOMUX_PIN(0xff, 237),
	MX31_PIN_SD25		= IOMUX_PIN(0xff, 238),
	MX31_PIN_SD24		= IOMUX_PIN(0xff, 239),
	MX31_PIN_SD23		= IOMUX_PIN(0xff, 240),
	MX31_PIN_SD22		= IOMUX_PIN(0xff, 241),
	MX31_PIN_SD21		= IOMUX_PIN(0xff, 242),
	MX31_PIN_SD20		= IOMUX_PIN(0xff, 243),
	MX31_PIN_SD19		= IOMUX_PIN(0xff, 244),
	MX31_PIN_SD18		= IOMUX_PIN(0xff, 245),
	MX31_PIN_SD17		= IOMUX_PIN(0xff, 246),
	MX31_PIN_SD16		= IOMUX_PIN(0xff, 247),
	MX31_PIN_SD15		= IOMUX_PIN(0xff, 248),
	MX31_PIN_SD14		= IOMUX_PIN(0xff, 249),
	MX31_PIN_SD13		= IOMUX_PIN(0xff, 250),
	MX31_PIN_SD12		= IOMUX_PIN(0xff, 251),
	MX31_PIN_SD11		= IOMUX_PIN(0xff, 252),
	MX31_PIN_SD10		= IOMUX_PIN(0xff, 253),
	MX31_PIN_SD9		= IOMUX_PIN(0xff, 254),
	MX31_PIN_SD8		= IOMUX_PIN(0xff, 255),
	MX31_PIN_SD7		= IOMUX_PIN(0xff, 256),
	MX31_PIN_SD6		= IOMUX_PIN(0xff, 257),
	MX31_PIN_SD5		= IOMUX_PIN(0xff, 258),
	MX31_PIN_SD4		= IOMUX_PIN(0xff, 259),
	MX31_PIN_SD3		= IOMUX_PIN(0xff, 260),
	MX31_PIN_SD2		= IOMUX_PIN(0xff, 261),
	MX31_PIN_SD1		= IOMUX_PIN(0xff, 262),
	MX31_PIN_SD0		= IOMUX_PIN(0xff, 263),
	MX31_PIN_SDBA0		= IOMUX_PIN(0xff, 264),
	MX31_PIN_SDBA1		= IOMUX_PIN(0xff, 265),
	MX31_PIN_A25		= IOMUX_PIN(0xff, 266),
	MX31_PIN_A24		= IOMUX_PIN(0xff, 267),
	MX31_PIN_A23		= IOMUX_PIN(0xff, 268),
	MX31_PIN_A22		= IOMUX_PIN(0xff, 269),
	MX31_PIN_A21		= IOMUX_PIN(0xff, 270),
	MX31_PIN_A20		= IOMUX_PIN(0xff, 271),
	MX31_PIN_A19		= IOMUX_PIN(0xff, 272),
	MX31_PIN_A18		= IOMUX_PIN(0xff, 273),
	MX31_PIN_A17		= IOMUX_PIN(0xff, 274),
	MX31_PIN_A16		= IOMUX_PIN(0xff, 275),
	MX31_PIN_A14		= IOMUX_PIN(0xff, 276),
	MX31_PIN_A15		= IOMUX_PIN(0xff, 277),
	MX31_PIN_A13		= IOMUX_PIN(0xff, 278),
	MX31_PIN_A12		= IOMUX_PIN(0xff, 279),
	MX31_PIN_A11		= IOMUX_PIN(0xff, 280),
	MX31_PIN_MA10		= IOMUX_PIN(0xff, 281),
	MX31_PIN_A10		= IOMUX_PIN(0xff, 282),
	MX31_PIN_A9		= IOMUX_PIN(0xff, 283),
	MX31_PIN_A8		= IOMUX_PIN(0xff, 284),
	MX31_PIN_A7		= IOMUX_PIN(0xff, 285),
	MX31_PIN_A6		= IOMUX_PIN(0xff, 286),
	MX31_PIN_A5		= IOMUX_PIN(0xff, 287),
	MX31_PIN_A4		= IOMUX_PIN(0xff, 288),
	MX31_PIN_A3		= IOMUX_PIN(0xff, 289),
	MX31_PIN_A2		= IOMUX_PIN(0xff, 290),
	MX31_PIN_A1		= IOMUX_PIN(0xff, 291),
	MX31_PIN_A0		= IOMUX_PIN(0xff, 292),
	MX31_PIN_VPG1		= IOMUX_PIN(0xff, 293),
	MX31_PIN_VPG0		= IOMUX_PIN(0xff, 294),
	MX31_PIN_DVFS1		= IOMUX_PIN(0xff, 295),
	MX31_PIN_DVFS0		= IOMUX_PIN(0xff, 296),
	MX31_PIN_VSTBY		= IOMUX_PIN(0xff, 297),
	MX31_PIN_POWER_FAIL	= IOMUX_PIN(0xff, 298),
	MX31_PIN_CKIL		= IOMUX_PIN(0xff, 299),
	MX31_PIN_BOOT_MODE4	= IOMUX_PIN(0xff, 300),
	MX31_PIN_BOOT_MODE3	= IOMUX_PIN(0xff, 301),
	MX31_PIN_BOOT_MODE2	= IOMUX_PIN(0xff, 302),
	MX31_PIN_BOOT_MODE1	= IOMUX_PIN(0xff, 303),
	MX31_PIN_BOOT_MODE0	= IOMUX_PIN(0xff, 304),
	MX31_PIN_CLKO		= IOMUX_PIN(0xff, 305),
	MX31_PIN_POR_B		= IOMUX_PIN(0xff, 306),
	MX31_PIN_RESET_IN_B	= IOMUX_PIN(0xff, 307),
	MX31_PIN_CKIH		= IOMUX_PIN(0xff, 308),
	MX31_PIN_SIMPD0		= IOMUX_PIN(35,   309),
	MX31_PIN_SRX0		= IOMUX_PIN(34,   310),
	MX31_PIN_STX0		= IOMUX_PIN(33,   311),
	MX31_PIN_SVEN0		= IOMUX_PIN(32,   312),
	MX31_PIN_SRST0		= IOMUX_PIN(67,   313),
	MX31_PIN_SCLK0		= IOMUX_PIN(66,   314),
	MX31_PIN_GPIO3_1	= IOMUX_PIN(65,   315),
	MX31_PIN_GPIO3_0	= IOMUX_PIN(64,   316),
	MX31_PIN_GPIO1_6	= IOMUX_PIN( 6,   317),
	MX31_PIN_GPIO1_5	= IOMUX_PIN( 5,   318),
	MX31_PIN_GPIO1_4	= IOMUX_PIN( 4,   319),
	MX31_PIN_GPIO1_3	= IOMUX_PIN( 3,   320),
	MX31_PIN_GPIO1_2	= IOMUX_PIN( 2,   321),
	MX31_PIN_GPIO1_1	= IOMUX_PIN( 1,   322),
	MX31_PIN_GPIO1_0	= IOMUX_PIN( 0,   323),
	MX31_PIN_PWMO		= IOMUX_PIN( 9,   324),
	MX31_PIN_WATCHDOG_RST	= IOMUX_PIN(0xff, 325),
	MX31_PIN_COMPARE	= IOMUX_PIN( 8,   326),
	MX31_PIN_CAPTURE	= IOMUX_PIN( 7,   327),
};

#define PIN_MAX 327
#define NB_PORTS 12 /* NB_PINS/32, we chose 32 pins per "PORT" */

/*
 * Convenience values for use with mxc_iomux_mode()
 *
 * Format here is MX31_PIN_(pin name)__(function)
 */
#define MX31_PIN_CSPI3_MOSI__RXD3	IOMUX_MODE(MX31_PIN_CSPI3_MOSI, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI3_MISO__TXD3	IOMUX_MODE(MX31_PIN_CSPI3_MISO, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI3_SCLK__RTS3	IOMUX_MODE(MX31_PIN_CSPI3_SCLK, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI3_SPI_RDY__CTS3	IOMUX_MODE(MX31_PIN_CSPI3_SPI_RDY, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CTS1__CTS1		IOMUX_MODE(MX31_PIN_CTS1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_RTS1__RTS1		IOMUX_MODE(MX31_PIN_RTS1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_RTS1__SFS		IOMUX_MODE(MX31_PIN_RTS1, IOMUX_CONFIG_ALT2)
#define MX31_PIN_TXD1__TXD1		IOMUX_MODE(MX31_PIN_TXD1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_TXD1__SCK		IOMUX_MODE(MX31_PIN_TXD1, IOMUX_CONFIG_ALT2)
#define MX31_PIN_RXD1__RXD1		IOMUX_MODE(MX31_PIN_RXD1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_RXD1__STXDA		IOMUX_MODE(MX31_PIN_RXD1, IOMUX_CONFIG_ALT2)
#define MX31_PIN_DCD_DCE1__DCD_DCE1	IOMUX_MODE(MX31_PIN_DCD_DCE1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_RI_DCE1__RI_DCE1	IOMUX_MODE(MX31_PIN_RI_DCE1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_DSR_DCE1__DSR_DCE1	IOMUX_MODE(MX31_PIN_DSR_DCE1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_DTR_DCE1__DTR_DCE1	IOMUX_MODE(MX31_PIN_DTR_DCE1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_DTR_DCE1__SRXDA	IOMUX_MODE(MX31_PIN_DTR_DCE1, IOMUX_CONFIG_ALT2)
#define MX31_PIN_CTS2__CTS2		IOMUX_MODE(MX31_PIN_CTS2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_RTS2__RTS2		IOMUX_MODE(MX31_PIN_RTS2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_TXD2__TXD2		IOMUX_MODE(MX31_PIN_TXD2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_RXD2__RXD2		IOMUX_MODE(MX31_PIN_RXD2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_DCD_DTE1__DCD_DTE2	IOMUX_MODE(MX31_PIN_DCD_DTE1, IOMUX_CONFIG_ALT1)
#define MX31_PIN_RI_DTE1__RI_DTE2	IOMUX_MODE(MX31_PIN_RI_DTE1, IOMUX_CONFIG_ALT1)
#define MX31_PIN_DSR_DTE1__DSR_DTE2	IOMUX_MODE(MX31_PIN_DSR_DTE1, IOMUX_CONFIG_ALT1)
#define MX31_PIN_DTR_DTE1__DTR_DTE2	IOMUX_MODE(MX31_PIN_DTR_DTE1, IOMUX_OCONFIG_ALT3 | IOMUX_ICONFIG_NONE)
#define MX31_PIN_PC_RST__CTS5		IOMUX_MODE(MX31_PIN_PC_RST, IOMUX_CONFIG_ALT2)
#define MX31_PIN_PC_VS2__RTS5		IOMUX_MODE(MX31_PIN_PC_VS2, IOMUX_CONFIG_ALT2)
#define MX31_PIN_PC_BVD2__TXD5		IOMUX_MODE(MX31_PIN_PC_BVD2, IOMUX_CONFIG_ALT2)
#define MX31_PIN_PC_BVD1__RXD5		IOMUX_MODE(MX31_PIN_PC_BVD1, IOMUX_CONFIG_ALT2)
#define MX31_PIN_CSPI1_MOSI__MOSI	IOMUX_MODE(MX31_PIN_CSPI1_MOSI, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI1_MISO__MISO	IOMUX_MODE(MX31_PIN_CSPI1_MISO, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI1_SCLK__SCLK	IOMUX_MODE(MX31_PIN_CSPI1_SCLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI1_SPI_RDY__SPI_RDY	IOMUX_MODE(MX31_PIN_CSPI1_SPI_RDY, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI1_SS0__SS0		IOMUX_MODE(MX31_PIN_CSPI1_SS0, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI1_SS1__SS1		IOMUX_MODE(MX31_PIN_CSPI1_SS1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI1_SS2__SS2		IOMUX_MODE(MX31_PIN_CSPI1_SS2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI2_MOSI__MOSI	IOMUX_MODE(MX31_PIN_CSPI2_MOSI, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI2_MOSI__SCL	IOMUX_MODE(MX31_PIN_CSPI2_MOSI, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI2_MISO__MISO	IOMUX_MODE(MX31_PIN_CSPI2_MISO, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI2_MISO__SDA	IOMUX_MODE(MX31_PIN_CSPI2_MISO, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI2_SCLK__SCLK	IOMUX_MODE(MX31_PIN_CSPI2_SCLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI2_SPI_RDY__SPI_RDY	IOMUX_MODE(MX31_PIN_CSPI2_SPI_RDY, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI2_SS0__SS0		IOMUX_MODE(MX31_PIN_CSPI2_SS0, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI2_SS1__SS1		IOMUX_MODE(MX31_PIN_CSPI2_SS1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI2_SS2__SS2		IOMUX_MODE(MX31_PIN_CSPI2_SS2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI3_MOSI__MOSI	IOMUX_MODE(MX31_PIN_CSPI3_MOSI, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI3_MISO__MISO	IOMUX_MODE(MX31_PIN_CSPI3_MISO, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI3_SCLK__SCLK	IOMUX_MODE(MX31_PIN_CSPI3_SCLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI3_SPI_RDY__SPI_RDY	IOMUX_MODE(MX31_PIN_CSPI3_SPI_RDY, IOMUX_CONFIG_FUNC)
#define MX31_PIN_BATT_LINE__OWIRE	IOMUX_MODE(MX31_PIN_BATT_LINE, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CS4__CS4		IOMUX_MODE(MX31_PIN_CS4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SD1_DATA3__SD1_DATA3	IOMUX_MODE(MX31_PIN_SD1_DATA3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SD1_DATA2__SD1_DATA2	IOMUX_MODE(MX31_PIN_SD1_DATA2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SD1_DATA1__SD1_DATA1	IOMUX_MODE(MX31_PIN_SD1_DATA1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SD1_DATA0__SD1_DATA0	IOMUX_MODE(MX31_PIN_SD1_DATA0, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SD1_CLK__SD1_CLK	IOMUX_MODE(MX31_PIN_SD1_CLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SD1_CMD__SD1_CMD	IOMUX_MODE(MX31_PIN_SD1_CMD, IOMUX_CONFIG_FUNC)
#define MX31_PIN_ATA_CS0__GPIO3_26	IOMUX_MODE(MX31_PIN_ATA_CS0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_ATA_CS1__GPIO3_27	IOMUX_MODE(MX31_PIN_ATA_CS1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_PC_PWRON__SD2_DATA3	IOMUX_MODE(MX31_PIN_PC_PWRON, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_VS1__SD2_DATA2	IOMUX_MODE(MX31_PIN_PC_VS1, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_READY__SD2_DATA1	IOMUX_MODE(MX31_PIN_PC_READY, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_WAIT_B__SD2_DATA0	IOMUX_MODE(MX31_PIN_PC_WAIT_B, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_CD2_B__SD2_CLK	IOMUX_MODE(MX31_PIN_PC_CD2_B, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_CD1_B__SD2_CMD	IOMUX_MODE(MX31_PIN_PC_CD1_B, IOMUX_CONFIG_ALT1)
#define MX31_PIN_ATA_DIOR__GPIO3_28	IOMUX_MODE(MX31_PIN_ATA_DIOR, IOMUX_CONFIG_GPIO)
#define MX31_PIN_ATA_DIOW__GPIO3_29	IOMUX_MODE(MX31_PIN_ATA_DIOW, IOMUX_CONFIG_GPIO)
#define MX31_PIN_LD0__LD0		IOMUX_MODE(MX31_PIN_LD0, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD1__LD1		IOMUX_MODE(MX31_PIN_LD1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD2__LD2		IOMUX_MODE(MX31_PIN_LD2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD3__LD3		IOMUX_MODE(MX31_PIN_LD3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD4__LD4		IOMUX_MODE(MX31_PIN_LD4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD5__LD5		IOMUX_MODE(MX31_PIN_LD5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD6__LD6		IOMUX_MODE(MX31_PIN_LD6, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD7__LD7		IOMUX_MODE(MX31_PIN_LD7, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD8__LD8		IOMUX_MODE(MX31_PIN_LD8, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD9__LD9		IOMUX_MODE(MX31_PIN_LD9, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD10__LD10		IOMUX_MODE(MX31_PIN_LD10, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD11__LD11		IOMUX_MODE(MX31_PIN_LD11, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD12__LD12		IOMUX_MODE(MX31_PIN_LD12, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD13__LD13		IOMUX_MODE(MX31_PIN_LD13, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD14__LD14		IOMUX_MODE(MX31_PIN_LD14, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD15__LD15		IOMUX_MODE(MX31_PIN_LD15, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD16__LD16		IOMUX_MODE(MX31_PIN_LD16, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LD17__LD17		IOMUX_MODE(MX31_PIN_LD17, IOMUX_CONFIG_FUNC)
#define MX31_PIN_VSYNC3__VSYNC3		IOMUX_MODE(MX31_PIN_VSYNC3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_HSYNC__HSYNC		IOMUX_MODE(MX31_PIN_HSYNC, IOMUX_CONFIG_FUNC)
#define MX31_PIN_FPSHIFT__FPSHIFT	IOMUX_MODE(MX31_PIN_FPSHIFT, IOMUX_CONFIG_FUNC)
#define MX31_PIN_DRDY0__DRDY0		IOMUX_MODE(MX31_PIN_DRDY0, IOMUX_CONFIG_FUNC)
#define MX31_PIN_D3_REV__D3_REV		IOMUX_MODE(MX31_PIN_D3_REV, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CONTRAST__CONTRAST	IOMUX_MODE(MX31_PIN_CONTRAST, IOMUX_CONFIG_FUNC)
#define MX31_PIN_D3_SPL__D3_SPL		IOMUX_MODE(MX31_PIN_D3_SPL, IOMUX_CONFIG_FUNC)
#define MX31_PIN_D3_CLS__D3_CLS		IOMUX_MODE(MX31_PIN_D3_CLS, IOMUX_CONFIG_FUNC)
#define MX31_PIN_LCS0__GPI03_23		IOMUX_MODE(MX31_PIN_LCS0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_GPIO1_1__GPIO          IOMUX_MODE(MX31_PIN_GPIO1_1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_I2C_CLK__SCL		IOMUX_MODE(MX31_PIN_I2C_CLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_I2C_DAT__SDA		IOMUX_MODE(MX31_PIN_I2C_DAT, IOMUX_CONFIG_FUNC)
#define MX31_PIN_DCD_DTE1__I2C2_SDA	IOMUX_MODE(MX31_PIN_DCD_DTE1, IOMUX_CONFIG_ALT2)
#define MX31_PIN_RI_DTE1__I2C2_SCL	IOMUX_MODE(MX31_PIN_RI_DTE1, IOMUX_CONFIG_ALT2)
#define MX31_PIN_CSPI2_SS2__I2C3_SDA	IOMUX_MODE(MX31_PIN_CSPI2_SS2, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI2_SCLK__I2C3_SCL	IOMUX_MODE(MX31_PIN_CSPI2_SCLK, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSI_D4__CSI_D4		IOMUX_MODE(MX31_PIN_CSI_D4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D5__CSI_D5		IOMUX_MODE(MX31_PIN_CSI_D5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D6__CSI_D6		IOMUX_MODE(MX31_PIN_CSI_D6, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D7__CSI_D7		IOMUX_MODE(MX31_PIN_CSI_D7, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D8__CSI_D8		IOMUX_MODE(MX31_PIN_CSI_D8, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D9__CSI_D9		IOMUX_MODE(MX31_PIN_CSI_D9, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D10__CSI_D10	IOMUX_MODE(MX31_PIN_CSI_D10, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D11__CSI_D11	IOMUX_MODE(MX31_PIN_CSI_D11, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D12__CSI_D12	IOMUX_MODE(MX31_PIN_CSI_D12, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D13__CSI_D13	IOMUX_MODE(MX31_PIN_CSI_D13, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D14__CSI_D14	IOMUX_MODE(MX31_PIN_CSI_D14, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D15__CSI_D15	IOMUX_MODE(MX31_PIN_CSI_D15, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_HSYNC__CSI_HSYNC	IOMUX_MODE(MX31_PIN_CSI_HSYNC, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_MCLK__CSI_MCLK	IOMUX_MODE(MX31_PIN_CSI_MCLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_PIXCLK__CSI_PIXCLK	IOMUX_MODE(MX31_PIN_CSI_PIXCLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_VSYNC__CSI_VSYNC	IOMUX_MODE(MX31_PIN_CSI_VSYNC, IOMUX_CONFIG_FUNC)
#define MX31_PIN_GPIO3_0__GPIO3_0	IOMUX_MODE(MX31_PIN_GPIO3_0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_GPIO3_1__GPIO3_1	IOMUX_MODE(MX31_PIN_GPIO3_1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_TXD2__GPIO1_28		IOMUX_MODE(MX31_PIN_TXD2, IOMUX_CONFIG_GPIO)
#define MX31_PIN_CSI_D4__GPIO3_4	IOMUX_MODE(MX31_PIN_CSI_D4, IOMUX_CONFIG_GPIO)
#define MX31_PIN_CSI_D5__GPIO3_5	IOMUX_MODE(MX31_PIN_CSI_D5, IOMUX_CONFIG_GPIO)
#define MX31_PIN_USBOTG_DATA0__USBOTG_DATA0	IOMUX_MODE(MX31_PIN_USBOTG_DATA0, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_DATA1__USBOTG_DATA1	IOMUX_MODE(MX31_PIN_USBOTG_DATA1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_DATA2__USBOTG_DATA2	IOMUX_MODE(MX31_PIN_USBOTG_DATA2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_DATA3__USBOTG_DATA3	IOMUX_MODE(MX31_PIN_USBOTG_DATA3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_DATA4__USBOTG_DATA4	IOMUX_MODE(MX31_PIN_USBOTG_DATA4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_DATA5__USBOTG_DATA5	IOMUX_MODE(MX31_PIN_USBOTG_DATA5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_DATA6__USBOTG_DATA6	IOMUX_MODE(MX31_PIN_USBOTG_DATA6, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_DATA7__USBOTG_DATA7	IOMUX_MODE(MX31_PIN_USBOTG_DATA7, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_CLK__USBOTG_CLK		IOMUX_MODE(MX31_PIN_USBOTG_CLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_DIR__USBOTG_DIR		IOMUX_MODE(MX31_PIN_USBOTG_DIR, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_NXT__USBOTG_NXT		IOMUX_MODE(MX31_PIN_USBOTG_NXT, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBOTG_STP__USBOTG_STP		IOMUX_MODE(MX31_PIN_USBOTG_STP, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSPI1_MOSI__USBH1_RXDM		IOMUX_MODE(MX31_PIN_CSPI1_MOSI, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI1_MISO__USBH1_RXDP		IOMUX_MODE(MX31_PIN_CSPI1_MISO, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI1_SS0__USBH1_TXDM		IOMUX_MODE(MX31_PIN_CSPI1_SS0, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI1_SS1__USBH1_TXDP		IOMUX_MODE(MX31_PIN_CSPI1_SS1, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI1_SS2__USBH1_RCV		IOMUX_MODE(MX31_PIN_CSPI1_SS2, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI1_SCLK__USBH1_OEB		IOMUX_MODE(MX31_PIN_CSPI1_SCLK, IOMUX_CONFIG_ALT1)
#define MX31_PIN_CSPI1_SPI_RDY__USBH1_FS	IOMUX_MODE(MX31_PIN_CSPI1_SPI_RDY, IOMUX_CONFIG_ALT1)
#define MX31_PIN_SFS6__USBH1_SUSPEND	IOMUX_MODE(MX31_PIN_SFS6, IOMUX_CONFIG_FUNC)
#define MX31_PIN_NFRE_B__GPIO1_11	IOMUX_MODE(MX31_PIN_NFRE_B, IOMUX_CONFIG_GPIO)
#define MX31_PIN_NFALE__GPIO1_12	IOMUX_MODE(MX31_PIN_NFALE, IOMUX_CONFIG_GPIO)
#define MX31_PIN_USBH2_DATA0__USBH2_DATA0	IOMUX_MODE(MX31_PIN_USBH2_DATA0, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBH2_DATA1__USBH2_DATA1	IOMUX_MODE(MX31_PIN_USBH2_DATA1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_STXD3__USBH2_DATA2	IOMUX_MODE(MX31_PIN_STXD3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SRXD3__USBH2_DATA3	IOMUX_MODE(MX31_PIN_SRXD3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SCK3__USBH2_DATA4	IOMUX_MODE(MX31_PIN_SCK3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SFS3__USBH2_DATA5	IOMUX_MODE(MX31_PIN_SFS3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_STXD6__USBH2_DATA6	IOMUX_MODE(MX31_PIN_STXD6, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SRXD6__USBH2_DATA7	IOMUX_MODE(MX31_PIN_SRXD6, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBH2_CLK__USBH2_CLK		IOMUX_MODE(MX31_PIN_USBH2_CLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBH2_DIR__USBH2_DIR		IOMUX_MODE(MX31_PIN_USBH2_DIR, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBH2_NXT__USBH2_NXT		IOMUX_MODE(MX31_PIN_USBH2_NXT, IOMUX_CONFIG_FUNC)
#define MX31_PIN_USBH2_STP__USBH2_STP		IOMUX_MODE(MX31_PIN_USBH2_STP, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SCK6__GPIO1_25		IOMUX_MODE(MX31_PIN_SCK6, IOMUX_CONFIG_GPIO)
#define MX31_PIN_USB_OC__GPIO1_30	IOMUX_MODE(MX31_PIN_USB_OC, IOMUX_CONFIG_GPIO)
#define MX31_PIN_I2C_DAT__I2C1_SDA	IOMUX_MODE(MX31_PIN_I2C_DAT, IOMUX_CONFIG_FUNC)
#define MX31_PIN_I2C_CLK__I2C1_SCL	IOMUX_MODE(MX31_PIN_I2C_CLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_DCD_DTE1__I2C2_SDA	IOMUX_MODE(MX31_PIN_DCD_DTE1, IOMUX_CONFIG_ALT2)
#define MX31_PIN_RI_DTE1__I2C2_SCL	IOMUX_MODE(MX31_PIN_RI_DTE1, IOMUX_CONFIG_ALT2)
#define MX31_PIN_ATA_CS0__GPIO3_26	IOMUX_MODE(MX31_PIN_ATA_CS0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_ATA_CS1__GPIO3_27	IOMUX_MODE(MX31_PIN_ATA_CS1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_PC_PWRON__SD2_DATA3	IOMUX_MODE(MX31_PIN_PC_PWRON, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_VS1__SD2_DATA2	IOMUX_MODE(MX31_PIN_PC_VS1, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_READY__SD2_DATA1	IOMUX_MODE(MX31_PIN_PC_READY, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_WAIT_B__SD2_DATA0	IOMUX_MODE(MX31_PIN_PC_WAIT_B, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_CD2_B__SD2_CLK	IOMUX_MODE(MX31_PIN_PC_CD2_B, IOMUX_CONFIG_ALT1)
#define MX31_PIN_PC_CD1_B__SD2_CMD	IOMUX_MODE(MX31_PIN_PC_CD1_B, IOMUX_CONFIG_ALT1)
#define MX31_PIN_ATA_DIOR__GPIO3_28	IOMUX_MODE(MX31_PIN_ATA_DIOR, IOMUX_CONFIG_GPIO)
#define MX31_PIN_ATA_DIOW__GPIO3_29	IOMUX_MODE(MX31_PIN_ATA_DIOW, IOMUX_CONFIG_GPIO)
#define MX31_PIN_CSI_D4__CSI_D4		IOMUX_MODE(MX31_PIN_CSI_D4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D5__CSI_D5		IOMUX_MODE(MX31_PIN_CSI_D5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D6__CSI_D6		IOMUX_MODE(MX31_PIN_CSI_D6, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D7__CSI_D7		IOMUX_MODE(MX31_PIN_CSI_D7, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D8__CSI_D8		IOMUX_MODE(MX31_PIN_CSI_D8, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D9__CSI_D9		IOMUX_MODE(MX31_PIN_CSI_D9, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D10__CSI_D10	IOMUX_MODE(MX31_PIN_CSI_D10, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D11__CSI_D11	IOMUX_MODE(MX31_PIN_CSI_D11, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D12__CSI_D12	IOMUX_MODE(MX31_PIN_CSI_D12, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D13__CSI_D13	IOMUX_MODE(MX31_PIN_CSI_D13, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D14__CSI_D14	IOMUX_MODE(MX31_PIN_CSI_D14, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_D15__CSI_D15	IOMUX_MODE(MX31_PIN_CSI_D15, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_HSYNC__CSI_HSYNC	IOMUX_MODE(MX31_PIN_CSI_HSYNC, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_MCLK__CSI_MCLK	IOMUX_MODE(MX31_PIN_CSI_MCLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_PIXCLK__CSI_PIXCLK	IOMUX_MODE(MX31_PIN_CSI_PIXCLK, IOMUX_CONFIG_FUNC)
#define MX31_PIN_CSI_VSYNC__CSI_VSYNC	IOMUX_MODE(MX31_PIN_CSI_VSYNC, IOMUX_CONFIG_FUNC)
#define MX31_PIN_GPIO3_0__GPIO3_0	IOMUX_MODE(MX31_PIN_GPIO3_0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_GPIO3_1__GPIO3_1	IOMUX_MODE(MX31_PIN_GPIO3_1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_TXD2__GPIO1_28		IOMUX_MODE(MX31_PIN_TXD2, IOMUX_CONFIG_GPIO)
#define MX31_PIN_GPIO1_0__GPIO1_0	IOMUX_MODE(MX31_PIN_GPIO1_0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_SVEN0__GPIO2_0		IOMUX_MODE(MX31_PIN_SVEN0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_STX0__GPIO2_1		IOMUX_MODE(MX31_PIN_STX0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_SRX0__GPIO2_2		IOMUX_MODE(MX31_PIN_SRX0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_SIMPD0__GPIO2_3	IOMUX_MODE(MX31_PIN_SIMPD0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_DTR_DCE1__GPIO2_8	IOMUX_MODE(MX31_PIN_DTR_DCE1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_DSR_DCE1__GPIO2_9	IOMUX_MODE(MX31_PIN_DSR_DCE1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_RI_DCE1__GPIO2_10	IOMUX_MODE(MX31_PIN_RI_DCE1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_DCD_DCE1__GPIO2_11	IOMUX_MODE(MX31_PIN_DCD_DCE1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_STXD5__GPIO1_21	IOMUX_MODE(MX31_PIN_STXD5, IOMUX_CONFIG_GPIO)
#define MX31_PIN_SRXD5__GPIO1_22	IOMUX_MODE(MX31_PIN_SRXD5, IOMUX_CONFIG_GPIO)
#define MX31_PIN_GPIO1_3__GPIO1_3	IOMUX_MODE(MX31_PIN_GPIO1_3, IOMUX_CONFIG_GPIO)
#define MX31_PIN_CSPI2_SS1__CSPI3_SS1	IOMUX_MODE(MX31_PIN_CSPI2_SS1, IOMUX_CONFIG_ALT1)
#define MX31_PIN_RTS1__GPIO2_6		IOMUX_MODE(MX31_PIN_RTS1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_CTS1__GPIO2_7		IOMUX_MODE(MX31_PIN_CTS1, IOMUX_CONFIG_GPIO)
#define MX31_PIN_LCS0__GPIO3_23		IOMUX_MODE(MX31_PIN_LCS0, IOMUX_CONFIG_GPIO)
#define MX31_PIN_STXD4__STXD4		IOMUX_MODE(MX31_PIN_STXD4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SRXD4__SRXD4		IOMUX_MODE(MX31_PIN_SRXD4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SCK4__SCK4		IOMUX_MODE(MX31_PIN_SCK4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SFS4__SFS4		IOMUX_MODE(MX31_PIN_SFS4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_STXD5__STXD5		IOMUX_MODE(MX31_PIN_STXD5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SRXD5__SRXD5		IOMUX_MODE(MX31_PIN_SRXD5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SCK5__SCK5		IOMUX_MODE(MX31_PIN_SCK5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_SFS5__SFS5		IOMUX_MODE(MX31_PIN_SFS5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_ROW0_KEY_ROW0	IOMUX_MODE(MX31_PIN_KEY_ROW0, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_ROW1_KEY_ROW1	IOMUX_MODE(MX31_PIN_KEY_ROW1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_ROW2_KEY_ROW2	IOMUX_MODE(MX31_PIN_KEY_ROW2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_ROW3_KEY_ROW3	IOMUX_MODE(MX31_PIN_KEY_ROW3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_ROW4_KEY_ROW4	IOMUX_MODE(MX31_PIN_KEY_ROW4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_ROW4_GPIO		IOMUX_MODE(MX31_PIN_KEY_ROW4, IOMUX_CONFIG_GPIO)
#define MX31_PIN_KEY_ROW5_KEY_ROW5	IOMUX_MODE(MX31_PIN_KEY_ROW5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_ROW6_KEY_ROW6	IOMUX_MODE(MX31_PIN_KEY_ROW6, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_ROW7_KEY_ROW7	IOMUX_MODE(MX31_PIN_KEY_ROW7, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_COL0_KEY_COL0	IOMUX_MODE(MX31_PIN_KEY_COL0, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_COL1_KEY_COL1	IOMUX_MODE(MX31_PIN_KEY_COL1, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_COL2_KEY_COL2	IOMUX_MODE(MX31_PIN_KEY_COL2, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_COL3_KEY_COL3	IOMUX_MODE(MX31_PIN_KEY_COL3, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_COL4_KEY_COL4	IOMUX_MODE(MX31_PIN_KEY_COL4, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_COL5_KEY_COL5	IOMUX_MODE(MX31_PIN_KEY_COL5, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_COL6_KEY_COL6	IOMUX_MODE(MX31_PIN_KEY_COL6, IOMUX_CONFIG_FUNC)
#define MX31_PIN_KEY_COL7_KEY_COL7	IOMUX_MODE(MX31_PIN_KEY_COL7, IOMUX_CONFIG_FUNC)
#define MX31_PIN_WATCHDOG_RST__WATCHDOG_RST	IOMUX_MODE(MX31_PIN_WATCHDOG_RST, IOMUX_CONFIG_FUNC)


/*
 * XXX: The SS0, SS1, SS2, SS3 lines of spi3 are multiplexed with cspi2_ss0,
 * cspi2_ss1, cspi1_ss0 cspi1_ss1
 */

/*
 * This function configures the pad value for a IOMUX pin.
 */
void mxc_iomux_set_pad(enum iomux_pins, u32);

#endif /* ifndef __MACH_IOMUX_MX3_H__ */
