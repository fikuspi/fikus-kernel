/*
 * arch/arm/mach-at91/at91sam9261_devices.c
 *
 *  Copyright (C) 2005 Thibaut VARENE <varenet@parisc-fikus.org>
 *  Copyright (C) 2005 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <fikus/dma-mapping.h>
#include <fikus/gpio.h>
#include <fikus/platform_device.h>
#include <fikus/i2c-gpio.h>

#include <fikus/fb.h>
#include <video/atmel_lcdc.h>

#include <mach/at91sam9261.h>
#include <mach/at91sam9261_matrix.h>
#include <mach/at91_matrix.h>
#include <mach/at91sam9_smc.h>

#include "board.h"
#include "generic.h"


/* --------------------------------------------------------------------
 *  USB Host
 * -------------------------------------------------------------------- */

#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)
static u64 ohci_dmamask = DMA_BIT_MASK(32);
static struct at91_usbh_data usbh_data;

static struct resource usbh_resources[] = {
	[0] = {
		.start	= AT91SAM9261_UHP_BASE,
		.end	= AT91SAM9261_UHP_BASE + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_UHP,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_UHP,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_usbh_device = {
	.name		= "at91_ohci",
	.id		= -1,
	.dev		= {
				.dma_mask		= &ohci_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
				.platform_data		= &usbh_data,
	},
	.resource	= usbh_resources,
	.num_resources	= ARRAY_SIZE(usbh_resources),
};

void __init at91_add_device_usbh(struct at91_usbh_data *data)
{
	int i;

	if (!data)
		return;

	/* Enable overcurrent notification */
	for (i = 0; i < data->ports; i++) {
		if (gpio_is_valid(data->overcurrent_pin[i]))
			at91_set_gpio_input(data->overcurrent_pin[i], 1);
	}

	usbh_data = *data;
	platform_device_register(&at91sam9261_usbh_device);
}
#else
void __init at91_add_device_usbh(struct at91_usbh_data *data) {}
#endif


/* --------------------------------------------------------------------
 *  USB Device (Gadget)
 * -------------------------------------------------------------------- */

#if defined(CONFIG_USB_AT91) || defined(CONFIG_USB_AT91_MODULE)
static struct at91_udc_data udc_data;

static struct resource udc_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_UDP,
		.end	= AT91SAM9261_BASE_UDP + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_UDP,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_UDP,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_udc_device = {
	.name		= "at91_udc",
	.id		= -1,
	.dev		= {
				.platform_data		= &udc_data,
	},
	.resource	= udc_resources,
	.num_resources	= ARRAY_SIZE(udc_resources),
};

void __init at91_add_device_udc(struct at91_udc_data *data)
{
	if (!data)
		return;

	if (gpio_is_valid(data->vbus_pin)) {
		at91_set_gpio_input(data->vbus_pin, 0);
		at91_set_deglitch(data->vbus_pin, 1);
	}

	/* Pullup pin is handled internally by USB device peripheral */

	udc_data = *data;
	platform_device_register(&at91sam9261_udc_device);
}
#else
void __init at91_add_device_udc(struct at91_udc_data *data) {}
#endif

/* --------------------------------------------------------------------
 *  MMC / SD
 * -------------------------------------------------------------------- */

#if IS_ENABLED(CONFIG_MMC_ATMELMCI)
static u64 mmc_dmamask = DMA_BIT_MASK(32);
static struct mci_platform_data mmc_data;

static struct resource mmc_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_MCI,
		.end	= AT91SAM9261_BASE_MCI + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_MCI,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_MCI,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_mmc_device = {
	.name		= "atmel_mci",
	.id		= -1,
	.dev		= {
				.dma_mask		= &mmc_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
				.platform_data		= &mmc_data,
	},
	.resource	= mmc_resources,
	.num_resources	= ARRAY_SIZE(mmc_resources),
};

void __init at91_add_device_mci(short mmc_id, struct mci_platform_data *data)
{
	if (!data)
		return;

	if (data->slot[0].bus_width) {
		/* input/irq */
		if (gpio_is_valid(data->slot[0].detect_pin)) {
			at91_set_gpio_input(data->slot[0].detect_pin, 1);
			at91_set_deglitch(data->slot[0].detect_pin, 1);
		}
		if (gpio_is_valid(data->slot[0].wp_pin))
			at91_set_gpio_input(data->slot[0].wp_pin, 1);

		/* CLK */
		at91_set_B_periph(AT91_PIN_PA2, 0);

		/* CMD */
		at91_set_B_periph(AT91_PIN_PA1, 1);

		/* DAT0, maybe DAT1..DAT3 */
		at91_set_B_periph(AT91_PIN_PA0, 1);
		if (data->slot[0].bus_width == 4) {
			at91_set_B_periph(AT91_PIN_PA4, 1);
			at91_set_B_periph(AT91_PIN_PA5, 1);
			at91_set_B_periph(AT91_PIN_PA6, 1);
		}

		mmc_data = *data;
		platform_device_register(&at91sam9261_mmc_device);
	}
}
#else
void __init at91_add_device_mci(short mmc_id, struct mci_platform_data *data) {}
#endif


/* --------------------------------------------------------------------
 *  NAND / SmartMedia
 * -------------------------------------------------------------------- */

#if defined(CONFIG_MTD_NAND_ATMEL) || defined(CONFIG_MTD_NAND_ATMEL_MODULE)
static struct atmel_nand_data nand_data;

#define NAND_BASE	AT91_CHIPSELECT_3

static struct resource nand_resources[] = {
	{
		.start	= NAND_BASE,
		.end	= NAND_BASE + SZ_256M - 1,
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device atmel_nand_device = {
	.name		= "atmel_nand",
	.id		= -1,
	.dev		= {
				.platform_data	= &nand_data,
	},
	.resource	= nand_resources,
	.num_resources	= ARRAY_SIZE(nand_resources),
};

void __init at91_add_device_nand(struct atmel_nand_data *data)
{
	unsigned long csa;

	if (!data)
		return;

	csa = at91_matrix_read(AT91_MATRIX_EBICSA);
	at91_matrix_write(AT91_MATRIX_EBICSA, csa | AT91_MATRIX_CS3A_SMC_SMARTMEDIA);

	/* enable pin */
	if (gpio_is_valid(data->enable_pin))
		at91_set_gpio_output(data->enable_pin, 1);

	/* ready/busy pin */
	if (gpio_is_valid(data->rdy_pin))
		at91_set_gpio_input(data->rdy_pin, 1);

	/* card detect pin */
	if (gpio_is_valid(data->det_pin))
		at91_set_gpio_input(data->det_pin, 1);

	at91_set_A_periph(AT91_PIN_PC0, 0);		/* NANDOE */
	at91_set_A_periph(AT91_PIN_PC1, 0);		/* NANDWE */

	nand_data = *data;
	platform_device_register(&atmel_nand_device);
}

#else
void __init at91_add_device_nand(struct atmel_nand_data *data) {}
#endif


/* --------------------------------------------------------------------
 *  TWI (i2c)
 * -------------------------------------------------------------------- */

/*
 * Prefer the GPIO code since the TWI controller isn't robust
 * (gets overruns and underruns under load) and can only issue
 * repeated STARTs in one scenario (the driver doesn't yet handle them).
 */
#if defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C_GPIO_MODULE)

static struct i2c_gpio_platform_data pdata = {
	.sda_pin		= AT91_PIN_PA7,
	.sda_is_open_drain	= 1,
	.scl_pin		= AT91_PIN_PA8,
	.scl_is_open_drain	= 1,
	.udelay			= 2,		/* ~100 kHz */
};

static struct platform_device at91sam9261_twi_device = {
	.name			= "i2c-gpio",
	.id			= 0,
	.dev.platform_data	= &pdata,
};

void __init at91_add_device_i2c(struct i2c_board_info *devices, int nr_devices)
{
	at91_set_GPIO_periph(AT91_PIN_PA7, 1);		/* TWD (SDA) */
	at91_set_multi_drive(AT91_PIN_PA7, 1);

	at91_set_GPIO_periph(AT91_PIN_PA8, 1);		/* TWCK (SCL) */
	at91_set_multi_drive(AT91_PIN_PA8, 1);

	i2c_register_board_info(0, devices, nr_devices);
	platform_device_register(&at91sam9261_twi_device);
}

#elif defined(CONFIG_I2C_AT91) || defined(CONFIG_I2C_AT91_MODULE)

static struct resource twi_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_TWI,
		.end	= AT91SAM9261_BASE_TWI + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_TWI,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_TWI,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_twi_device = {
	.id		= 0,
	.resource	= twi_resources,
	.num_resources	= ARRAY_SIZE(twi_resources),
};

void __init at91_add_device_i2c(struct i2c_board_info *devices, int nr_devices)
{
	/* IP version is not the same on 9261 and g10 */
	if (cpu_is_at91sam9g10()) {
		at91sam9261_twi_device.name = "i2c-at91sam9g10";
		/* I2C PIO must not be configured as open-drain on this chip */
	} else {
		at91sam9261_twi_device.name = "i2c-at91sam9261";
		at91_set_multi_drive(AT91_PIN_PA7, 1);
		at91_set_multi_drive(AT91_PIN_PA8, 1);
	}

	/* pins used for TWI interface */
	at91_set_A_periph(AT91_PIN_PA7, 0);		/* TWD */
	at91_set_A_periph(AT91_PIN_PA8, 0);		/* TWCK */

	i2c_register_board_info(0, devices, nr_devices);
	platform_device_register(&at91sam9261_twi_device);
}
#else
void __init at91_add_device_i2c(struct i2c_board_info *devices, int nr_devices) {}
#endif


/* --------------------------------------------------------------------
 *  SPI
 * -------------------------------------------------------------------- */

#if defined(CONFIG_SPI_ATMEL) || defined(CONFIG_SPI_ATMEL_MODULE)
static u64 spi_dmamask = DMA_BIT_MASK(32);

static struct resource spi0_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_SPI0,
		.end	= AT91SAM9261_BASE_SPI0 + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_SPI0,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_SPI0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_spi0_device = {
	.name		= "atmel_spi",
	.id		= 0,
	.dev		= {
				.dma_mask		= &spi_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.resource	= spi0_resources,
	.num_resources	= ARRAY_SIZE(spi0_resources),
};

static const unsigned spi0_standard_cs[4] = { AT91_PIN_PA3, AT91_PIN_PA4, AT91_PIN_PA5, AT91_PIN_PA6 };

static struct resource spi1_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_SPI1,
		.end	= AT91SAM9261_BASE_SPI1 + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_SPI1,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_SPI1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_spi1_device = {
	.name		= "atmel_spi",
	.id		= 1,
	.dev		= {
				.dma_mask		= &spi_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.resource	= spi1_resources,
	.num_resources	= ARRAY_SIZE(spi1_resources),
};

static const unsigned spi1_standard_cs[4] = { AT91_PIN_PB28, AT91_PIN_PA24, AT91_PIN_PA25, AT91_PIN_PA26 };

void __init at91_add_device_spi(struct spi_board_info *devices, int nr_devices)
{
	int i;
	unsigned long cs_pin;
	short enable_spi0 = 0;
	short enable_spi1 = 0;

	/* Choose SPI chip-selects */
	for (i = 0; i < nr_devices; i++) {
		if (devices[i].controller_data)
			cs_pin = (unsigned long) devices[i].controller_data;
		else if (devices[i].bus_num == 0)
			cs_pin = spi0_standard_cs[devices[i].chip_select];
		else
			cs_pin = spi1_standard_cs[devices[i].chip_select];

		if (!gpio_is_valid(cs_pin))
			continue;

		if (devices[i].bus_num == 0)
			enable_spi0 = 1;
		else
			enable_spi1 = 1;

		/* enable chip-select pin */
		at91_set_gpio_output(cs_pin, 1);

		/* pass chip-select pin to driver */
		devices[i].controller_data = (void *) cs_pin;
	}

	spi_register_board_info(devices, nr_devices);

	/* Configure SPI bus(es) */
	if (enable_spi0) {
		at91_set_A_periph(AT91_PIN_PA0, 0);	/* SPI0_MISO */
		at91_set_A_periph(AT91_PIN_PA1, 0);	/* SPI0_MOSI */
		at91_set_A_periph(AT91_PIN_PA2, 0);	/* SPI0_SPCK */

		platform_device_register(&at91sam9261_spi0_device);
	}
	if (enable_spi1) {
		at91_set_A_periph(AT91_PIN_PB30, 0);	/* SPI1_MISO */
		at91_set_A_periph(AT91_PIN_PB31, 0);	/* SPI1_MOSI */
		at91_set_A_periph(AT91_PIN_PB29, 0);	/* SPI1_SPCK */

		platform_device_register(&at91sam9261_spi1_device);
	}
}
#else
void __init at91_add_device_spi(struct spi_board_info *devices, int nr_devices) {}
#endif


/* --------------------------------------------------------------------
 *  LCD Controller
 * -------------------------------------------------------------------- */

#if defined(CONFIG_FB_ATMEL) || defined(CONFIG_FB_ATMEL_MODULE)
static u64 lcdc_dmamask = DMA_BIT_MASK(32);
static struct atmel_lcdfb_info lcdc_data;

static struct resource lcdc_resources[] = {
	[0] = {
		.start	= AT91SAM9261_LCDC_BASE,
		.end	= AT91SAM9261_LCDC_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_LCDC,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_LCDC,
		.flags	= IORESOURCE_IRQ,
	},
#if defined(CONFIG_FB_INTSRAM)
	[2] = {
		.start	= AT91SAM9261_SRAM_BASE,
		.end	= AT91SAM9261_SRAM_BASE + AT91SAM9261_SRAM_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
#endif
};

static struct platform_device at91_lcdc_device = {
	.id		= 0,
	.dev		= {
				.dma_mask		= &lcdc_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
				.platform_data		= &lcdc_data,
	},
	.resource	= lcdc_resources,
	.num_resources	= ARRAY_SIZE(lcdc_resources),
};

void __init at91_add_device_lcdc(struct atmel_lcdfb_info *data)
{
	if (!data) {
		return;
	}

	if (cpu_is_at91sam9g10())
		at91_lcdc_device.name = "at91sam9g10-lcdfb";
	else
		at91_lcdc_device.name = "at91sam9261-lcdfb";

#if defined(CONFIG_FB_ATMEL_STN)
	at91_set_A_periph(AT91_PIN_PB0, 0);     /* LCDVSYNC */
	at91_set_A_periph(AT91_PIN_PB1, 0);     /* LCDHSYNC */
	at91_set_A_periph(AT91_PIN_PB2, 0);     /* LCDDOTCK */
	at91_set_A_periph(AT91_PIN_PB3, 0);     /* LCDDEN */
	at91_set_A_periph(AT91_PIN_PB4, 0);     /* LCDCC */
	at91_set_A_periph(AT91_PIN_PB5, 0);     /* LCDD0 */
	at91_set_A_periph(AT91_PIN_PB6, 0);     /* LCDD1 */
	at91_set_A_periph(AT91_PIN_PB7, 0);     /* LCDD2 */
	at91_set_A_periph(AT91_PIN_PB8, 0);     /* LCDD3 */
#else
	at91_set_A_periph(AT91_PIN_PB1, 0);	/* LCDHSYNC */
	at91_set_A_periph(AT91_PIN_PB2, 0);	/* LCDDOTCK */
	at91_set_A_periph(AT91_PIN_PB3, 0);	/* LCDDEN */
	at91_set_A_periph(AT91_PIN_PB4, 0);	/* LCDCC */
	at91_set_A_periph(AT91_PIN_PB7, 0);	/* LCDD2 */
	at91_set_A_periph(AT91_PIN_PB8, 0);	/* LCDD3 */
	at91_set_A_periph(AT91_PIN_PB9, 0);	/* LCDD4 */
	at91_set_A_periph(AT91_PIN_PB10, 0);	/* LCDD5 */
	at91_set_A_periph(AT91_PIN_PB11, 0);	/* LCDD6 */
	at91_set_A_periph(AT91_PIN_PB12, 0);	/* LCDD7 */
	at91_set_A_periph(AT91_PIN_PB15, 0);	/* LCDD10 */
	at91_set_A_periph(AT91_PIN_PB16, 0);	/* LCDD11 */
	at91_set_A_periph(AT91_PIN_PB17, 0);	/* LCDD12 */
	at91_set_A_periph(AT91_PIN_PB18, 0);	/* LCDD13 */
	at91_set_A_periph(AT91_PIN_PB19, 0);	/* LCDD14 */
	at91_set_A_periph(AT91_PIN_PB20, 0);	/* LCDD15 */
	at91_set_B_periph(AT91_PIN_PB23, 0);	/* LCDD18 */
	at91_set_B_periph(AT91_PIN_PB24, 0);	/* LCDD19 */
	at91_set_B_periph(AT91_PIN_PB25, 0);	/* LCDD20 */
	at91_set_B_periph(AT91_PIN_PB26, 0);	/* LCDD21 */
	at91_set_B_periph(AT91_PIN_PB27, 0);	/* LCDD22 */
	at91_set_B_periph(AT91_PIN_PB28, 0);	/* LCDD23 */
#endif

	if (ARRAY_SIZE(lcdc_resources) > 2) {
		void __iomem *fb;
		struct resource *fb_res = &lcdc_resources[2];
		size_t fb_len = resource_size(fb_res);

		fb = ioremap(fb_res->start, fb_len);
		if (fb) {
			memset(fb, 0, fb_len);
			iounmap(fb);
		}
	}
	lcdc_data = *data;
	platform_device_register(&at91_lcdc_device);
}
#else
void __init at91_add_device_lcdc(struct atmel_lcdfb_info *data) {}
#endif


/* --------------------------------------------------------------------
 *  Timer/Counter block
 * -------------------------------------------------------------------- */

#ifdef CONFIG_ATMEL_TCLIB

static struct resource tcb_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_TCB0,
		.end	= AT91SAM9261_BASE_TCB0 + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_TC0,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_TC0,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_TC1,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_TC1,
		.flags	= IORESOURCE_IRQ,
	},
	[3] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_TC2,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_TC2,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_tcb_device = {
	.name		= "atmel_tcb",
	.id		= 0,
	.resource	= tcb_resources,
	.num_resources	= ARRAY_SIZE(tcb_resources),
};

static void __init at91_add_device_tc(void)
{
	platform_device_register(&at91sam9261_tcb_device);
}
#else
static void __init at91_add_device_tc(void) { }
#endif


/* --------------------------------------------------------------------
 *  RTT
 * -------------------------------------------------------------------- */

static struct resource rtt_resources[] = {
	{
		.start	= AT91SAM9261_BASE_RTT,
		.end	= AT91SAM9261_BASE_RTT + SZ_16 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.flags	= IORESOURCE_MEM,
	}, {
		.flags  = IORESOURCE_IRQ,
	}
};

static struct platform_device at91sam9261_rtt_device = {
	.name		= "at91_rtt",
	.id		= 0,
	.resource	= rtt_resources,
};

#if IS_ENABLED(CONFIG_RTC_DRV_AT91SAM9)
static void __init at91_add_device_rtt_rtc(void)
{
	at91sam9261_rtt_device.name = "rtc-at91sam9";
	/*
	 * The second resource is needed:
	 * GPBR will serve as the storage for RTC time offset
	 */
	at91sam9261_rtt_device.num_resources = 3;
	rtt_resources[1].start = AT91SAM9261_BASE_GPBR +
				 4 * CONFIG_RTC_DRV_AT91SAM9_GPBR;
	rtt_resources[1].end = rtt_resources[1].start + 3;
	rtt_resources[2].start = NR_IRQS_LEGACY + AT91_ID_SYS;
	rtt_resources[2].end = NR_IRQS_LEGACY + AT91_ID_SYS;
}
#else
static void __init at91_add_device_rtt_rtc(void)
{
	/* Only one resource is needed: RTT not used as RTC */
	at91sam9261_rtt_device.num_resources = 1;
}
#endif

static void __init at91_add_device_rtt(void)
{
	at91_add_device_rtt_rtc();
	platform_device_register(&at91sam9261_rtt_device);
}


/* --------------------------------------------------------------------
 *  Watchdog
 * -------------------------------------------------------------------- */

#if defined(CONFIG_AT91SAM9X_WATCHDOG) || defined(CONFIG_AT91SAM9X_WATCHDOG_MODULE)
static struct resource wdt_resources[] = {
	{
		.start	= AT91SAM9261_BASE_WDT,
		.end	= AT91SAM9261_BASE_WDT + SZ_16 - 1,
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device at91sam9261_wdt_device = {
	.name		= "at91_wdt",
	.id		= -1,
	.resource	= wdt_resources,
	.num_resources	= ARRAY_SIZE(wdt_resources),
};

static void __init at91_add_device_watchdog(void)
{
	platform_device_register(&at91sam9261_wdt_device);
}
#else
static void __init at91_add_device_watchdog(void) {}
#endif


/* --------------------------------------------------------------------
 *  SSC -- Synchronous Serial Controller
 * -------------------------------------------------------------------- */

#if defined(CONFIG_ATMEL_SSC) || defined(CONFIG_ATMEL_SSC_MODULE)
static u64 ssc0_dmamask = DMA_BIT_MASK(32);

static struct resource ssc0_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_SSC0,
		.end	= AT91SAM9261_BASE_SSC0 + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_SSC0,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_SSC0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_ssc0_device = {
	.name	= "at91rm9200_ssc",
	.id	= 0,
	.dev	= {
		.dma_mask		= &ssc0_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.resource	= ssc0_resources,
	.num_resources	= ARRAY_SIZE(ssc0_resources),
};

static inline void configure_ssc0_pins(unsigned pins)
{
	if (pins & ATMEL_SSC_TF)
		at91_set_A_periph(AT91_PIN_PB21, 1);
	if (pins & ATMEL_SSC_TK)
		at91_set_A_periph(AT91_PIN_PB22, 1);
	if (pins & ATMEL_SSC_TD)
		at91_set_A_periph(AT91_PIN_PB23, 1);
	if (pins & ATMEL_SSC_RD)
		at91_set_A_periph(AT91_PIN_PB24, 1);
	if (pins & ATMEL_SSC_RK)
		at91_set_A_periph(AT91_PIN_PB25, 1);
	if (pins & ATMEL_SSC_RF)
		at91_set_A_periph(AT91_PIN_PB26, 1);
}

static u64 ssc1_dmamask = DMA_BIT_MASK(32);

static struct resource ssc1_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_SSC1,
		.end	= AT91SAM9261_BASE_SSC1 + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_SSC1,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_SSC1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_ssc1_device = {
	.name	= "at91rm9200_ssc",
	.id	= 1,
	.dev	= {
		.dma_mask		= &ssc1_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.resource	= ssc1_resources,
	.num_resources	= ARRAY_SIZE(ssc1_resources),
};

static inline void configure_ssc1_pins(unsigned pins)
{
	if (pins & ATMEL_SSC_TF)
		at91_set_B_periph(AT91_PIN_PA17, 1);
	if (pins & ATMEL_SSC_TK)
		at91_set_B_periph(AT91_PIN_PA18, 1);
	if (pins & ATMEL_SSC_TD)
		at91_set_B_periph(AT91_PIN_PA19, 1);
	if (pins & ATMEL_SSC_RD)
		at91_set_B_periph(AT91_PIN_PA20, 1);
	if (pins & ATMEL_SSC_RK)
		at91_set_B_periph(AT91_PIN_PA21, 1);
	if (pins & ATMEL_SSC_RF)
		at91_set_B_periph(AT91_PIN_PA22, 1);
}

static u64 ssc2_dmamask = DMA_BIT_MASK(32);

static struct resource ssc2_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_SSC2,
		.end	= AT91SAM9261_BASE_SSC2 + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_SSC2,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_SSC2,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device at91sam9261_ssc2_device = {
	.name	= "at91rm9200_ssc",
	.id	= 2,
	.dev	= {
		.dma_mask		= &ssc2_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.resource	= ssc2_resources,
	.num_resources	= ARRAY_SIZE(ssc2_resources),
};

static inline void configure_ssc2_pins(unsigned pins)
{
	if (pins & ATMEL_SSC_TF)
		at91_set_B_periph(AT91_PIN_PC25, 1);
	if (pins & ATMEL_SSC_TK)
		at91_set_B_periph(AT91_PIN_PC26, 1);
	if (pins & ATMEL_SSC_TD)
		at91_set_B_periph(AT91_PIN_PC27, 1);
	if (pins & ATMEL_SSC_RD)
		at91_set_B_periph(AT91_PIN_PC28, 1);
	if (pins & ATMEL_SSC_RK)
		at91_set_B_periph(AT91_PIN_PC29, 1);
	if (pins & ATMEL_SSC_RF)
		at91_set_B_periph(AT91_PIN_PC30, 1);
}

/*
 * SSC controllers are accessed through library code, instead of any
 * kind of all-singing/all-dancing driver.  For example one could be
 * used by a particular I2S audio codec's driver, while another one
 * on the same system might be used by a custom data capture driver.
 */
void __init at91_add_device_ssc(unsigned id, unsigned pins)
{
	struct platform_device *pdev;

	/*
	 * NOTE: caller is responsible for passing information matching
	 * "pins" to whatever will be using each particular controller.
	 */
	switch (id) {
	case AT91SAM9261_ID_SSC0:
		pdev = &at91sam9261_ssc0_device;
		configure_ssc0_pins(pins);
		break;
	case AT91SAM9261_ID_SSC1:
		pdev = &at91sam9261_ssc1_device;
		configure_ssc1_pins(pins);
		break;
	case AT91SAM9261_ID_SSC2:
		pdev = &at91sam9261_ssc2_device;
		configure_ssc2_pins(pins);
		break;
	default:
		return;
	}

	platform_device_register(pdev);
}

#else
void __init at91_add_device_ssc(unsigned id, unsigned pins) {}
#endif


/* --------------------------------------------------------------------
 *  UART
 * -------------------------------------------------------------------- */

#if defined(CONFIG_SERIAL_ATMEL)
static struct resource dbgu_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_DBGU,
		.end	= AT91SAM9261_BASE_DBGU + SZ_512 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91_ID_SYS,
		.end	= NR_IRQS_LEGACY + AT91_ID_SYS,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct atmel_uart_data dbgu_data = {
	.use_dma_tx	= 0,
	.use_dma_rx	= 0,		/* DBGU not capable of receive DMA */
};

static u64 dbgu_dmamask = DMA_BIT_MASK(32);

static struct platform_device at91sam9261_dbgu_device = {
	.name		= "atmel_usart",
	.id		= 0,
	.dev		= {
				.dma_mask		= &dbgu_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
				.platform_data		= &dbgu_data,
	},
	.resource	= dbgu_resources,
	.num_resources	= ARRAY_SIZE(dbgu_resources),
};

static inline void configure_dbgu_pins(void)
{
	at91_set_A_periph(AT91_PIN_PA9, 0);		/* DRXD */
	at91_set_A_periph(AT91_PIN_PA10, 1);		/* DTXD */
}

static struct resource uart0_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_US0,
		.end	= AT91SAM9261_BASE_US0 + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_US0,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_US0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct atmel_uart_data uart0_data = {
	.use_dma_tx	= 1,
	.use_dma_rx	= 1,
};

static u64 uart0_dmamask = DMA_BIT_MASK(32);

static struct platform_device at91sam9261_uart0_device = {
	.name		= "atmel_usart",
	.id		= 1,
	.dev		= {
				.dma_mask		= &uart0_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
				.platform_data		= &uart0_data,
	},
	.resource	= uart0_resources,
	.num_resources	= ARRAY_SIZE(uart0_resources),
};

static inline void configure_usart0_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC8, 1);		/* TXD0 */
	at91_set_A_periph(AT91_PIN_PC9, 0);		/* RXD0 */

	if (pins & ATMEL_UART_RTS)
		at91_set_A_periph(AT91_PIN_PC10, 0);	/* RTS0 */
	if (pins & ATMEL_UART_CTS)
		at91_set_A_periph(AT91_PIN_PC11, 0);	/* CTS0 */
}

static struct resource uart1_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_US1,
		.end	= AT91SAM9261_BASE_US1 + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_US1,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_US1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct atmel_uart_data uart1_data = {
	.use_dma_tx	= 1,
	.use_dma_rx	= 1,
};

static u64 uart1_dmamask = DMA_BIT_MASK(32);

static struct platform_device at91sam9261_uart1_device = {
	.name		= "atmel_usart",
	.id		= 2,
	.dev		= {
				.dma_mask		= &uart1_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
				.platform_data		= &uart1_data,
	},
	.resource	= uart1_resources,
	.num_resources	= ARRAY_SIZE(uart1_resources),
};

static inline void configure_usart1_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC12, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PC13, 0);		/* RXD1 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA12, 0);	/* RTS1 */
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA13, 0);	/* CTS1 */
}

static struct resource uart2_resources[] = {
	[0] = {
		.start	= AT91SAM9261_BASE_US2,
		.end	= AT91SAM9261_BASE_US2 + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9261_ID_US2,
		.end	= NR_IRQS_LEGACY + AT91SAM9261_ID_US2,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct atmel_uart_data uart2_data = {
	.use_dma_tx	= 1,
	.use_dma_rx	= 1,
};

static u64 uart2_dmamask = DMA_BIT_MASK(32);

static struct platform_device at91sam9261_uart2_device = {
	.name		= "atmel_usart",
	.id		= 3,
	.dev		= {
				.dma_mask		= &uart2_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
				.platform_data		= &uart2_data,
	},
	.resource	= uart2_resources,
	.num_resources	= ARRAY_SIZE(uart2_resources),
};

static inline void configure_usart2_pins(unsigned pins)
{
	at91_set_A_periph(AT91_PIN_PC15, 0);		/* RXD2 */
	at91_set_A_periph(AT91_PIN_PC14, 1);		/* TXD2 */

	if (pins & ATMEL_UART_RTS)
		at91_set_B_periph(AT91_PIN_PA15, 0);	/* RTS2*/
	if (pins & ATMEL_UART_CTS)
		at91_set_B_periph(AT91_PIN_PA16, 0);	/* CTS2 */
}

static struct platform_device *__initdata at91_uarts[ATMEL_MAX_UART];	/* the UARTs to use */

void __init at91_register_uart(unsigned id, unsigned portnr, unsigned pins)
{
	struct platform_device *pdev;
	struct atmel_uart_data *pdata;

	switch (id) {
		case 0:		/* DBGU */
			pdev = &at91sam9261_dbgu_device;
			configure_dbgu_pins();
			break;
		case AT91SAM9261_ID_US0:
			pdev = &at91sam9261_uart0_device;
			configure_usart0_pins(pins);
			break;
		case AT91SAM9261_ID_US1:
			pdev = &at91sam9261_uart1_device;
			configure_usart1_pins(pins);
			break;
		case AT91SAM9261_ID_US2:
			pdev = &at91sam9261_uart2_device;
			configure_usart2_pins(pins);
			break;
		default:
			return;
	}
	pdata = pdev->dev.platform_data;
	pdata->num = portnr;		/* update to mapped ID */

	if (portnr < ATMEL_MAX_UART)
		at91_uarts[portnr] = pdev;
}

void __init at91_add_device_serial(void)
{
	int i;

	for (i = 0; i < ATMEL_MAX_UART; i++) {
		if (at91_uarts[i])
			platform_device_register(at91_uarts[i]);
	}
}
#else
void __init at91_register_uart(unsigned id, unsigned portnr, unsigned pins) {}
void __init at91_add_device_serial(void) {}
#endif


/* -------------------------------------------------------------------- */

/*
 * These devices are always present and don't need any board-specific
 * setup.
 */
static int __init at91_add_standard_devices(void)
{
	at91_add_device_rtt();
	at91_add_device_watchdog();
	at91_add_device_tc();
	return 0;
}

arch_initcall(at91_add_standard_devices);
