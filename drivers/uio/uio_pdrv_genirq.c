/*
 * drivers/uio/uio_pdrv_genirq.c
 *
 * Userspace I/O platform driver with generic IRQ handling code.
 *
 * Copyright (C) 2008 Magnus Damm
 *
 * Based on uio_pdrv.c by Uwe Kleine-Koenig,
 * Copyright (C) 2008 by Digi International Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <fikus/platform_device.h>
#include <fikus/uio_driver.h>
#include <fikus/spinlock.h>
#include <fikus/bitops.h>
#include <fikus/module.h>
#include <fikus/interrupt.h>
#include <fikus/stringify.h>
#include <fikus/pm_runtime.h>
#include <fikus/slab.h>

#include <fikus/of.h>
#include <fikus/of_platform.h>
#include <fikus/of_address.h>

#define DRIVER_NAME "uio_pdrv_genirq"

struct uio_pdrv_genirq_platdata {
	struct uio_info *uioinfo;
	spinlock_t lock;
	unsigned long flags;
	struct platform_device *pdev;
};

/* Bits in uio_pdrv_genirq_platdata.flags */
enum {
	UIO_IRQ_DISABLED = 0,
};

static int uio_pdrv_genirq_open(struct uio_info *info, struct inode *inode)
{
	struct uio_pdrv_genirq_platdata *priv = info->priv;

	/* Wait until the Runtime PM code has woken up the device */
	pm_runtime_get_sync(&priv->pdev->dev);
	return 0;
}

static int uio_pdrv_genirq_release(struct uio_info *info, struct inode *inode)
{
	struct uio_pdrv_genirq_platdata *priv = info->priv;

	/* Tell the Runtime PM code that the device has become idle */
	pm_runtime_put_sync(&priv->pdev->dev);
	return 0;
}

static irqreturn_t uio_pdrv_genirq_handler(int irq, struct uio_info *dev_info)
{
	struct uio_pdrv_genirq_platdata *priv = dev_info->priv;

	/* Just disable the interrupt in the interrupt controller, and
	 * remember the state so we can allow user space to enable it later.
	 */

	spin_lock(&priv->lock);
	if (!__test_and_set_bit(UIO_IRQ_DISABLED, &priv->flags))
		disable_irq_nosync(irq);
	spin_unlock(&priv->lock);

	return IRQ_HANDLED;
}

static int uio_pdrv_genirq_irqcontrol(struct uio_info *dev_info, s32 irq_on)
{
	struct uio_pdrv_genirq_platdata *priv = dev_info->priv;
	unsigned long flags;

	/* Allow user space to enable and disable the interrupt
	 * in the interrupt controller, but keep track of the
	 * state to prevent per-irq depth damage.
	 *
	 * Serialize this operation to support multiple tasks and concurrency
	 * with irq handler on SMP systems.
	 */

	spin_lock_irqsave(&priv->lock, flags);
	if (irq_on) {
		if (__test_and_clear_bit(UIO_IRQ_DISABLED, &priv->flags))
			enable_irq(dev_info->irq);
	} else {
		if (!__test_and_set_bit(UIO_IRQ_DISABLED, &priv->flags))
			disable_irq_nosync(dev_info->irq);
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static int uio_pdrv_genirq_probe(struct platform_device *pdev)
{
	struct uio_info *uioinfo = dev_get_platdata(&pdev->dev);
	struct uio_pdrv_genirq_platdata *priv;
	struct uio_mem *uiomem;
	int ret = -EINVAL;
	int i;

	if (pdev->dev.of_node) {
		/* alloc uioinfo for one device */
		uioinfo = kzalloc(sizeof(*uioinfo), GFP_KERNEL);
		if (!uioinfo) {
			ret = -ENOMEM;
			dev_err(&pdev->dev, "unable to kmalloc\n");
			return ret;
		}
		uioinfo->name = pdev->dev.of_node->name;
		uioinfo->version = "devicetree";
		/* Multiple IRQs are not supported */
	}

	if (!uioinfo || !uioinfo->name || !uioinfo->version) {
		dev_err(&pdev->dev, "missing platform_data\n");
		goto bad0;
	}

	if (uioinfo->handler || uioinfo->irqcontrol ||
	    uioinfo->irq_flags & IRQF_SHARED) {
		dev_err(&pdev->dev, "interrupt configuration error\n");
		goto bad0;
	}

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "unable to kmalloc\n");
		goto bad0;
	}

	priv->uioinfo = uioinfo;
	spin_lock_init(&priv->lock);
	priv->flags = 0; /* interrupt is enabled to begin with */
	priv->pdev = pdev;

	if (!uioinfo->irq) {
		ret = platform_get_irq(pdev, 0);
		uioinfo->irq = ret;
		if (ret == -ENXIO && pdev->dev.of_node)
			uioinfo->irq = UIO_IRQ_NONE;
		else if (ret < 0) {
			dev_err(&pdev->dev, "failed to get IRQ\n");
			goto bad1;
		}
	}

	uiomem = &uioinfo->mem[0];

	for (i = 0; i < pdev->num_resources; ++i) {
		struct resource *r = &pdev->resource[i];

		if (r->flags != IORESOURCE_MEM)
			continue;

		if (uiomem >= &uioinfo->mem[MAX_UIO_MAPS]) {
			dev_warn(&pdev->dev, "device has more than "
					__stringify(MAX_UIO_MAPS)
					" I/O memory resources.\n");
			break;
		}

		uiomem->memtype = UIO_MEM_PHYS;
		uiomem->addr = r->start;
		uiomem->size = resource_size(r);
		uiomem->name = r->name;
		++uiomem;
	}

	while (uiomem < &uioinfo->mem[MAX_UIO_MAPS]) {
		uiomem->size = 0;
		++uiomem;
	}

	/* This driver requires no hardware specific kernel code to handle
	 * interrupts. Instead, the interrupt handler simply disables the
	 * interrupt in the interrupt controller. User space is responsible
	 * for performing hardware specific acknowledge and re-enabling of
	 * the interrupt in the interrupt controller.
	 *
	 * Interrupt sharing is not supported.
	 */

	uioinfo->handler = uio_pdrv_genirq_handler;
	uioinfo->irqcontrol = uio_pdrv_genirq_irqcontrol;
	uioinfo->open = uio_pdrv_genirq_open;
	uioinfo->release = uio_pdrv_genirq_release;
	uioinfo->priv = priv;

	/* Enable Runtime PM for this device:
	 * The device starts in suspended state to allow the hardware to be
	 * turned off by default. The Runtime PM bus code should power on the
	 * hardware and enable clocks at open().
	 */
	pm_runtime_enable(&pdev->dev);

	ret = uio_register_device(&pdev->dev, priv->uioinfo);
	if (ret) {
		dev_err(&pdev->dev, "unable to register uio device\n");
		goto bad2;
	}

	platform_set_drvdata(pdev, priv);
	return 0;
 bad2:
	pm_runtime_disable(&pdev->dev);
 bad1:
	kfree(priv);
 bad0:
	/* kfree uioinfo for OF */
	if (pdev->dev.of_node)
		kfree(uioinfo);
	return ret;
}

static int uio_pdrv_genirq_remove(struct platform_device *pdev)
{
	struct uio_pdrv_genirq_platdata *priv = platform_get_drvdata(pdev);

	uio_unregister_device(priv->uioinfo);
	pm_runtime_disable(&pdev->dev);

	priv->uioinfo->handler = NULL;
	priv->uioinfo->irqcontrol = NULL;

	/* kfree uioinfo for OF */
	if (pdev->dev.of_node)
		kfree(priv->uioinfo);

	kfree(priv);
	return 0;
}

static int uio_pdrv_genirq_runtime_nop(struct device *dev)
{
	/* Runtime PM callback shared between ->runtime_suspend()
	 * and ->runtime_resume(). Simply returns success.
	 *
	 * In this driver pm_runtime_get_sync() and pm_runtime_put_sync()
	 * are used at open() and release() time. This allows the
	 * Runtime PM code to turn off power to the device while the
	 * device is unused, ie before open() and after release().
	 *
	 * This Runtime PM callback does not need to save or restore
	 * any registers since user space is responsbile for hardware
	 * register reinitialization after open().
	 */
	return 0;
}

static const struct dev_pm_ops uio_pdrv_genirq_dev_pm_ops = {
	.runtime_suspend = uio_pdrv_genirq_runtime_nop,
	.runtime_resume = uio_pdrv_genirq_runtime_nop,
};

#ifdef CONFIG_OF
static struct of_device_id uio_of_genirq_match[] = {
	{ /* This is filled with module_parm */ },
	{ /* Sentinel */ },
};
MODULE_DEVICE_TABLE(of, uio_of_genirq_match);
module_param_string(of_id, uio_of_genirq_match[0].compatible, 128, 0);
MODULE_PARM_DESC(of_id, "Openfirmware id of the device to be handled by uio");
#endif

static struct platform_driver uio_pdrv_genirq = {
	.probe = uio_pdrv_genirq_probe,
	.remove = uio_pdrv_genirq_remove,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.pm = &uio_pdrv_genirq_dev_pm_ops,
		.of_match_table = of_match_ptr(uio_of_genirq_match),
	},
};

module_platform_driver(uio_pdrv_genirq);

MODULE_AUTHOR("Magnus Damm");
MODULE_DESCRIPTION("Userspace I/O platform driver with generic IRQ handling");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRIVER_NAME);
