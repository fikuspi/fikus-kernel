/*
 * Battery driver for One Laptop Per Child board.
 *
 *	Copyright © 2006-2010  David Woodhouse <dwmw2@infradead.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/types.h>
#include <fikus/err.h>
#include <fikus/device.h>
#include <fikus/platform_device.h>
#include <fikus/power_supply.h>
#include <fikus/jiffies.h>
#include <fikus/sched.h>
#include <fikus/olpc-ec.h>
#include <asm/olpc.h>


#define EC_BAT_VOLTAGE	0x10	/* uint16_t,	*9.76/32,    mV   */
#define EC_BAT_CURRENT	0x11	/* int16_t,	*15.625/120, mA   */
#define EC_BAT_ACR	0x12	/* int16_t,	*6250/15,    µAh  */
#define EC_BAT_TEMP	0x13	/* uint16_t,	*100/256,   °C  */
#define EC_AMB_TEMP	0x14	/* uint16_t,	*100/256,   °C  */
#define EC_BAT_STATUS	0x15	/* uint8_t,	bitmask */
#define EC_BAT_SOC	0x16	/* uint8_t,	percentage */
#define EC_BAT_SERIAL	0x17	/* uint8_t[6] */
#define EC_BAT_EEPROM	0x18	/* uint8_t adr as input, uint8_t output */
#define EC_BAT_ERRCODE	0x1f	/* uint8_t,	bitmask */

#define BAT_STAT_PRESENT	0x01
#define BAT_STAT_FULL		0x02
#define BAT_STAT_LOW		0x04
#define BAT_STAT_DESTROY	0x08
#define BAT_STAT_AC		0x10
#define BAT_STAT_CHARGING	0x20
#define BAT_STAT_DISCHARGING	0x40
#define BAT_STAT_TRICKLE	0x80

#define BAT_ERR_INFOFAIL	0x02
#define BAT_ERR_OVERVOLTAGE	0x04
#define BAT_ERR_OVERTEMP	0x05
#define BAT_ERR_GAUGESTOP	0x06
#define BAT_ERR_OUT_OF_CONTROL	0x07
#define BAT_ERR_ID_FAIL		0x09
#define BAT_ERR_ACR_FAIL	0x10

#define BAT_ADDR_MFR_TYPE	0x5F

/*********************************************************************
 *		Power
 *********************************************************************/

static int olpc_ac_get_prop(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	int ret = 0;
	uint8_t status;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = olpc_ec_cmd(EC_BAT_STATUS, NULL, 0, &status, 1);
		if (ret)
			return ret;

		val->intval = !!(status & BAT_STAT_AC);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static enum power_supply_property olpc_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static struct power_supply olpc_ac = {
	.name = "olpc-ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = olpc_ac_props,
	.num_properties = ARRAY_SIZE(olpc_ac_props),
	.get_property = olpc_ac_get_prop,
};

static char bat_serial[17]; /* Ick */

static int olpc_bat_get_status(union power_supply_propval *val, uint8_t ec_byte)
{
	if (olpc_platform_info.ecver > 0x44) {
		if (ec_byte & (BAT_STAT_CHARGING | BAT_STAT_TRICKLE))
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (ec_byte & BAT_STAT_DISCHARGING)
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (ec_byte & BAT_STAT_FULL)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else /* er,... */
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
	} else {
		/* Older EC didn't report charge/discharge bits */
		if (!(ec_byte & BAT_STAT_AC)) /* No AC means discharging */
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (ec_byte & BAT_STAT_FULL)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else /* Not _necessarily_ true but EC doesn't tell all yet */
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
	}

	return 0;
}

static int olpc_bat_get_health(union power_supply_propval *val)
{
	uint8_t ec_byte;
	int ret;

	ret = olpc_ec_cmd(EC_BAT_ERRCODE, NULL, 0, &ec_byte, 1);
	if (ret)
		return ret;

	switch (ec_byte) {
	case 0:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;

	case BAT_ERR_OVERTEMP:
		val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
		break;

	case BAT_ERR_OVERVOLTAGE:
		val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		break;

	case BAT_ERR_INFOFAIL:
	case BAT_ERR_OUT_OF_CONTROL:
	case BAT_ERR_ID_FAIL:
	case BAT_ERR_ACR_FAIL:
		val->intval = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		break;

	default:
		/* Eep. We don't know this failure code */
		ret = -EIO;
	}

	return ret;
}

static int olpc_bat_get_mfr(union power_supply_propval *val)
{
	uint8_t ec_byte;
	int ret;

	ec_byte = BAT_ADDR_MFR_TYPE;
	ret = olpc_ec_cmd(EC_BAT_EEPROM, &ec_byte, 1, &ec_byte, 1);
	if (ret)
		return ret;

	switch (ec_byte >> 4) {
	case 1:
		val->strval = "Gold Peak";
		break;
	case 2:
		val->strval = "BYD";
		break;
	default:
		val->strval = "Unknown";
		break;
	}

	return ret;
}

static int olpc_bat_get_tech(union power_supply_propval *val)
{
	uint8_t ec_byte;
	int ret;

	ec_byte = BAT_ADDR_MFR_TYPE;
	ret = olpc_ec_cmd(EC_BAT_EEPROM, &ec_byte, 1, &ec_byte, 1);
	if (ret)
		return ret;

	switch (ec_byte & 0xf) {
	case 1:
		val->intval = POWER_SUPPLY_TECHNOLOGY_NiMH;
		break;
	case 2:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LiFe;
		break;
	default:
		val->intval = POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
		break;
	}

	return ret;
}

static int olpc_bat_get_charge_full_design(union power_supply_propval *val)
{
	uint8_t ec_byte;
	union power_supply_propval tech;
	int ret, mfr;

	ret = olpc_bat_get_tech(&tech);
	if (ret)
		return ret;

	ec_byte = BAT_ADDR_MFR_TYPE;
	ret = olpc_ec_cmd(EC_BAT_EEPROM, &ec_byte, 1, &ec_byte, 1);
	if (ret)
		return ret;

	mfr = ec_byte >> 4;

	switch (tech.intval) {
	case POWER_SUPPLY_TECHNOLOGY_NiMH:
		switch (mfr) {
		case 1: /* Gold Peak */
			val->intval = 3000000*.8;
			break;
		default:
			return -EIO;
		}
		break;

	case POWER_SUPPLY_TECHNOLOGY_LiFe:
		switch (mfr) {
		case 1: /* Gold Peak, fall through */
		case 2: /* BYD */
			val->intval = 2800000;
			break;
		default:
			return -EIO;
		}
		break;

	default:
		return -EIO;
	}

	return ret;
}

static int olpc_bat_get_charge_now(union power_supply_propval *val)
{
	uint8_t soc;
	union power_supply_propval full;
	int ret;

	ret = olpc_ec_cmd(EC_BAT_SOC, NULL, 0, &soc, 1);
	if (ret)
		return ret;

	ret = olpc_bat_get_charge_full_design(&full);
	if (ret)
		return ret;

	val->intval = soc * (full.intval / 100);
	return 0;
}

static int olpc_bat_get_voltage_max_design(union power_supply_propval *val)
{
	uint8_t ec_byte;
	union power_supply_propval tech;
	int mfr;
	int ret;

	ret = olpc_bat_get_tech(&tech);
	if (ret)
		return ret;

	ec_byte = BAT_ADDR_MFR_TYPE;
	ret = olpc_ec_cmd(EC_BAT_EEPROM, &ec_byte, 1, &ec_byte, 1);
	if (ret)
		return ret;

	mfr = ec_byte >> 4;

	switch (tech.intval) {
	case POWER_SUPPLY_TECHNOLOGY_NiMH:
		switch (mfr) {
		case 1: /* Gold Peak */
			val->intval = 6000000;
			break;
		default:
			return -EIO;
		}
		break;

	case POWER_SUPPLY_TECHNOLOGY_LiFe:
		switch (mfr) {
		case 1: /* Gold Peak */
			val->intval = 6400000;
			break;
		case 2: /* BYD */
			val->intval = 6500000;
			break;
		default:
			return -EIO;
		}
		break;

	default:
		return -EIO;
	}

	return ret;
}

/*********************************************************************
 *		Battery properties
 *********************************************************************/
static int olpc_bat_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val)
{
	int ret = 0;
	__be16 ec_word;
	uint8_t ec_byte;
	__be64 ser_buf;

	ret = olpc_ec_cmd(EC_BAT_STATUS, NULL, 0, &ec_byte, 1);
	if (ret)
		return ret;

	/* Theoretically there's a race here -- the battery could be
	   removed immediately after we check whether it's present, and
	   then we query for some other property of the now-absent battery.
	   It doesn't matter though -- the EC will return the last-known
	   information, and it's as if we just ran that _little_ bit faster
	   and managed to read it out before the battery went away. */
	if (!(ec_byte & (BAT_STAT_PRESENT | BAT_STAT_TRICKLE)) &&
			psp != POWER_SUPPLY_PROP_PRESENT)
		return -ENODEV;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = olpc_bat_get_status(val, ec_byte);
		if (ret)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (ec_byte & BAT_STAT_TRICKLE)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
		else if (ec_byte & BAT_STAT_CHARGING)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = !!(ec_byte & (BAT_STAT_PRESENT |
					    BAT_STAT_TRICKLE));
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		if (ec_byte & BAT_STAT_DESTROY)
			val->intval = POWER_SUPPLY_HEALTH_DEAD;
		else {
			ret = olpc_bat_get_health(val);
			if (ret)
				return ret;
		}
		break;

	case POWER_SUPPLY_PROP_MANUFACTURER:
		ret = olpc_bat_get_mfr(val);
		if (ret)
			return ret;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		ret = olpc_bat_get_tech(val);
		if (ret)
			return ret;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = olpc_ec_cmd(EC_BAT_VOLTAGE, NULL, 0, (void *)&ec_word, 2);
		if (ret)
			return ret;

		val->intval = (s16)be16_to_cpu(ec_word) * 9760L / 32;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		ret = olpc_ec_cmd(EC_BAT_CURRENT, NULL, 0, (void *)&ec_word, 2);
		if (ret)
			return ret;

		val->intval = (s16)be16_to_cpu(ec_word) * 15625L / 120;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = olpc_ec_cmd(EC_BAT_SOC, NULL, 0, &ec_byte, 1);
		if (ret)
			return ret;
		val->intval = ec_byte;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		if (ec_byte & BAT_STAT_FULL)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
		else if (ec_byte & BAT_STAT_LOW)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
		else
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		ret = olpc_bat_get_charge_full_design(val);
		if (ret)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		ret = olpc_bat_get_charge_now(val);
		if (ret)
			return ret;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = olpc_ec_cmd(EC_BAT_TEMP, NULL, 0, (void *)&ec_word, 2);
		if (ret)
			return ret;

		val->intval = (s16)be16_to_cpu(ec_word) * 100 / 256;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		ret = olpc_ec_cmd(EC_AMB_TEMP, NULL, 0, (void *)&ec_word, 2);
		if (ret)
			return ret;

		val->intval = (int)be16_to_cpu(ec_word) * 100 / 256;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		ret = olpc_ec_cmd(EC_BAT_ACR, NULL, 0, (void *)&ec_word, 2);
		if (ret)
			return ret;

		val->intval = (s16)be16_to_cpu(ec_word) * 6250 / 15;
		break;
	case POWER_SUPPLY_PROP_SERIAL_NUMBER:
		ret = olpc_ec_cmd(EC_BAT_SERIAL, NULL, 0, (void *)&ser_buf, 8);
		if (ret)
			return ret;

		sprintf(bat_serial, "%016llx", (long long)be64_to_cpu(ser_buf));
		val->strval = bat_serial;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = olpc_bat_get_voltage_max_design(val);
		if (ret)
			return ret;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property olpc_xo1_bat_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
};

/* XO-1.5 does not have ambient temperature property */
static enum power_supply_property olpc_xo15_bat_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
};

/* EEPROM reading goes completely around the power_supply API, sadly */

#define EEPROM_START	0x20
#define EEPROM_END	0x80
#define EEPROM_SIZE	(EEPROM_END - EEPROM_START)

static ssize_t olpc_bat_eeprom_read(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	uint8_t ec_byte;
	int ret;
	int i;

	if (off >= EEPROM_SIZE)
		return 0;
	if (off + count > EEPROM_SIZE)
		count = EEPROM_SIZE - off;

	for (i = 0; i < count; i++) {
		ec_byte = EEPROM_START + off + i;
		ret = olpc_ec_cmd(EC_BAT_EEPROM, &ec_byte, 1, &buf[i], 1);
		if (ret) {
			pr_err("olpc-battery: "
			       "EC_BAT_EEPROM cmd @ 0x%x failed - %d!\n",
			       ec_byte, ret);
			return -EIO;
		}
	}

	return count;
}

static struct bin_attribute olpc_bat_eeprom = {
	.attr = {
		.name = "eeprom",
		.mode = S_IRUGO,
	},
	.size = 0,
	.read = olpc_bat_eeprom_read,
};

/* Allow userspace to see the specific error value pulled from the EC */

static ssize_t olpc_bat_error_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	uint8_t ec_byte;
	ssize_t ret;

	ret = olpc_ec_cmd(EC_BAT_ERRCODE, NULL, 0, &ec_byte, 1);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", ec_byte);
}

static struct device_attribute olpc_bat_error = {
	.attr = {
		.name = "error",
		.mode = S_IRUGO,
	},
	.show = olpc_bat_error_read,
};

/*********************************************************************
 *		Initialisation
 *********************************************************************/

static struct power_supply olpc_bat = {
	.name = "olpc-battery",
	.get_property = olpc_bat_get_property,
	.use_for_apm = 1,
};

static int olpc_battery_suspend(struct platform_device *pdev,
				pm_message_t state)
{
	if (device_may_wakeup(olpc_ac.dev))
		olpc_ec_wakeup_set(EC_SCI_SRC_ACPWR);
	else
		olpc_ec_wakeup_clear(EC_SCI_SRC_ACPWR);

	if (device_may_wakeup(olpc_bat.dev))
		olpc_ec_wakeup_set(EC_SCI_SRC_BATTERY | EC_SCI_SRC_BATSOC
				   | EC_SCI_SRC_BATERR);
	else
		olpc_ec_wakeup_clear(EC_SCI_SRC_BATTERY | EC_SCI_SRC_BATSOC
				     | EC_SCI_SRC_BATERR);

	return 0;
}

static int olpc_battery_probe(struct platform_device *pdev)
{
	int ret;
	uint8_t status;

	/*
	 * We've seen a number of EC protocol changes; this driver requires
	 * the latest EC protocol, supported by 0x44 and above.
	 */
	if (olpc_platform_info.ecver < 0x44) {
		printk(KERN_NOTICE "OLPC EC version 0x%02x too old for "
			"battery driver.\n", olpc_platform_info.ecver);
		return -ENXIO;
	}

	ret = olpc_ec_cmd(EC_BAT_STATUS, NULL, 0, &status, 1);
	if (ret)
		return ret;

	/* Ignore the status. It doesn't actually matter */

	ret = power_supply_register(&pdev->dev, &olpc_ac);
	if (ret)
		return ret;

	if (olpc_board_at_least(olpc_board_pre(0xd0))) { /* XO-1.5 */
		olpc_bat.properties = olpc_xo15_bat_props;
		olpc_bat.num_properties = ARRAY_SIZE(olpc_xo15_bat_props);
	} else { /* XO-1 */
		olpc_bat.properties = olpc_xo1_bat_props;
		olpc_bat.num_properties = ARRAY_SIZE(olpc_xo1_bat_props);
	}

	ret = power_supply_register(&pdev->dev, &olpc_bat);
	if (ret)
		goto battery_failed;

	ret = device_create_bin_file(olpc_bat.dev, &olpc_bat_eeprom);
	if (ret)
		goto eeprom_failed;

	ret = device_create_file(olpc_bat.dev, &olpc_bat_error);
	if (ret)
		goto error_failed;

	if (olpc_ec_wakeup_available()) {
		device_set_wakeup_capable(olpc_ac.dev, true);
		device_set_wakeup_capable(olpc_bat.dev, true);
	}

	return 0;

error_failed:
	device_remove_bin_file(olpc_bat.dev, &olpc_bat_eeprom);
eeprom_failed:
	power_supply_unregister(&olpc_bat);
battery_failed:
	power_supply_unregister(&olpc_ac);
	return ret;
}

static int olpc_battery_remove(struct platform_device *pdev)
{
	device_remove_file(olpc_bat.dev, &olpc_bat_error);
	device_remove_bin_file(olpc_bat.dev, &olpc_bat_eeprom);
	power_supply_unregister(&olpc_bat);
	power_supply_unregister(&olpc_ac);
	return 0;
}

static const struct of_device_id olpc_battery_ids[] = {
	{ .compatible = "olpc,xo1-battery" },
	{}
};
MODULE_DEVICE_TABLE(of, olpc_battery_ids);

static struct platform_driver olpc_battery_driver = {
	.driver = {
		.name = "olpc-battery",
		.owner = THIS_MODULE,
		.of_match_table = olpc_battery_ids,
	},
	.probe = olpc_battery_probe,
	.remove = olpc_battery_remove,
	.suspend = olpc_battery_suspend,
};

module_platform_driver(olpc_battery_driver);

MODULE_AUTHOR("David Woodhouse <dwmw2@infradead.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Battery driver for One Laptop Per Child 'XO' machine");
