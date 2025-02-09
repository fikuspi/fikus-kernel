/*
 * Copyright 2010 Tilera Corporation. All Rights Reserved.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#include <fikus/stddef.h>
#include <fikus/reboot.h>
#include <fikus/smp.h>
#include <fikus/pm.h>
#include <fikus/export.h>
#include <asm/page.h>
#include <asm/setup.h>
#include <hv/hypervisor.h>

#ifndef CONFIG_SMP
#define smp_send_stop()
#endif

void machine_halt(void)
{
	arch_local_irq_disable_all();
	smp_send_stop();
	hv_halt();
}

void machine_power_off(void)
{
	arch_local_irq_disable_all();
	smp_send_stop();
	hv_power_off();
}

void machine_restart(char *cmd)
{
	arch_local_irq_disable_all();
	smp_send_stop();
	hv_restart((HV_VirtAddr) "vmfikus", (HV_VirtAddr) cmd);
}

/* No interesting distinction to be made here. */
void (*pm_power_off)(void) = NULL;
EXPORT_SYMBOL(pm_power_off);
