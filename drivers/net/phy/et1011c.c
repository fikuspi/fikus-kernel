/*
 * drivers/net/phy/et1011c.c
 *
 * Driver for LSI ET1011C PHYs
 *
 * Author: Chaithrika U S
 *
 * Copyright (c) 2008 Texas Instruments
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <fikus/kernel.h>
#include <fikus/string.h>
#include <fikus/errno.h>
#include <fikus/unistd.h>
#include <fikus/interrupt.h>
#include <fikus/init.h>
#include <fikus/delay.h>
#include <fikus/netdevice.h>
#include <fikus/etherdevice.h>
#include <fikus/skbuff.h>
#include <fikus/spinlock.h>
#include <fikus/mm.h>
#include <fikus/module.h>
#include <fikus/mii.h>
#include <fikus/ethtool.h>
#include <fikus/phy.h>
#include <fikus/io.h>
#include <fikus/uaccess.h>
#include <asm/irq.h>

#define ET1011C_STATUS_REG	(0x1A)
#define ET1011C_CONFIG_REG	(0x16)
#define ET1011C_SPEED_MASK		(0x0300)
#define ET1011C_GIGABIT_SPEED		(0x0200)
#define ET1011C_TX_FIFO_MASK		(0x3000)
#define ET1011C_TX_FIFO_DEPTH_8		(0x0000)
#define ET1011C_TX_FIFO_DEPTH_16	(0x1000)
#define ET1011C_INTERFACE_MASK		(0x0007)
#define ET1011C_GMII_INTERFACE		(0x0002)
#define ET1011C_SYS_CLK_EN		(0x01 << 4)


MODULE_DESCRIPTION("LSI ET1011C PHY driver");
MODULE_AUTHOR("Chaithrika U S");
MODULE_LICENSE("GPL");

static int et1011c_config_aneg(struct phy_device *phydev)
{
	int ctl = 0;
	ctl = phy_read(phydev, MII_BMCR);
	if (ctl < 0)
		return ctl;
	ctl &= ~(BMCR_FULLDPLX | BMCR_SPEED100 | BMCR_SPEED1000 |
		 BMCR_ANENABLE);
	/* First clear the PHY */
	phy_write(phydev, MII_BMCR, ctl | BMCR_RESET);

	return genphy_config_aneg(phydev);
}

static int et1011c_read_status(struct phy_device *phydev)
{
	int ret;
	u32 val;
	static int speed;
	ret = genphy_read_status(phydev);

	if (speed != phydev->speed) {
		speed = phydev->speed;
		val = phy_read(phydev, ET1011C_STATUS_REG);
		if ((val & ET1011C_SPEED_MASK) ==
					ET1011C_GIGABIT_SPEED) {
			val = phy_read(phydev, ET1011C_CONFIG_REG);
			val &= ~ET1011C_TX_FIFO_MASK;
			phy_write(phydev, ET1011C_CONFIG_REG, val\
					| ET1011C_GMII_INTERFACE\
					| ET1011C_SYS_CLK_EN\
					| ET1011C_TX_FIFO_DEPTH_16);

		}
	}
	return ret;
}

static struct phy_driver et1011c_driver = {
	.phy_id		= 0x0282f014,
	.name		= "ET1011C",
	.phy_id_mask	= 0xfffffff0,
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_1000baseT_Full),
	.flags		= PHY_POLL,
	.config_aneg	= et1011c_config_aneg,
	.read_status	= et1011c_read_status,
	.driver 	= { .owner = THIS_MODULE,},
};

static int __init et1011c_init(void)
{
	return phy_driver_register(&et1011c_driver);
}

static void __exit et1011c_exit(void)
{
	phy_driver_unregister(&et1011c_driver);
}

module_init(et1011c_init);
module_exit(et1011c_exit);

static struct mdio_device_id __maybe_unused et1011c_tbl[] = {
	{ 0x0282f014, 0xfffffff0 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, et1011c_tbl);
