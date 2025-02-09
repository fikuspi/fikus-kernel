/*
 * OMAP4 PRM instance functions
 *
 * Copyright (C) 2009 Nokia Corporation
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/kernel.h>
#include <fikus/types.h>
#include <fikus/errno.h>
#include <fikus/err.h>
#include <fikus/io.h>

#include "iomap.h"
#include "common.h"
#include "prcm-common.h"
#include "prm44xx.h"
#include "prm54xx.h"
#include "prm7xx.h"
#include "prminst44xx.h"
#include "prm-regbits-44xx.h"
#include "prcm44xx.h"
#include "prcm_mpu44xx.h"
#include "soc.h"

static void __iomem *_prm_bases[OMAP4_MAX_PRCM_PARTITIONS];

/**
 * omap_prm_base_init - Populates the prm partitions
 *
 * Populates the base addresses of the _prm_bases
 * array used for read/write of prm module registers.
 */
void omap_prm_base_init(void)
{
	_prm_bases[OMAP4430_PRM_PARTITION] = prm_base;
	_prm_bases[OMAP4430_PRCM_MPU_PARTITION] = prcm_mpu_base;
}

/* Read a register in a PRM instance */
u32 omap4_prminst_read_inst_reg(u8 part, s16 inst, u16 idx)
{
	BUG_ON(part >= OMAP4_MAX_PRCM_PARTITIONS ||
	       part == OMAP4430_INVALID_PRCM_PARTITION ||
	       !_prm_bases[part]);
	return __raw_readl(_prm_bases[part] + inst + idx);
}

/* Write into a register in a PRM instance */
void omap4_prminst_write_inst_reg(u32 val, u8 part, s16 inst, u16 idx)
{
	BUG_ON(part >= OMAP4_MAX_PRCM_PARTITIONS ||
	       part == OMAP4430_INVALID_PRCM_PARTITION ||
	       !_prm_bases[part]);
	__raw_writel(val, _prm_bases[part] + inst + idx);
}

/* Read-modify-write a register in PRM. Caller must lock */
u32 omap4_prminst_rmw_inst_reg_bits(u32 mask, u32 bits, u8 part, s16 inst,
				    u16 idx)
{
	u32 v;

	v = omap4_prminst_read_inst_reg(part, inst, idx);
	v &= ~mask;
	v |= bits;
	omap4_prminst_write_inst_reg(v, part, inst, idx);

	return v;
}

/*
 * Address offset (in bytes) between the reset control and the reset
 * status registers: 4 bytes on OMAP4
 */
#define OMAP4_RST_CTRL_ST_OFFSET		4

/**
 * omap4_prminst_is_hardreset_asserted - read the HW reset line state of
 * submodules contained in the hwmod module
 * @rstctrl_reg: RM_RSTCTRL register address for this module
 * @shift: register bit shift corresponding to the reset line to check
 *
 * Returns 1 if the (sub)module hardreset line is currently asserted,
 * 0 if the (sub)module hardreset line is not currently asserted, or
 * -EINVAL upon parameter error.
 */
int omap4_prminst_is_hardreset_asserted(u8 shift, u8 part, s16 inst,
					u16 rstctrl_offs)
{
	u32 v;

	v = omap4_prminst_read_inst_reg(part, inst, rstctrl_offs);
	v &= 1 << shift;
	v >>= shift;

	return v;
}

/**
 * omap4_prminst_assert_hardreset - assert the HW reset line of a submodule
 * @rstctrl_reg: RM_RSTCTRL register address for this module
 * @shift: register bit shift corresponding to the reset line to assert
 *
 * Some IPs like dsp, ipu or iva contain processors that require an HW
 * reset line to be asserted / deasserted in order to fully enable the
 * IP.  These modules may have multiple hard-reset lines that reset
 * different 'submodules' inside the IP block.  This function will
 * place the submodule into reset.  Returns 0 upon success or -EINVAL
 * upon an argument error.
 */
int omap4_prminst_assert_hardreset(u8 shift, u8 part, s16 inst,
				   u16 rstctrl_offs)
{
	u32 mask = 1 << shift;

	omap4_prminst_rmw_inst_reg_bits(mask, mask, part, inst, rstctrl_offs);

	return 0;
}

/**
 * omap4_prminst_deassert_hardreset - deassert a submodule hardreset line and
 * wait
 * @rstctrl_reg: RM_RSTCTRL register address for this module
 * @shift: register bit shift corresponding to the reset line to deassert
 *
 * Some IPs like dsp, ipu or iva contain processors that require an HW
 * reset line to be asserted / deasserted in order to fully enable the
 * IP.  These modules may have multiple hard-reset lines that reset
 * different 'submodules' inside the IP block.  This function will
 * take the submodule out of reset and wait until the PRCM indicates
 * that the reset has completed before returning.  Returns 0 upon success or
 * -EINVAL upon an argument error, -EEXIST if the submodule was already out
 * of reset, or -EBUSY if the submodule did not exit reset promptly.
 */
int omap4_prminst_deassert_hardreset(u8 shift, u8 part, s16 inst,
				     u16 rstctrl_offs)
{
	int c;
	u32 mask = 1 << shift;
	u16 rstst_offs = rstctrl_offs + OMAP4_RST_CTRL_ST_OFFSET;

	/* Check the current status to avoid de-asserting the line twice */
	if (omap4_prminst_is_hardreset_asserted(shift, part, inst,
						rstctrl_offs) == 0)
		return -EEXIST;

	/* Clear the reset status by writing 1 to the status bit */
	omap4_prminst_rmw_inst_reg_bits(0xffffffff, mask, part, inst,
					rstst_offs);
	/* de-assert the reset control line */
	omap4_prminst_rmw_inst_reg_bits(mask, 0, part, inst, rstctrl_offs);
	/* wait the status to be set */
	omap_test_timeout(omap4_prminst_is_hardreset_asserted(shift, part, inst,
							      rstst_offs),
			  MAX_MODULE_HARDRESET_WAIT, c);

	return (c == MAX_MODULE_HARDRESET_WAIT) ? -EBUSY : 0;
}


void omap4_prminst_global_warm_sw_reset(void)
{
	u32 v;
	s16 dev_inst;

	if (cpu_is_omap44xx())
		dev_inst = OMAP4430_PRM_DEVICE_INST;
	else if (soc_is_omap54xx())
		dev_inst = OMAP54XX_PRM_DEVICE_INST;
	else if (soc_is_dra7xx())
		dev_inst = DRA7XX_PRM_DEVICE_INST;
	else
		return;

	v = omap4_prminst_read_inst_reg(OMAP4430_PRM_PARTITION, dev_inst,
					OMAP4_PRM_RSTCTRL_OFFSET);
	v |= OMAP4430_RST_GLOBAL_WARM_SW_MASK;
	omap4_prminst_write_inst_reg(v, OMAP4430_PRM_PARTITION,
				 OMAP4430_PRM_DEVICE_INST,
				 OMAP4_PRM_RSTCTRL_OFFSET);

	/* OCP barrier */
	v = omap4_prminst_read_inst_reg(OMAP4430_PRM_PARTITION,
				    OMAP4430_PRM_DEVICE_INST,
				    OMAP4_PRM_RSTCTRL_OFFSET);
}
