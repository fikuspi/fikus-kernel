/*
 * fam15h_power.c - AMD Family 15h processor power monitoring
 *
 * Copyright (c) 2011 Advanced Micro Devices, Inc.
 * Author: Andreas Herrmann <herrmann.der.user@googlemail.com>
 *
 *
 * This driver is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This driver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this driver; if not, see <http://www.gnu.org/licenses/>.
 */

#include <fikus/err.h>
#include <fikus/hwmon.h>
#include <fikus/hwmon-sysfs.h>
#include <fikus/init.h>
#include <fikus/module.h>
#include <fikus/pci.h>
#include <fikus/bitops.h>
#include <asm/processor.h>

MODULE_DESCRIPTION("AMD Family 15h CPU processor power monitor");
MODULE_AUTHOR("Andreas Herrmann <herrmann.der.user@googlemail.com>");
MODULE_LICENSE("GPL");

/* Family 16h Northbridge's function 4 PCI ID */
#define PCI_DEVICE_ID_AMD_16H_NB_F4	0x1534

/* D18F3 */
#define REG_NORTHBRIDGE_CAP		0xe8

/* D18F4 */
#define REG_PROCESSOR_TDP		0x1b8

/* D18F5 */
#define REG_TDP_RUNNING_AVERAGE		0xe0
#define REG_TDP_LIMIT3			0xe8

struct fam15h_power_data {
	struct device *hwmon_dev;
	unsigned int tdp_to_watts;
	unsigned int base_tdp;
	unsigned int processor_pwr_watts;
};

static ssize_t show_power(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	u32 val, tdp_limit, running_avg_range;
	s32 running_avg_capture;
	u64 curr_pwr_watts;
	struct pci_dev *f4 = to_pci_dev(dev);
	struct fam15h_power_data *data = dev_get_drvdata(dev);

	pci_bus_read_config_dword(f4->bus, PCI_DEVFN(PCI_SLOT(f4->devfn), 5),
				  REG_TDP_RUNNING_AVERAGE, &val);
	running_avg_capture = (val >> 4) & 0x3fffff;
	running_avg_capture = sign_extend32(running_avg_capture, 21);
	running_avg_range = (val & 0xf) + 1;

	pci_bus_read_config_dword(f4->bus, PCI_DEVFN(PCI_SLOT(f4->devfn), 5),
				  REG_TDP_LIMIT3, &val);

	tdp_limit = val >> 16;
	curr_pwr_watts = ((u64)(tdp_limit +
				data->base_tdp)) << running_avg_range;
	curr_pwr_watts -= running_avg_capture;
	curr_pwr_watts *= data->tdp_to_watts;

	/*
	 * Convert to microWatt
	 *
	 * power is in Watt provided as fixed point integer with
	 * scaling factor 1/(2^16).  For conversion we use
	 * (10^6)/(2^16) = 15625/(2^10)
	 */
	curr_pwr_watts = (curr_pwr_watts * 15625) >> (10 + running_avg_range);
	return sprintf(buf, "%u\n", (unsigned int) curr_pwr_watts);
}
static DEVICE_ATTR(power1_input, S_IRUGO, show_power, NULL);

static ssize_t show_power_crit(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct fam15h_power_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", data->processor_pwr_watts);
}
static DEVICE_ATTR(power1_crit, S_IRUGO, show_power_crit, NULL);

static ssize_t show_name(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "fam15h_power\n");
}
static DEVICE_ATTR(name, S_IRUGO, show_name, NULL);

static struct attribute *fam15h_power_attrs[] = {
	&dev_attr_power1_input.attr,
	&dev_attr_power1_crit.attr,
	&dev_attr_name.attr,
	NULL
};

static const struct attribute_group fam15h_power_attr_group = {
	.attrs	= fam15h_power_attrs,
};

static bool fam15h_power_is_internal_node0(struct pci_dev *f4)
{
	u32 val;

	pci_bus_read_config_dword(f4->bus, PCI_DEVFN(PCI_SLOT(f4->devfn), 3),
				  REG_NORTHBRIDGE_CAP, &val);
	if ((val & BIT(29)) && ((val >> 30) & 3))
		return false;

	return true;
}

/*
 * Newer BKDG versions have an updated recommendation on how to properly
 * initialize the running average range (was: 0xE, now: 0x9). This avoids
 * counter saturations resulting in bogus power readings.
 * We correct this value ourselves to cope with older BIOSes.
 */
static const struct pci_device_id affected_device[] = {
	{ PCI_VDEVICE(AMD, PCI_DEVICE_ID_AMD_15H_NB_F4) },
	{ 0 }
};

static void tweak_runavg_range(struct pci_dev *pdev)
{
	u32 val;

	/*
	 * let this quirk apply only to the current version of the
	 * northbridge, since future versions may change the behavior
	 */
	if (!pci_match_id(affected_device, pdev))
		return;

	pci_bus_read_config_dword(pdev->bus,
		PCI_DEVFN(PCI_SLOT(pdev->devfn), 5),
		REG_TDP_RUNNING_AVERAGE, &val);
	if ((val & 0xf) != 0xe)
		return;

	val &= ~0xf;
	val |=  0x9;
	pci_bus_write_config_dword(pdev->bus,
		PCI_DEVFN(PCI_SLOT(pdev->devfn), 5),
		REG_TDP_RUNNING_AVERAGE, val);
}

#ifdef CONFIG_PM
static int fam15h_power_resume(struct pci_dev *pdev)
{
	tweak_runavg_range(pdev);
	return 0;
}
#else
#define fam15h_power_resume NULL
#endif

static void fam15h_power_init_data(struct pci_dev *f4,
					     struct fam15h_power_data *data)
{
	u32 val;
	u64 tmp;

	pci_read_config_dword(f4, REG_PROCESSOR_TDP, &val);
	data->base_tdp = val >> 16;
	tmp = val & 0xffff;

	pci_bus_read_config_dword(f4->bus, PCI_DEVFN(PCI_SLOT(f4->devfn), 5),
				  REG_TDP_LIMIT3, &val);

	data->tdp_to_watts = ((val & 0x3ff) << 6) | ((val >> 10) & 0x3f);
	tmp *= data->tdp_to_watts;

	/* result not allowed to be >= 256W */
	if ((tmp >> 16) >= 256)
		dev_warn(&f4->dev,
			 "Bogus value for ProcessorPwrWatts (processor_pwr_watts>=%u)\n",
			 (unsigned int) (tmp >> 16));

	/* convert to microWatt */
	data->processor_pwr_watts = (tmp * 15625) >> 10;
}

static int fam15h_power_probe(struct pci_dev *pdev,
					const struct pci_device_id *id)
{
	struct fam15h_power_data *data;
	struct device *dev = &pdev->dev;
	int err;

	/*
	 * though we ignore every other northbridge, we still have to
	 * do the tweaking on _each_ node in MCM processors as the counters
	 * are working hand-in-hand
	 */
	tweak_runavg_range(pdev);

	if (!fam15h_power_is_internal_node0(pdev))
		return -ENODEV;

	data = devm_kzalloc(dev, sizeof(struct fam15h_power_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	fam15h_power_init_data(pdev, data);

	dev_set_drvdata(dev, data);
	err = sysfs_create_group(&dev->kobj, &fam15h_power_attr_group);
	if (err)
		return err;

	data->hwmon_dev = hwmon_device_register(dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove_group;
	}

	return 0;

exit_remove_group:
	sysfs_remove_group(&dev->kobj, &fam15h_power_attr_group);
	return err;
}

static void fam15h_power_remove(struct pci_dev *pdev)
{
	struct device *dev;
	struct fam15h_power_data *data;

	dev = &pdev->dev;
	data = dev_get_drvdata(dev);
	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&dev->kobj, &fam15h_power_attr_group);
}

static DEFINE_PCI_DEVICE_TABLE(fam15h_power_id_table) = {
	{ PCI_VDEVICE(AMD, PCI_DEVICE_ID_AMD_15H_NB_F4) },
	{ PCI_VDEVICE(AMD, PCI_DEVICE_ID_AMD_16H_NB_F4) },
	{}
};
MODULE_DEVICE_TABLE(pci, fam15h_power_id_table);

static struct pci_driver fam15h_power_driver = {
	.name = "fam15h_power",
	.id_table = fam15h_power_id_table,
	.probe = fam15h_power_probe,
	.remove = fam15h_power_remove,
	.resume = fam15h_power_resume,
};

module_pci_driver(fam15h_power_driver);
