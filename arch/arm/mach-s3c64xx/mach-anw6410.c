/* fikus/arch/arm/mach-s3c64xx/mach-anw6410.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armfikus.simtec.co.uk/
 * Copyright 2009 Kwangwoo Lee
 * 	Kwangwoo Lee <kwangwoo.lee@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <fikus/kernel.h>
#include <fikus/types.h>
#include <fikus/interrupt.h>
#include <fikus/list.h>
#include <fikus/timer.h>
#include <fikus/init.h>
#include <fikus/serial_core.h>
#include <fikus/platform_device.h>
#include <fikus/io.h>
#include <fikus/i2c.h>
#include <fikus/fb.h>
#include <fikus/gpio.h>
#include <fikus/delay.h>
#include <fikus/dm9000.h>

#include <video/platform_lcd.h>
#include <video/samsung_fimd.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <fikus/platform_data/i2c-s3c2410.h>
#include <plat/fb.h>

#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <mach/regs-gpio.h>
#include <plat/samsung-time.h>

#include "common.h"
#include "regs-modem.h"

/* DM9000 */
#define ANW6410_PA_DM9000	(0x18000000)

/* A hardware buffer to control external devices is mapped at 0x30000000.
 * It can not be read. So current status must be kept in anw6410_extdev_status.
 */
#define ANW6410_VA_EXTDEV	S3C_ADDR(0x02000000)
#define ANW6410_PA_EXTDEV	(0x30000000)

#define ANW6410_EN_DM9000	(1<<11)
#define ANW6410_EN_LCD		(1<<14)

static __u32 anw6410_extdev_status;

static struct s3c2410_uartcfg anw6410_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = 0x3c5,
		.ulcon	     = 0x03,
		.ufcon	     = 0x51,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = 0x3c5,
		.ulcon	     = 0x03,
		.ufcon	     = 0x51,
	},
};

/* framebuffer and LCD setup. */
static void __init anw6410_lcd_mode_set(void)
{
	u32 tmp;

	/* set the LCD type */
	tmp = __raw_readl(S3C64XX_SPCON);
	tmp &= ~S3C64XX_SPCON_LCD_SEL_MASK;
	tmp |= S3C64XX_SPCON_LCD_SEL_RGB;
	__raw_writel(tmp, S3C64XX_SPCON);

	/* remove the LCD bypass */
	tmp = __raw_readl(S3C64XX_MODEM_MIFPCON);
	tmp &= ~MIFPCON_LCD_BYPASS;
	__raw_writel(tmp, S3C64XX_MODEM_MIFPCON);
}

/* GPF1 = LCD panel power
 * GPF4 = LCD backlight control
 */
static void anw6410_lcd_power_set(struct plat_lcd_data *pd,
				   unsigned int power)
{
	if (power) {
		anw6410_extdev_status |= (ANW6410_EN_LCD << 16);
		__raw_writel(anw6410_extdev_status, ANW6410_VA_EXTDEV);

		gpio_direction_output(S3C64XX_GPF(1), 1);
		gpio_direction_output(S3C64XX_GPF(4), 1);
	} else {
		anw6410_extdev_status &= ~(ANW6410_EN_LCD << 16);
		__raw_writel(anw6410_extdev_status, ANW6410_VA_EXTDEV);

		gpio_direction_output(S3C64XX_GPF(1), 0);
		gpio_direction_output(S3C64XX_GPF(4), 0);
	}
}

static struct plat_lcd_data anw6410_lcd_power_data = {
	.set_power	= anw6410_lcd_power_set,
};

static struct platform_device anw6410_lcd_powerdev = {
	.name			= "platform-lcd",
	.dev.parent		= &s3c_device_fb.dev,
	.dev.platform_data	= &anw6410_lcd_power_data,
};

static struct s3c_fb_pd_win anw6410_fb_win0 = {
	.max_bpp	= 32,
	.default_bpp	= 16,
	.xres		= 800,
	.yres		= 480,
};

static struct fb_videomode anw6410_lcd_timing = {
	.left_margin	= 8,
	.right_margin	= 13,
	.upper_margin	= 7,
	.lower_margin	= 5,
	.hsync_len	= 3,
	.vsync_len	= 1,
	.xres		= 800,
	.yres		= 480,
};

/* 405566 clocks per frame => 60Hz refresh requires 24333960Hz clock */
static struct s3c_fb_platdata anw6410_lcd_pdata __initdata = {
	.setup_gpio	= s3c64xx_fb_gpio_setup_24bpp,
	.vtiming	= &anw6410_lcd_timing,
	.win[0]		= &anw6410_fb_win0,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
};

/* DM9000AEP 10/100 ethernet controller */
static void __init anw6410_dm9000_enable(void)
{
	anw6410_extdev_status |= (ANW6410_EN_DM9000 << 16);
	__raw_writel(anw6410_extdev_status, ANW6410_VA_EXTDEV);
}

static struct resource anw6410_dm9000_resource[] = {
	[0] = DEFINE_RES_MEM(ANW6410_PA_DM9000, 4),
	[1] = DEFINE_RES_MEM(ANW6410_PA_DM9000 + 4, 501),
	[2] = DEFINE_RES_NAMED(IRQ_EINT(15), 1, NULL, IORESOURCE_IRQ \
					| IRQF_TRIGGER_HIGH),
};

static struct dm9000_plat_data anw6410_dm9000_pdata = {
	.flags	  = (DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM),
	/* dev_addr can be set to provide hwaddr. */
};

static struct platform_device anw6410_device_eth = {
	.name	= "dm9000",
	.id	= -1,
	.num_resources	= ARRAY_SIZE(anw6410_dm9000_resource),
	.resource	= anw6410_dm9000_resource,
	.dev	= {
		.platform_data  = &anw6410_dm9000_pdata,
	},
};

static struct map_desc anw6410_iodesc[] __initdata = {
	{
		.virtual	= (unsigned long)ANW6410_VA_EXTDEV,
		.pfn		= __phys_to_pfn(ANW6410_PA_EXTDEV),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	},
};

static struct platform_device *anw6410_devices[] __initdata = {
	&s3c_device_fb,
	&anw6410_lcd_powerdev,
	&anw6410_device_eth,
};

static void __init anw6410_map_io(void)
{
	s3c64xx_init_io(anw6410_iodesc, ARRAY_SIZE(anw6410_iodesc));
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(anw6410_uartcfgs, ARRAY_SIZE(anw6410_uartcfgs));
	samsung_set_timer_source(SAMSUNG_PWM3, SAMSUNG_PWM4);

	anw6410_lcd_mode_set();
}

static void __init anw6410_machine_init(void)
{
	s3c_fb_set_platdata(&anw6410_lcd_pdata);

	gpio_request(S3C64XX_GPF(1), "panel power");
	gpio_request(S3C64XX_GPF(4), "LCD backlight");

	anw6410_dm9000_enable();

	platform_add_devices(anw6410_devices, ARRAY_SIZE(anw6410_devices));
}

MACHINE_START(ANW6410, "A&W6410")
	/* Maintainer: Kwangwoo Lee <kwangwoo.lee@gmail.com> */
	.atag_offset	= 0x100,

	.init_irq	= s3c6410_init_irq,
	.map_io		= anw6410_map_io,
	.init_machine	= anw6410_machine_init,
	.init_late	= s3c64xx_init_late,
	.init_time	= samsung_timer_init,
	.restart	= s3c64xx_restart,
MACHINE_END
