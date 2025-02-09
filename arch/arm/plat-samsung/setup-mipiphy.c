/*
 * Copyright (C) 2011 Samsung Electronics Co., Ltd.
 *
 * S5P - Helper functions for MIPI-CSIS and MIPI-DSIM D-PHY control
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/export.h>
#include <fikus/kernel.h>
#include <fikus/platform_device.h>
#include <fikus/io.h>
#include <fikus/spinlock.h>
#include <mach/regs-clock.h>

static int __s5p_mipi_phy_control(int id, bool on, u32 reset)
{
	static DEFINE_SPINLOCK(lock);
	void __iomem *addr;
	unsigned long flags;
	u32 cfg;

	id = max(0, id);
	if (id > 1)
		return -EINVAL;

	addr = S5P_MIPI_DPHY_CONTROL(id);

	spin_lock_irqsave(&lock, flags);

	cfg = __raw_readl(addr);
	cfg = on ? (cfg | reset) : (cfg & ~reset);
	__raw_writel(cfg, addr);

	if (on) {
		cfg |= S5P_MIPI_DPHY_ENABLE;
	} else if (!(cfg & (S5P_MIPI_DPHY_SRESETN |
			    S5P_MIPI_DPHY_MRESETN) & ~reset)) {
		cfg &= ~S5P_MIPI_DPHY_ENABLE;
	}

	__raw_writel(cfg, addr);
	spin_unlock_irqrestore(&lock, flags);

	return 0;
}

int s5p_csis_phy_enable(int id, bool on)
{
	return __s5p_mipi_phy_control(id, on, S5P_MIPI_DPHY_SRESETN);
}
EXPORT_SYMBOL(s5p_csis_phy_enable);

int s5p_dsim_phy_enable(struct platform_device *pdev, bool on)
{
	return __s5p_mipi_phy_control(pdev->id, on, S5P_MIPI_DPHY_MRESETN);
}
EXPORT_SYMBOL(s5p_dsim_phy_enable);
