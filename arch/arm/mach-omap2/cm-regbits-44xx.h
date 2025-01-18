/*
 * OMAP44xx Clock Management register bits
 *
 * Copyright (C) 2009-2012 Texas Instruments, Inc.
 * Copyright (C) 2009-2010 Nokia Corporation
 *
 * Paul Walmsley (paul@pwsan.com)
 * Rajendra Nayak (rnayak@ti.com)
 * Benoit Cousson (b-cousson@ti.com)
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

#ifndef __ARCH_ARM_MACH_OMAP2_CM_REGBITS_44XX_H
#define __ARCH_ARM_MACH_OMAP2_CM_REGBITS_44XX_H

#define OMAP4430_ABE_STATDEP_SHIFT				3
#define OMAP4430_AUTO_DPLL_MODE_MASK				(0x7 << 0)
#define OMAP4430_CLKSEL_SHIFT					24
#define OMAP4430_CLKSEL_WIDTH					0x1
#define OMAP4430_CLKSEL_MASK					(1 << 24)
#define OMAP4430_CLKSEL_0_0_SHIFT				0
#define OMAP4430_CLKSEL_0_0_WIDTH				0x1
#define OMAP4430_CLKSEL_0_1_SHIFT				0
#define OMAP4430_CLKSEL_0_1_WIDTH				0x2
#define OMAP4430_CLKSEL_24_25_SHIFT				24
#define OMAP4430_CLKSEL_24_25_WIDTH				0x2
#define OMAP4430_CLKSEL_60M_SHIFT				24
#define OMAP4430_CLKSEL_60M_WIDTH				0x1
#define OMAP4430_CLKSEL_AESS_FCLK_SHIFT				24
#define OMAP4430_CLKSEL_AESS_FCLK_WIDTH				0x1
#define OMAP4430_CLKSEL_CORE_SHIFT				0
#define OMAP4430_CLKSEL_CORE_WIDTH				0x1
#define OMAP4430_CLKSEL_DIV_SHIFT				24
#define OMAP4430_CLKSEL_DIV_WIDTH				0x1
#define OMAP4430_CLKSEL_FCLK_SHIFT				24
#define OMAP4430_CLKSEL_FCLK_WIDTH				0x2
#define OMAP4430_CLKSEL_INTERNAL_SOURCE_SHIFT			25
#define OMAP4430_CLKSEL_INTERNAL_SOURCE_WIDTH			0x1
#define OMAP4430_CLKSEL_L3_SHIFT				4
#define OMAP4430_CLKSEL_L3_WIDTH				0x1
#define OMAP4430_CLKSEL_L4_SHIFT				8
#define OMAP4430_CLKSEL_L4_WIDTH				0x1
#define OMAP4430_CLKSEL_OPP_SHIFT				0
#define OMAP4430_CLKSEL_OPP_WIDTH				0x2
#define OMAP4430_CLKSEL_PMD_STM_CLK_SHIFT			27
#define OMAP4430_CLKSEL_PMD_STM_CLK_WIDTH			0x3
#define OMAP4430_CLKSEL_PMD_TRACE_CLK_MASK			(0x7 << 24)
#define OMAP4430_CLKSEL_SGX_FCLK_MASK				(1 << 24)
#define OMAP4430_CLKSEL_SOURCE_MASK				(0x3 << 24)
#define OMAP4430_CLKSEL_SOURCE_24_24_MASK			(1 << 24)
#define OMAP4430_CLKSEL_UTMI_P1_SHIFT				24
#define OMAP4430_CLKSEL_UTMI_P1_WIDTH				0x1
#define OMAP4430_CLKSEL_UTMI_P2_SHIFT				25
#define OMAP4430_CLKSEL_UTMI_P2_WIDTH				0x1
#define OMAP4430_CLKTRCTRL_SHIFT				0
#define OMAP4430_CLKTRCTRL_MASK					(0x3 << 0)
#define OMAP4430_DPLL_BYP_CLKSEL_SHIFT				23
#define OMAP4430_DPLL_BYP_CLKSEL_WIDTH				0x1
#define OMAP4430_DPLL_CLKOUTHIF_DIV_MASK			(0x1f << 0)
#define OMAP4430_DPLL_CLKOUTHIF_GATE_CTRL_SHIFT			8
#define OMAP4430_DPLL_CLKOUTX2_GATE_CTRL_MASK			(1 << 10)
#define OMAP4430_DPLL_CLKOUT_DIV_SHIFT				0
#define OMAP4430_DPLL_CLKOUT_DIV_WIDTH				0x5
#define OMAP4430_DPLL_CLKOUT_DIV_MASK				(0x1f << 0)
#define OMAP4430_DPLL_CLKOUT_DIV_0_6_MASK			(0x7f << 0)
#define OMAP4430_DPLL_CLKOUT_GATE_CTRL_MASK			(1 << 8)
#define OMAP4430_DPLL_DIV_MASK					(0x7f << 0)
#define OMAP4430_DPLL_DIV_0_7_MASK				(0xff << 0)
#define OMAP4430_DPLL_EN_MASK					(0x7 << 0)
#define OMAP4430_DPLL_LPMODE_EN_MASK				(1 << 10)
#define OMAP4430_DPLL_MULT_MASK					(0x7ff << 8)
#define OMAP4430_DPLL_MULT_USB_MASK				(0xfff << 8)
#define OMAP4430_DPLL_REGM4XEN_MASK				(1 << 11)
#define OMAP4430_DPLL_SD_DIV_MASK				(0xff << 24)
#define OMAP4430_DSS_STATDEP_SHIFT				8
#define OMAP4430_DUCATI_STATDEP_SHIFT				0
#define OMAP4430_GFX_STATDEP_SHIFT				10
#define OMAP4430_HSDIVIDER_CLKOUT1_DIV_MASK			(0x1f << 0)
#define OMAP4430_HSDIVIDER_CLKOUT2_DIV_MASK			(0x1f << 0)
#define OMAP4430_HSDIVIDER_CLKOUT3_DIV_MASK			(0x1f << 0)
#define OMAP4430_HSDIVIDER_CLKOUT4_DIV_MASK			(0x1f << 0)
#define OMAP4430_IDLEST_SHIFT					16
#define OMAP4430_IDLEST_MASK					(0x3 << 16)
#define OMAP4430_IVAHD_STATDEP_SHIFT				2
#define OMAP4430_L3INIT_STATDEP_SHIFT				7
#define OMAP4430_L3_1_STATDEP_SHIFT				5
#define OMAP4430_L3_2_STATDEP_SHIFT				6
#define OMAP4430_L4CFG_STATDEP_SHIFT				12
#define OMAP4430_L4PER_STATDEP_SHIFT				13
#define OMAP4430_L4SEC_STATDEP_SHIFT				14
#define OMAP4430_L4WKUP_STATDEP_SHIFT				15
#define OMAP4430_MEMIF_STATDEP_SHIFT				4
#define OMAP4430_MODULEMODE_SHIFT				0
#define OMAP4430_MODULEMODE_MASK				(0x3 << 0)
#define OMAP4430_OPTFCLKEN_48MHZ_CLK_SHIFT			9
#define OMAP4430_OPTFCLKEN_BGAP_32K_SHIFT			8
#define OMAP4430_OPTFCLKEN_CLK32K_SHIFT				8
#define OMAP4430_OPTFCLKEN_CTRLCLK_SHIFT			8
#define OMAP4430_OPTFCLKEN_DBCLK_SHIFT				8
#define OMAP4430_OPTFCLKEN_DSSCLK_SHIFT				8
#define OMAP4430_OPTFCLKEN_FCLK_SHIFT				8
#define OMAP4430_OPTFCLKEN_FCLK0_SHIFT				8
#define OMAP4430_OPTFCLKEN_FCLK1_SHIFT				9
#define OMAP4430_OPTFCLKEN_FCLK2_SHIFT				10
#define OMAP4430_OPTFCLKEN_FUNC48MCLK_SHIFT			15
#define OMAP4430_OPTFCLKEN_HSIC480M_P1_CLK_SHIFT		13
#define OMAP4430_OPTFCLKEN_HSIC480M_P2_CLK_SHIFT		14
#define OMAP4430_OPTFCLKEN_HSIC60M_P1_CLK_SHIFT			11
#define OMAP4430_OPTFCLKEN_HSIC60M_P2_CLK_SHIFT			12
#define OMAP4430_OPTFCLKEN_PER24MC_GFCLK_SHIFT			8
#define OMAP4430_OPTFCLKEN_PERABE24M_GFCLK_SHIFT		9
#define OMAP4430_OPTFCLKEN_PHY_48M_SHIFT			8
#define OMAP4430_OPTFCLKEN_SLIMBUS_CLK_SHIFT			10
#define OMAP4430_OPTFCLKEN_SLIMBUS_CLK_11_11_SHIFT		11
#define OMAP4430_OPTFCLKEN_SYS_CLK_SHIFT			10
#define OMAP4460_OPTFCLKEN_TS_FCLK_SHIFT			8
#define OMAP4430_OPTFCLKEN_TV_CLK_SHIFT				11
#define OMAP4430_OPTFCLKEN_USB_CH0_CLK_SHIFT			8
#define OMAP4430_OPTFCLKEN_USB_CH1_CLK_SHIFT			9
#define OMAP4430_OPTFCLKEN_USB_CH2_CLK_SHIFT			10
#define OMAP4430_OPTFCLKEN_UTMI_P1_CLK_SHIFT			8
#define OMAP4430_OPTFCLKEN_UTMI_P2_CLK_SHIFT			9
#define OMAP4430_OPTFCLKEN_UTMI_P3_CLK_SHIFT			10
#define OMAP4430_OPTFCLKEN_XCLK_SHIFT				8
#define OMAP4430_PAD_CLKS_GATE_SHIFT				8
#define OMAP4430_PMD_STM_MUX_CTRL_SHIFT				20
#define OMAP4430_PMD_STM_MUX_CTRL_WIDTH				0x2
#define OMAP4430_PMD_TRACE_MUX_CTRL_SHIFT			22
#define OMAP4430_PMD_TRACE_MUX_CTRL_WIDTH			0x2
#define OMAP4430_SCALE_FCLK_SHIFT				0
#define OMAP4430_SCALE_FCLK_WIDTH				0x1
#define OMAP4430_SLIMBUS_CLK_GATE_SHIFT				10
#define OMAP4430_ST_DPLL_CLK_MASK				(1 << 0)
#define OMAP4430_SYS_CLKSEL_SHIFT				0
#define OMAP4430_SYS_CLKSEL_WIDTH				0x3
#define OMAP4430_TESLA_STATDEP_SHIFT				1
#endif
