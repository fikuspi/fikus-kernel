/*
 * OMAP44xx CTRL_MODULE_CORE registers and bitfields
 *
 * Copyright (C) 2009-2010 Texas Instruments, Inc.
 *
 * Benoit Cousson (b-cousson@ti.com)
 * Santosh Shilimkar (santosh.shilimkar@ti.com)
 *
 * This file is automatically generated from the OMAP hardware databases.
 * We respectfully ask that any modifications to this file be coordinated
 * with the public fikus-omap@vger.kernel.org mailing list and the
 * authors above to ensure that the autogeneration scripts are kept
 * up-to-date with the file contents.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ARCH_ARM_MACH_OMAP2_CTRL_MODULE_CORE_44XX_H
#define __ARCH_ARM_MACH_OMAP2_CTRL_MODULE_CORE_44XX_H


/* Base address */
#define OMAP4_CTRL_MODULE_CORE					0x4a002000

/* Registers offset */
#define OMAP4_CTRL_MODULE_CORE_IP_REVISION			0x0000
#define OMAP4_CTRL_MODULE_CORE_IP_HWINFO			0x0004
#define OMAP4_CTRL_MODULE_CORE_IP_SYSCONFIG			0x0010
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_DIE_ID_0		0x0200
#define OMAP4_CTRL_MODULE_CORE_ID_CODE				0x0204
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_DIE_ID_1		0x0208
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_DIE_ID_2		0x020c
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_DIE_ID_3		0x0210
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_PROD_ID_0		0x0214
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_PROD_ID_1		0x0218
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_USB_CONF		0x021c
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_OPP_VDD_WKUP		0x0228
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_OPP_BGAP		0x0260
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_OPP_DPLL_0		0x0264
#define OMAP4_CTRL_MODULE_CORE_STD_FUSE_OPP_DPLL_1		0x0268
#define OMAP4_CTRL_MODULE_CORE_STATUS				0x02c4
#define OMAP4_CTRL_MODULE_CORE_DEV_CONF				0x0300
#define OMAP4_CTRL_MODULE_CORE_DSP_BOOTADDR			0x0304
#define OMAP4_CTRL_MODULE_CORE_LDOVBB_IVA_VOLTAGE_CTRL		0x0314
#define OMAP4_CTRL_MODULE_CORE_LDOVBB_MPU_VOLTAGE_CTRL		0x0318
#define OMAP4_CTRL_MODULE_CORE_LDOSRAM_IVA_VOLTAGE_CTRL		0x0320
#define OMAP4_CTRL_MODULE_CORE_LDOSRAM_MPU_VOLTAGE_CTRL		0x0324
#define OMAP4_CTRL_MODULE_CORE_LDOSRAM_CORE_VOLTAGE_CTRL	0x0328
#define OMAP4_CTRL_MODULE_CORE_TEMP_SENSOR			0x032c
#define OMAP4_CTRL_MODULE_CORE_DPLL_NWELL_TRIM_0		0x0330
#define OMAP4_CTRL_MODULE_CORE_DPLL_NWELL_TRIM_1		0x0334
#define OMAP4_CTRL_MODULE_CORE_USBOTGHS_CONTROL			0x033c
#define OMAP4_CTRL_MODULE_CORE_DSS_CONTROL			0x0340
#define OMAP4_CTRL_MODULE_CORE_HWOBS_CONTROL			0x0350
#define OMAP4_CTRL_MODULE_CORE_DEBOBS_FINAL_MUX_SEL		0x0400
#define OMAP4_CTRL_MODULE_CORE_DEBOBS_MMR_MPU			0x0408
#define OMAP4_CTRL_MODULE_CORE_CONF_SDMA_REQ_SEL0		0x042c
#define OMAP4_CTRL_MODULE_CORE_CONF_SDMA_REQ_SEL1		0x0430
#define OMAP4_CTRL_MODULE_CORE_CONF_SDMA_REQ_SEL2		0x0434
#define OMAP4_CTRL_MODULE_CORE_CONF_SDMA_REQ_SEL3		0x0438
#define OMAP4_CTRL_MODULE_CORE_CONF_CLK_SEL0			0x0440
#define OMAP4_CTRL_MODULE_CORE_CONF_CLK_SEL1			0x0444
#define OMAP4_CTRL_MODULE_CORE_CONF_CLK_SEL2			0x0448
#define OMAP4_CTRL_MODULE_CORE_CONF_DPLL_FREQLOCK_SEL		0x044c
#define OMAP4_CTRL_MODULE_CORE_CONF_DPLL_TINITZ_SEL		0x0450
#define OMAP4_CTRL_MODULE_CORE_CONF_DPLL_PHASELOCK_SEL		0x0454
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_0		0x0480
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_1		0x0484
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_2		0x0488
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_3		0x048c
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_4		0x0490
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_5		0x0494
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_6		0x0498
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_7		0x049c
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_8		0x04a0
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_9		0x04a4
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_10		0x04a8
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_11		0x04ac
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_12		0x04b0
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_13		0x04b4
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_14		0x04b8
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_15		0x04bc
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_16		0x04c0
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_17		0x04c4
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_18		0x04c8
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_19		0x04cc
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_20		0x04d0
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_21		0x04d4
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_22		0x04d8
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_23		0x04dc
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_24		0x04e0
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_25		0x04e4
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_26		0x04e8
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_27		0x04ec
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_28		0x04f0
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_29		0x04f4
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_30		0x04f8
#define OMAP4_CTRL_MODULE_CORE_CONF_DEBUG_SEL_TST_31		0x04fc

/* Registers shifts and masks */

/* IP_REVISION */
#define OMAP4_IP_REV_SCHEME_SHIFT			30
#define OMAP4_IP_REV_SCHEME_MASK			(0x3 << 30)
#define OMAP4_IP_REV_FUNC_SHIFT				16
#define OMAP4_IP_REV_FUNC_MASK				(0xfff << 16)
#define OMAP4_IP_REV_RTL_SHIFT				11
#define OMAP4_IP_REV_RTL_MASK				(0x1f << 11)
#define OMAP4_IP_REV_MAJOR_SHIFT			8
#define OMAP4_IP_REV_MAJOR_MASK				(0x7 << 8)
#define OMAP4_IP_REV_CUSTOM_SHIFT			6
#define OMAP4_IP_REV_CUSTOM_MASK			(0x3 << 6)
#define OMAP4_IP_REV_MINOR_SHIFT			0
#define OMAP4_IP_REV_MINOR_MASK				(0x3f << 0)

/* IP_HWINFO */
#define OMAP4_IP_HWINFO_SHIFT				0
#define OMAP4_IP_HWINFO_MASK				(0xffffffff << 0)

/* IP_SYSCONFIG */
#define OMAP4_IP_SYSCONFIG_IDLEMODE_SHIFT		2
#define OMAP4_IP_SYSCONFIG_IDLEMODE_MASK		(0x3 << 2)

/* STD_FUSE_DIE_ID_0 */
#define OMAP4_STD_FUSE_DIE_ID_0_SHIFT			0
#define OMAP4_STD_FUSE_DIE_ID_0_MASK			(0xffffffff << 0)

/* ID_CODE */
#define OMAP4_STD_FUSE_IDCODE_SHIFT			0
#define OMAP4_STD_FUSE_IDCODE_MASK			(0xffffffff << 0)

/* STD_FUSE_DIE_ID_1 */
#define OMAP4_STD_FUSE_DIE_ID_1_SHIFT			0
#define OMAP4_STD_FUSE_DIE_ID_1_MASK			(0xffffffff << 0)

/* STD_FUSE_DIE_ID_2 */
#define OMAP4_STD_FUSE_DIE_ID_2_SHIFT			0
#define OMAP4_STD_FUSE_DIE_ID_2_MASK			(0xffffffff << 0)

/* STD_FUSE_DIE_ID_3 */
#define OMAP4_STD_FUSE_DIE_ID_3_SHIFT			0
#define OMAP4_STD_FUSE_DIE_ID_3_MASK			(0xffffffff << 0)

/* STD_FUSE_PROD_ID_0 */
#define OMAP4_STD_FUSE_PROD_ID_0_SHIFT			0
#define OMAP4_STD_FUSE_PROD_ID_0_MASK			(0xffffffff << 0)

/* STD_FUSE_PROD_ID_1 */
#define OMAP4_STD_FUSE_PROD_ID_1_SHIFT			0
#define OMAP4_STD_FUSE_PROD_ID_1_MASK			(0xffffffff << 0)

/* STD_FUSE_USB_CONF */
#define OMAP4_USB_PROD_ID_SHIFT				16
#define OMAP4_USB_PROD_ID_MASK				(0xffff << 16)
#define OMAP4_USB_VENDOR_ID_SHIFT			0
#define OMAP4_USB_VENDOR_ID_MASK			(0xffff << 0)

/* STD_FUSE_OPP_VDD_WKUP */
#define OMAP4_STD_FUSE_OPP_VDD_WKUP_SHIFT		0
#define OMAP4_STD_FUSE_OPP_VDD_WKUP_MASK		(0xffffffff << 0)

/* STD_FUSE_OPP_BGAP */
#define OMAP4_STD_FUSE_OPP_BGAP_SHIFT			0
#define OMAP4_STD_FUSE_OPP_BGAP_MASK			(0xffffffff << 0)

/* STD_FUSE_OPP_DPLL_0 */
#define OMAP4_STD_FUSE_OPP_DPLL_0_SHIFT			0
#define OMAP4_STD_FUSE_OPP_DPLL_0_MASK			(0xffffffff << 0)

/* STD_FUSE_OPP_DPLL_1 */
#define OMAP4_STD_FUSE_OPP_DPLL_1_SHIFT			0
#define OMAP4_STD_FUSE_OPP_DPLL_1_MASK			(0xffffffff << 0)

/* STATUS */
#define OMAP4_ATTILA_CONF_SHIFT				11
#define OMAP4_ATTILA_CONF_MASK				(0x3 << 11)
#define OMAP4_DEVICE_TYPE_SHIFT				8
#define OMAP4_DEVICE_TYPE_MASK				(0x7 << 8)
#define OMAP4_SYS_BOOT_SHIFT				0
#define OMAP4_SYS_BOOT_MASK				(0xff << 0)

/* DEV_CONF */
#define OMAP4_DEV_CONF_SHIFT				1
#define OMAP4_DEV_CONF_MASK				(0x7fffffff << 1)
#define OMAP4_USBPHY_PD_SHIFT				0
#define OMAP4_USBPHY_PD_MASK				(1 << 0)

/* LDOVBB_IVA_VOLTAGE_CTRL */
#define OMAP4_LDOVBBIVA_RBB_MUX_CTRL_SHIFT		26
#define OMAP4_LDOVBBIVA_RBB_MUX_CTRL_MASK		(1 << 26)
#define OMAP4_LDOVBBIVA_RBB_VSET_IN_SHIFT		21
#define OMAP4_LDOVBBIVA_RBB_VSET_IN_MASK		(0x1f << 21)
#define OMAP4_LDOVBBIVA_RBB_VSET_OUT_SHIFT		16
#define OMAP4_LDOVBBIVA_RBB_VSET_OUT_MASK		(0x1f << 16)
#define OMAP4_LDOVBBIVA_FBB_MUX_CTRL_SHIFT		10
#define OMAP4_LDOVBBIVA_FBB_MUX_CTRL_MASK		(1 << 10)
#define OMAP4_LDOVBBIVA_FBB_VSET_IN_SHIFT		5
#define OMAP4_LDOVBBIVA_FBB_VSET_IN_MASK		(0x1f << 5)
#define OMAP4_LDOVBBIVA_FBB_VSET_OUT_SHIFT		0
#define OMAP4_LDOVBBIVA_FBB_VSET_OUT_MASK		(0x1f << 0)

/* LDOVBB_MPU_VOLTAGE_CTRL */
#define OMAP4_LDOVBBMPU_RBB_MUX_CTRL_SHIFT		26
#define OMAP4_LDOVBBMPU_RBB_MUX_CTRL_MASK		(1 << 26)
#define OMAP4_LDOVBBMPU_RBB_VSET_IN_SHIFT		21
#define OMAP4_LDOVBBMPU_RBB_VSET_IN_MASK		(0x1f << 21)
#define OMAP4_LDOVBBMPU_RBB_VSET_OUT_SHIFT		16
#define OMAP4_LDOVBBMPU_RBB_VSET_OUT_MASK		(0x1f << 16)
#define OMAP4_LDOVBBMPU_FBB_MUX_CTRL_SHIFT		10
#define OMAP4_LDOVBBMPU_FBB_MUX_CTRL_MASK		(1 << 10)
#define OMAP4_LDOVBBMPU_FBB_VSET_IN_SHIFT		5
#define OMAP4_LDOVBBMPU_FBB_VSET_IN_MASK		(0x1f << 5)
#define OMAP4_LDOVBBMPU_FBB_VSET_OUT_SHIFT		0
#define OMAP4_LDOVBBMPU_FBB_VSET_OUT_MASK		(0x1f << 0)

/* LDOSRAM_IVA_VOLTAGE_CTRL */
#define OMAP4_LDOSRAMIVA_RETMODE_MUX_CTRL_SHIFT		26
#define OMAP4_LDOSRAMIVA_RETMODE_MUX_CTRL_MASK		(1 << 26)
#define OMAP4_LDOSRAMIVA_RETMODE_VSET_IN_SHIFT		21
#define OMAP4_LDOSRAMIVA_RETMODE_VSET_IN_MASK		(0x1f << 21)
#define OMAP4_LDOSRAMIVA_RETMODE_VSET_OUT_SHIFT		16
#define OMAP4_LDOSRAMIVA_RETMODE_VSET_OUT_MASK		(0x1f << 16)
#define OMAP4_LDOSRAMIVA_ACTMODE_MUX_CTRL_SHIFT		10
#define OMAP4_LDOSRAMIVA_ACTMODE_MUX_CTRL_MASK		(1 << 10)
#define OMAP4_LDOSRAMIVA_ACTMODE_VSET_IN_SHIFT		5
#define OMAP4_LDOSRAMIVA_ACTMODE_VSET_IN_MASK		(0x1f << 5)
#define OMAP4_LDOSRAMIVA_ACTMODE_VSET_OUT_SHIFT		0
#define OMAP4_LDOSRAMIVA_ACTMODE_VSET_OUT_MASK		(0x1f << 0)

/* LDOSRAM_MPU_VOLTAGE_CTRL */
#define OMAP4_LDOSRAMMPU_RETMODE_MUX_CTRL_SHIFT		26
#define OMAP4_LDOSRAMMPU_RETMODE_MUX_CTRL_MASK		(1 << 26)
#define OMAP4_LDOSRAMMPU_RETMODE_VSET_IN_SHIFT		21
#define OMAP4_LDOSRAMMPU_RETMODE_VSET_IN_MASK		(0x1f << 21)
#define OMAP4_LDOSRAMMPU_RETMODE_VSET_OUT_SHIFT		16
#define OMAP4_LDOSRAMMPU_RETMODE_VSET_OUT_MASK		(0x1f << 16)
#define OMAP4_LDOSRAMMPU_ACTMODE_MUX_CTRL_SHIFT		10
#define OMAP4_LDOSRAMMPU_ACTMODE_MUX_CTRL_MASK		(1 << 10)
#define OMAP4_LDOSRAMMPU_ACTMODE_VSET_IN_SHIFT		5
#define OMAP4_LDOSRAMMPU_ACTMODE_VSET_IN_MASK		(0x1f << 5)
#define OMAP4_LDOSRAMMPU_ACTMODE_VSET_OUT_SHIFT		0
#define OMAP4_LDOSRAMMPU_ACTMODE_VSET_OUT_MASK		(0x1f << 0)

/* LDOSRAM_CORE_VOLTAGE_CTRL */
#define OMAP4_LDOSRAMCORE_RETMODE_MUX_CTRL_SHIFT	26
#define OMAP4_LDOSRAMCORE_RETMODE_MUX_CTRL_MASK		(1 << 26)
#define OMAP4_LDOSRAMCORE_RETMODE_VSET_IN_SHIFT		21
#define OMAP4_LDOSRAMCORE_RETMODE_VSET_IN_MASK		(0x1f << 21)
#define OMAP4_LDOSRAMCORE_RETMODE_VSET_OUT_SHIFT	16
#define OMAP4_LDOSRAMCORE_RETMODE_VSET_OUT_MASK		(0x1f << 16)
#define OMAP4_LDOSRAMCORE_ACTMODE_MUX_CTRL_SHIFT	10
#define OMAP4_LDOSRAMCORE_ACTMODE_MUX_CTRL_MASK		(1 << 10)
#define OMAP4_LDOSRAMCORE_ACTMODE_VSET_IN_SHIFT		5
#define OMAP4_LDOSRAMCORE_ACTMODE_VSET_IN_MASK		(0x1f << 5)
#define OMAP4_LDOSRAMCORE_ACTMODE_VSET_OUT_SHIFT	0
#define OMAP4_LDOSRAMCORE_ACTMODE_VSET_OUT_MASK		(0x1f << 0)

/* TEMP_SENSOR */
#define OMAP4_BGAP_TEMPSOFF_SHIFT			12
#define OMAP4_BGAP_TEMPSOFF_MASK			(1 << 12)
#define OMAP4_BGAP_TSHUT_SHIFT				11
#define OMAP4_BGAP_TSHUT_MASK				(1 << 11)
#define OMAP4_BGAP_TEMP_SENSOR_CONTCONV_SHIFT		10
#define OMAP4_BGAP_TEMP_SENSOR_CONTCONV_MASK		(1 << 10)
#define OMAP4_BGAP_TEMP_SENSOR_SOC_SHIFT		9
#define OMAP4_BGAP_TEMP_SENSOR_SOC_MASK			(1 << 9)
#define OMAP4_BGAP_TEMP_SENSOR_EOCZ_SHIFT		8
#define OMAP4_BGAP_TEMP_SENSOR_EOCZ_MASK		(1 << 8)
#define OMAP4_BGAP_TEMP_SENSOR_DTEMP_SHIFT		0
#define OMAP4_BGAP_TEMP_SENSOR_DTEMP_MASK		(0xff << 0)

/* DPLL_NWELL_TRIM_0 */
#define OMAP4_DPLL_ABE_NWELL_TRIM_MUX_CTRL_SHIFT	29
#define OMAP4_DPLL_ABE_NWELL_TRIM_MUX_CTRL_MASK		(1 << 29)
#define OMAP4_DPLL_ABE_NWELL_TRIM_SHIFT			24
#define OMAP4_DPLL_ABE_NWELL_TRIM_MASK			(0x1f << 24)
#define OMAP4_DPLL_PER_NWELL_TRIM_MUX_CTRL_SHIFT	23
#define OMAP4_DPLL_PER_NWELL_TRIM_MUX_CTRL_MASK		(1 << 23)
#define OMAP4_DPLL_PER_NWELL_TRIM_SHIFT			18
#define OMAP4_DPLL_PER_NWELL_TRIM_MASK			(0x1f << 18)
#define OMAP4_DPLL_CORE_NWELL_TRIM_MUX_CTRL_SHIFT	17
#define OMAP4_DPLL_CORE_NWELL_TRIM_MUX_CTRL_MASK	(1 << 17)
#define OMAP4_DPLL_CORE_NWELL_TRIM_SHIFT		12
#define OMAP4_DPLL_CORE_NWELL_TRIM_MASK			(0x1f << 12)
#define OMAP4_DPLL_IVA_NWELL_TRIM_MUX_CTRL_SHIFT	11
#define OMAP4_DPLL_IVA_NWELL_TRIM_MUX_CTRL_MASK		(1 << 11)
#define OMAP4_DPLL_IVA_NWELL_TRIM_SHIFT			6
#define OMAP4_DPLL_IVA_NWELL_TRIM_MASK			(0x1f << 6)
#define OMAP4_DPLL_MPU_NWELL_TRIM_MUX_CTRL_SHIFT	5
#define OMAP4_DPLL_MPU_NWELL_TRIM_MUX_CTRL_MASK		(1 << 5)
#define OMAP4_DPLL_MPU_NWELL_TRIM_SHIFT			0
#define OMAP4_DPLL_MPU_NWELL_TRIM_MASK			(0x1f << 0)

/* DPLL_NWELL_TRIM_1 */
#define OMAP4_DPLL_UNIPRO_NWELL_TRIM_MUX_CTRL_SHIFT	29
#define OMAP4_DPLL_UNIPRO_NWELL_TRIM_MUX_CTRL_MASK	(1 << 29)
#define OMAP4_DPLL_UNIPRO_NWELL_TRIM_SHIFT		24
#define OMAP4_DPLL_UNIPRO_NWELL_TRIM_MASK		(0x1f << 24)
#define OMAP4_DPLL_USB_NWELL_TRIM_MUX_CTRL_SHIFT	23
#define OMAP4_DPLL_USB_NWELL_TRIM_MUX_CTRL_MASK		(1 << 23)
#define OMAP4_DPLL_USB_NWELL_TRIM_SHIFT			18
#define OMAP4_DPLL_USB_NWELL_TRIM_MASK			(0x1f << 18)
#define OMAP4_DPLL_HDMI_NWELL_TRIM_MUX_CTRL_SHIFT	17
#define OMAP4_DPLL_HDMI_NWELL_TRIM_MUX_CTRL_MASK	(1 << 17)
#define OMAP4_DPLL_HDMI_NWELL_TRIM_SHIFT		12
#define OMAP4_DPLL_HDMI_NWELL_TRIM_MASK			(0x1f << 12)
#define OMAP4_DPLL_DSI2_NWELL_TRIM_MUX_CTRL_SHIFT	11
#define OMAP4_DPLL_DSI2_NWELL_TRIM_MUX_CTRL_MASK	(1 << 11)
#define OMAP4_DPLL_DSI2_NWELL_TRIM_SHIFT		6
#define OMAP4_DPLL_DSI2_NWELL_TRIM_MASK			(0x1f << 6)
#define OMAP4_DPLL_DSI1_NWELL_TRIM_MUX_CTRL_SHIFT	5
#define OMAP4_DPLL_DSI1_NWELL_TRIM_MUX_CTRL_MASK	(1 << 5)
#define OMAP4_DPLL_DSI1_NWELL_TRIM_SHIFT		0
#define OMAP4_DPLL_DSI1_NWELL_TRIM_MASK			(0x1f << 0)

/* USBOTGHS_CONTROL */
#define OMAP4_DISCHRGVBUS_SHIFT				8
#define OMAP4_DISCHRGVBUS_MASK				(1 << 8)
#define OMAP4_CHRGVBUS_SHIFT				7
#define OMAP4_CHRGVBUS_MASK				(1 << 7)
#define OMAP4_DRVVBUS_SHIFT				6
#define OMAP4_DRVVBUS_MASK				(1 << 6)
#define OMAP4_IDPULLUP_SHIFT				5
#define OMAP4_IDPULLUP_MASK				(1 << 5)
#define OMAP4_IDDIG_SHIFT				4
#define OMAP4_IDDIG_MASK				(1 << 4)
#define OMAP4_SESSEND_SHIFT				3
#define OMAP4_SESSEND_MASK				(1 << 3)
#define OMAP4_VBUSVALID_SHIFT				2
#define OMAP4_VBUSVALID_MASK				(1 << 2)
#define OMAP4_BVALID_SHIFT				1
#define OMAP4_BVALID_MASK				(1 << 1)
#define OMAP4_AVALID_SHIFT				0
#define OMAP4_AVALID_MASK				(1 << 0)

/* DSS_CONTROL */
#define OMAP4_DSS_MUX6_SELECT_SHIFT			0
#define OMAP4_DSS_MUX6_SELECT_MASK			(1 << 0)

/* HWOBS_CONTROL */
#define OMAP4_HWOBS_CLKDIV_SEL_SHIFT			3
#define OMAP4_HWOBS_CLKDIV_SEL_MASK			(0x1f << 3)
#define OMAP4_HWOBS_ALL_ZERO_MODE_SHIFT			2
#define OMAP4_HWOBS_ALL_ZERO_MODE_MASK			(1 << 2)
#define OMAP4_HWOBS_ALL_ONE_MODE_SHIFT			1
#define OMAP4_HWOBS_ALL_ONE_MODE_MASK			(1 << 1)
#define OMAP4_HWOBS_MACRO_ENABLE_SHIFT			0
#define OMAP4_HWOBS_MACRO_ENABLE_MASK			(1 << 0)

/* DEBOBS_FINAL_MUX_SEL */
#define OMAP4_SELECT_SHIFT				0
#define OMAP4_SELECT_MASK				(0xffffffff << 0)

/* DEBOBS_MMR_MPU */
#define OMAP4_SELECT_DEBOBS_MMR_MPU_SHIFT		0
#define OMAP4_SELECT_DEBOBS_MMR_MPU_MASK		(0xf << 0)

/* CONF_SDMA_REQ_SEL0 */
#define OMAP4_MULT_SHIFT				0
#define OMAP4_MULT_MASK					(0x7f << 0)

/* CONF_CLK_SEL0 */
#define OMAP4_MULT_CONF_CLK_SEL0_SHIFT			0
#define OMAP4_MULT_CONF_CLK_SEL0_MASK			(0x7 << 0)

/* CONF_CLK_SEL1 */
#define OMAP4_MULT_CONF_CLK_SEL1_SHIFT			0
#define OMAP4_MULT_CONF_CLK_SEL1_MASK			(0x7 << 0)

/* CONF_CLK_SEL2 */
#define OMAP4_MULT_CONF_CLK_SEL2_SHIFT			0
#define OMAP4_MULT_CONF_CLK_SEL2_MASK			(0x7 << 0)

/* CONF_DPLL_FREQLOCK_SEL */
#define OMAP4_MULT_CONF_DPLL_FREQLOCK_SEL_SHIFT		0
#define OMAP4_MULT_CONF_DPLL_FREQLOCK_SEL_MASK		(0x7 << 0)

/* CONF_DPLL_TINITZ_SEL */
#define OMAP4_MULT_CONF_DPLL_TINITZ_SEL_SHIFT		0
#define OMAP4_MULT_CONF_DPLL_TINITZ_SEL_MASK		(0x7 << 0)

/* CONF_DPLL_PHASELOCK_SEL */
#define OMAP4_MULT_CONF_DPLL_PHASELOCK_SEL_SHIFT	0
#define OMAP4_MULT_CONF_DPLL_PHASELOCK_SEL_MASK		(0x7 << 0)

/* CONF_DEBUG_SEL_TST_0 */
#define OMAP4_MODE_SHIFT				0
#define OMAP4_MODE_MASK					(0xf << 0)

#endif
