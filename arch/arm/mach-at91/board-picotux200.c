/*
 * fikus/arch/arm/mach-at91/board-picotux200.c
 *
 *  Copyright (C) 2005 SAN People
 *  Copyright (C) 2007 Kleinhenz Elektronik GmbH
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
#include <fikus/spi/spi.h>
#include <fikus/mtd/physmap.h>

#include <mach/hardware.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/irq.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/at91rm9200_mc.h>
#include <mach/at91_ramc.h>

#include "at91_aic.h"
#include "board.h"
#include "generic.h"


static void __init picotux200_init_early(void)
{
	/* Initialize processor: 18.432 MHz crystal */
	at91_initialize(18432000);
}

static struct macb_platform_data __initdata picotux200_eth_data = {
	.phy_irq_pin	= AT91_PIN_PC4,
	.is_rmii	= 1,
};

static struct at91_usbh_data __initdata picotux200_usbh_data = {
	.ports		= 1,
	.vbus_pin	= {-EINVAL, -EINVAL},
	.overcurrent_pin= {-EINVAL, -EINVAL},
};

static struct mci_platform_data __initdata picotux200_mci0_data = {
	.slot[0] = {
		.bus_width	= 4,
		.detect_pin	= AT91_PIN_PB27,
		.wp_pin		= AT91_PIN_PA17,
	},
};

#define PICOTUX200_FLASH_BASE	AT91_CHIPSELECT_0
#define PICOTUX200_FLASH_SIZE	SZ_4M

static struct physmap_flash_data picotux200_flash_data = {
	.width	= 2,
};

static struct resource picotux200_flash_resource = {
	.start		= PICOTUX200_FLASH_BASE,
	.end		= PICOTUX200_FLASH_BASE + PICOTUX200_FLASH_SIZE - 1,
	.flags		= IORESOURCE_MEM,
};

static struct platform_device picotux200_flash = {
	.name		= "physmap-flash",
	.id		= 0,
	.dev		= {
				.platform_data	= &picotux200_flash_data,
			},
	.resource	= &picotux200_flash_resource,
	.num_resources	= 1,
};

static void __init picotux200_board_init(void)
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
	at91_add_device_eth(&picotux200_eth_data);
	/* USB Host */
	at91_add_device_usbh(&picotux200_usbh_data);
	/* I2C */
	at91_add_device_i2c(NULL, 0);
	/* MMC */
	at91_set_gpio_output(AT91_PIN_PB22, 1);	/* this MMC card slot can optionally use SPI signaling (CS3). */
	at91_add_device_mci(0, &picotux200_mci0_data);
	/* NOR Flash */
	platform_device_register(&picotux200_flash);
}

MACHINE_START(PICOTUX2XX, "picotux 200")
	/* Maintainer: Kleinhenz Elektronik GmbH */
	.init_time	= at91rm9200_timer_init,
	.map_io		= at91_map_io,
	.handle_irq	= at91_aic_handle_irq,
	.init_early	= picotux200_init_early,
	.init_irq	= at91_init_irq_default,
	.init_machine	= picotux200_board_init,
MACHINE_END
