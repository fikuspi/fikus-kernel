/*
 * Renesas Technology Europe RSK+ Support.
 *
 * Copyright (C) 2008 Paul Mundt
 * Copyright (C) 2008 Peter Griffin <pgriffin@mpc-data.co.uk>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <fikus/init.h>
#include <fikus/types.h>
#include <fikus/platform_device.h>
#include <fikus/interrupt.h>
#include <fikus/mtd/mtd.h>
#include <fikus/mtd/partitions.h>
#include <fikus/mtd/physmap.h>
#include <fikus/mtd/map.h>
#include <fikus/regulator/fixed.h>
#include <fikus/regulator/machine.h>
#include <asm/machvec.h>
#include <asm/io.h>

/* Dummy supplies, where voltage doesn't matter */
static struct regulator_consumer_supply dummy_supplies[] = {
	REGULATOR_SUPPLY("vddvario", "smsc911x"),
	REGULATOR_SUPPLY("vdd33a", "smsc911x"),
};

static const char *part_probes[] = { "cmdlinepart", NULL };

static struct mtd_partition rsk_partitions[] = {
	{
		.name		= "Bootloader",
		.offset		= 0x00000000,
		.size		= 0x00040000,
		.mask_flags	= MTD_WRITEABLE,
	}, {
		.name		= "Kernel",
		.offset		= MTDPART_OFS_NXTBLK,
		.size		= 0x001c0000,
	}, {
		.name		= "Flash_FS",
		.offset		= MTDPART_OFS_NXTBLK,
		.size		= MTDPART_SIZ_FULL,
	}
};

static struct physmap_flash_data flash_data = {
	.parts			= rsk_partitions,
	.nr_parts		= ARRAY_SIZE(rsk_partitions),
	.width			= 2,
	.part_probe_types	= part_probes,
};

static struct resource flash_resource = {
	.start		= 0x20000000,
	.end		= 0x20400000,
	.flags		= IORESOURCE_MEM,
};

static struct platform_device flash_device = {
	.name		= "physmap-flash",
	.id		= -1,
	.resource	= &flash_resource,
	.num_resources	= 1,
	.dev		= {
		.platform_data = &flash_data,
	},
};

static struct platform_device *rsk_devices[] __initdata = {
	&flash_device,
};

static int __init rsk_devices_setup(void)
{
	regulator_register_fixed(0, dummy_supplies, ARRAY_SIZE(dummy_supplies));

	return platform_add_devices(rsk_devices,
				    ARRAY_SIZE(rsk_devices));
}
device_initcall(rsk_devices_setup);

/*
 * The Machine Vector
 */
static struct sh_machine_vector mv_rsk __initmv = {
	.mv_name        = "RSK+",
};
