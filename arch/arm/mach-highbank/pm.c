/*
 * Copyright 2011 Calxeda, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fikus/cpu_pm.h>
#include <fikus/init.h>
#include <fikus/io.h>
#include <fikus/suspend.h>

#include <asm/cacheflush.h>
#include <asm/proc-fns.h>
#include <asm/suspend.h>

#include "core.h"
#include "sysregs.h"

static int highbank_suspend_finish(unsigned long val)
{
	outer_flush_all();
	outer_disable();

	highbank_set_pwr_suspend();

	cpu_do_idle();

	highbank_clear_pwr_request();
	return 0;
}

static int highbank_pm_enter(suspend_state_t state)
{
	cpu_pm_enter();
	cpu_cluster_pm_enter();

	highbank_set_cpu_jump(0, cpu_resume);
	cpu_suspend(0, highbank_suspend_finish);

	cpu_cluster_pm_exit();
	cpu_pm_exit();

	highbank_smc1(0x102, 0x1);
	if (scu_base_addr)
		scu_enable(scu_base_addr);
	return 0;
}

static const struct platform_suspend_ops highbank_pm_ops = {
	.enter = highbank_pm_enter,
	.valid = suspend_valid_only_mem,
};

void __init highbank_pm_init(void)
{
	suspend_set_ops(&highbank_pm_ops);
}
