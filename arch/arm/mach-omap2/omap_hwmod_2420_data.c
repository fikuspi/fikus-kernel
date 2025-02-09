/*
 * omap_hwmod_2420_data.c - hardware modules present on the OMAP2420 chips
 *
 * Copyright (C) 2009-2011 Nokia Corporation
 * Copyright (C) 2012 Texas Instruments, Inc.
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * XXX handle crossbar/shared link difference for L3?
 * XXX these should be marked initdata for multi-OMAP kernels
 */

#include <fikus/i2c-omap.h>
#include <fikus/platform_data/spi-omap2-mcspi.h>
#include <fikus/omap-dma.h>
#include <fikus/platform_data/mailbox-omap.h>
#include <plat/dmtimer.h>

#include "omap_hwmod.h"
#include "l3_2xxx.h"
#include "l4_2xxx.h"

#include "omap_hwmod_common_data.h"

#include "cm-regbits-24xx.h"
#include "prm-regbits-24xx.h"
#include "i2c.h"
#include "mmc.h"
#include "serial.h"
#include "wd_timer.h"

/*
 * OMAP2420 hardware module integration data
 *
 * All of the data in this section should be autogeneratable from the
 * TI hardware database or other technical documentation.  Data that
 * is driver-specific or driver-kernel integration-specific belongs
 * elsewhere.
 */

/*
 * IP blocks
 */

/* IVA1 (IVA1) */
static struct omap_hwmod_class iva1_hwmod_class = {
	.name		= "iva1",
};

static struct omap_hwmod_rst_info omap2420_iva_resets[] = {
	{ .name = "iva", .rst_shift = 8 },
};

static struct omap_hwmod omap2420_iva_hwmod = {
	.name		= "iva",
	.class		= &iva1_hwmod_class,
	.clkdm_name	= "iva1_clkdm",
	.rst_lines	= omap2420_iva_resets,
	.rst_lines_cnt	= ARRAY_SIZE(omap2420_iva_resets),
	.main_clk	= "iva1_ifck",
};

/* DSP */
static struct omap_hwmod_class dsp_hwmod_class = {
	.name		= "dsp",
};

static struct omap_hwmod_rst_info omap2420_dsp_resets[] = {
	{ .name = "logic", .rst_shift = 0 },
	{ .name = "mmu", .rst_shift = 1 },
};

static struct omap_hwmod omap2420_dsp_hwmod = {
	.name		= "dsp",
	.class		= &dsp_hwmod_class,
	.clkdm_name	= "dsp_clkdm",
	.rst_lines	= omap2420_dsp_resets,
	.rst_lines_cnt	= ARRAY_SIZE(omap2420_dsp_resets),
	.main_clk	= "dsp_fck",
};

/* I2C common */
static struct omap_hwmod_class_sysconfig i2c_sysc = {
	.rev_offs	= 0x00,
	.sysc_offs	= 0x20,
	.syss_offs	= 0x10,
	.sysc_flags	= (SYSC_HAS_SOFTRESET | SYSS_HAS_RESET_STATUS),
	.sysc_fields	= &omap_hwmod_sysc_type1,
};

static struct omap_hwmod_class i2c_class = {
	.name		= "i2c",
	.sysc		= &i2c_sysc,
	.rev		= OMAP_I2C_IP_VERSION_1,
	.reset		= &omap_i2c_reset,
};

static struct omap_i2c_dev_attr i2c_dev_attr = {
	.flags		= OMAP_I2C_FLAG_NO_FIFO |
			  OMAP_I2C_FLAG_SIMPLE_CLOCK |
			  OMAP_I2C_FLAG_16BIT_DATA_REG |
			  OMAP_I2C_FLAG_BUS_SHIFT_2,
};

/* I2C1 */
static struct omap_hwmod omap2420_i2c1_hwmod = {
	.name		= "i2c1",
	.mpu_irqs	= omap2_i2c1_mpu_irqs,
	.sdma_reqs	= omap2_i2c1_sdma_reqs,
	.main_clk	= "i2c1_fck",
	.prcm		= {
		.omap2 = {
			.module_offs = CORE_MOD,
			.prcm_reg_id = 1,
			.module_bit = OMAP2420_EN_I2C1_SHIFT,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP2420_ST_I2C1_SHIFT,
		},
	},
	.class		= &i2c_class,
	.dev_attr	= &i2c_dev_attr,
	/*
	 * From mach-omap2/pm24xx.c: "Putting MPU into the WFI state
	 * while a transfer is active seems to cause the I2C block to
	 * timeout. Why? Good question."
	 */
	.flags		= (HWMOD_16BIT_REG | HWMOD_BLOCK_WFI),
};

/* I2C2 */
static struct omap_hwmod omap2420_i2c2_hwmod = {
	.name		= "i2c2",
	.mpu_irqs	= omap2_i2c2_mpu_irqs,
	.sdma_reqs	= omap2_i2c2_sdma_reqs,
	.main_clk	= "i2c2_fck",
	.prcm		= {
		.omap2 = {
			.module_offs = CORE_MOD,
			.prcm_reg_id = 1,
			.module_bit = OMAP2420_EN_I2C2_SHIFT,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP2420_ST_I2C2_SHIFT,
		},
	},
	.class		= &i2c_class,
	.dev_attr	= &i2c_dev_attr,
	.flags		= HWMOD_16BIT_REG,
};

/* dma attributes */
static struct omap_dma_dev_attr dma_dev_attr = {
	.dev_caps  = RESERVE_CHANNEL | DMA_LINKED_LCH | GLOBAL_PRIORITY |
						IS_CSSA_32 | IS_CDSA_32,
	.lch_count = 32,
};

static struct omap_hwmod omap2420_dma_system_hwmod = {
	.name		= "dma",
	.class		= &omap2xxx_dma_hwmod_class,
	.mpu_irqs	= omap2_dma_system_irqs,
	.main_clk	= "core_l3_ck",
	.dev_attr	= &dma_dev_attr,
	.flags		= HWMOD_NO_IDLEST,
};

/* mailbox */
static struct omap_mbox_dev_info omap2420_mailbox_info[] = {
	{ .name = "dsp", .tx_id = 0, .rx_id = 1, .irq_id = 0, .usr_id = 0 },
	{ .name = "iva", .tx_id = 2, .rx_id = 3, .irq_id = 1, .usr_id = 3 },
};

static struct omap_mbox_pdata omap2420_mailbox_attrs = {
	.num_users	= 4,
	.num_fifos	= 6,
	.info_cnt	= ARRAY_SIZE(omap2420_mailbox_info),
	.info		= omap2420_mailbox_info,
};

static struct omap_hwmod_irq_info omap2420_mailbox_irqs[] = {
	{ .name = "dsp", .irq = 26 + OMAP_INTC_START, },
	{ .name = "iva", .irq = 34 + OMAP_INTC_START, },
	{ .irq = -1 },
};

static struct omap_hwmod omap2420_mailbox_hwmod = {
	.name		= "mailbox",
	.class		= &omap2xxx_mailbox_hwmod_class,
	.mpu_irqs	= omap2420_mailbox_irqs,
	.main_clk	= "mailboxes_ick",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP24XX_EN_MAILBOXES_SHIFT,
			.module_offs = CORE_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP24XX_ST_MAILBOXES_SHIFT,
		},
	},
	.dev_attr	= &omap2420_mailbox_attrs,
};

/*
 * 'mcbsp' class
 * multi channel buffered serial port controller
 */

static struct omap_hwmod_class omap2420_mcbsp_hwmod_class = {
	.name = "mcbsp",
};

static struct omap_hwmod_opt_clk mcbsp_opt_clks[] = {
	{ .role = "pad_fck", .clk = "mcbsp_clks" },
	{ .role = "prcm_fck", .clk = "func_96m_ck" },
};

/* mcbsp1 */
static struct omap_hwmod_irq_info omap2420_mcbsp1_irqs[] = {
	{ .name = "tx", .irq = 59 + OMAP_INTC_START, },
	{ .name = "rx", .irq = 60 + OMAP_INTC_START, },
	{ .irq = -1 },
};

static struct omap_hwmod omap2420_mcbsp1_hwmod = {
	.name		= "mcbsp1",
	.class		= &omap2420_mcbsp_hwmod_class,
	.mpu_irqs	= omap2420_mcbsp1_irqs,
	.sdma_reqs	= omap2_mcbsp1_sdma_reqs,
	.main_clk	= "mcbsp1_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP24XX_EN_MCBSP1_SHIFT,
			.module_offs = CORE_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP24XX_ST_MCBSP1_SHIFT,
		},
	},
	.opt_clks	= mcbsp_opt_clks,
	.opt_clks_cnt	= ARRAY_SIZE(mcbsp_opt_clks),
};

/* mcbsp2 */
static struct omap_hwmod_irq_info omap2420_mcbsp2_irqs[] = {
	{ .name = "tx", .irq = 62 + OMAP_INTC_START, },
	{ .name = "rx", .irq = 63 + OMAP_INTC_START, },
	{ .irq = -1 },
};

static struct omap_hwmod omap2420_mcbsp2_hwmod = {
	.name		= "mcbsp2",
	.class		= &omap2420_mcbsp_hwmod_class,
	.mpu_irqs	= omap2420_mcbsp2_irqs,
	.sdma_reqs	= omap2_mcbsp2_sdma_reqs,
	.main_clk	= "mcbsp2_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP24XX_EN_MCBSP2_SHIFT,
			.module_offs = CORE_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP24XX_ST_MCBSP2_SHIFT,
		},
	},
	.opt_clks	= mcbsp_opt_clks,
	.opt_clks_cnt	= ARRAY_SIZE(mcbsp_opt_clks),
};

static struct omap_hwmod_class_sysconfig omap2420_msdi_sysc = {
	.rev_offs	= 0x3c,
	.sysc_offs	= 0x64,
	.syss_offs	= 0x68,
	.sysc_flags	= (SYSC_HAS_SOFTRESET | SYSS_HAS_RESET_STATUS),
	.sysc_fields	= &omap_hwmod_sysc_type1,
};

static struct omap_hwmod_class omap2420_msdi_hwmod_class = {
	.name	= "msdi",
	.sysc	= &omap2420_msdi_sysc,
	.reset	= &omap_msdi_reset,
};

/* msdi1 */
static struct omap_hwmod_irq_info omap2420_msdi1_irqs[] = {
	{ .irq = 83 + OMAP_INTC_START, },
	{ .irq = -1 },
};

static struct omap_hwmod_dma_info omap2420_msdi1_sdma_reqs[] = {
	{ .name = "tx", .dma_req = 61 }, /* OMAP24XX_DMA_MMC1_TX */
	{ .name = "rx", .dma_req = 62 }, /* OMAP24XX_DMA_MMC1_RX */
	{ .dma_req = -1 }
};

static struct omap_hwmod omap2420_msdi1_hwmod = {
	.name		= "msdi1",
	.class		= &omap2420_msdi_hwmod_class,
	.mpu_irqs	= omap2420_msdi1_irqs,
	.sdma_reqs	= omap2420_msdi1_sdma_reqs,
	.main_clk	= "mmc_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP2420_EN_MMC_SHIFT,
			.module_offs = CORE_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP2420_ST_MMC_SHIFT,
		},
	},
	.flags		= HWMOD_16BIT_REG,
};

/* HDQ1W/1-wire */
static struct omap_hwmod omap2420_hdq1w_hwmod = {
	.name		= "hdq1w",
	.mpu_irqs	= omap2_hdq1w_mpu_irqs,
	.main_clk	= "hdq_fck",
	.prcm		= {
		.omap2 = {
			.module_offs = CORE_MOD,
			.prcm_reg_id = 1,
			.module_bit = OMAP24XX_EN_HDQ_SHIFT,
			.idlest_reg_id = 1,
			.idlest_idle_bit = OMAP24XX_ST_HDQ_SHIFT,
		},
	},
	.class		= &omap2_hdq1w_class,
};

/*
 * interfaces
 */

/* L4 CORE -> I2C1 interface */
static struct omap_hwmod_ocp_if omap2420_l4_core__i2c1 = {
	.master		= &omap2xxx_l4_core_hwmod,
	.slave		= &omap2420_i2c1_hwmod,
	.clk		= "i2c1_ick",
	.addr		= omap2_i2c1_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 CORE -> I2C2 interface */
static struct omap_hwmod_ocp_if omap2420_l4_core__i2c2 = {
	.master		= &omap2xxx_l4_core_hwmod,
	.slave		= &omap2420_i2c2_hwmod,
	.clk		= "i2c2_ick",
	.addr		= omap2_i2c2_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* IVA <- L3 interface */
static struct omap_hwmod_ocp_if omap2420_l3__iva = {
	.master		= &omap2xxx_l3_main_hwmod,
	.slave		= &omap2420_iva_hwmod,
	.clk		= "core_l3_ck",
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* DSP <- L3 interface */
static struct omap_hwmod_ocp_if omap2420_l3__dsp = {
	.master		= &omap2xxx_l3_main_hwmod,
	.slave		= &omap2420_dsp_hwmod,
	.clk		= "dsp_ick",
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

static struct omap_hwmod_addr_space omap2420_timer1_addrs[] = {
	{
		.pa_start	= 0x48028000,
		.pa_end		= 0x48028000 + SZ_1K - 1,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

/* l4_wkup -> timer1 */
static struct omap_hwmod_ocp_if omap2420_l4_wkup__timer1 = {
	.master		= &omap2xxx_l4_wkup_hwmod,
	.slave		= &omap2xxx_timer1_hwmod,
	.clk		= "gpt1_ick",
	.addr		= omap2420_timer1_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_wkup -> wd_timer2 */
static struct omap_hwmod_addr_space omap2420_wd_timer2_addrs[] = {
	{
		.pa_start	= 0x48022000,
		.pa_end		= 0x4802207f,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

static struct omap_hwmod_ocp_if omap2420_l4_wkup__wd_timer2 = {
	.master		= &omap2xxx_l4_wkup_hwmod,
	.slave		= &omap2xxx_wd_timer2_hwmod,
	.clk		= "mpu_wdt_ick",
	.addr		= omap2420_wd_timer2_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_wkup -> gpio1 */
static struct omap_hwmod_addr_space omap2420_gpio1_addr_space[] = {
	{
		.pa_start	= 0x48018000,
		.pa_end		= 0x480181ff,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

static struct omap_hwmod_ocp_if omap2420_l4_wkup__gpio1 = {
	.master		= &omap2xxx_l4_wkup_hwmod,
	.slave		= &omap2xxx_gpio1_hwmod,
	.clk		= "gpios_ick",
	.addr		= omap2420_gpio1_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_wkup -> gpio2 */
static struct omap_hwmod_addr_space omap2420_gpio2_addr_space[] = {
	{
		.pa_start	= 0x4801a000,
		.pa_end		= 0x4801a1ff,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

static struct omap_hwmod_ocp_if omap2420_l4_wkup__gpio2 = {
	.master		= &omap2xxx_l4_wkup_hwmod,
	.slave		= &omap2xxx_gpio2_hwmod,
	.clk		= "gpios_ick",
	.addr		= omap2420_gpio2_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_wkup -> gpio3 */
static struct omap_hwmod_addr_space omap2420_gpio3_addr_space[] = {
	{
		.pa_start	= 0x4801c000,
		.pa_end		= 0x4801c1ff,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

static struct omap_hwmod_ocp_if omap2420_l4_wkup__gpio3 = {
	.master		= &omap2xxx_l4_wkup_hwmod,
	.slave		= &omap2xxx_gpio3_hwmod,
	.clk		= "gpios_ick",
	.addr		= omap2420_gpio3_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_wkup -> gpio4 */
static struct omap_hwmod_addr_space omap2420_gpio4_addr_space[] = {
	{
		.pa_start	= 0x4801e000,
		.pa_end		= 0x4801e1ff,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

static struct omap_hwmod_ocp_if omap2420_l4_wkup__gpio4 = {
	.master		= &omap2xxx_l4_wkup_hwmod,
	.slave		= &omap2xxx_gpio4_hwmod,
	.clk		= "gpios_ick",
	.addr		= omap2420_gpio4_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* dma_system -> L3 */
static struct omap_hwmod_ocp_if omap2420_dma_system__l3 = {
	.master		= &omap2420_dma_system_hwmod,
	.slave		= &omap2xxx_l3_main_hwmod,
	.clk		= "core_l3_ck",
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_core -> dma_system */
static struct omap_hwmod_ocp_if omap2420_l4_core__dma_system = {
	.master		= &omap2xxx_l4_core_hwmod,
	.slave		= &omap2420_dma_system_hwmod,
	.clk		= "sdma_ick",
	.addr		= omap2_dma_system_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_core -> mailbox */
static struct omap_hwmod_ocp_if omap2420_l4_core__mailbox = {
	.master		= &omap2xxx_l4_core_hwmod,
	.slave		= &omap2420_mailbox_hwmod,
	.addr		= omap2_mailbox_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_core -> mcbsp1 */
static struct omap_hwmod_ocp_if omap2420_l4_core__mcbsp1 = {
	.master		= &omap2xxx_l4_core_hwmod,
	.slave		= &omap2420_mcbsp1_hwmod,
	.clk		= "mcbsp1_ick",
	.addr		= omap2_mcbsp1_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_core -> mcbsp2 */
static struct omap_hwmod_ocp_if omap2420_l4_core__mcbsp2 = {
	.master		= &omap2xxx_l4_core_hwmod,
	.slave		= &omap2420_mcbsp2_hwmod,
	.clk		= "mcbsp2_ick",
	.addr		= omap2xxx_mcbsp2_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

static struct omap_hwmod_addr_space omap2420_msdi1_addrs[] = {
	{
		.pa_start	= 0x4809c000,
		.pa_end		= 0x4809c000 + SZ_128 - 1,
		.flags		= ADDR_TYPE_RT,
	},
	{ }
};

/* l4_core -> msdi1 */
static struct omap_hwmod_ocp_if omap2420_l4_core__msdi1 = {
	.master		= &omap2xxx_l4_core_hwmod,
	.slave		= &omap2420_msdi1_hwmod,
	.clk		= "mmc_ick",
	.addr		= omap2420_msdi1_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* l4_core -> hdq1w interface */
static struct omap_hwmod_ocp_if omap2420_l4_core__hdq1w = {
	.master		= &omap2xxx_l4_core_hwmod,
	.slave		= &omap2420_hdq1w_hwmod,
	.clk		= "hdq_ick",
	.addr		= omap2_hdq1w_addr_space,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
	.flags		= OMAP_FIREWALL_L4 | OCPIF_SWSUP_IDLE,
};


/* l4_wkup -> 32ksync_counter */
static struct omap_hwmod_addr_space omap2420_counter_32k_addrs[] = {
	{
		.pa_start	= 0x48004000,
		.pa_end		= 0x4800401f,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

static struct omap_hwmod_addr_space omap2420_gpmc_addrs[] = {
	{
		.pa_start	= 0x6800a000,
		.pa_end		= 0x6800afff,
		.flags		= ADDR_TYPE_RT
	},
	{ }
};

static struct omap_hwmod_ocp_if omap2420_l4_wkup__counter_32k = {
	.master		= &omap2xxx_l4_wkup_hwmod,
	.slave		= &omap2xxx_counter_32k_hwmod,
	.clk		= "sync_32k_ick",
	.addr		= omap2420_counter_32k_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

static struct omap_hwmod_ocp_if omap2420_l3__gpmc = {
	.master		= &omap2xxx_l3_main_hwmod,
	.slave		= &omap2xxx_gpmc_hwmod,
	.clk		= "core_l3_ck",
	.addr		= omap2420_gpmc_addrs,
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

static struct omap_hwmod_ocp_if *omap2420_hwmod_ocp_ifs[] __initdata = {
	&omap2xxx_l3_main__l4_core,
	&omap2xxx_mpu__l3_main,
	&omap2xxx_dss__l3,
	&omap2xxx_l4_core__mcspi1,
	&omap2xxx_l4_core__mcspi2,
	&omap2xxx_l4_core__l4_wkup,
	&omap2_l4_core__uart1,
	&omap2_l4_core__uart2,
	&omap2_l4_core__uart3,
	&omap2420_l4_core__i2c1,
	&omap2420_l4_core__i2c2,
	&omap2420_l3__iva,
	&omap2420_l3__dsp,
	&omap2420_l4_wkup__timer1,
	&omap2xxx_l4_core__timer2,
	&omap2xxx_l4_core__timer3,
	&omap2xxx_l4_core__timer4,
	&omap2xxx_l4_core__timer5,
	&omap2xxx_l4_core__timer6,
	&omap2xxx_l4_core__timer7,
	&omap2xxx_l4_core__timer8,
	&omap2xxx_l4_core__timer9,
	&omap2xxx_l4_core__timer10,
	&omap2xxx_l4_core__timer11,
	&omap2xxx_l4_core__timer12,
	&omap2420_l4_wkup__wd_timer2,
	&omap2xxx_l4_core__dss,
	&omap2xxx_l4_core__dss_dispc,
	&omap2xxx_l4_core__dss_rfbi,
	&omap2xxx_l4_core__dss_venc,
	&omap2420_l4_wkup__gpio1,
	&omap2420_l4_wkup__gpio2,
	&omap2420_l4_wkup__gpio3,
	&omap2420_l4_wkup__gpio4,
	&omap2420_dma_system__l3,
	&omap2420_l4_core__dma_system,
	&omap2420_l4_core__mailbox,
	&omap2420_l4_core__mcbsp1,
	&omap2420_l4_core__mcbsp2,
	&omap2420_l4_core__msdi1,
	&omap2xxx_l4_core__rng,
	&omap2xxx_l4_core__sham,
	&omap2xxx_l4_core__aes,
	&omap2420_l4_core__hdq1w,
	&omap2420_l4_wkup__counter_32k,
	&omap2420_l3__gpmc,
	NULL,
};

int __init omap2420_hwmod_init(void)
{
	omap_hwmod_init();
	return omap_hwmod_register_links(omap2420_hwmod_ocp_ifs);
}
