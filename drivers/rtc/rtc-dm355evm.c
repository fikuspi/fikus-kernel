/*
 * rtc-dm355evm.c - access battery-backed counter in MSP430 firmware
 *
 * Copyright (c) 2008 by David Brownell
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/rtc.h>
#include <fikus/platform_device.h>

#include <fikus/i2c/dm355evm_msp.h>
#include <fikus/module.h>


/*
 * The MSP430 firmware on the DM355 EVM uses a watch crystal to feed
 * a 1 Hz counter.  When a backup battery is supplied, that makes a
 * reasonable RTC for applications where alarms and non-NTP drift
 * compensation aren't important.
 *
 * The only real glitch is the inability to read or write all four
 * counter bytes atomically:  the count may increment in the middle
 * of an operation, causing trouble when the LSB rolls over.
 *
 * This driver was tested with firmware revision A4.
 */
union evm_time {
	u8	bytes[4];
	u32	value;
};

static int dm355evm_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	union evm_time	time;
	int		status;
	int		tries = 0;

	do {
		/*
		 * Read LSB(0) to MSB(3) bytes.  Defend against the counter
		 * rolling over by re-reading until the value is stable,
		 * and assuming the four reads take at most a few seconds.
		 */
		status = dm355evm_msp_read(DM355EVM_MSP_RTC_0);
		if (status < 0)
			return status;
		if (tries && time.bytes[0] == status)
			break;
		time.bytes[0] = status;

		status = dm355evm_msp_read(DM355EVM_MSP_RTC_1);
		if (status < 0)
			return status;
		if (tries && time.bytes[1] == status)
			break;
		time.bytes[1] = status;

		status = dm355evm_msp_read(DM355EVM_MSP_RTC_2);
		if (status < 0)
			return status;
		if (tries && time.bytes[2] == status)
			break;
		time.bytes[2] = status;

		status = dm355evm_msp_read(DM355EVM_MSP_RTC_3);
		if (status < 0)
			return status;
		if (tries && time.bytes[3] == status)
			break;
		time.bytes[3] = status;

	} while (++tries < 5);

	dev_dbg(dev, "read timestamp %08x\n", time.value);

	rtc_time_to_tm(le32_to_cpu(time.value), tm);
	return 0;
}

static int dm355evm_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	union evm_time	time;
	unsigned long	value;
	int		status;

	rtc_tm_to_time(tm, &value);
	time.value = cpu_to_le32(value);

	dev_dbg(dev, "write timestamp %08x\n", time.value);

	/*
	 * REVISIT handle non-atomic writes ... maybe just retry until
	 * byte[1] sticks (no rollover)?
	 */
	status = dm355evm_msp_write(time.bytes[0], DM355EVM_MSP_RTC_0);
	if (status < 0)
		return status;

	status = dm355evm_msp_write(time.bytes[1], DM355EVM_MSP_RTC_1);
	if (status < 0)
		return status;

	status = dm355evm_msp_write(time.bytes[2], DM355EVM_MSP_RTC_2);
	if (status < 0)
		return status;

	status = dm355evm_msp_write(time.bytes[3], DM355EVM_MSP_RTC_3);
	if (status < 0)
		return status;

	return 0;
}

static struct rtc_class_ops dm355evm_rtc_ops = {
	.read_time	= dm355evm_rtc_read_time,
	.set_time	= dm355evm_rtc_set_time,
};

/*----------------------------------------------------------------------*/

static int dm355evm_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;

	rtc = devm_rtc_device_register(&pdev->dev, pdev->name,
					&dm355evm_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "can't register RTC device, err %ld\n",
			PTR_ERR(rtc));
		return PTR_ERR(rtc);
	}
	platform_set_drvdata(pdev, rtc);

	return 0;
}

/*
 * I2C is used to talk to the MSP430, but this platform device is
 * exposed by an MFD driver that manages I2C communications.
 */
static struct platform_driver rtc_dm355evm_driver = {
	.probe		= dm355evm_rtc_probe,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "rtc-dm355evm",
	},
};

module_platform_driver(rtc_dm355evm_driver);

MODULE_LICENSE("GPL");
