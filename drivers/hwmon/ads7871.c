/*
 *  ads7871 - driver for TI ADS7871 A/D converter
 *
 *  Copyright (c) 2010 Paul Thomas <pthomas8589@gmail.com>
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 or
 *  later as publishhed by the Free Software Foundation.
 *
 *	You need to have something like this in struct spi_board_info
 *	{
 *		.modalias	= "ads7871",
 *		.max_speed_hz	= 2*1000*1000,
 *		.chip_select	= 0,
 *		.bus_num	= 1,
 *	},
 */

/*From figure 18 in the datasheet*/
/*Register addresses*/
#define REG_LS_BYTE	0 /*A/D Output Data, LS Byte*/
#define REG_MS_BYTE	1 /*A/D Output Data, MS Byte*/
#define REG_PGA_VALID	2 /*PGA Valid Register*/
#define REG_AD_CONTROL	3 /*A/D Control Register*/
#define REG_GAIN_MUX	4 /*Gain/Mux Register*/
#define REG_IO_STATE	5 /*Digital I/O State Register*/
#define REG_IO_CONTROL	6 /*Digital I/O Control Register*/
#define REG_OSC_CONTROL	7 /*Rev/Oscillator Control Register*/
#define REG_SER_CONTROL 24 /*Serial Interface Control Register*/
#define REG_ID		31 /*ID Register*/

/*
 * From figure 17 in the datasheet
 * These bits get ORed with the address to form
 * the instruction byte
 */
/*Instruction Bit masks*/
#define INST_MODE_BM	(1 << 7)
#define INST_READ_BM	(1 << 6)
#define INST_16BIT_BM	(1 << 5)

/*From figure 18 in the datasheet*/
/*bit masks for Rev/Oscillator Control Register*/
#define MUX_CNV_BV	7
#define MUX_CNV_BM	(1 << MUX_CNV_BV)
#define MUX_M3_BM	(1 << 3) /*M3 selects single ended*/
#define MUX_G_BV	4 /*allows for reg = (gain << MUX_G_BV) | ...*/

/*From figure 18 in the datasheet*/
/*bit masks for Rev/Oscillator Control Register*/
#define OSC_OSCR_BM	(1 << 5)
#define OSC_OSCE_BM	(1 << 4)
#define OSC_REFE_BM	(1 << 3)
#define OSC_BUFE_BM	(1 << 2)
#define OSC_R2V_BM	(1 << 1)
#define OSC_RBG_BM	(1 << 0)

#include <fikus/module.h>
#include <fikus/init.h>
#include <fikus/spi/spi.h>
#include <fikus/hwmon.h>
#include <fikus/hwmon-sysfs.h>
#include <fikus/err.h>
#include <fikus/mutex.h>
#include <fikus/delay.h>

#define DEVICE_NAME	"ads7871"

struct ads7871_data {
	struct device	*hwmon_dev;
	struct mutex	update_lock;
};

static int ads7871_read_reg8(struct spi_device *spi, int reg)
{
	int ret;
	reg = reg | INST_READ_BM;
	ret = spi_w8r8(spi, reg);
	return ret;
}

static int ads7871_read_reg16(struct spi_device *spi, int reg)
{
	int ret;
	reg = reg | INST_READ_BM | INST_16BIT_BM;
	ret = spi_w8r16(spi, reg);
	return ret;
}

static int ads7871_write_reg8(struct spi_device *spi, int reg, u8 val)
{
	u8 tmp[2] = {reg, val};
	return spi_write(spi, tmp, sizeof(tmp));
}

static ssize_t show_voltage(struct device *dev,
		struct device_attribute *da, char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	int ret, val, i = 0;
	uint8_t channel, mux_cnv;

	channel = attr->index;
	/*
	 * TODO: add support for conversions
	 * other than single ended with a gain of 1
	 */
	/*MUX_M3_BM forces single ended*/
	/*This is also where the gain of the PGA would be set*/
	ads7871_write_reg8(spi, REG_GAIN_MUX,
		(MUX_CNV_BM | MUX_M3_BM | channel));

	ret = ads7871_read_reg8(spi, REG_GAIN_MUX);
	mux_cnv = ((ret & MUX_CNV_BM) >> MUX_CNV_BV);
	/*
	 * on 400MHz arm9 platform the conversion
	 * is already done when we do this test
	 */
	while ((i < 2) && mux_cnv) {
		i++;
		ret = ads7871_read_reg8(spi, REG_GAIN_MUX);
		mux_cnv = ((ret & MUX_CNV_BM) >> MUX_CNV_BV);
		msleep_interruptible(1);
	}

	if (mux_cnv == 0) {
		val = ads7871_read_reg16(spi, REG_LS_BYTE);
		/*result in volts*10000 = (val/8192)*2.5*10000*/
		val = ((val >> 2) * 25000) / 8192;
		return sprintf(buf, "%d\n", val);
	} else {
		return -1;
	}
}

static ssize_t ads7871_show_name(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	return sprintf(buf, "%s\n", to_spi_device(dev)->modalias);
}

static SENSOR_DEVICE_ATTR(in0_input, S_IRUGO, show_voltage, NULL, 0);
static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, show_voltage, NULL, 1);
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, show_voltage, NULL, 2);
static SENSOR_DEVICE_ATTR(in3_input, S_IRUGO, show_voltage, NULL, 3);
static SENSOR_DEVICE_ATTR(in4_input, S_IRUGO, show_voltage, NULL, 4);
static SENSOR_DEVICE_ATTR(in5_input, S_IRUGO, show_voltage, NULL, 5);
static SENSOR_DEVICE_ATTR(in6_input, S_IRUGO, show_voltage, NULL, 6);
static SENSOR_DEVICE_ATTR(in7_input, S_IRUGO, show_voltage, NULL, 7);

static DEVICE_ATTR(name, S_IRUGO, ads7871_show_name, NULL);

static struct attribute *ads7871_attributes[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in4_input.dev_attr.attr,
	&sensor_dev_attr_in5_input.dev_attr.attr,
	&sensor_dev_attr_in6_input.dev_attr.attr,
	&sensor_dev_attr_in7_input.dev_attr.attr,
	&dev_attr_name.attr,
	NULL
};

static const struct attribute_group ads7871_group = {
	.attrs = ads7871_attributes,
};

static int ads7871_probe(struct spi_device *spi)
{
	int ret, err;
	uint8_t val;
	struct ads7871_data *pdata;

	dev_dbg(&spi->dev, "probe\n");

	/* Configure the SPI bus */
	spi->mode = (SPI_MODE_0);
	spi->bits_per_word = 8;
	spi_setup(spi);

	ads7871_write_reg8(spi, REG_SER_CONTROL, 0);
	ads7871_write_reg8(spi, REG_AD_CONTROL, 0);

	val = (OSC_OSCR_BM | OSC_OSCE_BM | OSC_REFE_BM | OSC_BUFE_BM);
	ads7871_write_reg8(spi, REG_OSC_CONTROL, val);
	ret = ads7871_read_reg8(spi, REG_OSC_CONTROL);

	dev_dbg(&spi->dev, "REG_OSC_CONTROL write:%x, read:%x\n", val, ret);
	/*
	 * because there is no other error checking on an SPI bus
	 * we need to make sure we really have a chip
	 */
	if (val != ret)
		return -ENODEV;

	pdata = devm_kzalloc(&spi->dev, sizeof(struct ads7871_data),
			     GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	err = sysfs_create_group(&spi->dev.kobj, &ads7871_group);
	if (err < 0)
		return err;

	spi_set_drvdata(spi, pdata);

	pdata->hwmon_dev = hwmon_device_register(&spi->dev);
	if (IS_ERR(pdata->hwmon_dev)) {
		err = PTR_ERR(pdata->hwmon_dev);
		goto error_remove;
	}

	return 0;

error_remove:
	sysfs_remove_group(&spi->dev.kobj, &ads7871_group);
	return err;
}

static int ads7871_remove(struct spi_device *spi)
{
	struct ads7871_data *pdata = spi_get_drvdata(spi);

	hwmon_device_unregister(pdata->hwmon_dev);
	sysfs_remove_group(&spi->dev.kobj, &ads7871_group);
	return 0;
}

static struct spi_driver ads7871_driver = {
	.driver = {
		.name = DEVICE_NAME,
		.owner = THIS_MODULE,
	},

	.probe = ads7871_probe,
	.remove = ads7871_remove,
};

module_spi_driver(ads7871_driver);

MODULE_AUTHOR("Paul Thomas <pthomas8589@gmail.com>");
MODULE_DESCRIPTION("TI ADS7871 A/D driver");
MODULE_LICENSE("GPL");
