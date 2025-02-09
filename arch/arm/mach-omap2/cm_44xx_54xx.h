/*
 * OMAP44xx and OMAP54xx CM1/CM2 function prototypes
 *
 * Copyright (C) 2009-2013 Texas Instruments, Inc.
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
 *
 */

#ifndef __ARCH_ARM_MACH_OMAP2_CM_44XX_54XX_H
#define __ARCH_ARM_MACH_OMAP2_CM_44XX_55XX_H

/* CM1 Function prototypes */
extern u32 omap4_cm1_read_inst_reg(s16 inst, u16 idx);
extern void omap4_cm1_write_inst_reg(u32 val, s16 inst, u16 idx);
extern u32 omap4_cm1_rmw_inst_reg_bits(u32 mask, u32 bits, s16 inst, s16 idx);

/* CM2 Function prototypes */
extern u32 omap4_cm2_read_inst_reg(s16 inst, u16 idx);
extern void omap4_cm2_write_inst_reg(u32 val, s16 inst, u16 idx);
extern u32 omap4_cm2_rmw_inst_reg_bits(u32 mask, u32 bits, s16 inst, s16 idx);

#endif
