/*
 * fikus/arch/arm/mach-at91/board-carmeva.c
 *
 *  Copyright (c) 2005 Peer Georgi
 *  		       Conitec Datasystems
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fikus/types.h>
#include <fikus/gpio.h>
#include <fikus/init.h>
#include <fikus/mm.h>
#include <fikus/module.h>
#include <fikus/platform_device.h>

#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/irq.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>

#include "at91_aic.h"
#include "board.h"
#include "generic.h"


static void __init carmeva_init_early(void)
{
	/* Initialize processor: 20.000 MHz crystal */
	at91_initialize(20000000);
}

static struct macb_platform_data __initdata carmeva_eth_data = {
	.phy_irq_pin	= AT91_PIN_PC4,
	.is_rmii	= 1,
};

static struct at91_usbh_data __initdata carmeva_usbh_data = {
	.ports		= 2,
	.vbus_pin	= {-EINVAL, -EINVAL},
	.overcurrent_pin= {-EINVAL, -EINVAL},
};

static struct at91_udc_data __initdata carmeva_udc_data = {
	.vbus_pin	= AT91_PIN_PD12,
	.pullup_pin	= AT91_PIN_PD9,
};

/* FIXME: user dependent */
// static struct at91_cf_data __initdata carmeva_cf_data = {
//	.det_pin	= AT91_PIN_PB0,
//	.rst_pin	= AT91_PIN_PC5,
	// .irq_pin	= -EINVAL,
	// .vcc_pin	= -EINVAL,
// };

static struct mci_platform_data __initdata carmeva_mci0_data = {
	.slot[0] = {
		.bus_width	= 4,
		.detect_pin	= AT91_PIN_PB10,
		.wp_pin		= AT91_PIN_PC14,
	},
};

static struct spi_board_info carmeva_spi_devices[] = {
	{ /* DataFlash chip */
		.modalias = "mtd_dataflash",
		.chip_select  = 0,
		.max_speed_hz = 10 * 1000 * 1000,
	},
	{ /* User accessible spi - cs1 (250KHz) */
		.modalias = "spi-cs1",
		.chip_select  = 1,
		.max_speed_hz = 250 *  1000,
	},
	{ /* User accessible spi - cs2 (1MHz) */
		.modalias = "spi-cs2",
		.chip_select  = 2,
		.max_speed_hz = 1 * 1000 *  1000,
	},
	{ /* User accessible spi - cs3 (10MHz) */
		.modalias = "spi-cs3",
		.chip_select  = 3,
		.max_speed_hz = 10 * 1000 *  1000,
	},
};

static struct gpio_led carmeva_leds[] = {
	{ /* "user led 1", LED9 */
		.name			= "led9",
		.gpio			= AT91_PIN_PA21,
		.active_low		= 1,
		.default_trigger	= "heartbeat",
	},
	{ /* "user led 2", LED10 */
		.name			= "led10",
		.gpio			= AT91_PIN_PA25,
		.active_low		= 1,
	},
	{ /* "user led 3", LED11 */
		.name			= "led11",
		.gpio			= AT91_PIN_PA26,
		.active_low		= 1,
	},
	{ /* "user led 4", LED12 */
		.name			= "led12",
		.gpio			= AT91_PIN_PA18,
		.active_low		= 1,
	}
};

static void __init carmeva_board_init(void)
{
	/* Serial */
	/* DBGU on ttyS0. (Rx & Tx only) */
	at91_register_uart(0, 0, 0);

	/* USART1 on ttyS1. (Rx, Tx, CTS, RTS, DTR, DSR, DCD, RI) */
	at91_register_uart(AT91RM9200_ID_US1, 1, ATMEL_UART_CTS | ATMEL_UART_RTS
			   | ATMEL_UART_DTR | ATMEL_UART_DSR | ATMEL_UART_DCD
			   | ATMEL_UART_RI);
	at91_add_device_serial();
	/* Ethernet */
	at91_add_device_eth(&carmeva_eth_data);
	/* USB Host */
	at91_add_device_usbh(&carmeva_usbh_data);
	/* USB Device */
	at91_add_device_udc(&carmeva_udc_data);
	/* I2C */
	at91_add_device_i2c(NULL, 0);
	/* SPI */
	at91_add_device_spi(carmeva_spi_devices, ARRAY_SIZE(carmeva_spi_devices));
	/* Compact Flash */
//	at91_add_device_cf(&carmeva_cf_data);
	/* MMC */
	at91_add_device_mci(0, &carmeva_mci0_data);
	/* LEDs */
	at91_gpio_leds(carmeva_leds, ARRAY_SIZE(carmeva_leds));
}

MACHINE_START(CARMEVA, "Carmeva")
	/* Maintainer: Conitec Datasystems */
	.init_time	= at91rm9200_timer_init,
	.map_io		= at91_map_io,
	.handle_irq	= at91_aic_handle_irq,
	.init_early	= carmeva_init_early,
	.init_irq	= at91_init_irq_default,
	.init_machine	= carmeva_board_init,
MACHINE_END
