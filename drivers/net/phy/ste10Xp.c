/*
 * drivers/net/phy/ste10Xp.c
 *
 * Driver for STMicroelectronics STe10Xp PHYs
 *
 * Author: Giuseppe Cavallaro <peppe.cavallaro@st.com>
 *
 * Copyright (c) 2008 STMicroelectronics Limited
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <fikus/module.h>
#include <fikus/init.h>
#include <fikus/sched.h>
#include <fikus/kernel.h>
#include <fikus/moduleparam.h>
#include <fikus/interrupt.h>
#include <fikus/netdevice.h>
#include <fikus/ethtool.h>
#include <fikus/mii.h>
#include <fikus/phy.h>

#define MII_XCIIS   	0x11	/* Configuration Info IRQ & Status Reg */
#define MII_XIE     	0x12	/* Interrupt Enable Register */
#define MII_XIE_DEFAULT_MASK 0x0070 /* ANE complete, Remote Fault, Link Down */

#define STE101P_PHY_ID		0x00061c50
#define STE100P_PHY_ID       	0x1c040011

static int ste10Xp_config_init(struct phy_device *phydev)
{
	int value, err;

	/* Software Reset PHY */
	value = phy_read(phydev, MII_BMCR);
	if (value < 0)
		return value;

	value |= BMCR_RESET;
	err = phy_write(phydev, MII_BMCR, value);
	if (err < 0)
		return err;

	do {
		value = phy_read(phydev, MII_BMCR);
	} while (value & BMCR_RESET);

	return 0;
}

static int ste10Xp_config_intr(struct phy_device *phydev)
{
	int err, value;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED) {
		/* Enable all STe101P interrupts (PR12) */
		err = phy_write(phydev, MII_XIE, MII_XIE_DEFAULT_MASK);
		/* clear any pending interrupts */
		if (err == 0) {
			value = phy_read(phydev, MII_XCIIS);
			if (value < 0)
				err = value;
		}
	} else
		err = phy_write(phydev, MII_XIE, 0);

	return err;
}

static int ste10Xp_ack_interrupt(struct phy_device *phydev)
{
	int err = phy_read(phydev, MII_XCIIS);
	if (err < 0)
		return err;

	return 0;
}

static struct phy_driver ste10xp_pdriver[] = {
{
	.phy_id = STE101P_PHY_ID,
	.phy_id_mask = 0xfffffff0,
	.name = "STe101p",
	.features = PHY_BASIC_FEATURES | SUPPORTED_Pause,
	.flags = PHY_HAS_INTERRUPT,
	.config_init = ste10Xp_config_init,
	.config_aneg = genphy_config_aneg,
	.read_status = genphy_read_status,
	.ack_interrupt = ste10Xp_ack_interrupt,
	.config_intr = ste10Xp_config_intr,
	.suspend = genphy_suspend,
	.resume = genphy_resume,
	.driver = {.owner = THIS_MODULE,}
}, {
	.phy_id = STE100P_PHY_ID,
	.phy_id_mask = 0xffffffff,
	.name = "STe100p",
	.features = PHY_BASIC_FEATURES | SUPPORTED_Pause,
	.flags = PHY_HAS_INTERRUPT,
	.config_init = ste10Xp_config_init,
	.config_aneg = genphy_config_aneg,
	.read_status = genphy_read_status,
	.ack_interrupt = ste10Xp_ack_interrupt,
	.config_intr = ste10Xp_config_intr,
	.suspend = genphy_suspend,
	.resume = genphy_resume,
	.driver = {.owner = THIS_MODULE,}
} };

static int __init ste10Xp_init(void)
{
	return phy_drivers_register(ste10xp_pdriver,
		ARRAY_SIZE(ste10xp_pdriver));
}

static void __exit ste10Xp_exit(void)
{
	phy_drivers_unregister(ste10xp_pdriver,
		ARRAY_SIZE(ste10xp_pdriver));
}

module_init(ste10Xp_init);
module_exit(ste10Xp_exit);

static struct mdio_device_id __maybe_unused ste10Xp_tbl[] = {
	{ STE101P_PHY_ID, 0xfffffff0 },
	{ STE100P_PHY_ID, 0xffffffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, ste10Xp_tbl);

MODULE_DESCRIPTION("STMicroelectronics STe10Xp PHY driver");
MODULE_AUTHOR("Giuseppe Cavallaro <peppe.cavallaro@st.com>");
MODULE_LICENSE("GPL");
