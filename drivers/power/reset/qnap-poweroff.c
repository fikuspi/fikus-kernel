/*
 * QNAP Turbo NAS Board power off
 *
 * Copyright (C) 2012 Andrew Lunn <andrew@lunn.ch>
 *
 * Based on the code from:
 *
 * Copyright (C) 2009  Martin Michlmayr <tbm@cyrius.com>
 * Copyright (C) 2008  Byron Bradley <byron.bbradley@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/platform_device.h>
#include <fikus/serial_reg.h>
#include <fikus/kallsyms.h>
#include <fikus/of.h>
#include <fikus/io.h>
#include <fikus/clk.h>

#define UART1_REG(x)	(base + ((UART_##x) << 2))

static void __iomem *base;
static unsigned long tclk;

static void qnap_power_off(void)
{
	/* 19200 baud divisor */
	const unsigned divisor = ((tclk + (8 * 19200)) / (16 * 19200));

	pr_err("%s: triggering power-off...\n", __func__);

	/* hijack UART1 and reset into sane state (19200,8n1) */
	writel(0x83, UART1_REG(LCR));
	writel(divisor & 0xff, UART1_REG(DLL));
	writel((divisor >> 8) & 0xff, UART1_REG(DLM));
	writel(0x03, UART1_REG(LCR));
	writel(0x00, UART1_REG(IER));
	writel(0x00, UART1_REG(FCR));
	writel(0x00, UART1_REG(MCR));

	/* send the power-off command 'A' to PIC */
	writel('A', UART1_REG(TX));
}

static int qnap_power_off_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct clk *clk;
	char symname[KSYM_NAME_LEN];

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Missing resource");
		return -EINVAL;
	}

	base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!base) {
		dev_err(&pdev->dev, "Unable to map resource");
		return -EINVAL;
	}

	/* We need to know tclk in order to calculate the UART divisor */
	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Clk missing");
		return PTR_ERR(clk);
	}

	tclk = clk_get_rate(clk);

	/* Check that nothing else has already setup a handler */
	if (pm_power_off) {
		lookup_symbol_name((ulong)pm_power_off, symname);
		dev_err(&pdev->dev,
			"pm_power_off already claimed %p %s",
			pm_power_off, symname);
		return -EBUSY;
	}
	pm_power_off = qnap_power_off;

	return 0;
}

static int qnap_power_off_remove(struct platform_device *pdev)
{
	pm_power_off = NULL;
	return 0;
}

static const struct of_device_id qnap_power_off_of_match_table[] = {
	{ .compatible = "qnap,power-off", },
	{}
};
MODULE_DEVICE_TABLE(of, qnap_power_off_of_match_table);

static struct platform_driver qnap_power_off_driver = {
	.probe	= qnap_power_off_probe,
	.remove	= qnap_power_off_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "qnap_power_off",
		.of_match_table = of_match_ptr(qnap_power_off_of_match_table),
	},
};
module_platform_driver(qnap_power_off_driver);

MODULE_AUTHOR("Andrew Lunn <andrew@lunn.ch>");
MODULE_DESCRIPTION("QNAP Power off driver");
MODULE_LICENSE("GPL v2");
