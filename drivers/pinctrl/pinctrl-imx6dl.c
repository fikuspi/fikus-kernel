/*
 * Copyright (C) 2013 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/err.h>
#include <fikus/init.h>
#include <fikus/io.h>
#include <fikus/module.h>
#include <fikus/of.h>
#include <fikus/of_device.h>
#include <fikus/pinctrl/pinctrl.h>

#include "pinctrl-imx.h"

enum imx6dl_pads {
	MX6DL_PAD_RESERVE0 = 0,
	MX6DL_PAD_RESERVE1 = 1,
	MX6DL_PAD_RESERVE2 = 2,
	MX6DL_PAD_RESERVE3 = 3,
	MX6DL_PAD_RESERVE4 = 4,
	MX6DL_PAD_RESERVE5 = 5,
	MX6DL_PAD_RESERVE6 = 6,
	MX6DL_PAD_RESERVE7 = 7,
	MX6DL_PAD_RESERVE8 = 8,
	MX6DL_PAD_RESERVE9 = 9,
	MX6DL_PAD_RESERVE10 = 10,
	MX6DL_PAD_RESERVE11 = 11,
	MX6DL_PAD_RESERVE12 = 12,
	MX6DL_PAD_RESERVE13 = 13,
	MX6DL_PAD_RESERVE14 = 14,
	MX6DL_PAD_RESERVE15 = 15,
	MX6DL_PAD_RESERVE16 = 16,
	MX6DL_PAD_RESERVE17 = 17,
	MX6DL_PAD_RESERVE18 = 18,
	MX6DL_PAD_CSI0_DAT10 = 19,
	MX6DL_PAD_CSI0_DAT11 = 20,
	MX6DL_PAD_CSI0_DAT12 = 21,
	MX6DL_PAD_CSI0_DAT13 = 22,
	MX6DL_PAD_CSI0_DAT14 = 23,
	MX6DL_PAD_CSI0_DAT15 = 24,
	MX6DL_PAD_CSI0_DAT16 = 25,
	MX6DL_PAD_CSI0_DAT17 = 26,
	MX6DL_PAD_CSI0_DAT18 = 27,
	MX6DL_PAD_CSI0_DAT19 = 28,
	MX6DL_PAD_CSI0_DAT4 = 29,
	MX6DL_PAD_CSI0_DAT5 = 30,
	MX6DL_PAD_CSI0_DAT6 = 31,
	MX6DL_PAD_CSI0_DAT7 = 32,
	MX6DL_PAD_CSI0_DAT8 = 33,
	MX6DL_PAD_CSI0_DAT9 = 34,
	MX6DL_PAD_CSI0_DATA_EN = 35,
	MX6DL_PAD_CSI0_MCLK = 36,
	MX6DL_PAD_CSI0_PIXCLK = 37,
	MX6DL_PAD_CSI0_VSYNC = 38,
	MX6DL_PAD_DI0_DISP_CLK = 39,
	MX6DL_PAD_DI0_PIN15 = 40,
	MX6DL_PAD_DI0_PIN2 = 41,
	MX6DL_PAD_DI0_PIN3 = 42,
	MX6DL_PAD_DI0_PIN4 = 43,
	MX6DL_PAD_DISP0_DAT0 = 44,
	MX6DL_PAD_DISP0_DAT1 = 45,
	MX6DL_PAD_DISP0_DAT10 = 46,
	MX6DL_PAD_DISP0_DAT11 = 47,
	MX6DL_PAD_DISP0_DAT12 = 48,
	MX6DL_PAD_DISP0_DAT13 = 49,
	MX6DL_PAD_DISP0_DAT14 = 50,
	MX6DL_PAD_DISP0_DAT15 = 51,
	MX6DL_PAD_DISP0_DAT16 = 52,
	MX6DL_PAD_DISP0_DAT17 = 53,
	MX6DL_PAD_DISP0_DAT18 = 54,
	MX6DL_PAD_DISP0_DAT19 = 55,
	MX6DL_PAD_DISP0_DAT2 = 56,
	MX6DL_PAD_DISP0_DAT20 = 57,
	MX6DL_PAD_DISP0_DAT21 = 58,
	MX6DL_PAD_DISP0_DAT22 = 59,
	MX6DL_PAD_DISP0_DAT23 = 60,
	MX6DL_PAD_DISP0_DAT3 = 61,
	MX6DL_PAD_DISP0_DAT4 = 62,
	MX6DL_PAD_DISP0_DAT5 = 63,
	MX6DL_PAD_DISP0_DAT6 = 64,
	MX6DL_PAD_DISP0_DAT7 = 65,
	MX6DL_PAD_DISP0_DAT8 = 66,
	MX6DL_PAD_DISP0_DAT9 = 67,
	MX6DL_PAD_EIM_A16 = 68,
	MX6DL_PAD_EIM_A17 = 69,
	MX6DL_PAD_EIM_A18 = 70,
	MX6DL_PAD_EIM_A19 = 71,
	MX6DL_PAD_EIM_A20 = 72,
	MX6DL_PAD_EIM_A21 = 73,
	MX6DL_PAD_EIM_A22 = 74,
	MX6DL_PAD_EIM_A23 = 75,
	MX6DL_PAD_EIM_A24 = 76,
	MX6DL_PAD_EIM_A25 = 77,
	MX6DL_PAD_EIM_BCLK = 78,
	MX6DL_PAD_EIM_CS0 = 79,
	MX6DL_PAD_EIM_CS1 = 80,
	MX6DL_PAD_EIM_D16 = 81,
	MX6DL_PAD_EIM_D17 = 82,
	MX6DL_PAD_EIM_D18 = 83,
	MX6DL_PAD_EIM_D19 = 84,
	MX6DL_PAD_EIM_D20 = 85,
	MX6DL_PAD_EIM_D21 = 86,
	MX6DL_PAD_EIM_D22 = 87,
	MX6DL_PAD_EIM_D23 = 88,
	MX6DL_PAD_EIM_D24 = 89,
	MX6DL_PAD_EIM_D25 = 90,
	MX6DL_PAD_EIM_D26 = 91,
	MX6DL_PAD_EIM_D27 = 92,
	MX6DL_PAD_EIM_D28 = 93,
	MX6DL_PAD_EIM_D29 = 94,
	MX6DL_PAD_EIM_D30 = 95,
	MX6DL_PAD_EIM_D31 = 96,
	MX6DL_PAD_EIM_DA0 = 97,
	MX6DL_PAD_EIM_DA1 = 98,
	MX6DL_PAD_EIM_DA10 = 99,
	MX6DL_PAD_EIM_DA11 = 100,
	MX6DL_PAD_EIM_DA12 = 101,
	MX6DL_PAD_EIM_DA13 = 102,
	MX6DL_PAD_EIM_DA14 = 103,
	MX6DL_PAD_EIM_DA15 = 104,
	MX6DL_PAD_EIM_DA2 = 105,
	MX6DL_PAD_EIM_DA3 = 106,
	MX6DL_PAD_EIM_DA4 = 107,
	MX6DL_PAD_EIM_DA5 = 108,
	MX6DL_PAD_EIM_DA6 = 109,
	MX6DL_PAD_EIM_DA7 = 110,
	MX6DL_PAD_EIM_DA8 = 111,
	MX6DL_PAD_EIM_DA9 = 112,
	MX6DL_PAD_EIM_EB0 = 113,
	MX6DL_PAD_EIM_EB1 = 114,
	MX6DL_PAD_EIM_EB2 = 115,
	MX6DL_PAD_EIM_EB3 = 116,
	MX6DL_PAD_EIM_LBA = 117,
	MX6DL_PAD_EIM_OE = 118,
	MX6DL_PAD_EIM_RW = 119,
	MX6DL_PAD_EIM_WAIT = 120,
	MX6DL_PAD_ENET_CRS_DV = 121,
	MX6DL_PAD_ENET_MDC = 122,
	MX6DL_PAD_ENET_MDIO = 123,
	MX6DL_PAD_ENET_REF_CLK = 124,
	MX6DL_PAD_ENET_RX_ER = 125,
	MX6DL_PAD_ENET_RXD0 = 126,
	MX6DL_PAD_ENET_RXD1 = 127,
	MX6DL_PAD_ENET_TX_EN = 128,
	MX6DL_PAD_ENET_TXD0 = 129,
	MX6DL_PAD_ENET_TXD1 = 130,
	MX6DL_PAD_GPIO_0 = 131,
	MX6DL_PAD_GPIO_1 = 132,
	MX6DL_PAD_GPIO_16 = 133,
	MX6DL_PAD_GPIO_17 = 134,
	MX6DL_PAD_GPIO_18 = 135,
	MX6DL_PAD_GPIO_19 = 136,
	MX6DL_PAD_GPIO_2 = 137,
	MX6DL_PAD_GPIO_3 = 138,
	MX6DL_PAD_GPIO_4 = 139,
	MX6DL_PAD_GPIO_5 = 140,
	MX6DL_PAD_GPIO_6 = 141,
	MX6DL_PAD_GPIO_7 = 142,
	MX6DL_PAD_GPIO_8 = 143,
	MX6DL_PAD_GPIO_9 = 144,
	MX6DL_PAD_KEY_COL0 = 145,
	MX6DL_PAD_KEY_COL1 = 146,
	MX6DL_PAD_KEY_COL2 = 147,
	MX6DL_PAD_KEY_COL3 = 148,
	MX6DL_PAD_KEY_COL4 = 149,
	MX6DL_PAD_KEY_ROW0 = 150,
	MX6DL_PAD_KEY_ROW1 = 151,
	MX6DL_PAD_KEY_ROW2 = 152,
	MX6DL_PAD_KEY_ROW3 = 153,
	MX6DL_PAD_KEY_ROW4 = 154,
	MX6DL_PAD_NANDF_ALE = 155,
	MX6DL_PAD_NANDF_CLE = 156,
	MX6DL_PAD_NANDF_CS0 = 157,
	MX6DL_PAD_NANDF_CS1 = 158,
	MX6DL_PAD_NANDF_CS2 = 159,
	MX6DL_PAD_NANDF_CS3 = 160,
	MX6DL_PAD_NANDF_D0 = 161,
	MX6DL_PAD_NANDF_D1 = 162,
	MX6DL_PAD_NANDF_D2 = 163,
	MX6DL_PAD_NANDF_D3 = 164,
	MX6DL_PAD_NANDF_D4 = 165,
	MX6DL_PAD_NANDF_D5 = 166,
	MX6DL_PAD_NANDF_D6 = 167,
	MX6DL_PAD_NANDF_D7 = 168,
	MX6DL_PAD_NANDF_RB0 = 169,
	MX6DL_PAD_NANDF_WP_B = 170,
	MX6DL_PAD_RGMII_RD0 = 171,
	MX6DL_PAD_RGMII_RD1 = 172,
	MX6DL_PAD_RGMII_RD2 = 173,
	MX6DL_PAD_RGMII_RD3 = 174,
	MX6DL_PAD_RGMII_RX_CTL = 175,
	MX6DL_PAD_RGMII_RXC = 176,
	MX6DL_PAD_RGMII_TD0 = 177,
	MX6DL_PAD_RGMII_TD1 = 178,
	MX6DL_PAD_RGMII_TD2 = 179,
	MX6DL_PAD_RGMII_TD3 = 180,
	MX6DL_PAD_RGMII_TX_CTL = 181,
	MX6DL_PAD_RGMII_TXC = 182,
	MX6DL_PAD_SD1_CLK = 183,
	MX6DL_PAD_SD1_CMD = 184,
	MX6DL_PAD_SD1_DAT0 = 185,
	MX6DL_PAD_SD1_DAT1 = 186,
	MX6DL_PAD_SD1_DAT2 = 187,
	MX6DL_PAD_SD1_DAT3 = 188,
	MX6DL_PAD_SD2_CLK = 189,
	MX6DL_PAD_SD2_CMD = 190,
	MX6DL_PAD_SD2_DAT0 = 191,
	MX6DL_PAD_SD2_DAT1 = 192,
	MX6DL_PAD_SD2_DAT2 = 193,
	MX6DL_PAD_SD2_DAT3 = 194,
	MX6DL_PAD_SD3_CLK = 195,
	MX6DL_PAD_SD3_CMD = 196,
	MX6DL_PAD_SD3_DAT0 = 197,
	MX6DL_PAD_SD3_DAT1 = 198,
	MX6DL_PAD_SD3_DAT2 = 199,
	MX6DL_PAD_SD3_DAT3 = 200,
	MX6DL_PAD_SD3_DAT4 = 201,
	MX6DL_PAD_SD3_DAT5 = 202,
	MX6DL_PAD_SD3_DAT6 = 203,
	MX6DL_PAD_SD3_DAT7 = 204,
	MX6DL_PAD_SD3_RST = 205,
	MX6DL_PAD_SD4_CLK = 206,
	MX6DL_PAD_SD4_CMD = 207,
	MX6DL_PAD_SD4_DAT0 = 208,
	MX6DL_PAD_SD4_DAT1 = 209,
	MX6DL_PAD_SD4_DAT2 = 210,
	MX6DL_PAD_SD4_DAT3 = 211,
	MX6DL_PAD_SD4_DAT4 = 212,
	MX6DL_PAD_SD4_DAT5 = 213,
	MX6DL_PAD_SD4_DAT6 = 214,
	MX6DL_PAD_SD4_DAT7 = 215,
};

/* Pad names for the pinmux subsystem */
static const struct pinctrl_pin_desc imx6dl_pinctrl_pads[] = {
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE0),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE1),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE2),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE3),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE4),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE5),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE6),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE7),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE8),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE9),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE10),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE11),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE12),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE13),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE14),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE15),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE16),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE17),
	IMX_PINCTRL_PIN(MX6DL_PAD_RESERVE18),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT10),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT11),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT12),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT13),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT14),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT15),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT16),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT17),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT18),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT19),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT4),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT5),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT6),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT7),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT8),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DAT9),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_DATA_EN),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_MCLK),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_PIXCLK),
	IMX_PINCTRL_PIN(MX6DL_PAD_CSI0_VSYNC),
	IMX_PINCTRL_PIN(MX6DL_PAD_DI0_DISP_CLK),
	IMX_PINCTRL_PIN(MX6DL_PAD_DI0_PIN15),
	IMX_PINCTRL_PIN(MX6DL_PAD_DI0_PIN2),
	IMX_PINCTRL_PIN(MX6DL_PAD_DI0_PIN3),
	IMX_PINCTRL_PIN(MX6DL_PAD_DI0_PIN4),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT0),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT1),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT10),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT11),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT12),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT13),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT14),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT15),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT16),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT17),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT18),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT19),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT2),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT20),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT21),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT22),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT23),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT3),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT4),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT5),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT6),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT7),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT8),
	IMX_PINCTRL_PIN(MX6DL_PAD_DISP0_DAT9),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A16),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A17),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A18),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A19),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A20),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A21),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A22),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A23),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A24),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_A25),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_BCLK),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_CS0),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_CS1),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D16),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D17),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D18),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D19),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D20),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D21),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D22),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D23),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D24),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D25),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D26),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D27),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D28),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D29),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D30),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_D31),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA0),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA1),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA10),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA11),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA12),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA13),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA14),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA15),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA2),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA3),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA4),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA5),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA6),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA7),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA8),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_DA9),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_EB0),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_EB1),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_EB2),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_EB3),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_LBA),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_OE),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_RW),
	IMX_PINCTRL_PIN(MX6DL_PAD_EIM_WAIT),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_CRS_DV),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_MDC),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_MDIO),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_REF_CLK),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_RX_ER),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_RXD0),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_RXD1),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_TX_EN),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_TXD0),
	IMX_PINCTRL_PIN(MX6DL_PAD_ENET_TXD1),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_0),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_1),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_16),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_17),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_18),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_19),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_2),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_3),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_4),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_5),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_6),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_7),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_8),
	IMX_PINCTRL_PIN(MX6DL_PAD_GPIO_9),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_COL0),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_COL1),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_COL2),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_COL3),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_COL4),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_ROW0),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_ROW1),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_ROW2),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_ROW3),
	IMX_PINCTRL_PIN(MX6DL_PAD_KEY_ROW4),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_ALE),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_CLE),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_CS0),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_CS1),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_CS2),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_CS3),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_D0),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_D1),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_D2),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_D3),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_D4),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_D5),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_D6),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_D7),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_RB0),
	IMX_PINCTRL_PIN(MX6DL_PAD_NANDF_WP_B),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_RD0),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_RD1),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_RD2),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_RD3),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_RX_CTL),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_RXC),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_TD0),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_TD1),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_TD2),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_TD3),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_TX_CTL),
	IMX_PINCTRL_PIN(MX6DL_PAD_RGMII_TXC),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD1_CLK),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD1_CMD),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD1_DAT0),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD1_DAT1),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD1_DAT2),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD1_DAT3),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD2_CLK),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD2_CMD),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD2_DAT0),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD2_DAT1),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD2_DAT2),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD2_DAT3),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_CLK),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_CMD),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_DAT0),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_DAT1),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_DAT2),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_DAT3),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_DAT4),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_DAT5),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_DAT6),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_DAT7),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD3_RST),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_CLK),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_CMD),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_DAT0),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_DAT1),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_DAT2),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_DAT3),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_DAT4),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_DAT5),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_DAT6),
	IMX_PINCTRL_PIN(MX6DL_PAD_SD4_DAT7),
};

static struct imx_pinctrl_soc_info imx6dl_pinctrl_info = {
	.pins = imx6dl_pinctrl_pads,
	.npins = ARRAY_SIZE(imx6dl_pinctrl_pads),
};

static struct of_device_id imx6dl_pinctrl_of_match[] = {
	{ .compatible = "fsl,imx6dl-iomuxc", },
	{ /* sentinel */ }
};

static int imx6dl_pinctrl_probe(struct platform_device *pdev)
{
	return imx_pinctrl_probe(pdev, &imx6dl_pinctrl_info);
}

static struct platform_driver imx6dl_pinctrl_driver = {
	.driver = {
		.name = "imx6dl-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(imx6dl_pinctrl_of_match),
	},
	.probe = imx6dl_pinctrl_probe,
	.remove = imx_pinctrl_remove,
};

static int __init imx6dl_pinctrl_init(void)
{
	return platform_driver_register(&imx6dl_pinctrl_driver);
}
arch_initcall(imx6dl_pinctrl_init);

static void __exit imx6dl_pinctrl_exit(void)
{
	platform_driver_unregister(&imx6dl_pinctrl_driver);
}
module_exit(imx6dl_pinctrl_exit);

MODULE_AUTHOR("Shawn Guo <shawn.guo@linaro.org>");
MODULE_DESCRIPTION("Freescale imx6dl pinctrl driver");
MODULE_LICENSE("GPL v2");
