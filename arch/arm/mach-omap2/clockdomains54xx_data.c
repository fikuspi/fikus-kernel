/*
 * OMAP54XX Clock domains framework
 *
 * Copyright (C) 2013 Texas Instruments, Inc.
 *
 * Abhijit Pagare (abhijitpagare@ti.com)
 * Benoit Cousson (b-cousson@ti.com)
 * Paul Walmsley (paul@pwsan.com)
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

#include <fikus/kernel.h>
#include <fikus/io.h>

#include "clockdomain.h"
#include "cm1_54xx.h"
#include "cm2_54xx.h"

#include "cm-regbits-54xx.h"
#include "prm54xx.h"
#include "prcm44xx.h"
#include "prcm_mpu54xx.h"

/* Static Dependencies for OMAP4 Clock Domains */

static struct clkdm_dep c2c_wkup_sleep_deps[] = {
	{ .clkdm_name = "abe_clkdm" },
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l3init_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ .clkdm_name = "l3main2_clkdm" },
	{ .clkdm_name = "l4cfg_clkdm" },
	{ .clkdm_name = "l4per_clkdm" },
	{ NULL },
};

static struct clkdm_dep cam_wkup_sleep_deps[] = {
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ NULL },
};

static struct clkdm_dep dma_wkup_sleep_deps[] = {
	{ .clkdm_name = "abe_clkdm" },
	{ .clkdm_name = "dss_clkdm" },
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "ipu_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l3init_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ .clkdm_name = "l4cfg_clkdm" },
	{ .clkdm_name = "l4per_clkdm" },
	{ .clkdm_name = "l4sec_clkdm" },
	{ .clkdm_name = "wkupaon_clkdm" },
	{ NULL },
};

static struct clkdm_dep dsp_wkup_sleep_deps[] = {
	{ .clkdm_name = "abe_clkdm" },
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l3init_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ .clkdm_name = "l3main2_clkdm" },
	{ .clkdm_name = "l4cfg_clkdm" },
	{ .clkdm_name = "l4per_clkdm" },
	{ .clkdm_name = "wkupaon_clkdm" },
	{ NULL },
};

static struct clkdm_dep dss_wkup_sleep_deps[] = {
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l3main2_clkdm" },
	{ NULL },
};

static struct clkdm_dep gpu_wkup_sleep_deps[] = {
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ NULL },
};

static struct clkdm_dep ipu_wkup_sleep_deps[] = {
	{ .clkdm_name = "abe_clkdm" },
	{ .clkdm_name = "dsp_clkdm" },
	{ .clkdm_name = "dss_clkdm" },
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "gpu_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l3init_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ .clkdm_name = "l3main2_clkdm" },
	{ .clkdm_name = "l4cfg_clkdm" },
	{ .clkdm_name = "l4per_clkdm" },
	{ .clkdm_name = "l4sec_clkdm" },
	{ .clkdm_name = "wkupaon_clkdm" },
	{ NULL },
};

static struct clkdm_dep iva_wkup_sleep_deps[] = {
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ NULL },
};

static struct clkdm_dep l3init_wkup_sleep_deps[] = {
	{ .clkdm_name = "abe_clkdm" },
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l4cfg_clkdm" },
	{ .clkdm_name = "l4per_clkdm" },
	{ .clkdm_name = "l4sec_clkdm" },
	{ .clkdm_name = "wkupaon_clkdm" },
	{ NULL },
};

static struct clkdm_dep l4sec_wkup_sleep_deps[] = {
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ .clkdm_name = "l4per_clkdm" },
	{ NULL },
};

static struct clkdm_dep mipiext_wkup_sleep_deps[] = {
	{ .clkdm_name = "abe_clkdm" },
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l3init_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ .clkdm_name = "l3main2_clkdm" },
	{ .clkdm_name = "l4cfg_clkdm" },
	{ .clkdm_name = "l4per_clkdm" },
	{ NULL },
};

static struct clkdm_dep mpu_wkup_sleep_deps[] = {
	{ .clkdm_name = "abe_clkdm" },
	{ .clkdm_name = "dsp_clkdm" },
	{ .clkdm_name = "dss_clkdm" },
	{ .clkdm_name = "emif_clkdm" },
	{ .clkdm_name = "gpu_clkdm" },
	{ .clkdm_name = "ipu_clkdm" },
	{ .clkdm_name = "iva_clkdm" },
	{ .clkdm_name = "l3init_clkdm" },
	{ .clkdm_name = "l3main1_clkdm" },
	{ .clkdm_name = "l3main2_clkdm" },
	{ .clkdm_name = "l4cfg_clkdm" },
	{ .clkdm_name = "l4per_clkdm" },
	{ .clkdm_name = "l4sec_clkdm" },
	{ .clkdm_name = "wkupaon_clkdm" },
	{ NULL },
};

static struct clockdomain l4sec_54xx_clkdm = {
	.name		  = "l4sec_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_L4SEC_CDOFFS,
	.dep_bit	  = OMAP54XX_L4SEC_STATDEP_SHIFT,
	.wkdep_srcs	  = l4sec_wkup_sleep_deps,
	.sleepdep_srcs	  = l4sec_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain iva_54xx_clkdm = {
	.name		  = "iva_clkdm",
	.pwrdm		  = { .name = "iva_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_IVA_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_IVA_IVA_CDOFFS,
	.dep_bit	  = OMAP54XX_IVA_STATDEP_SHIFT,
	.wkdep_srcs	  = iva_wkup_sleep_deps,
	.sleepdep_srcs	  = iva_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain mipiext_54xx_clkdm = {
	.name		  = "mipiext_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_MIPIEXT_CDOFFS,
	.wkdep_srcs	  = mipiext_wkup_sleep_deps,
	.sleepdep_srcs	  = mipiext_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain l3main2_54xx_clkdm = {
	.name		  = "l3main2_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_L3MAIN2_CDOFFS,
	.dep_bit	  = OMAP54XX_L3MAIN2_STATDEP_SHIFT,
	.flags		  = CLKDM_CAN_HWSUP,
};

static struct clockdomain l3main1_54xx_clkdm = {
	.name		  = "l3main1_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_L3MAIN1_CDOFFS,
	.dep_bit	  = OMAP54XX_L3MAIN1_STATDEP_SHIFT,
	.flags		  = CLKDM_CAN_HWSUP,
};

static struct clockdomain custefuse_54xx_clkdm = {
	.name		  = "custefuse_clkdm",
	.pwrdm		  = { .name = "custefuse_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CUSTEFUSE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CUSTEFUSE_CUSTEFUSE_CDOFFS,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain ipu_54xx_clkdm = {
	.name		  = "ipu_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_IPU_CDOFFS,
	.dep_bit	  = OMAP54XX_IPU_STATDEP_SHIFT,
	.wkdep_srcs	  = ipu_wkup_sleep_deps,
	.sleepdep_srcs	  = ipu_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain l4cfg_54xx_clkdm = {
	.name		  = "l4cfg_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_L4CFG_CDOFFS,
	.dep_bit	  = OMAP54XX_L4CFG_STATDEP_SHIFT,
	.flags		  = CLKDM_CAN_HWSUP,
};

static struct clockdomain abe_54xx_clkdm = {
	.name		  = "abe_clkdm",
	.pwrdm		  = { .name = "abe_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_AON_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_AON_ABE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_AON_ABE_ABE_CDOFFS,
	.dep_bit	  = OMAP54XX_ABE_STATDEP_SHIFT,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain dss_54xx_clkdm = {
	.name		  = "dss_clkdm",
	.pwrdm		  = { .name = "dss_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_DSS_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_DSS_DSS_CDOFFS,
	.dep_bit	  = OMAP54XX_DSS_STATDEP_SHIFT,
	.wkdep_srcs	  = dss_wkup_sleep_deps,
	.sleepdep_srcs	  = dss_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain dsp_54xx_clkdm = {
	.name		  = "dsp_clkdm",
	.pwrdm		  = { .name = "dsp_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_AON_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_AON_DSP_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_AON_DSP_DSP_CDOFFS,
	.dep_bit	  = OMAP54XX_DSP_STATDEP_SHIFT,
	.wkdep_srcs	  = dsp_wkup_sleep_deps,
	.sleepdep_srcs	  = dsp_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain c2c_54xx_clkdm = {
	.name		  = "c2c_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_C2C_CDOFFS,
	.wkdep_srcs	  = c2c_wkup_sleep_deps,
	.sleepdep_srcs	  = c2c_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain l4per_54xx_clkdm = {
	.name		  = "l4per_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_L4PER_CDOFFS,
	.dep_bit	  = OMAP54XX_L4PER_STATDEP_SHIFT,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain gpu_54xx_clkdm = {
	.name		  = "gpu_clkdm",
	.pwrdm		  = { .name = "gpu_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_GPU_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_GPU_GPU_CDOFFS,
	.dep_bit	  = OMAP54XX_GPU_STATDEP_SHIFT,
	.wkdep_srcs	  = gpu_wkup_sleep_deps,
	.sleepdep_srcs	  = gpu_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain wkupaon_54xx_clkdm = {
	.name		  = "wkupaon_clkdm",
	.pwrdm		  = { .name = "wkupaon_pwrdm" },
	.prcm_partition	  = OMAP54XX_PRM_PARTITION,
	.cm_inst	  = OMAP54XX_PRM_WKUPAON_CM_INST,
	.clkdm_offs	  = OMAP54XX_PRM_WKUPAON_CM_WKUPAON_CDOFFS,
	.dep_bit	  = OMAP54XX_WKUPAON_STATDEP_SHIFT,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain mpu0_54xx_clkdm = {
	.name		  = "mpu0_clkdm",
	.pwrdm		  = { .name = "cpu0_pwrdm" },
	.prcm_partition	  = OMAP54XX_PRCM_MPU_PARTITION,
	.cm_inst	  = OMAP54XX_PRCM_MPU_CM_C0_INST,
	.clkdm_offs	  = OMAP54XX_PRCM_MPU_CM_C0_CPU0_CDOFFS,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain mpu1_54xx_clkdm = {
	.name		  = "mpu1_clkdm",
	.pwrdm		  = { .name = "cpu1_pwrdm" },
	.prcm_partition	  = OMAP54XX_PRCM_MPU_PARTITION,
	.cm_inst	  = OMAP54XX_PRCM_MPU_CM_C1_INST,
	.clkdm_offs	  = OMAP54XX_PRCM_MPU_CM_C1_CPU1_CDOFFS,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain coreaon_54xx_clkdm = {
	.name		  = "coreaon_clkdm",
	.pwrdm		  = { .name = "coreaon_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_COREAON_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_COREAON_COREAON_CDOFFS,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain mpu_54xx_clkdm = {
	.name		  = "mpu_clkdm",
	.pwrdm		  = { .name = "mpu_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_AON_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_AON_MPU_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_AON_MPU_MPU_CDOFFS,
	.wkdep_srcs	  = mpu_wkup_sleep_deps,
	.sleepdep_srcs	  = mpu_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain l3init_54xx_clkdm = {
	.name		  = "l3init_clkdm",
	.pwrdm		  = { .name = "l3init_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_L3INIT_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_L3INIT_L3INIT_CDOFFS,
	.dep_bit	  = OMAP54XX_L3INIT_STATDEP_SHIFT,
	.wkdep_srcs	  = l3init_wkup_sleep_deps,
	.sleepdep_srcs	  = l3init_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

static struct clockdomain dma_54xx_clkdm = {
	.name		  = "dma_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_DMA_CDOFFS,
	.wkdep_srcs	  = dma_wkup_sleep_deps,
	.sleepdep_srcs	  = dma_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain l3instr_54xx_clkdm = {
	.name		  = "l3instr_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_L3INSTR_CDOFFS,
};

static struct clockdomain emif_54xx_clkdm = {
	.name		  = "emif_clkdm",
	.pwrdm		  = { .name = "core_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CORE_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CORE_EMIF_CDOFFS,
	.dep_bit	  = OMAP54XX_EMIF_STATDEP_SHIFT,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain emu_54xx_clkdm = {
	.name		  = "emu_clkdm",
	.pwrdm		  = { .name = "emu_pwrdm" },
	.prcm_partition	  = OMAP54XX_PRM_PARTITION,
	.cm_inst	  = OMAP54XX_PRM_EMU_CM_INST,
	.clkdm_offs	  = OMAP54XX_PRM_EMU_CM_EMU_CDOFFS,
	.flags		  = CLKDM_CAN_FORCE_WAKEUP | CLKDM_CAN_HWSUP,
};

static struct clockdomain cam_54xx_clkdm = {
	.name		  = "cam_clkdm",
	.pwrdm		  = { .name = "cam_pwrdm" },
	.prcm_partition	  = OMAP54XX_CM_CORE_PARTITION,
	.cm_inst	  = OMAP54XX_CM_CORE_CAM_INST,
	.clkdm_offs	  = OMAP54XX_CM_CORE_CAM_CAM_CDOFFS,
	.wkdep_srcs	  = cam_wkup_sleep_deps,
	.sleepdep_srcs	  = cam_wkup_sleep_deps,
	.flags		  = CLKDM_CAN_HWSUP_SWSUP,
};

/* As clockdomains are added or removed above, this list must also be changed */
static struct clockdomain *clockdomains_omap54xx[] __initdata = {
	&l4sec_54xx_clkdm,
	&iva_54xx_clkdm,
	&mipiext_54xx_clkdm,
	&l3main2_54xx_clkdm,
	&l3main1_54xx_clkdm,
	&custefuse_54xx_clkdm,
	&ipu_54xx_clkdm,
	&l4cfg_54xx_clkdm,
	&abe_54xx_clkdm,
	&dss_54xx_clkdm,
	&dsp_54xx_clkdm,
	&c2c_54xx_clkdm,
	&l4per_54xx_clkdm,
	&gpu_54xx_clkdm,
	&wkupaon_54xx_clkdm,
	&mpu0_54xx_clkdm,
	&mpu1_54xx_clkdm,
	&coreaon_54xx_clkdm,
	&mpu_54xx_clkdm,
	&l3init_54xx_clkdm,
	&dma_54xx_clkdm,
	&l3instr_54xx_clkdm,
	&emif_54xx_clkdm,
	&emu_54xx_clkdm,
	&cam_54xx_clkdm,
	NULL
};

void __init omap54xx_clockdomains_init(void)
{
	clkdm_register_platform_funcs(&omap4_clkdm_operations);
	clkdm_register_clkdms(clockdomains_omap54xx);
	clkdm_complete_init();
}
