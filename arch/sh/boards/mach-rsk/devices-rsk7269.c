/*
 * RSK+SH7269 Support
 *
 * Copyright (C) 2012  Renesas Electronics Europe Ltd
 * Copyright (C) 2012  Phil Edworthy
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <fikus/init.h>
#include <fikus/types.h>
#include <fikus/platform_device.h>
#include <fikus/interrupt.h>
#include <fikus/input.h>
#include <fikus/smsc911x.h>
#include <fikus/gpio.h>
#include <asm/machvec.h>
#include <asm/io.h>

static struct smsc911x_platform_config smsc911x_config = {
	.phy_interface	= PHY_INTERFACE_MODE_MII,
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_16BIT | SMSC911X_SWAP_FIFO,
};

static struct resource smsc911x_resources[] = {
	[0] = {
		.start		= 0x24000000,
		.end		= 0x240000ff,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= 85,
		.end		= 85,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device smsc911x_device = {
	.name		= "smsc911x",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smsc911x_resources),
	.resource	= smsc911x_resources,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
};

static struct platform_device *rsk7269_devices[] __initdata = {
	&smsc911x_device,
};

static int __init rsk7269_devices_setup(void)
{
	return platform_add_devices(rsk7269_devices,
				    ARRAY_SIZE(rsk7269_devices));
}
device_initcall(rsk7269_devices_setup);
