/*
 * Renesas - AP-325RXA
 * (Compatible with Algo System ., LTD. - AP-320A)
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Author : Yusuke Goda <goda.yuske@renesas.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <fikus/init.h>
#include <fikus/device.h>
#include <fikus/interrupt.h>
#include <fikus/platform_device.h>
#include <fikus/mmc/host.h>
#include <fikus/mmc/sh_mobile_sdhi.h>
#include <fikus/mtd/physmap.h>
#include <fikus/mtd/sh_flctl.h>
#include <fikus/delay.h>
#include <fikus/i2c.h>
#include <fikus/regulator/fixed.h>
#include <fikus/regulator/machine.h>
#include <fikus/smsc911x.h>
#include <fikus/gpio.h>
#include <fikus/videodev2.h>
#include <fikus/sh_intc.h>
#include <media/ov772x.h>
#include <media/soc_camera.h>
#include <media/soc_camera_platform.h>
#include <media/sh_mobile_ceu.h>
#include <video/sh_mobile_lcdc.h>
#include <asm/io.h>
#include <asm/clock.h>
#include <asm/suspend.h>
#include <cpu/sh7723.h>

/* Dummy supplies, where voltage doesn't matter */
static struct regulator_consumer_supply dummy_supplies[] = {
	REGULATOR_SUPPLY("vddvario", "smsc911x"),
	REGULATOR_SUPPLY("vdd33a", "smsc911x"),
};

static struct smsc911x_platform_config smsc911x_config = {
	.phy_interface	= PHY_INTERFACE_MODE_MII,
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_OPEN_DRAIN,
	.flags		= SMSC911X_USE_32BIT,
};

static struct resource smsc9118_resources[] = {
	[0] = {
		.start	= 0xb6080000,
		.end	= 0xb60fffff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= evt2irq(0x660),
		.end	= evt2irq(0x660),
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device smsc9118_device = {
	.name		= "smsc911x",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smsc9118_resources),
	.resource	= smsc9118_resources,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
};

/*
 * AP320 and AP325RXA has CPLD data in NOR Flash(0xA80000-0xABFFFF).
 * If this area erased, this board can not boot.
 */
static struct mtd_partition ap325rxa_nor_flash_partitions[] = {
	{
		.name = "uboot",
		.offset = 0,
		.size = (1 * 1024 * 1024),
		.mask_flags = MTD_WRITEABLE,	/* Read-only */
	}, {
		.name = "kernel",
		.offset = MTDPART_OFS_APPEND,
		.size = (2 * 1024 * 1024),
	}, {
		.name = "free-area0",
		.offset = MTDPART_OFS_APPEND,
		.size = ((7 * 1024 * 1024) + (512 * 1024)),
	}, {
		.name = "CPLD-Data",
		.offset = MTDPART_OFS_APPEND,
		.mask_flags = MTD_WRITEABLE,	/* Read-only */
		.size = (1024 * 128 * 2),
	}, {
		.name = "free-area1",
		.offset = MTDPART_OFS_APPEND,
		.size = MTDPART_SIZ_FULL,
	},
};

static struct physmap_flash_data ap325rxa_nor_flash_data = {
	.width		= 2,
	.parts		= ap325rxa_nor_flash_partitions,
	.nr_parts	= ARRAY_SIZE(ap325rxa_nor_flash_partitions),
};

static struct resource ap325rxa_nor_flash_resources[] = {
	[0] = {
		.name	= "NOR Flash",
		.start	= 0x00000000,
		.end	= 0x00ffffff,
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device ap325rxa_nor_flash_device = {
	.name		= "physmap-flash",
	.resource	= ap325rxa_nor_flash_resources,
	.num_resources	= ARRAY_SIZE(ap325rxa_nor_flash_resources),
	.dev		= {
		.platform_data = &ap325rxa_nor_flash_data,
	},
};

static struct mtd_partition nand_partition_info[] = {
	{
		.name	= "nand_data",
		.offset	= 0,
		.size	= MTDPART_SIZ_FULL,
	},
};

static struct resource nand_flash_resources[] = {
	[0] = {
		.start	= 0xa4530000,
		.end	= 0xa45300ff,
		.flags	= IORESOURCE_MEM,
	}
};

static struct sh_flctl_platform_data nand_flash_data = {
	.parts		= nand_partition_info,
	.nr_parts	= ARRAY_SIZE(nand_partition_info),
	.flcmncr_val	= FCKSEL_E | TYPESEL_SET | NANWF_E,
	.has_hwecc	= 1,
};

static struct platform_device nand_flash_device = {
	.name		= "sh_flctl",
	.resource	= nand_flash_resources,
	.num_resources	= ARRAY_SIZE(nand_flash_resources),
	.dev		= {
		.platform_data = &nand_flash_data,
	},
};

#define FPGA_LCDREG	0xB4100180
#define FPGA_BKLREG	0xB4100212
#define FPGA_LCDREG_VAL	0x0018
#define PORT_MSELCRB	0xA4050182
#define PORT_HIZCRC	0xA405015C
#define PORT_DRVCRA	0xA405018A
#define PORT_DRVCRB	0xA405018C

static int ap320_wvga_set_brightness(int brightness)
{
	if (brightness) {
		gpio_set_value(GPIO_PTS3, 0);
		__raw_writew(0x100, FPGA_BKLREG);
	} else {
		__raw_writew(0, FPGA_BKLREG);
		gpio_set_value(GPIO_PTS3, 1);
	}

	return 0;
}

static void ap320_wvga_power_on(void)
{
	msleep(100);

	/* ASD AP-320/325 LCD ON */
	__raw_writew(FPGA_LCDREG_VAL, FPGA_LCDREG);
}

static void ap320_wvga_power_off(void)
{
	/* ASD AP-320/325 LCD OFF */
	__raw_writew(0, FPGA_LCDREG);
}

static const struct fb_videomode ap325rxa_lcdc_modes[] = {
	{
		.name = "LB070WV1",
		.xres = 800,
		.yres = 480,
		.left_margin = 32,
		.right_margin = 160,
		.hsync_len = 8,
		.upper_margin = 63,
		.lower_margin = 80,
		.vsync_len = 1,
		.sync = 0, /* hsync and vsync are active low */
	},
};

static struct sh_mobile_lcdc_info lcdc_info = {
	.clock_source = LCDC_CLK_EXTERNAL,
	.ch[0] = {
		.chan = LCDC_CHAN_MAINLCD,
		.fourcc = V4L2_PIX_FMT_RGB565,
		.interface_type = RGB18,
		.clock_divider = 1,
		.lcd_modes = ap325rxa_lcdc_modes,
		.num_modes = ARRAY_SIZE(ap325rxa_lcdc_modes),
		.panel_cfg = {
			.width = 152,	/* 7.0 inch */
			.height = 91,
			.display_on = ap320_wvga_power_on,
			.display_off = ap320_wvga_power_off,
		},
		.bl_info = {
			.name = "sh_mobile_lcdc_bl",
			.max_brightness = 1,
			.set_brightness = ap320_wvga_set_brightness,
		},
	}
};

static struct resource lcdc_resources[] = {
	[0] = {
		.name	= "LCDC",
		.start	= 0xfe940000, /* P4-only space */
		.end	= 0xfe942fff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= evt2irq(0x580),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device lcdc_device = {
	.name		= "sh_mobile_lcdc_fb",
	.num_resources	= ARRAY_SIZE(lcdc_resources),
	.resource	= lcdc_resources,
	.dev		= {
		.platform_data	= &lcdc_info,
	},
};

static void camera_power(int val)
{
	gpio_set_value(GPIO_PTZ5, val); /* RST_CAM/RSTB */
	mdelay(10);
}

#ifdef CONFIG_I2C
/* support for the old ncm03j camera */
static unsigned char camera_ncm03j_magic[] =
{
	0x87, 0x00, 0x88, 0x08, 0x89, 0x01, 0x8A, 0xE8,
	0x1D, 0x00, 0x1E, 0x8A, 0x21, 0x00, 0x33, 0x36,
	0x36, 0x60, 0x37, 0x08, 0x3B, 0x31, 0x44, 0x0F,
	0x46, 0xF0, 0x4B, 0x28, 0x4C, 0x21, 0x4D, 0x55,
	0x4E, 0x1B, 0x4F, 0xC7, 0x50, 0xFC, 0x51, 0x12,
	0x58, 0x02, 0x66, 0xC0, 0x67, 0x46, 0x6B, 0xA0,
	0x6C, 0x34, 0x7E, 0x25, 0x7F, 0x25, 0x8D, 0x0F,
	0x92, 0x40, 0x93, 0x04, 0x94, 0x26, 0x95, 0x0A,
	0x99, 0x03, 0x9A, 0xF0, 0x9B, 0x14, 0x9D, 0x7A,
	0xC5, 0x02, 0xD6, 0x07, 0x59, 0x00, 0x5A, 0x1A,
	0x5B, 0x2A, 0x5C, 0x37, 0x5D, 0x42, 0x5E, 0x56,
	0xC8, 0x00, 0xC9, 0x1A, 0xCA, 0x2A, 0xCB, 0x37,
	0xCC, 0x42, 0xCD, 0x56, 0xCE, 0x00, 0xCF, 0x1A,
	0xD0, 0x2A, 0xD1, 0x37, 0xD2, 0x42, 0xD3, 0x56,
	0x5F, 0x68, 0x60, 0x87, 0x61, 0xA3, 0x62, 0xBC,
	0x63, 0xD4, 0x64, 0xEA, 0xD6, 0x0F,
};

static int camera_probe(void)
{
	struct i2c_adapter *a = i2c_get_adapter(0);
	struct i2c_msg msg;
	int ret;

	if (!a)
		return -ENODEV;

	camera_power(1);
	msg.addr = 0x6e;
	msg.buf = camera_ncm03j_magic;
	msg.len = 2;
	msg.flags = 0;
	ret = i2c_transfer(a, &msg, 1);
	camera_power(0);

	return ret;
}

static int camera_set_capture(struct soc_camera_platform_info *info,
			      int enable)
{
	struct i2c_adapter *a = i2c_get_adapter(0);
	struct i2c_msg msg;
	int ret = 0;
	int i;

	camera_power(0);
	if (!enable)
		return 0; /* no disable for now */

	camera_power(1);
	for (i = 0; i < ARRAY_SIZE(camera_ncm03j_magic); i += 2) {
		u_int8_t buf[8];

		msg.addr = 0x6e;
		msg.buf = buf;
		msg.len = 2;
		msg.flags = 0;

		buf[0] = camera_ncm03j_magic[i];
		buf[1] = camera_ncm03j_magic[i + 1];

		ret = (ret < 0) ? ret : i2c_transfer(a, &msg, 1);
	}

	return ret;
}

static int ap325rxa_camera_add(struct soc_camera_device *icd);
static void ap325rxa_camera_del(struct soc_camera_device *icd);

static struct soc_camera_platform_info camera_info = {
	.format_name = "UYVY",
	.format_depth = 16,
	.format = {
		.code = V4L2_MBUS_FMT_UYVY8_2X8,
		.colorspace = V4L2_COLORSPACE_SMPTE170M,
		.field = V4L2_FIELD_NONE,
		.width = 640,
		.height = 480,
	},
	.mbus_param = V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_MASTER |
	V4L2_MBUS_VSYNC_ACTIVE_HIGH | V4L2_MBUS_HSYNC_ACTIVE_HIGH |
	V4L2_MBUS_DATA_ACTIVE_HIGH,
	.mbus_type = V4L2_MBUS_PARALLEL,
	.set_capture = camera_set_capture,
};

static struct soc_camera_link camera_link = {
	.bus_id		= 0,
	.add_device	= ap325rxa_camera_add,
	.del_device	= ap325rxa_camera_del,
	.module_name	= "soc_camera_platform",
	.priv		= &camera_info,
};

static struct platform_device *camera_device;

static void ap325rxa_camera_release(struct device *dev)
{
	soc_camera_platform_release(&camera_device);
}

static int ap325rxa_camera_add(struct soc_camera_device *icd)
{
	int ret = soc_camera_platform_add(icd, &camera_device, &camera_link,
					  ap325rxa_camera_release, 0);
	if (ret < 0)
		return ret;

	ret = camera_probe();
	if (ret < 0)
		soc_camera_platform_del(icd, camera_device, &camera_link);

	return ret;
}

static void ap325rxa_camera_del(struct soc_camera_device *icd)
{
	soc_camera_platform_del(icd, camera_device, &camera_link);
}
#endif /* CONFIG_I2C */

static int ov7725_power(struct device *dev, int mode)
{
	camera_power(0);
	if (mode)
		camera_power(1);

	return 0;
}

static struct sh_mobile_ceu_info sh_mobile_ceu_info = {
	.flags = SH_CEU_FLAG_USE_8BIT_BUS,
};

static struct resource ceu_resources[] = {
	[0] = {
		.name	= "CEU",
		.start	= 0xfe910000,
		.end	= 0xfe91009f,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = evt2irq(0x880),
		.flags  = IORESOURCE_IRQ,
	},
	[2] = {
		/* place holder for contiguous memory */
	},
};

static struct platform_device ceu_device = {
	.name		= "sh_mobile_ceu",
	.id             = 0, /* "ceu0" clock */
	.num_resources	= ARRAY_SIZE(ceu_resources),
	.resource	= ceu_resources,
	.dev		= {
		.platform_data	= &sh_mobile_ceu_info,
	},
};

/* Fixed 3.3V regulators to be used by SDHI0, SDHI1 */
static struct regulator_consumer_supply fixed3v3_power_consumers[] =
{
	REGULATOR_SUPPLY("vmmc", "sh_mobile_sdhi.0"),
	REGULATOR_SUPPLY("vqmmc", "sh_mobile_sdhi.0"),
	REGULATOR_SUPPLY("vmmc", "sh_mobile_sdhi.1"),
	REGULATOR_SUPPLY("vqmmc", "sh_mobile_sdhi.1"),
};

static struct resource sdhi0_cn3_resources[] = {
	[0] = {
		.name	= "SDHI0",
		.start	= 0x04ce0000,
		.end	= 0x04ce00ff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= evt2irq(0xe80),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct sh_mobile_sdhi_info sdhi0_cn3_data = {
	.tmio_caps      = MMC_CAP_SDIO_IRQ,
};

static struct platform_device sdhi0_cn3_device = {
	.name		= "sh_mobile_sdhi",
	.id             = 0, /* "sdhi0" clock */
	.num_resources	= ARRAY_SIZE(sdhi0_cn3_resources),
	.resource	= sdhi0_cn3_resources,
	.dev = {
		.platform_data = &sdhi0_cn3_data,
	},
};

static struct resource sdhi1_cn7_resources[] = {
	[0] = {
		.name	= "SDHI1",
		.start	= 0x04cf0000,
		.end	= 0x04cf00ff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= evt2irq(0x4e0),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct sh_mobile_sdhi_info sdhi1_cn7_data = {
	.tmio_caps      = MMC_CAP_SDIO_IRQ,
};

static struct platform_device sdhi1_cn7_device = {
	.name		= "sh_mobile_sdhi",
	.id             = 1, /* "sdhi1" clock */
	.num_resources	= ARRAY_SIZE(sdhi1_cn7_resources),
	.resource	= sdhi1_cn7_resources,
	.dev = {
		.platform_data = &sdhi1_cn7_data,
	},
};

static struct i2c_board_info __initdata ap325rxa_i2c_devices[] = {
	{
		I2C_BOARD_INFO("pcf8563", 0x51),
	},
};

static struct i2c_board_info ap325rxa_i2c_camera[] = {
	{
		I2C_BOARD_INFO("ov772x", 0x21),
	},
};

static struct ov772x_camera_info ov7725_info = {
	.flags		= OV772X_FLAG_VFLIP | OV772X_FLAG_HFLIP,
	.edgectrl	= OV772X_AUTO_EDGECTRL(0xf, 0),
};

static struct soc_camera_link ov7725_link = {
	.bus_id		= 0,
	.power		= ov7725_power,
	.board_info	= &ap325rxa_i2c_camera[0],
	.i2c_adapter_id	= 0,
	.priv		= &ov7725_info,
};

static struct platform_device ap325rxa_camera[] = {
	{
		.name	= "soc-camera-pdrv",
		.id	= 0,
		.dev	= {
			.platform_data = &ov7725_link,
		},
	}, {
		.name	= "soc-camera-pdrv",
		.id	= 1,
		.dev	= {
			.platform_data = &camera_link,
		},
	},
};

static struct platform_device *ap325rxa_devices[] __initdata = {
	&smsc9118_device,
	&ap325rxa_nor_flash_device,
	&lcdc_device,
	&ceu_device,
	&nand_flash_device,
	&sdhi0_cn3_device,
	&sdhi1_cn7_device,
	&ap325rxa_camera[0],
	&ap325rxa_camera[1],
};

extern char ap325rxa_sdram_enter_start;
extern char ap325rxa_sdram_enter_end;
extern char ap325rxa_sdram_leave_start;
extern char ap325rxa_sdram_leave_end;

static int __init ap325rxa_devices_setup(void)
{
	/* register board specific self-refresh code */
	sh_mobile_register_self_refresh(SUSP_SH_STANDBY | SUSP_SH_SF,
					&ap325rxa_sdram_enter_start,
					&ap325rxa_sdram_enter_end,
					&ap325rxa_sdram_leave_start,
					&ap325rxa_sdram_leave_end);

	regulator_register_always_on(0, "fixed-3.3V", fixed3v3_power_consumers,
				     ARRAY_SIZE(fixed3v3_power_consumers), 3300000);
	regulator_register_fixed(1, dummy_supplies, ARRAY_SIZE(dummy_supplies));

	/* LD3 and LD4 LEDs */
	gpio_request(GPIO_PTX5, NULL); /* RUN */
	gpio_direction_output(GPIO_PTX5, 1);
	gpio_export(GPIO_PTX5, 0);

	gpio_request(GPIO_PTX4, NULL); /* INDICATOR */
	gpio_direction_output(GPIO_PTX4, 0);
	gpio_export(GPIO_PTX4, 0);

	/* SW1 input */
	gpio_request(GPIO_PTF7, NULL); /* MODE */
	gpio_direction_input(GPIO_PTF7);
	gpio_export(GPIO_PTF7, 0);

	/* LCDC */
	gpio_request(GPIO_FN_LCDD15, NULL);
	gpio_request(GPIO_FN_LCDD14, NULL);
	gpio_request(GPIO_FN_LCDD13, NULL);
	gpio_request(GPIO_FN_LCDD12, NULL);
	gpio_request(GPIO_FN_LCDD11, NULL);
	gpio_request(GPIO_FN_LCDD10, NULL);
	gpio_request(GPIO_FN_LCDD9, NULL);
	gpio_request(GPIO_FN_LCDD8, NULL);
	gpio_request(GPIO_FN_LCDD7, NULL);
	gpio_request(GPIO_FN_LCDD6, NULL);
	gpio_request(GPIO_FN_LCDD5, NULL);
	gpio_request(GPIO_FN_LCDD4, NULL);
	gpio_request(GPIO_FN_LCDD3, NULL);
	gpio_request(GPIO_FN_LCDD2, NULL);
	gpio_request(GPIO_FN_LCDD1, NULL);
	gpio_request(GPIO_FN_LCDD0, NULL);
	gpio_request(GPIO_FN_LCDLCLK_PTR, NULL);
	gpio_request(GPIO_FN_LCDDCK, NULL);
	gpio_request(GPIO_FN_LCDVEPWC, NULL);
	gpio_request(GPIO_FN_LCDVCPWC, NULL);
	gpio_request(GPIO_FN_LCDVSYN, NULL);
	gpio_request(GPIO_FN_LCDHSYN, NULL);
	gpio_request(GPIO_FN_LCDDISP, NULL);
	gpio_request(GPIO_FN_LCDDON, NULL);

	/* LCD backlight */
	gpio_request(GPIO_PTS3, NULL);
	gpio_direction_output(GPIO_PTS3, 1);

	/* CEU */
	gpio_request(GPIO_FN_VIO_CLK2, NULL);
	gpio_request(GPIO_FN_VIO_VD2, NULL);
	gpio_request(GPIO_FN_VIO_HD2, NULL);
	gpio_request(GPIO_FN_VIO_FLD, NULL);
	gpio_request(GPIO_FN_VIO_CKO, NULL);
	gpio_request(GPIO_FN_VIO_D15, NULL);
	gpio_request(GPIO_FN_VIO_D14, NULL);
	gpio_request(GPIO_FN_VIO_D13, NULL);
	gpio_request(GPIO_FN_VIO_D12, NULL);
	gpio_request(GPIO_FN_VIO_D11, NULL);
	gpio_request(GPIO_FN_VIO_D10, NULL);
	gpio_request(GPIO_FN_VIO_D9, NULL);
	gpio_request(GPIO_FN_VIO_D8, NULL);

	gpio_request(GPIO_PTZ7, NULL);
	gpio_direction_output(GPIO_PTZ7, 0); /* OE_CAM */
	gpio_request(GPIO_PTZ6, NULL);
	gpio_direction_output(GPIO_PTZ6, 0); /* STBY_CAM */
	gpio_request(GPIO_PTZ5, NULL);
	gpio_direction_output(GPIO_PTZ5, 0); /* RST_CAM */
	gpio_request(GPIO_PTZ4, NULL);
	gpio_direction_output(GPIO_PTZ4, 0); /* SADDR */

	__raw_writew(__raw_readw(PORT_MSELCRB) & ~0x0001, PORT_MSELCRB);

	/* FLCTL */
	gpio_request(GPIO_FN_FCE, NULL);
	gpio_request(GPIO_FN_NAF7, NULL);
	gpio_request(GPIO_FN_NAF6, NULL);
	gpio_request(GPIO_FN_NAF5, NULL);
	gpio_request(GPIO_FN_NAF4, NULL);
	gpio_request(GPIO_FN_NAF3, NULL);
	gpio_request(GPIO_FN_NAF2, NULL);
	gpio_request(GPIO_FN_NAF1, NULL);
	gpio_request(GPIO_FN_NAF0, NULL);
	gpio_request(GPIO_FN_FCDE, NULL);
	gpio_request(GPIO_FN_FOE, NULL);
	gpio_request(GPIO_FN_FSC, NULL);
	gpio_request(GPIO_FN_FWE, NULL);
	gpio_request(GPIO_FN_FRB, NULL);

	__raw_writew(0, PORT_HIZCRC);
	__raw_writew(0xFFFF, PORT_DRVCRA);
	__raw_writew(0xFFFF, PORT_DRVCRB);

	platform_resource_setup_memory(&ceu_device, "ceu", 4 << 20);

	/* SDHI0 - CN3 - SD CARD */
	gpio_request(GPIO_FN_SDHI0CD_PTD, NULL);
	gpio_request(GPIO_FN_SDHI0WP_PTD, NULL);
	gpio_request(GPIO_FN_SDHI0D3_PTD, NULL);
	gpio_request(GPIO_FN_SDHI0D2_PTD, NULL);
	gpio_request(GPIO_FN_SDHI0D1_PTD, NULL);
	gpio_request(GPIO_FN_SDHI0D0_PTD, NULL);
	gpio_request(GPIO_FN_SDHI0CMD_PTD, NULL);
	gpio_request(GPIO_FN_SDHI0CLK_PTD, NULL);

	/* SDHI1 - CN7 - MICRO SD CARD */
	gpio_request(GPIO_FN_SDHI1CD, NULL);
	gpio_request(GPIO_FN_SDHI1D3, NULL);
	gpio_request(GPIO_FN_SDHI1D2, NULL);
	gpio_request(GPIO_FN_SDHI1D1, NULL);
	gpio_request(GPIO_FN_SDHI1D0, NULL);
	gpio_request(GPIO_FN_SDHI1CMD, NULL);
	gpio_request(GPIO_FN_SDHI1CLK, NULL);

	i2c_register_board_info(0, ap325rxa_i2c_devices,
				ARRAY_SIZE(ap325rxa_i2c_devices));

	return platform_add_devices(ap325rxa_devices,
				ARRAY_SIZE(ap325rxa_devices));
}
arch_initcall(ap325rxa_devices_setup);

/* Return the board specific boot mode pin configuration */
static int ap325rxa_mode_pins(void)
{
	/* MD0=0, MD1=0, MD2=0: Clock Mode 0
	 * MD3=0: 16-bit Area0 Bus Width
	 * MD5=1: Little Endian
	 * TSTMD=1, MD8=1: Test Mode Disabled
	 */
	return MODE_PIN5 | MODE_PIN8;
}

static struct sh_machine_vector mv_ap325rxa __initmv = {
	.mv_name = "AP-325RXA",
	.mv_mode_pins = ap325rxa_mode_pins,
};
