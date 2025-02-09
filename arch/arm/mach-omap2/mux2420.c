/*
 * Copyright (C) 2010 Nokia
 * Copyright (C) 2010 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/module.h>
#include <fikus/init.h>

#include "mux.h"

#ifdef CONFIG_OMAP_MUX

#define _OMAP2420_MUXENTRY(M0, g, m0, m1, m2, m3, m4, m5, m6, m7)		\
{									\
	.reg_offset	= (OMAP2420_CONTROL_PADCONF_##M0##_OFFSET),	\
	.gpio		= (g),						\
	.muxnames	= { m0, m1, m2, m3, m4, m5, m6, m7 },		\
}

#else

#define _OMAP2420_MUXENTRY(M0, g, m0, m1, m2, m3, m4, m5, m6, m7)		\
{									\
	.reg_offset	= (OMAP2420_CONTROL_PADCONF_##M0##_OFFSET),	\
	.gpio		= (g),						\
}

#endif

#define _OMAP2420_BALLENTRY(M0, bb, bt)					\
{									\
	.reg_offset	= (OMAP2420_CONTROL_PADCONF_##M0##_OFFSET),	\
	.balls		= { bb, bt },					\
}

/*
 * Superset of all mux modes for omap2420
 */
static struct omap_mux __initdata omap2420_muxmodes[] = {
	_OMAP2420_MUXENTRY(CAM_D0, 54,
		"cam_d0", "hw_dbg2", "sti_dout", "gpio_54",
		NULL, NULL, "etk_d2", NULL),
	_OMAP2420_MUXENTRY(CAM_D1, 53,
		"cam_d1", "hw_dbg3", "sti_din", "gpio_53",
		NULL, NULL, "etk_d3", NULL),
	_OMAP2420_MUXENTRY(CAM_D2, 52,
		"cam_d2", "hw_dbg4", "mcbsp1_clkx", "gpio_52",
		NULL, NULL, "etk_d4", NULL),
	_OMAP2420_MUXENTRY(CAM_D3, 51,
		"cam_d3", "hw_dbg5", "mcbsp1_dr", "gpio_51",
		NULL, NULL, "etk_d5", NULL),
	_OMAP2420_MUXENTRY(CAM_D4, 50,
		"cam_d4", "hw_dbg6", "mcbsp1_fsr", "gpio_50",
		NULL, NULL, "etk_d6", NULL),
	_OMAP2420_MUXENTRY(CAM_D5, 49,
		"cam_d5", "hw_dbg7", "mcbsp1_clkr", "gpio_49",
		NULL, NULL, "etk_d7", NULL),
	_OMAP2420_MUXENTRY(CAM_D6, 0,
		"cam_d6", "hw_dbg8", NULL, NULL,
		NULL, NULL, "etk_d8", NULL),
	_OMAP2420_MUXENTRY(CAM_D7, 0,
		"cam_d7", "hw_dbg9", NULL, NULL,
		NULL, NULL, "etk_d9", NULL),
	_OMAP2420_MUXENTRY(CAM_D8, 54,
		"cam_d8", "hw_dbg10", NULL, "gpio_54",
		NULL, NULL, "etk_d10", NULL),
	_OMAP2420_MUXENTRY(CAM_D9, 53,
		"cam_d9", "hw_dbg11", NULL, "gpio_53",
		NULL, NULL, "etk_d11", NULL),
	_OMAP2420_MUXENTRY(CAM_HS, 55,
		"cam_hs", "hw_dbg1", "mcbsp1_dx", "gpio_55",
		NULL, NULL, "etk_d1", NULL),
	_OMAP2420_MUXENTRY(CAM_LCLK, 57,
		"cam_lclk", NULL, "mcbsp_clks", "gpio_57",
		NULL, NULL, "etk_c1", NULL),
	_OMAP2420_MUXENTRY(CAM_VS, 56,
		"cam_vs", "hw_dbg0", "mcbsp1_fsx", "gpio_56",
		NULL, NULL, "etk_d0", NULL),
	_OMAP2420_MUXENTRY(CAM_XCLK, 0,
		"cam_xclk", NULL, "sti_clk", NULL,
		NULL, NULL, "etk_c2", NULL),
	_OMAP2420_MUXENTRY(DSS_ACBIAS, 48,
		"dss_acbias", NULL, "mcbsp2_fsx", "gpio_48",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA10, 40,
		"dss_data10", NULL, NULL, "gpio_40",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA11, 41,
		"dss_data11", NULL, NULL, "gpio_41",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA12, 42,
		"dss_data12", NULL, NULL, "gpio_42",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA13, 43,
		"dss_data13", NULL, NULL, "gpio_43",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA14, 44,
		"dss_data14", NULL, NULL, "gpio_44",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA15, 45,
		"dss_data15", NULL, NULL, "gpio_45",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA16, 46,
		"dss_data16", NULL, NULL, "gpio_46",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA17, 47,
		"dss_data17", NULL, NULL, "gpio_47",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA8, 38,
		"dss_data8", NULL, NULL, "gpio_38",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(DSS_DATA9, 39,
		"dss_data9", NULL, NULL, "gpio_39",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(EAC_AC_DIN, 115,
		"eac_ac_din", "mcbsp2_dr", NULL, "gpio_115",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(EAC_AC_DOUT, 116,
		"eac_ac_dout", "mcbsp2_dx", NULL, "gpio_116",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(EAC_AC_FS, 114,
		"eac_ac_fs", "mcbsp2_fsx", NULL, "gpio_114",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(EAC_AC_MCLK, 117,
		"eac_ac_mclk", NULL, NULL, "gpio_117",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(EAC_AC_RST, 118,
		"eac_ac_rst", "eac_bt_din", NULL, "gpio_118",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(EAC_AC_SCLK, 113,
		"eac_ac_sclk", "mcbsp2_clkx", NULL, "gpio_113",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(EAC_BT_DIN, 73,
		"eac_bt_din", NULL, NULL, "gpio_73",
		NULL, NULL, "etk_d9", NULL),
	_OMAP2420_MUXENTRY(EAC_BT_DOUT, 74,
		"eac_bt_dout", NULL, "sti_clk", "gpio_74",
		NULL, NULL, "etk_d8", NULL),
	_OMAP2420_MUXENTRY(EAC_BT_FS, 72,
		"eac_bt_fs", NULL, NULL, "gpio_72",
		NULL, NULL, "etk_d10", NULL),
	_OMAP2420_MUXENTRY(EAC_BT_SCLK, 71,
		"eac_bt_sclk", NULL, NULL, "gpio_71",
		NULL, NULL, "etk_d11", NULL),
	_OMAP2420_MUXENTRY(GPIO_119, 119,
		"gpio_119", NULL, "sti_din", "gpio_119",
		NULL, "sys_boot0", "etk_d12", NULL),
	_OMAP2420_MUXENTRY(GPIO_120, 120,
		"gpio_120", NULL, "sti_dout", "gpio_120",
		"cam_d9", "sys_boot1", "etk_d13", NULL),
	_OMAP2420_MUXENTRY(GPIO_121, 121,
		"gpio_121", NULL, NULL, "gpio_121",
		"jtag_emu2", "sys_boot2", "etk_d14", NULL),
	_OMAP2420_MUXENTRY(GPIO_122, 122,
		"gpio_122", NULL, NULL, "gpio_122",
		"jtag_emu3", "sys_boot3", "etk_d15", NULL),
	_OMAP2420_MUXENTRY(GPIO_124, 124,
		"gpio_124", NULL, NULL, "gpio_124",
		NULL, "sys_boot5", NULL, NULL),
	_OMAP2420_MUXENTRY(GPIO_125, 125,
		"gpio_125", "sys_jtagsel1", "sys_jtagsel2", "gpio_125",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPIO_36, 36,
		"gpio_36", NULL, NULL, "gpio_36",
		NULL, "sys_boot4", NULL, NULL),
	_OMAP2420_MUXENTRY(GPIO_62, 62,
		"gpio_62", "uart1_rx", "usb1_dat", "gpio_62",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPIO_6, 6,
		"gpio_6", "tv_detpulse", NULL, "gpio_6",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A10, 3,
		"gpmc_a10", NULL, "sys_ndmareq5", "gpio_3",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A1, 12,
		"gpmc_a1", "dss_data18", NULL, "gpio_12",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A2, 11,
		"gpmc_a2", "dss_data19", NULL, "gpio_11",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A3, 10,
		"gpmc_a3", "dss_data20", NULL, "gpio_10",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A4, 9,
		"gpmc_a4", "dss_data21", NULL, "gpio_9",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A5, 8,
		"gpmc_a5", "dss_data22", NULL, "gpio_8",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A6, 7,
		"gpmc_a6", "dss_data23", NULL, "gpio_7",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A7, 6,
		"gpmc_a7", NULL, "sys_ndmareq2", "gpio_6",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A8, 5,
		"gpmc_a8", NULL, "sys_ndmareq3", "gpio_5",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_A9, 4,
		"gpmc_a9", NULL, "sys_ndmareq4", "gpio_4",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_CLK, 21,
		"gpmc_clk", NULL, NULL, "gpio_21",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_D10, 18,
		"gpmc_d10", "ssi2_rdy_rx", NULL, "gpio_18",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_D11, 17,
		"gpmc_d11", "ssi2_flag_rx", NULL, "gpio_17",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_D12, 16,
		"gpmc_d12", "ssi2_dat_rx", NULL, "gpio_16",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_D13, 15,
		"gpmc_d13", "ssi2_rdy_tx", NULL, "gpio_15",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_D14, 14,
		"gpmc_d14", "ssi2_flag_tx", NULL, "gpio_14",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_D15, 13,
		"gpmc_d15", "ssi2_dat_tx", NULL, "gpio_13",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_D8, 20,
		"gpmc_d8", NULL, NULL, "gpio_20",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_D9, 19,
		"gpmc_d9", "ssi2_wake", NULL, "gpio_19",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NBE0, 29,
		"gpmc_nbe0", NULL, NULL, "gpio_29",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NBE1, 30,
		"gpmc_nbe1", NULL, NULL, "gpio_30",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NCS1, 22,
		"gpmc_ncs1", NULL, NULL, "gpio_22",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NCS2, 23,
		"gpmc_ncs2", NULL, NULL, "gpio_23",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NCS3, 24,
		"gpmc_ncs3", "gpmc_io_dir", NULL, "gpio_24",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NCS4, 25,
		"gpmc_ncs4", NULL, NULL, "gpio_25",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NCS5, 26,
		"gpmc_ncs5", NULL, NULL, "gpio_26",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NCS6, 27,
		"gpmc_ncs6", NULL, NULL, "gpio_27",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NCS7, 28,
		"gpmc_ncs7", "gpmc_io_dir", "gpio_28", NULL,
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_NWP, 31,
		"gpmc_nwp", NULL, NULL, "gpio_31",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_WAIT1, 33,
		"gpmc_wait1", NULL, NULL, "gpio_33",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_WAIT2, 34,
		"gpmc_wait2", NULL, NULL, "gpio_34",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(GPMC_WAIT3, 35,
		"gpmc_wait3", NULL, NULL, "gpio_35",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(HDQ_SIO, 101,
		"hdq_sio", "usb2_tllse0", "sys_altclk", "gpio_101",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(I2C2_SCL, 99,
		"i2c2_scl", NULL, "gpt9_pwm_evt", "gpio_99",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(I2C2_SDA, 100,
		"i2c2_sda", NULL, "spi2_ncs1", "gpio_100",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(JTAG_EMU0, 127,
		"jtag_emu0", NULL, NULL, "gpio_127",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(JTAG_EMU1, 126,
		"jtag_emu1", NULL, NULL, "gpio_126",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MCBSP1_CLKR, 92,
		"mcbsp1_clkr", "ssi2_dat_tx", "vlynq_tx1", "gpio_92",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MCBSP1_CLKX, 98,
		"mcbsp1_clkx", "ssi2_wake", "vlynq_nla", "gpio_98",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MCBSP1_DR, 95,
		"mcbsp1_dr", "ssi2_dat_rx", "vlynq_rx1", "gpio_95",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MCBSP1_DX, 94,
		"mcbsp1_dx", "ssi2_rdy_tx", "vlynq_clk", "gpio_94",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MCBSP1_FSR, 93,
		"mcbsp1_fsr", "ssi2_flag_tx", "vlynq_tx0", "gpio_93",
		"spi2_ncs1", NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MCBSP1_FSX, 97,
		"mcbsp1_fsx", "ssi2_rdy_rx", NULL, "gpio_97",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MCBSP2_CLKX, 12,
		"mcbsp2_clkx", NULL, "dss_data23", "gpio_12",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MCBSP2_DR, 11,
		"mcbsp2_dr", NULL, "dss_data22", "gpio_11",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MCBSP_CLKS, 96,
		"mcbsp_clks", "ssi2_flag_rx", "vlynq_rx0", "gpio_96",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_CLKI, 59,
		"sdmmc_clki", "ms_clki", NULL, "gpio_59",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_CLKO, 0,
		"sdmmc_clko", "ms_clko", NULL, NULL,
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_CMD_DIR, 8,
		"sdmmc_cmd_dir", NULL, NULL, "gpio_8",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_CMD, 0,
		"sdmmc_cmd", "ms_bs", NULL, NULL,
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_DAT_DIR0, 7,
		"sdmmc_dat_dir0", "ms_dat0_dir", NULL, "gpio_7",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_DAT0, 0,
		"sdmmc_dat0", "ms_dat0", NULL, NULL,
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_DAT_DIR1, 78,
		"sdmmc_dat_dir1", "ms_datu_dir", "uart2_rts", "gpio_78",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_DAT1, 75,
		"sdmmc_dat1", "ms_dat1", NULL, "gpio_75",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_DAT_DIR2, 79,
		"sdmmc_dat_dir2", "ms_datu_dir", "uart2_tx", "gpio_79",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_DAT2, 76,
		"sdmmc_dat2", "ms_dat2", "uart2_cts", "gpio_76",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_DAT_DIR3, 80,
		"sdmmc_dat_dir3", "ms_datu_dir", "uart2_rx", "gpio_80",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(MMC_DAT3, 77,
		"sdmmc_dat3", "ms_dat3", NULL, "gpio_77",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SDRC_A12, 2,
		"sdrc_a12", NULL, NULL, "gpio_2",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SDRC_A13, 1,
		"sdrc_a13", NULL, NULL, "gpio_1",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SDRC_A14, 0,
		"sdrc_a14", NULL, NULL, "gpio_0",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SDRC_CKE1, 38,
		"sdrc_cke1", NULL, NULL, "gpio_38",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SDRC_NCS1, 37,
		"sdrc_ncs1", NULL, NULL, "gpio_37",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI1_CLK, 81,
		"spi1_clk", NULL, NULL, "gpio_81",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI1_NCS0, 84,
		"spi1_ncs0", NULL, NULL, "gpio_84",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI1_NCS1, 85,
		"spi1_ncs1", NULL, NULL, "gpio_85",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI1_NCS2, 86,
		"spi1_ncs2", NULL, NULL, "gpio_86",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI1_NCS3, 87,
		"spi1_ncs3", NULL, NULL, "gpio_87",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI1_SIMO, 82,
		"spi1_simo", NULL, NULL, "gpio_82",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI1_SOMI, 83,
		"spi1_somi", NULL, NULL, "gpio_83",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI2_CLK, 88,
		"spi2_clk", NULL, NULL, "gpio_88",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI2_NCS0, 91,
		"spi2_ncs0", "gpt12_pwm_evt", NULL, "gpio_91",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI2_SIMO, 89,
		"spi2_simo", "gpt10_pwm_evt", NULL, "gpio_89",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SPI2_SOMI, 90,
		"spi2_somi", "gpt11_pwm_evt", NULL, "gpio_90",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SSI1_DAT_RX, 63,
		"ssi1_dat_rx", "eac_md_sclk", NULL, "gpio_63",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SSI1_DAT_TX, 59,
		"ssi1_dat_tx", "uart1_tx", "usb1_se0", "gpio_59",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SSI1_FLAG_RX, 64,
		"ssi1_flag_rx", "eac_md_din", NULL, "gpio_64",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SSI1_FLAG_TX, 25,
		"ssi1_flag_tx", "uart1_rts", "usb1_rcv", "gpio_25",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SSI1_RDY_RX, 65,
		"ssi1_rdy_rx", "eac_md_dout", NULL, "gpio_65",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SSI1_RDY_TX, 61,
		"ssi1_rdy_tx", "uart1_cts", "usb1_txen", "gpio_61",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SSI1_WAKE, 66,
		"ssi1_wake", "eac_md_fs", NULL, "gpio_66",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SYS_CLKOUT, 123,
		"sys_clkout", NULL, NULL, "gpio_123",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SYS_CLKREQ, 52,
		"sys_clkreq", NULL, NULL, "gpio_52",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(SYS_NIRQ, 60,
		"sys_nirq", NULL, NULL, "gpio_60",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART1_CTS, 32,
		"uart1_cts", NULL, "dss_data18", "gpio_32",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART1_RTS, 8,
		"uart1_rts", NULL, "dss_data19", "gpio_8",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART1_RX, 10,
		"uart1_rx", NULL, "dss_data21", "gpio_10",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART1_TX, 9,
		"uart1_tx", NULL, "dss_data20", "gpio_9",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART2_CTS, 67,
		"uart2_cts", "usb1_rcv", "gpt9_pwm_evt", "gpio_67",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART2_RTS, 68,
		"uart2_rts", "usb1_txen", "gpt10_pwm_evt", "gpio_68",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART2_RX, 70,
		"uart2_rx", "usb1_dat", "gpt12_pwm_evt", "gpio_70",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART2_TX, 69,
		"uart2_tx", "usb1_se0", "gpt11_pwm_evt", "gpio_69",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART3_CTS_RCTX, 102,
		"uart3_cts_rctx", "uart3_rx_irrx", NULL, "gpio_102",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART3_RTS_SD, 103,
		"uart3_rts_sd", "uart3_tx_irtx", NULL, "gpio_103",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART3_RX_IRRX, 105,
		"uart3_rx_irrx", NULL, NULL, "gpio_105",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(UART3_TX_IRTX, 104,
		"uart3_tx_irtx", "uart3_cts_rctx", NULL, "gpio_104",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(USB0_DAT, 112,
		"usb0_dat", "uart3_rx_irrx", "uart2_rx", "gpio_112",
		"uart2_tx", NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(USB0_PUEN, 106,
		"usb0_puen", "mcbsp2_dx", NULL, "gpio_106",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(USB0_RCV, 109,
		"usb0_rcv", "mcbsp2_fsx", NULL, "gpio_109",
		"uart2_cts", NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(USB0_SE0, 111,
		"usb0_se0", "uart3_tx_irtx", "uart2_tx", "gpio_111",
		"uart2_rx", NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(USB0_TXEN, 110,
		"usb0_txen", "uart3_cts_rctx", "uart2_cts", "gpio_110",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(USB0_VM, 108,
		"usb0_vm", "mcbsp2_clkx", NULL, "gpio_108",
		"uart2_rx", NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(USB0_VP, 107,
		"usb0_vp", "mcbsp2_dr", NULL, "gpio_107",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(VLYNQ_CLK, 13,
		"vlynq_clk", "usb2_se0", "sys_ndmareq0", "gpio_13",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(VLYNQ_NLA, 58,
		"vlynq_nla", NULL, NULL, "gpio_58",
		"cam_d6", NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(VLYNQ_RX0, 15,
		"vlynq_rx0", "usb2_tllse0", NULL, "gpio_15",
		"cam_d7", NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(VLYNQ_RX1, 14,
		"vlynq_rx1", "usb2_rcv", "sys_ndmareq1", "gpio_14",
		"cam_d8", NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(VLYNQ_TX0, 17,
		"vlynq_tx0", "usb2_txen", NULL, "gpio_17",
		NULL, NULL, NULL, NULL),
	_OMAP2420_MUXENTRY(VLYNQ_TX1, 16,
		"vlynq_tx1", "usb2_dat", "sys_clkout2", "gpio_16",
		NULL, NULL, NULL, NULL),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

/*
 * Balls for 447-pin POP package
 */
#ifdef CONFIG_DEBUG_FS
static struct omap_ball __initdata omap2420_pop_ball[] = {
	_OMAP2420_BALLENTRY(CAM_D0, "y4", NULL),
	_OMAP2420_BALLENTRY(CAM_D1, "y3", NULL),
	_OMAP2420_BALLENTRY(CAM_D2, "u7", NULL),
	_OMAP2420_BALLENTRY(CAM_D3, "ab3", NULL),
	_OMAP2420_BALLENTRY(CAM_D4, "v2", NULL),
	_OMAP2420_BALLENTRY(CAM_D5, "ad3", NULL),
	_OMAP2420_BALLENTRY(CAM_D6, "aa4", NULL),
	_OMAP2420_BALLENTRY(CAM_D7, "ab4", NULL),
	_OMAP2420_BALLENTRY(CAM_D8, "ac6", NULL),
	_OMAP2420_BALLENTRY(CAM_D9, "ac7", NULL),
	_OMAP2420_BALLENTRY(CAM_HS, "v4", NULL),
	_OMAP2420_BALLENTRY(CAM_LCLK, "ad6", NULL),
	_OMAP2420_BALLENTRY(CAM_VS, "p7", NULL),
	_OMAP2420_BALLENTRY(CAM_XCLK, "w4", NULL),
	_OMAP2420_BALLENTRY(DSS_ACBIAS, "ae8", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA10, "ac12", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA11, "ae11", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA12, "ae13", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA13, "ad13", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA14, "ac13", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA15, "y12", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA16, "ad14", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA17, "y13", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA8, "ad11", NULL),
	_OMAP2420_BALLENTRY(DSS_DATA9, "ad12", NULL),
	_OMAP2420_BALLENTRY(EAC_AC_DIN, "ad19", NULL),
	_OMAP2420_BALLENTRY(EAC_AC_DOUT, "af22", NULL),
	_OMAP2420_BALLENTRY(EAC_AC_FS, "ad16", NULL),
	_OMAP2420_BALLENTRY(EAC_AC_MCLK, "y17", NULL),
	_OMAP2420_BALLENTRY(EAC_AC_RST, "ae22", NULL),
	_OMAP2420_BALLENTRY(EAC_AC_SCLK, "ac18", NULL),
	_OMAP2420_BALLENTRY(EAC_BT_DIN, "u8", NULL),
	_OMAP2420_BALLENTRY(EAC_BT_DOUT, "ad5", NULL),
	_OMAP2420_BALLENTRY(EAC_BT_FS, "w7", NULL),
	_OMAP2420_BALLENTRY(EAC_BT_SCLK, "ad4", NULL),
	_OMAP2420_BALLENTRY(GPIO_119, "af6", NULL),
	_OMAP2420_BALLENTRY(GPIO_120, "af4", NULL),
	_OMAP2420_BALLENTRY(GPIO_121, "ae6", NULL),
	_OMAP2420_BALLENTRY(GPIO_122, "w3", NULL),
	_OMAP2420_BALLENTRY(GPIO_124, "y19", NULL),
	_OMAP2420_BALLENTRY(GPIO_125, "ae24", NULL),
	_OMAP2420_BALLENTRY(GPIO_36, "y18", NULL),
	_OMAP2420_BALLENTRY(GPIO_6, "d6", NULL),
	_OMAP2420_BALLENTRY(GPIO_62, "ad18", NULL),
	_OMAP2420_BALLENTRY(GPMC_A1, "m8", NULL),
	_OMAP2420_BALLENTRY(GPMC_A10, "d5", NULL),
	_OMAP2420_BALLENTRY(GPMC_A2, "w9", NULL),
	_OMAP2420_BALLENTRY(GPMC_A3, "af10", NULL),
	_OMAP2420_BALLENTRY(GPMC_A4, "w8", NULL),
	_OMAP2420_BALLENTRY(GPMC_A5, "ae16", NULL),
	_OMAP2420_BALLENTRY(GPMC_A6, "af9", NULL),
	_OMAP2420_BALLENTRY(GPMC_A7, "e4", NULL),
	_OMAP2420_BALLENTRY(GPMC_A8, "j7", NULL),
	_OMAP2420_BALLENTRY(GPMC_A9, "ae18", NULL),
	_OMAP2420_BALLENTRY(GPMC_CLK, "p1", "l1"),
	_OMAP2420_BALLENTRY(GPMC_D10, "t1", "n1"),
	_OMAP2420_BALLENTRY(GPMC_D11, "u2", "p2"),
	_OMAP2420_BALLENTRY(GPMC_D12, "u1", "p1"),
	_OMAP2420_BALLENTRY(GPMC_D13, "p2", "m1"),
	_OMAP2420_BALLENTRY(GPMC_D14, "h2", "j2"),
	_OMAP2420_BALLENTRY(GPMC_D15, "h1", "k2"),
	_OMAP2420_BALLENTRY(GPMC_D8, "v1", "r1"),
	_OMAP2420_BALLENTRY(GPMC_D9, "y1", "t1"),
	_OMAP2420_BALLENTRY(GPMC_NBE0, "af12", "aa10"),
	_OMAP2420_BALLENTRY(GPMC_NBE1, "u3", NULL),
	_OMAP2420_BALLENTRY(GPMC_NCS1, "af14", "w1"),
	_OMAP2420_BALLENTRY(GPMC_NCS2, "g4", NULL),
	_OMAP2420_BALLENTRY(GPMC_NCS3, "t8", NULL),
	_OMAP2420_BALLENTRY(GPMC_NCS4, "h8", NULL),
	_OMAP2420_BALLENTRY(GPMC_NCS5, "k3", NULL),
	_OMAP2420_BALLENTRY(GPMC_NCS6, "m7", NULL),
	_OMAP2420_BALLENTRY(GPMC_NCS7, "p3", NULL),
	_OMAP2420_BALLENTRY(GPMC_NWP, "ae15", "y5"),
	_OMAP2420_BALLENTRY(GPMC_WAIT1, "ae20", "y8"),
	_OMAP2420_BALLENTRY(GPMC_WAIT2, "n2", NULL),
	_OMAP2420_BALLENTRY(GPMC_WAIT3, "t4", NULL),
	_OMAP2420_BALLENTRY(HDQ_SIO, "t23", NULL),
	_OMAP2420_BALLENTRY(I2C2_SCL, "l2", NULL),
	_OMAP2420_BALLENTRY(I2C2_SDA, "k19", NULL),
	_OMAP2420_BALLENTRY(JTAG_EMU0, "n24", NULL),
	_OMAP2420_BALLENTRY(JTAG_EMU1, "ac22", NULL),
	_OMAP2420_BALLENTRY(MCBSP1_CLKR, "y24", NULL),
	_OMAP2420_BALLENTRY(MCBSP1_CLKX, "t19", NULL),
	_OMAP2420_BALLENTRY(MCBSP1_DR, "u23", NULL),
	_OMAP2420_BALLENTRY(MCBSP1_DX, "r24", NULL),
	_OMAP2420_BALLENTRY(MCBSP1_FSR, "r20", NULL),
	_OMAP2420_BALLENTRY(MCBSP1_FSX, "r23", NULL),
	_OMAP2420_BALLENTRY(MCBSP2_CLKX, "t24", NULL),
	_OMAP2420_BALLENTRY(MCBSP2_DR, "p20", NULL),
	_OMAP2420_BALLENTRY(MCBSP_CLKS, "p23", NULL),
	_OMAP2420_BALLENTRY(MMC_CLKI, "c23", NULL),
	_OMAP2420_BALLENTRY(MMC_CLKO, "h23", NULL),
	_OMAP2420_BALLENTRY(MMC_CMD, "j23", NULL),
	_OMAP2420_BALLENTRY(MMC_CMD_DIR, "j24", NULL),
	_OMAP2420_BALLENTRY(MMC_DAT0, "h17", NULL),
	_OMAP2420_BALLENTRY(MMC_DAT_DIR0, "f23", NULL),
	_OMAP2420_BALLENTRY(MMC_DAT1, "g19", NULL),
	_OMAP2420_BALLENTRY(MMC_DAT_DIR1, "d23", NULL),
	_OMAP2420_BALLENTRY(MMC_DAT2, "h20", NULL),
	_OMAP2420_BALLENTRY(MMC_DAT_DIR2, "g23", NULL),
	_OMAP2420_BALLENTRY(MMC_DAT3, "d24", NULL),
	_OMAP2420_BALLENTRY(MMC_DAT_DIR3, "e23", NULL),
	_OMAP2420_BALLENTRY(SDRC_A12, "w26", "r21"),
	_OMAP2420_BALLENTRY(SDRC_A13, "w25", "aa15"),
	_OMAP2420_BALLENTRY(SDRC_A14, "aa26", "y12"),
	_OMAP2420_BALLENTRY(SDRC_CKE1, "ae25", "y13"),
	_OMAP2420_BALLENTRY(SDRC_NCS1, "y25", "t20"),
	_OMAP2420_BALLENTRY(SPI1_CLK, "y23", NULL),
	_OMAP2420_BALLENTRY(SPI1_NCS0, "w24", NULL),
	_OMAP2420_BALLENTRY(SPI1_NCS1, "w23", NULL),
	_OMAP2420_BALLENTRY(SPI1_NCS2, "v23", NULL),
	_OMAP2420_BALLENTRY(SPI1_NCS3, "u20", NULL),
	_OMAP2420_BALLENTRY(SPI1_SIMO, "h10", NULL),
	_OMAP2420_BALLENTRY(SPI1_SOMI, "v19", NULL),
	_OMAP2420_BALLENTRY(SPI2_CLK, "v24", NULL),
	_OMAP2420_BALLENTRY(SPI2_NCS0, "aa24", NULL),
	_OMAP2420_BALLENTRY(SPI2_SIMO, "u24", NULL),
	_OMAP2420_BALLENTRY(SPI2_SOMI, "v25", NULL),
	_OMAP2420_BALLENTRY(SSI1_DAT_RX, "w15", NULL),
	_OMAP2420_BALLENTRY(SSI1_DAT_TX, "w13", NULL),
	_OMAP2420_BALLENTRY(SSI1_FLAG_RX, "af11", NULL),
	_OMAP2420_BALLENTRY(SSI1_FLAG_TX, "ac15", NULL),
	_OMAP2420_BALLENTRY(SSI1_RDY_RX, "ac16", NULL),
	_OMAP2420_BALLENTRY(SSI1_RDY_TX, "af15", NULL),
	_OMAP2420_BALLENTRY(SSI1_WAKE, "ad15", NULL),
	_OMAP2420_BALLENTRY(SYS_CLKOUT, "ae19", NULL),
	_OMAP2420_BALLENTRY(SYS_CLKREQ, "ad20", NULL),
	_OMAP2420_BALLENTRY(SYS_NIRQ, "y20", NULL),
	_OMAP2420_BALLENTRY(UART1_CTS, "g20", NULL),
	_OMAP2420_BALLENTRY(UART1_RTS, "k20", NULL),
	_OMAP2420_BALLENTRY(UART1_RX, "t20", NULL),
	_OMAP2420_BALLENTRY(UART1_TX, "h12", NULL),
	_OMAP2420_BALLENTRY(UART2_CTS, "ac24", NULL),
	_OMAP2420_BALLENTRY(UART2_RTS, "w20", NULL),
	_OMAP2420_BALLENTRY(UART2_RX, "ad24", NULL),
	_OMAP2420_BALLENTRY(UART2_TX, "ab24", NULL),
	_OMAP2420_BALLENTRY(UART3_CTS_RCTX, "k24", NULL),
	_OMAP2420_BALLENTRY(UART3_RTS_SD, "m20", NULL),
	_OMAP2420_BALLENTRY(UART3_RX_IRRX, "h24", NULL),
	_OMAP2420_BALLENTRY(UART3_TX_IRTX, "g24", NULL),
	_OMAP2420_BALLENTRY(USB0_DAT, "j25", NULL),
	_OMAP2420_BALLENTRY(USB0_PUEN, "l23", NULL),
	_OMAP2420_BALLENTRY(USB0_RCV, "k23", NULL),
	_OMAP2420_BALLENTRY(USB0_SE0, "l24", NULL),
	_OMAP2420_BALLENTRY(USB0_TXEN, "m24", NULL),
	_OMAP2420_BALLENTRY(USB0_VM, "n23", NULL),
	_OMAP2420_BALLENTRY(USB0_VP, "m23", NULL),
	_OMAP2420_BALLENTRY(VLYNQ_CLK, "w12", NULL),
	_OMAP2420_BALLENTRY(VLYNQ_NLA, "ae10", NULL),
	_OMAP2420_BALLENTRY(VLYNQ_RX0, "ad7", NULL),
	_OMAP2420_BALLENTRY(VLYNQ_RX1, "w10", NULL),
	_OMAP2420_BALLENTRY(VLYNQ_TX0, "y15", NULL),
	_OMAP2420_BALLENTRY(VLYNQ_TX1, "w14", NULL),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define omap2420_pop_ball	 NULL
#endif

int __init omap2420_mux_init(struct omap_board_mux *board_subset, int flags)
{
	struct omap_ball *package_balls = NULL;

	switch (flags & OMAP_PACKAGE_MASK) {
	case OMAP_PACKAGE_ZAC:
		package_balls = omap2420_pop_ball;
		break;
	case OMAP_PACKAGE_ZAF:
		/* REVISIT: Please add data */
	default:
		pr_warning("%s: No ball data available for omap2420 package\n",
				__func__);
	}

	return omap_mux_init("core", OMAP_MUX_REG_8BIT | OMAP_MUX_GPIO_IN_MODE3,
			     OMAP2420_CONTROL_PADCONF_MUX_PBASE,
			     OMAP2420_CONTROL_PADCONF_MUX_SIZE,
			     omap2420_muxmodes, NULL, board_subset,
			     package_balls);
}
