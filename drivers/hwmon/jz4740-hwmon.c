/*
 * Copyright (C) 2009-2010, Lars-Peter Clausen <lars@metafoo.de>
 * JZ4740 SoC HWMON driver
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <fikus/err.h>
#include <fikus/interrupt.h>
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/mutex.h>
#include <fikus/platform_device.h>
#include <fikus/slab.h>
#include <fikus/io.h>

#include <fikus/completion.h>
#include <fikus/mfd/core.h>

#include <fikus/hwmon.h>

struct jz4740_hwmon {
	struct resource *mem;
	void __iomem *base;

	int irq;

	const struct mfd_cell *cell;
	struct device *hwmon;

	struct completion read_completion;

	struct mutex lock;
};

static ssize_t jz4740_hwmon_show_name(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	return sprintf(buf, "jz4740\n");
}

static irqreturn_t jz4740_hwmon_irq(int irq, void *data)
{
	struct jz4740_hwmon *hwmon = data;

	complete(&hwmon->read_completion);
	return IRQ_HANDLED;
}

static ssize_t jz4740_hwmon_read_adcin(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	struct jz4740_hwmon *hwmon = dev_get_drvdata(dev);
	struct completion *completion = &hwmon->read_completion;
	long t;
	unsigned long val;
	int ret;

	mutex_lock(&hwmon->lock);

	INIT_COMPLETION(*completion);

	enable_irq(hwmon->irq);
	hwmon->cell->enable(to_platform_device(dev));

	t = wait_for_completion_interruptible_timeout(completion, HZ);

	if (t > 0) {
		val = readw(hwmon->base) & 0xfff;
		val = (val * 3300) >> 12;
		ret = sprintf(buf, "%lu\n", val);
	} else {
		ret = t ? t : -ETIMEDOUT;
	}

	hwmon->cell->disable(to_platform_device(dev));
	disable_irq(hwmon->irq);

	mutex_unlock(&hwmon->lock);

	return ret;
}

static DEVICE_ATTR(name, S_IRUGO, jz4740_hwmon_show_name, NULL);
static DEVICE_ATTR(in0_input, S_IRUGO, jz4740_hwmon_read_adcin, NULL);

static struct attribute *jz4740_hwmon_attributes[] = {
	&dev_attr_name.attr,
	&dev_attr_in0_input.attr,
	NULL
};

static const struct attribute_group jz4740_hwmon_attr_group = {
	.attrs = jz4740_hwmon_attributes,
};

static int jz4740_hwmon_probe(struct platform_device *pdev)
{
	int ret;
	struct jz4740_hwmon *hwmon;

	hwmon = devm_kzalloc(&pdev->dev, sizeof(*hwmon), GFP_KERNEL);
	if (!hwmon)
		return -ENOMEM;

	hwmon->cell = mfd_get_cell(pdev);

	hwmon->irq = platform_get_irq(pdev, 0);
	if (hwmon->irq < 0) {
		dev_err(&pdev->dev, "Failed to get platform irq: %d\n",
			hwmon->irq);
		return hwmon->irq;
	}

	hwmon->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!hwmon->mem) {
		dev_err(&pdev->dev, "Failed to get platform mmio resource\n");
		return -ENOENT;
	}

	hwmon->mem = devm_request_mem_region(&pdev->dev, hwmon->mem->start,
			resource_size(hwmon->mem), pdev->name);
	if (!hwmon->mem) {
		dev_err(&pdev->dev, "Failed to request mmio memory region\n");
		return -EBUSY;
	}

	hwmon->base = devm_ioremap_nocache(&pdev->dev, hwmon->mem->start,
					   resource_size(hwmon->mem));
	if (!hwmon->base) {
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		return -EBUSY;
	}

	init_completion(&hwmon->read_completion);
	mutex_init(&hwmon->lock);

	platform_set_drvdata(pdev, hwmon);

	ret = devm_request_irq(&pdev->dev, hwmon->irq, jz4740_hwmon_irq, 0,
			       pdev->name, hwmon);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq: %d\n", ret);
		return ret;
	}
	disable_irq(hwmon->irq);

	ret = sysfs_create_group(&pdev->dev.kobj, &jz4740_hwmon_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create sysfs group: %d\n", ret);
		return ret;
	}

	hwmon->hwmon = hwmon_device_register(&pdev->dev);
	if (IS_ERR(hwmon->hwmon)) {
		ret = PTR_ERR(hwmon->hwmon);
		goto err_remove_file;
	}

	return 0;

err_remove_file:
	sysfs_remove_group(&pdev->dev.kobj, &jz4740_hwmon_attr_group);
	return ret;
}

static int jz4740_hwmon_remove(struct platform_device *pdev)
{
	struct jz4740_hwmon *hwmon = platform_get_drvdata(pdev);

	hwmon_device_unregister(hwmon->hwmon);
	sysfs_remove_group(&pdev->dev.kobj, &jz4740_hwmon_attr_group);

	return 0;
}

static struct platform_driver jz4740_hwmon_driver = {
	.probe	= jz4740_hwmon_probe,
	.remove = jz4740_hwmon_remove,
	.driver = {
		.name = "jz4740-hwmon",
		.owner = THIS_MODULE,
	},
};

module_platform_driver(jz4740_hwmon_driver);

MODULE_DESCRIPTION("JZ4740 SoC HWMON driver");
MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:jz4740-hwmon");
