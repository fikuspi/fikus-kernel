/*
 * fikus/arch/arm/mach-omap1/board-nand.c
 *
 * Common OMAP1 board NAND code
 *
 * Copyright (C) 2004, 2012 Texas Instruments, Inc.
 * Copyright (C) 2002 MontaVista Software, Inc.
 * Copyright (C) 2001 RidgeRun, Inc.
 * Author: RidgeRun, Inc.
 *         Greg Lonnon (glonnon@ridgerun.com) or info@ridgerun.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <fikus/kernel.h>
#include <fikus/io.h>
#include <fikus/mtd/mtd.h>
#include <fikus/mtd/nand.h>

#include "common.h"

void omap1_nand_cmd_ctl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *this = mtd->priv;
	unsigned long mask;

	if (cmd == NAND_CMD_NONE)
		return;

	mask = (ctrl & NAND_CLE) ? 0x02 : 0;
	if (ctrl & NAND_ALE)
		mask |= 0x04;

	writeb(cmd, this->IO_ADDR_W + mask);
}

