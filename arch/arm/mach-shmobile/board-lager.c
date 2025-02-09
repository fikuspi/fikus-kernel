/*
 * Lager board support
 *
 * Copyright (C) 2013  Renesas Solutions Corp.
 * Copyright (C) 2013  Magnus Damm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <fikus/gpio.h>
#include <fikus/gpio_keys.h>
#include <fikus/input.h>
#include <fikus/interrupt.h>
#include <fikus/kernel.h>
#include <fikus/leds.h>
#include <fikus/mmc/host.h>
#include <fikus/mmc/sh_mmcif.h>
#include <fikus/pinctrl/machine.h>
#include <fikus/platform_data/gpio-rcar.h>
#include <fikus/platform_device.h>
#include <fikus/phy.h>
#include <fikus/regulator/fixed.h>
#include <fikus/regulator/machine.h>
#include <fikus/sh_eth.h>
#include <mach/common.h>
#include <mach/irqs.h>
#include <mach/r8a7790.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

/* LEDS */
static struct gpio_led lager_leds[] = {
	{
		.name		= "led8",
		.gpio		= RCAR_GP_PIN(5, 17),
		.default_state	= LEDS_GPIO_DEFSTATE_ON,
	}, {
		.name		= "led7",
		.gpio		= RCAR_GP_PIN(4, 23),
		.default_state	= LEDS_GPIO_DEFSTATE_ON,
	}, {
		.name		= "led6",
		.gpio		= RCAR_GP_PIN(4, 22),
		.default_state	= LEDS_GPIO_DEFSTATE_ON,
	},
};

static __initdata struct gpio_led_platform_data lager_leds_pdata = {
	.leds		= lager_leds,
	.num_leds	= ARRAY_SIZE(lager_leds),
};

/* GPIO KEY */
#define GPIO_KEY(c, g, d, ...) \
	{ .code = c, .gpio = g, .desc = d, .active_low = 1 }

static struct gpio_keys_button gpio_buttons[] = {
	GPIO_KEY(KEY_4,		RCAR_GP_PIN(1, 28),	"SW2-pin4"),
	GPIO_KEY(KEY_3,		RCAR_GP_PIN(1, 26),	"SW2-pin3"),
	GPIO_KEY(KEY_2,		RCAR_GP_PIN(1, 24),	"SW2-pin2"),
	GPIO_KEY(KEY_1,		RCAR_GP_PIN(1, 14),	"SW2-pin1"),
};

static __initdata struct gpio_keys_platform_data lager_keys_pdata = {
	.buttons	= gpio_buttons,
	.nbuttons	= ARRAY_SIZE(gpio_buttons),
};

/* Fixed 3.3V regulator to be used by MMCIF */
static struct regulator_consumer_supply fixed3v3_power_consumers[] =
{
	REGULATOR_SUPPLY("vmmc", "sh_mmcif.1"),
};

/* MMCIF */
static struct sh_mmcif_plat_data mmcif1_pdata __initdata = {
	.caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_NONREMOVABLE,
};

static struct resource mmcif1_resources[] __initdata = {
	DEFINE_RES_MEM_NAMED(0xee220000, 0x80, "MMCIF1"),
	DEFINE_RES_IRQ(gic_spi(170)),
};

/* Ether */
static struct sh_eth_plat_data ether_pdata __initdata = {
	.phy			= 0x1,
	.edmac_endian		= EDMAC_LITTLE_ENDIAN,
	.phy_interface		= PHY_INTERFACE_MODE_RMII,
	.ether_link_active_low	= 1,
};

static struct resource ether_resources[] __initdata = {
	DEFINE_RES_MEM(0xee700000, 0x400),
	DEFINE_RES_IRQ(gic_spi(162)),
};

static const struct pinctrl_map lager_pinctrl_map[] = {
	/* SCIF0 (CN19: DEBUG SERIAL0) */
	PIN_MAP_MUX_GROUP_DEFAULT("sh-sci.6", "pfc-r8a7790",
				  "scif0_data", "scif0"),
	/* SCIF1 (CN20: DEBUG SERIAL1) */
	PIN_MAP_MUX_GROUP_DEFAULT("sh-sci.7", "pfc-r8a7790",
				  "scif1_data", "scif1"),
	/* MMCIF1 */
	PIN_MAP_MUX_GROUP_DEFAULT("sh_mmcif.1", "pfc-r8a7790",
				  "mmc1_data8", "mmc1"),
	PIN_MAP_MUX_GROUP_DEFAULT("sh_mmcif.1", "pfc-r8a7790",
				  "mmc1_ctrl", "mmc1"),
	/* Ether */
	PIN_MAP_MUX_GROUP_DEFAULT("r8a7790-ether", "pfc-r8a7790",
				  "eth_link", "eth"),
	PIN_MAP_MUX_GROUP_DEFAULT("r8a7790-ether", "pfc-r8a7790",
				  "eth_mdio", "eth"),
	PIN_MAP_MUX_GROUP_DEFAULT("r8a7790-ether", "pfc-r8a7790",
				  "eth_rmii", "eth"),
	PIN_MAP_MUX_GROUP_DEFAULT("r8a7790-ether", "pfc-r8a7790",
				  "intc_irq0", "intc"),
};

static void __init lager_add_standard_devices(void)
{
	r8a7790_clock_init();

	pinctrl_register_mappings(lager_pinctrl_map,
				  ARRAY_SIZE(lager_pinctrl_map));
	r8a7790_pinmux_init();

	r8a7790_add_standard_devices();
	platform_device_register_data(&platform_bus, "leds-gpio", -1,
				      &lager_leds_pdata,
				      sizeof(lager_leds_pdata));
	platform_device_register_data(&platform_bus, "gpio-keys", -1,
				      &lager_keys_pdata,
				      sizeof(lager_keys_pdata));
	regulator_register_always_on(0, "fixed-3.3V", fixed3v3_power_consumers,
				     ARRAY_SIZE(fixed3v3_power_consumers), 3300000);
	platform_device_register_resndata(&platform_bus, "sh_mmcif", 1,
					  mmcif1_resources, ARRAY_SIZE(mmcif1_resources),
					  &mmcif1_pdata, sizeof(mmcif1_pdata));

	platform_device_register_resndata(&platform_bus, "r8a7790-ether", -1,
					  ether_resources,
					  ARRAY_SIZE(ether_resources),
					  &ether_pdata, sizeof(ether_pdata));
}

/*
 * Ether LEDs on the Lager board are named LINK and ACTIVE which corresponds
 * to non-default 01 setting of the Micrel KSZ8041 PHY control register 1 bits
 * 14-15. We have to set them back to 01 from the default 00 value each time
 * the PHY is reset. It's also important because the PHY's LED0 signal is
 * connected to SoC's ETH_LINK signal and in the PHY's default mode it will
 * bounce on and off after each packet, which we apparently want to avoid.
 */
static int lager_ksz8041_fixup(struct phy_device *phydev)
{
	u16 phyctrl1 = phy_read(phydev, 0x1e);

	phyctrl1 &= ~0xc000;
	phyctrl1 |= 0x4000;
	return phy_write(phydev, 0x1e, phyctrl1);
}

static void __init lager_init(void)
{
	lager_add_standard_devices();

	phy_register_fixup_for_id("r8a7790-ether-ff:01", lager_ksz8041_fixup);
}

static const char *lager_boards_compat_dt[] __initdata = {
	"renesas,lager",
	NULL,
};

DT_MACHINE_START(LAGER_DT, "lager")
	.init_early	= r8a7790_init_delay,
	.init_time	= r8a7790_timer_init,
	.init_machine	= lager_init,
	.dt_compat	= lager_boards_compat_dt,
MACHINE_END
