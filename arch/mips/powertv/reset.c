/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 * Portions copyright (C) 2009 Cisco Systems, Inc.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 */
#include <fikus/pm.h>

#include <fikus/io.h>
#include <asm/reboot.h>			/* Not included by fikus/reboot.h */

#include <asm/mach-powertv/asic_regs.h>
#include "reset.h"

static void mips_machine_restart(char *command)
{
	writel(0x1, asic_reg_addr(watchdog));
}

void mips_reboot_setup(void)
{
	_machine_restart = mips_machine_restart;
}
