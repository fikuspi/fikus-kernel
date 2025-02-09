/*
 * Kontron PLD MFD core driver
 *
 * Copyright (c) 2010-2013 Kontron Europe GmbH
 * Author: Michael Brunner <michael.brunner@kontron.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <fikus/platform_device.h>
#include <fikus/mfd/core.h>
#include <fikus/mfd/kempld.h>
#include <fikus/module.h>
#include <fikus/dmi.h>
#include <fikus/io.h>
#include <fikus/delay.h>

#define MAX_ID_LEN 4
static char force_device_id[MAX_ID_LEN + 1] = "";
module_param_string(force_device_id, force_device_id, sizeof(force_device_id), 0);
MODULE_PARM_DESC(force_device_id, "Override detected product");

/*
 * Get hardware mutex to block firmware from accessing the pld.
 * It is possible for the firmware may hold the mutex for an extended length of
 * time. This function will block until access has been granted.
 */
static void kempld_get_hardware_mutex(struct kempld_device_data *pld)
{
	/* The mutex bit will read 1 until access has been granted */
	while (ioread8(pld->io_index) & KEMPLD_MUTEX_KEY)
		msleep(1);
}

static void kempld_release_hardware_mutex(struct kempld_device_data *pld)
{
	/* The harware mutex is released when 1 is written to the mutex bit. */
	iowrite8(KEMPLD_MUTEX_KEY, pld->io_index);
}

static int kempld_get_info_generic(struct kempld_device_data *pld)
{
	u16 version;
	u8 spec;

	kempld_get_mutex(pld);

	version = kempld_read16(pld, KEMPLD_VERSION);
	spec = kempld_read8(pld, KEMPLD_SPEC);
	pld->info.buildnr = kempld_read16(pld, KEMPLD_BUILDNR);

	pld->info.minor = KEMPLD_VERSION_GET_MINOR(version);
	pld->info.major = KEMPLD_VERSION_GET_MAJOR(version);
	pld->info.number = KEMPLD_VERSION_GET_NUMBER(version);
	pld->info.type = KEMPLD_VERSION_GET_TYPE(version);

	if (spec == 0xff) {
		pld->info.spec_minor = 0;
		pld->info.spec_major = 1;
	} else {
		pld->info.spec_minor = KEMPLD_SPEC_GET_MINOR(spec);
		pld->info.spec_major = KEMPLD_SPEC_GET_MAJOR(spec);
	}

	if (pld->info.spec_major > 0)
		pld->feature_mask = kempld_read16(pld, KEMPLD_FEATURE);
	else
		pld->feature_mask = 0;

	kempld_release_mutex(pld);

	return 0;
}

enum kempld_cells {
	KEMPLD_I2C = 0,
	KEMPLD_WDT,
	KEMPLD_GPIO,
	KEMPLD_UART,
};

static struct mfd_cell kempld_devs[] = {
	[KEMPLD_I2C] = {
		.name = "kempld-i2c",
	},
	[KEMPLD_WDT] = {
		.name = "kempld-wdt",
	},
	[KEMPLD_GPIO] = {
		.name = "kempld-gpio",
	},
	[KEMPLD_UART] = {
		.name = "kempld-uart",
	},
};

#define KEMPLD_MAX_DEVS	ARRAY_SIZE(kempld_devs)

static int kempld_register_cells_generic(struct kempld_device_data *pld)
{
	struct mfd_cell devs[KEMPLD_MAX_DEVS];
	int i = 0;

	if (pld->feature_mask & KEMPLD_FEATURE_BIT_I2C)
		devs[i++] = kempld_devs[KEMPLD_I2C];

	if (pld->feature_mask & KEMPLD_FEATURE_BIT_WATCHDOG)
		devs[i++] = kempld_devs[KEMPLD_WDT];

	if (pld->feature_mask & KEMPLD_FEATURE_BIT_GPIO)
		devs[i++] = kempld_devs[KEMPLD_GPIO];

	if (pld->feature_mask & KEMPLD_FEATURE_MASK_UART)
		devs[i++] = kempld_devs[KEMPLD_UART];

	return mfd_add_devices(pld->dev, -1, devs, i, NULL, 0, NULL);
}

static struct resource kempld_ioresource = {
	.start	= KEMPLD_IOINDEX,
	.end	= KEMPLD_IODATA,
	.flags	= IORESOURCE_IO,
};

static const struct kempld_platform_data kempld_platform_data_generic = {
	.pld_clock		= KEMPLD_CLK,
	.ioresource		= &kempld_ioresource,
	.get_hardware_mutex	= kempld_get_hardware_mutex,
	.release_hardware_mutex	= kempld_release_hardware_mutex,
	.get_info		= kempld_get_info_generic,
	.register_cells		= kempld_register_cells_generic,
};

static struct platform_device *kempld_pdev;

static int kempld_create_platform_device(const struct dmi_system_id *id)
{
	struct kempld_platform_data *pdata = id->driver_data;
	int ret;

	kempld_pdev = platform_device_alloc("kempld", -1);
	if (!kempld_pdev)
		return -ENOMEM;

	ret = platform_device_add_data(kempld_pdev, pdata, sizeof(*pdata));
	if (ret)
		goto err;

	ret = platform_device_add_resources(kempld_pdev, pdata->ioresource, 1);
	if (ret)
		goto err;

	ret = platform_device_add(kempld_pdev);
	if (ret)
		goto err;

	return 0;
err:
	platform_device_put(kempld_pdev);
	return ret;
}

/**
 * kempld_read8 - read 8 bit register
 * @pld: kempld_device_data structure describing the PLD
 * @index: register index on the chip
 *
 * kempld_get_mutex must be called prior to calling this function.
 */
u8 kempld_read8(struct kempld_device_data *pld, u8 index)
{
	iowrite8(index, pld->io_index);
	return ioread8(pld->io_data);
}
EXPORT_SYMBOL_GPL(kempld_read8);

/**
 * kempld_write8 - write 8 bit register
 * @pld: kempld_device_data structure describing the PLD
 * @index: register index on the chip
 * @data: new register value
 *
 * kempld_get_mutex must be called prior to calling this function.
 */
void kempld_write8(struct kempld_device_data *pld, u8 index, u8 data)
{
	iowrite8(index, pld->io_index);
	iowrite8(data, pld->io_data);
}
EXPORT_SYMBOL_GPL(kempld_write8);

/**
 * kempld_read16 - read 16 bit register
 * @pld: kempld_device_data structure describing the PLD
 * @index: register index on the chip
 *
 * kempld_get_mutex must be called prior to calling this function.
 */
u16 kempld_read16(struct kempld_device_data *pld, u8 index)
{
	return kempld_read8(pld, index) | kempld_read8(pld, index + 1) << 8;
}
EXPORT_SYMBOL_GPL(kempld_read16);

/**
 * kempld_write16 - write 16 bit register
 * @pld: kempld_device_data structure describing the PLD
 * @index: register index on the chip
 * @data: new register value
 *
 * kempld_get_mutex must be called prior to calling this function.
 */
void kempld_write16(struct kempld_device_data *pld, u8 index, u16 data)
{
	kempld_write8(pld, index, (u8)data);
	kempld_write8(pld, index + 1, (u8)(data >> 8));
}
EXPORT_SYMBOL_GPL(kempld_write16);

/**
 * kempld_read32 - read 32 bit register
 * @pld: kempld_device_data structure describing the PLD
 * @index: register index on the chip
 *
 * kempld_get_mutex must be called prior to calling this function.
 */
u32 kempld_read32(struct kempld_device_data *pld, u8 index)
{
	return kempld_read16(pld, index) | kempld_read16(pld, index + 2) << 16;
}
EXPORT_SYMBOL_GPL(kempld_read32);

/**
 * kempld_write32 - write 32 bit register
 * @pld: kempld_device_data structure describing the PLD
 * @index: register index on the chip
 * @data: new register value
 *
 * kempld_get_mutex must be called prior to calling this function.
 */
void kempld_write32(struct kempld_device_data *pld, u8 index, u32 data)
{
	kempld_write16(pld, index, (u16)data);
	kempld_write16(pld, index + 2, (u16)(data >> 16));
}
EXPORT_SYMBOL_GPL(kempld_write32);

/**
 * kempld_get_mutex - acquire PLD mutex
 * @pld: kempld_device_data structure describing the PLD
 */
void kempld_get_mutex(struct kempld_device_data *pld)
{
	struct kempld_platform_data *pdata = dev_get_platdata(pld->dev);

	mutex_lock(&pld->lock);
	pdata->get_hardware_mutex(pld);
}
EXPORT_SYMBOL_GPL(kempld_get_mutex);

/**
 * kempld_release_mutex - release PLD mutex
 * @pld: kempld_device_data structure describing the PLD
 */
void kempld_release_mutex(struct kempld_device_data *pld)
{
	struct kempld_platform_data *pdata = dev_get_platdata(pld->dev);

	pdata->release_hardware_mutex(pld);
	mutex_unlock(&pld->lock);
}
EXPORT_SYMBOL_GPL(kempld_release_mutex);

/**
 * kempld_get_info - update device specific information
 * @pld: kempld_device_data structure describing the PLD
 *
 * This function calls the configured board specific kempld_get_info_XXXX
 * function which is responsible for gathering information about the specific
 * hardware. The information is then stored within the pld structure.
 */
static int kempld_get_info(struct kempld_device_data *pld)
{
	struct kempld_platform_data *pdata = dev_get_platdata(pld->dev);

	return pdata->get_info(pld);
}

/*
 * kempld_register_cells - register cell drivers
 *
 * This function registers cell drivers for the detected hardware by calling
 * the configured kempld_register_cells_XXXX function which is responsible
 * to detect and register the needed cell drivers.
 */
static int kempld_register_cells(struct kempld_device_data *pld)
{
	struct kempld_platform_data *pdata = dev_get_platdata(pld->dev);

	return pdata->register_cells(pld);
}

static int kempld_detect_device(struct kempld_device_data *pld)
{
	char *version_type;
	u8 index_reg;
	int ret;

	mutex_lock(&pld->lock);

	/* Check for empty IO space */
	index_reg = ioread8(pld->io_index);
	if (index_reg == 0xff && ioread8(pld->io_data) == 0xff) {
		mutex_unlock(&pld->lock);
		return -ENODEV;
	}

	/* Release hardware mutex if aquired */
	if (!(index_reg & KEMPLD_MUTEX_KEY))
		iowrite8(KEMPLD_MUTEX_KEY, pld->io_index);

	mutex_unlock(&pld->lock);

	ret = kempld_get_info(pld);
	if (ret)
		return ret;

	switch (pld->info.type) {
	case 0:
		version_type = "release";
		break;
	case 1:
		version_type = "debug";
		break;
	case 2:
		version_type = "custom";
		break;
	default:
		version_type = "unspecified";
	}

	dev_info(pld->dev, "Found Kontron PLD %d\n", pld->info.number);
	dev_info(pld->dev, "%s version %d.%d build %d, specification %d.%d\n",
		 version_type, pld->info.major, pld->info.minor,
		 pld->info.buildnr, pld->info.spec_major,
		 pld->info.spec_minor);

	return kempld_register_cells(pld);
}

static int kempld_probe(struct platform_device *pdev)
{
	struct kempld_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct device *dev = &pdev->dev;
	struct kempld_device_data *pld;
	struct resource *ioport;
	int ret;

	pld = devm_kzalloc(dev, sizeof(*pld), GFP_KERNEL);
	if (!pld)
		return -ENOMEM;

	ioport = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!ioport)
		return -EINVAL;

	pld->io_base = devm_ioport_map(dev, ioport->start,
					ioport->end - ioport->start);
	if (!pld->io_base)
		return -ENOMEM;

	pld->io_index = pld->io_base;
	pld->io_data = pld->io_base + 1;
	pld->pld_clock = pdata->pld_clock;
	pld->dev = dev;

	mutex_init(&pld->lock);
	platform_set_drvdata(pdev, pld);

	ret = kempld_detect_device(pld);
	if (ret)
		return ret;

	return 0;
}

static int kempld_remove(struct platform_device *pdev)
{
	struct kempld_device_data *pld = platform_get_drvdata(pdev);
	struct kempld_platform_data *pdata = dev_get_platdata(pld->dev);

	mfd_remove_devices(&pdev->dev);
	pdata->release_hardware_mutex(pld);

	return 0;
}

static struct platform_driver kempld_driver = {
	.driver		= {
		.name	= "kempld",
		.owner	= THIS_MODULE,
	},
	.probe		= kempld_probe,
	.remove		= kempld_remove,
};

static struct dmi_system_id __initdata kempld_dmi_table[] = {
	{
		.ident = "BHL6",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-bHL6"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	},
	{
		.ident = "CCR2",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-bIP2"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CCR6",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-bIP6"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CHR2",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "ETXexpress-SC T2"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CHR2",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "ETXe-SC T2"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CHR2",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-bSC2"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CHR6",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "ETXexpress-SC T6"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CHR6",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "ETXe-SC T6"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CHR6",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-bSC6"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CNTG",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "ETXexpress-PC"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CNTG",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-bPC2"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "CNTX",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "PXT"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "FRI2",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BIOS_VERSION, "FRI2"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "FRI2",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "Fish River Island II"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "MBR1",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "ETX-OH"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "NTC1",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "nanoETXexpress-TT"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "NTC1",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "nETXe-TT"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "NTC1",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-mTT"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "NUP1",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-mCT"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "UNP1",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "microETXexpress-DC"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "UNP1",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-cDC2"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "UNTG",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "microETXexpress-PC"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "UNTG",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-cPC2"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	}, {
		.ident = "UUP6",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-cCT6"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	},
	{
		.ident = "UTH6",
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Kontron"),
			DMI_MATCH(DMI_BOARD_NAME, "COMe-cTH6"),
		},
		.driver_data = (void *)&kempld_platform_data_generic,
		.callback = kempld_create_platform_device,
	},
	{}
};
MODULE_DEVICE_TABLE(dmi, kempld_dmi_table);

static int __init kempld_init(void)
{
	const struct dmi_system_id *id;
	int ret;

	if (force_device_id[0]) {
		for (id = kempld_dmi_table; id->matches[0].slot != DMI_NONE; id++)
			if (strstr(id->ident, force_device_id))
				if (id->callback && id->callback(id))
					break;
		if (id->matches[0].slot == DMI_NONE)
			return -ENODEV;
	} else {
		if (!dmi_check_system(kempld_dmi_table))
			return -ENODEV;
	}

	ret = platform_driver_register(&kempld_driver);
	if (ret)
		return ret;

	return 0;
}

static void __exit kempld_exit(void)
{
	if (kempld_pdev)
		platform_device_unregister(kempld_pdev);

	platform_driver_unregister(&kempld_driver);
}

module_init(kempld_init);
module_exit(kempld_exit);

MODULE_DESCRIPTION("KEM PLD Core Driver");
MODULE_AUTHOR("Michael Brunner <michael.brunner@kontron.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:kempld-core");
