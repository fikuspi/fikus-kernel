/*
 * TI Palma series PMIC's GPIO driver.
 *
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * Author: Laxman Dewangan <ldewangan@nvidia.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fikus/gpio.h>
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/mfd/palmas.h>
#include <fikus/of.h>
#include <fikus/of_device.h>
#include <fikus/platform_device.h>

struct palmas_gpio {
	struct gpio_chip gpio_chip;
	struct palmas *palmas;
};

static inline struct palmas_gpio *to_palmas_gpio(struct gpio_chip *chip)
{
	return container_of(chip, struct palmas_gpio, gpio_chip);
}

static int palmas_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct palmas_gpio *pg = to_palmas_gpio(gc);
	struct palmas *palmas = pg->palmas;
	unsigned int val;
	int ret;

	ret = palmas_read(palmas, PALMAS_GPIO_BASE, PALMAS_GPIO_DATA_DIR, &val);
	if (ret < 0) {
		dev_err(gc->dev, "GPIO_DATA_DIR read failed, err = %d\n", ret);
		return ret;
	}

	if (val & (1 << offset)) {
		ret = palmas_read(palmas, PALMAS_GPIO_BASE,
				  PALMAS_GPIO_DATA_OUT, &val);
	} else {
		ret = palmas_read(palmas, PALMAS_GPIO_BASE,
				  PALMAS_GPIO_DATA_IN, &val);
	}
	if (ret < 0) {
		dev_err(gc->dev, "GPIO_DATA_IN/OUT read failed, err = %d\n",
			ret);
		return ret;
	}
	return !!(val & BIT(offset));
}

static void palmas_gpio_set(struct gpio_chip *gc, unsigned offset,
			int value)
{
	struct palmas_gpio *pg = to_palmas_gpio(gc);
	struct palmas *palmas = pg->palmas;
	int ret;

	if (value)
		ret = palmas_write(palmas, PALMAS_GPIO_BASE,
				PALMAS_GPIO_SET_DATA_OUT, BIT(offset));
	else
		ret = palmas_write(palmas, PALMAS_GPIO_BASE,
				PALMAS_GPIO_CLEAR_DATA_OUT, BIT(offset));
	if (ret < 0)
		dev_err(gc->dev, "%s write failed, err = %d\n",
			(value) ? "GPIO_SET_DATA_OUT" : "GPIO_CLEAR_DATA_OUT",
			ret);
}

static int palmas_gpio_output(struct gpio_chip *gc, unsigned offset,
				int value)
{
	struct palmas_gpio *pg = to_palmas_gpio(gc);
	struct palmas *palmas = pg->palmas;
	int ret;

	/* Set the initial value */
	palmas_gpio_set(gc, offset, value);

	ret = palmas_update_bits(palmas, PALMAS_GPIO_BASE,
		PALMAS_GPIO_DATA_DIR, BIT(offset), BIT(offset));
	if (ret < 0)
		dev_err(gc->dev, "GPIO_DATA_DIR write failed, err = %d\n", ret);
	return ret;
}

static int palmas_gpio_input(struct gpio_chip *gc, unsigned offset)
{
	struct palmas_gpio *pg = to_palmas_gpio(gc);
	struct palmas *palmas = pg->palmas;
	int ret;

	ret = palmas_update_bits(palmas, PALMAS_GPIO_BASE,
		PALMAS_GPIO_DATA_DIR, BIT(offset), 0);
	if (ret < 0)
		dev_err(gc->dev, "GPIO_DATA_DIR write failed, err = %d\n", ret);
	return ret;
}

static int palmas_gpio_to_irq(struct gpio_chip *gc, unsigned offset)
{
	struct palmas_gpio *pg = to_palmas_gpio(gc);
	struct palmas *palmas = pg->palmas;

	return palmas_irq_get_virq(palmas, PALMAS_GPIO_0_IRQ + offset);
}

static int palmas_gpio_probe(struct platform_device *pdev)
{
	struct palmas *palmas = dev_get_drvdata(pdev->dev.parent);
	struct palmas_platform_data *palmas_pdata;
	struct palmas_gpio *palmas_gpio;
	int ret;

	palmas_gpio = devm_kzalloc(&pdev->dev,
				sizeof(*palmas_gpio), GFP_KERNEL);
	if (!palmas_gpio) {
		dev_err(&pdev->dev, "Could not allocate palmas_gpio\n");
		return -ENOMEM;
	}

	palmas_gpio->palmas = palmas;
	palmas_gpio->gpio_chip.owner = THIS_MODULE;
	palmas_gpio->gpio_chip.label = dev_name(&pdev->dev);
	palmas_gpio->gpio_chip.ngpio = 8;
	palmas_gpio->gpio_chip.can_sleep = 1;
	palmas_gpio->gpio_chip.direction_input = palmas_gpio_input;
	palmas_gpio->gpio_chip.direction_output = palmas_gpio_output;
	palmas_gpio->gpio_chip.to_irq = palmas_gpio_to_irq;
	palmas_gpio->gpio_chip.set	= palmas_gpio_set;
	palmas_gpio->gpio_chip.get	= palmas_gpio_get;
	palmas_gpio->gpio_chip.dev = &pdev->dev;
#ifdef CONFIG_OF_GPIO
	palmas_gpio->gpio_chip.of_node = pdev->dev.of_node;
#endif
	palmas_pdata = dev_get_platdata(palmas->dev);
	if (palmas_pdata && palmas_pdata->gpio_base)
		palmas_gpio->gpio_chip.base = palmas_pdata->gpio_base;
	else
		palmas_gpio->gpio_chip.base = -1;

	ret = gpiochip_add(&palmas_gpio->gpio_chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "Could not register gpiochip, %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, palmas_gpio);
	return ret;
}

static int palmas_gpio_remove(struct platform_device *pdev)
{
	struct palmas_gpio *palmas_gpio = platform_get_drvdata(pdev);

	return gpiochip_remove(&palmas_gpio->gpio_chip);
}

static struct of_device_id of_palmas_gpio_match[] = {
	{ .compatible = "ti,palmas-gpio"},
	{ .compatible = "ti,tps65913-gpio"},
	{ .compatible = "ti,tps65914-gpio"},
	{ .compatible = "ti,tps80036-gpio"},
	{ },
};
MODULE_DEVICE_TABLE(of, of_palmas_gpio_match);

static struct platform_driver palmas_gpio_driver = {
	.driver.name	= "palmas-gpio",
	.driver.owner	= THIS_MODULE,
	.driver.of_match_table = of_palmas_gpio_match,
	.probe		= palmas_gpio_probe,
	.remove		= palmas_gpio_remove,
};

static int __init palmas_gpio_init(void)
{
	return platform_driver_register(&palmas_gpio_driver);
}
subsys_initcall(palmas_gpio_init);

static void __exit palmas_gpio_exit(void)
{
	platform_driver_unregister(&palmas_gpio_driver);
}
module_exit(palmas_gpio_exit);

MODULE_ALIAS("platform:palmas-gpio");
MODULE_AUTHOR("Laxman Dewangan <ldewangan@nvidia.com>");
MODULE_DESCRIPTION("GPIO driver for TI Palmas series PMICs");
MODULE_LICENSE("GPL v2");
