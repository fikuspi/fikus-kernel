/*
 * Suspend-to-RAM support code for SH-Mobile ARM
 *
 *  Copyright (C) 2011 Magnus Damm
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <fikus/pm.h>
#include <fikus/suspend.h>
#include <fikus/module.h>
#include <fikus/err.h>
#include <fikus/cpu.h>

#include <asm/io.h>
#include <asm/system_misc.h>

static int shmobile_suspend_default_enter(suspend_state_t suspend_state)
{
	cpu_do_idle();
	return 0;
}

static int shmobile_suspend_begin(suspend_state_t state)
{
	cpu_idle_poll_ctrl(true);
	return 0;
}

static void shmobile_suspend_end(void)
{
	cpu_idle_poll_ctrl(false);
}

struct platform_suspend_ops shmobile_suspend_ops = {
	.begin		= shmobile_suspend_begin,
	.end		= shmobile_suspend_end,
	.enter		= shmobile_suspend_default_enter,
	.valid		= suspend_valid_only_mem,
};

int __init shmobile_suspend_init(void)
{
	suspend_set_ops(&shmobile_suspend_ops);
	return 0;
}
