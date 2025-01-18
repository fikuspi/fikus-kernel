/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Rabin Vincent <rabin.vincent@stericsson.com> for ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 */

#include <fikus/kernel.h>
#include <fikus/platform_device.h>
#include <fikus/interrupt.h>
#include <fikus/io.h>
#include <fikus/amba/bus.h>

#include "setup.h"

#include "db8500-regs.h"

void __init amba_add_devices(struct amba_device *devs[], int num)
{
	int i;

	for (i = 0; i < num; i++) {
		struct amba_device *d = devs[i];
		amba_device_register(d, &iomem_resource);
	}
}
