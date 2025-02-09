/*
 * r8a7779 processor support
 *
 * Copyright (C) 2011, 2013  Renesas Solutions Corp.
 * Copyright (C) 2011  Magnus Damm
 * Copyright (C) 2013  Cogent Embedded, Inc.
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
#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/interrupt.h>
#include <fikus/irq.h>
#include <fikus/irqchip.h>
#include <fikus/irqchip/arm-gic.h>
#include <fikus/of_platform.h>
#include <fikus/platform_data/gpio-rcar.h>
#include <fikus/platform_data/irq-renesas-intc-irqpin.h>
#include <fikus/platform_device.h>
#include <fikus/delay.h>
#include <fikus/input.h>
#include <fikus/io.h>
#include <fikus/serial_sci.h>
#include <fikus/sh_timer.h>
#include <fikus/dma-mapping.h>
#include <fikus/usb/otg.h>
#include <fikus/usb/hcd.h>
#include <fikus/usb/ehci_pdriver.h>
#include <fikus/usb/ohci_pdriver.h>
#include <fikus/pm_runtime.h>
#include <mach/irqs.h>
#include <mach/r8a7779.h>
#include <mach/common.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/hardware/cache-l2x0.h>

static struct map_desc r8a7779_io_desc[] __initdata = {
	/* 2M entity map for 0xf0000000 (MPCORE) */
	{
		.virtual	= 0xf0000000,
		.pfn		= __phys_to_pfn(0xf0000000),
		.length		= SZ_2M,
		.type		= MT_DEVICE_NONSHARED
	},
	/* 16M entity map for 0xfexxxxxx (DMAC-S/HPBREG/INTC2/LRAM/DBSC) */
	{
		.virtual	= 0xfe000000,
		.pfn		= __phys_to_pfn(0xfe000000),
		.length		= SZ_16M,
		.type		= MT_DEVICE_NONSHARED
	},
};

void __init r8a7779_map_io(void)
{
	iotable_init(r8a7779_io_desc, ARRAY_SIZE(r8a7779_io_desc));
}

/* IRQ */
#define INT2SMSKCR0 IOMEM(0xfe7822a0)
#define INT2SMSKCR1 IOMEM(0xfe7822a4)
#define INT2SMSKCR2 IOMEM(0xfe7822a8)
#define INT2SMSKCR3 IOMEM(0xfe7822ac)
#define INT2SMSKCR4 IOMEM(0xfe7822b0)

#define INT2NTSR0 IOMEM(0xfe700060)
#define INT2NTSR1 IOMEM(0xfe700064)

static struct renesas_intc_irqpin_config irqpin0_platform_data __initdata = {
	.irq_base = irq_pin(0), /* IRQ0 -> IRQ3 */
	.sense_bitfield_width = 2,
};

static struct resource irqpin0_resources[] __initdata = {
	DEFINE_RES_MEM(0xfe78001c, 4), /* ICR1 */
	DEFINE_RES_MEM(0xfe780010, 4), /* INTPRI */
	DEFINE_RES_MEM(0xfe780024, 4), /* INTREQ */
	DEFINE_RES_MEM(0xfe780044, 4), /* INTMSK0 */
	DEFINE_RES_MEM(0xfe780064, 4), /* INTMSKCLR0 */
	DEFINE_RES_IRQ(gic_spi(27)), /* IRQ0 */
	DEFINE_RES_IRQ(gic_spi(28)), /* IRQ1 */
	DEFINE_RES_IRQ(gic_spi(29)), /* IRQ2 */
	DEFINE_RES_IRQ(gic_spi(30)), /* IRQ3 */
};

void __init r8a7779_init_irq_extpin(int irlm)
{
	void __iomem *icr0 = ioremap_nocache(0xfe780000, PAGE_SIZE);
	u32 tmp;

	if (!icr0) {
		pr_warn("r8a7779: unable to setup external irq pin mode\n");
		return;
	}

	tmp = ioread32(icr0);
	if (irlm)
		tmp |= 1 << 23; /* IRQ0 -> IRQ3 as individual pins */
	else
		tmp &= ~(1 << 23); /* IRL mode - not supported */
	tmp |= (1 << 21); /* LVLMODE = 1 */
	iowrite32(tmp, icr0);
	iounmap(icr0);

	if (irlm)
		platform_device_register_resndata(
			&platform_bus, "renesas_intc_irqpin", -1,
			irqpin0_resources, ARRAY_SIZE(irqpin0_resources),
			&irqpin0_platform_data, sizeof(irqpin0_platform_data));
}

/* PFC/GPIO */
static struct resource r8a7779_pfc_resources[] = {
	DEFINE_RES_MEM(0xfffc0000, 0x023c),
};

static struct platform_device r8a7779_pfc_device = {
	.name		= "pfc-r8a7779",
	.id		= -1,
	.resource	= r8a7779_pfc_resources,
	.num_resources	= ARRAY_SIZE(r8a7779_pfc_resources),
};

#define R8A7779_GPIO(idx, npins) \
static struct resource r8a7779_gpio##idx##_resources[] = {		\
	DEFINE_RES_MEM(0xffc40000 + (0x1000 * (idx)), 0x002c),		\
	DEFINE_RES_IRQ(gic_iid(0xad + (idx))),				\
};									\
									\
static struct gpio_rcar_config r8a7779_gpio##idx##_platform_data = {	\
	.gpio_base	= 32 * (idx),					\
	.irq_base	= 0,						\
	.number_of_pins	= npins,					\
	.pctl_name	= "pfc-r8a7779",				\
};									\
									\
static struct platform_device r8a7779_gpio##idx##_device = {		\
	.name		= "gpio_rcar",					\
	.id		= idx,						\
	.resource	= r8a7779_gpio##idx##_resources,		\
	.num_resources	= ARRAY_SIZE(r8a7779_gpio##idx##_resources),	\
	.dev		= {						\
		.platform_data	= &r8a7779_gpio##idx##_platform_data,	\
	},								\
}

R8A7779_GPIO(0, 32);
R8A7779_GPIO(1, 32);
R8A7779_GPIO(2, 32);
R8A7779_GPIO(3, 32);
R8A7779_GPIO(4, 32);
R8A7779_GPIO(5, 32);
R8A7779_GPIO(6, 9);

static struct platform_device *r8a7779_pinctrl_devices[] __initdata = {
	&r8a7779_pfc_device,
	&r8a7779_gpio0_device,
	&r8a7779_gpio1_device,
	&r8a7779_gpio2_device,
	&r8a7779_gpio3_device,
	&r8a7779_gpio4_device,
	&r8a7779_gpio5_device,
	&r8a7779_gpio6_device,
};

void __init r8a7779_pinmux_init(void)
{
	platform_add_devices(r8a7779_pinctrl_devices,
			    ARRAY_SIZE(r8a7779_pinctrl_devices));
}

static struct plat_sci_port scif0_platform_data = {
	.mapbase	= 0xffe40000,
	.flags		= UPF_BOOT_AUTOCONF | UPF_IOREMAP,
	.scscr		= SCSCR_RE | SCSCR_TE | SCSCR_CKE1,
	.scbrr_algo_id	= SCBRR_ALGO_2,
	.type		= PORT_SCIF,
	.irqs		= SCIx_IRQ_MUXED(gic_iid(0x78)),
};

static struct platform_device scif0_device = {
	.name		= "sh-sci",
	.id		= 0,
	.dev		= {
		.platform_data	= &scif0_platform_data,
	},
};

static struct plat_sci_port scif1_platform_data = {
	.mapbase	= 0xffe41000,
	.flags		= UPF_BOOT_AUTOCONF | UPF_IOREMAP,
	.scscr		= SCSCR_RE | SCSCR_TE | SCSCR_CKE1,
	.scbrr_algo_id	= SCBRR_ALGO_2,
	.type		= PORT_SCIF,
	.irqs		= SCIx_IRQ_MUXED(gic_iid(0x79)),
};

static struct platform_device scif1_device = {
	.name		= "sh-sci",
	.id		= 1,
	.dev		= {
		.platform_data	= &scif1_platform_data,
	},
};

static struct plat_sci_port scif2_platform_data = {
	.mapbase	= 0xffe42000,
	.flags		= UPF_BOOT_AUTOCONF | UPF_IOREMAP,
	.scscr		= SCSCR_RE | SCSCR_TE | SCSCR_CKE1,
	.scbrr_algo_id	= SCBRR_ALGO_2,
	.type		= PORT_SCIF,
	.irqs		= SCIx_IRQ_MUXED(gic_iid(0x7a)),
};

static struct platform_device scif2_device = {
	.name		= "sh-sci",
	.id		= 2,
	.dev		= {
		.platform_data	= &scif2_platform_data,
	},
};

static struct plat_sci_port scif3_platform_data = {
	.mapbase	= 0xffe43000,
	.flags		= UPF_BOOT_AUTOCONF | UPF_IOREMAP,
	.scscr		= SCSCR_RE | SCSCR_TE | SCSCR_CKE1,
	.scbrr_algo_id	= SCBRR_ALGO_2,
	.type		= PORT_SCIF,
	.irqs		= SCIx_IRQ_MUXED(gic_iid(0x7b)),
};

static struct platform_device scif3_device = {
	.name		= "sh-sci",
	.id		= 3,
	.dev		= {
		.platform_data	= &scif3_platform_data,
	},
};

static struct plat_sci_port scif4_platform_data = {
	.mapbase	= 0xffe44000,
	.flags		= UPF_BOOT_AUTOCONF | UPF_IOREMAP,
	.scscr		= SCSCR_RE | SCSCR_TE | SCSCR_CKE1,
	.scbrr_algo_id	= SCBRR_ALGO_2,
	.type		= PORT_SCIF,
	.irqs		= SCIx_IRQ_MUXED(gic_iid(0x7c)),
};

static struct platform_device scif4_device = {
	.name		= "sh-sci",
	.id		= 4,
	.dev		= {
		.platform_data	= &scif4_platform_data,
	},
};

static struct plat_sci_port scif5_platform_data = {
	.mapbase	= 0xffe45000,
	.flags		= UPF_BOOT_AUTOCONF | UPF_IOREMAP,
	.scscr		= SCSCR_RE | SCSCR_TE | SCSCR_CKE1,
	.scbrr_algo_id	= SCBRR_ALGO_2,
	.type		= PORT_SCIF,
	.irqs		= SCIx_IRQ_MUXED(gic_iid(0x7d)),
};

static struct platform_device scif5_device = {
	.name		= "sh-sci",
	.id		= 5,
	.dev		= {
		.platform_data	= &scif5_platform_data,
	},
};

/* TMU */
static struct sh_timer_config tmu00_platform_data = {
	.name = "TMU00",
	.channel_offset = 0x4,
	.timer_bit = 0,
	.clockevent_rating = 200,
};

static struct resource tmu00_resources[] = {
	[0] = {
		.name	= "TMU00",
		.start	= 0xffd80008,
		.end	= 0xffd80013,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_iid(0x40),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device tmu00_device = {
	.name		= "sh_tmu",
	.id		= 0,
	.dev = {
		.platform_data	= &tmu00_platform_data,
	},
	.resource	= tmu00_resources,
	.num_resources	= ARRAY_SIZE(tmu00_resources),
};

static struct sh_timer_config tmu01_platform_data = {
	.name = "TMU01",
	.channel_offset = 0x10,
	.timer_bit = 1,
	.clocksource_rating = 200,
};

static struct resource tmu01_resources[] = {
	[0] = {
		.name	= "TMU01",
		.start	= 0xffd80014,
		.end	= 0xffd8001f,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_iid(0x41),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device tmu01_device = {
	.name		= "sh_tmu",
	.id		= 1,
	.dev = {
		.platform_data	= &tmu01_platform_data,
	},
	.resource	= tmu01_resources,
	.num_resources	= ARRAY_SIZE(tmu01_resources),
};

/* I2C */
static struct resource rcar_i2c0_res[] = {
	{
		.start  = 0xffc70000,
		.end    = 0xffc70fff,
		.flags  = IORESOURCE_MEM,
	}, {
		.start  = gic_iid(0x6f),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device i2c0_device = {
	.name		= "i2c-rcar",
	.id		= 0,
	.resource	= rcar_i2c0_res,
	.num_resources	= ARRAY_SIZE(rcar_i2c0_res),
};

static struct resource rcar_i2c1_res[] = {
	{
		.start  = 0xffc71000,
		.end    = 0xffc71fff,
		.flags  = IORESOURCE_MEM,
	}, {
		.start  = gic_iid(0x72),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device i2c1_device = {
	.name		= "i2c-rcar",
	.id		= 1,
	.resource	= rcar_i2c1_res,
	.num_resources	= ARRAY_SIZE(rcar_i2c1_res),
};

static struct resource rcar_i2c2_res[] = {
	{
		.start  = 0xffc72000,
		.end    = 0xffc72fff,
		.flags  = IORESOURCE_MEM,
	}, {
		.start  = gic_iid(0x70),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device i2c2_device = {
	.name		= "i2c-rcar",
	.id		= 2,
	.resource	= rcar_i2c2_res,
	.num_resources	= ARRAY_SIZE(rcar_i2c2_res),
};

static struct resource rcar_i2c3_res[] = {
	{
		.start  = 0xffc73000,
		.end    = 0xffc73fff,
		.flags  = IORESOURCE_MEM,
	}, {
		.start  = gic_iid(0x71),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device i2c3_device = {
	.name		= "i2c-rcar",
	.id		= 3,
	.resource	= rcar_i2c3_res,
	.num_resources	= ARRAY_SIZE(rcar_i2c3_res),
};

static struct resource sata_resources[] = {
	[0] = {
		.name	= "rcar-sata",
		.start	= 0xfc600000,
		.end	= 0xfc601fff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_iid(0x84),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device sata_device = {
	.name		= "sata_rcar",
	.id		= -1,
	.resource	= sata_resources,
	.num_resources	= ARRAY_SIZE(sata_resources),
	.dev		= {
		.dma_mask		= &sata_device.dev.coherent_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

/* USB */
static struct usb_phy *phy;

static int usb_power_on(struct platform_device *pdev)
{
	if (IS_ERR(phy))
		return PTR_ERR(phy);

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	usb_phy_init(phy);

	return 0;
}

static void usb_power_off(struct platform_device *pdev)
{
	if (IS_ERR(phy))
		return;

	usb_phy_shutdown(phy);

	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
}

static int ehci_init_internal_buffer(struct usb_hcd *hcd)
{
	/*
	 * Below are recommended values from the datasheet;
	 * see [USB :: Setting of EHCI Internal Buffer].
	 */
	/* EHCI IP internal buffer setting */
	iowrite32(0x00ff0040, hcd->regs + 0x0094);
	/* EHCI IP internal buffer enable */
	iowrite32(0x00000001, hcd->regs + 0x009C);

	return 0;
}

static struct usb_ehci_pdata ehcix_pdata = {
	.power_on	= usb_power_on,
	.power_off	= usb_power_off,
	.power_suspend	= usb_power_off,
	.pre_setup	= ehci_init_internal_buffer,
};

static struct resource ehci0_resources[] = {
	[0] = {
		.start	= 0xffe70000,
		.end	= 0xffe70400 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_iid(0x4c),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ehci0_device = {
	.name	= "ehci-platform",
	.id	= 0,
	.dev	= {
		.dma_mask		= &ehci0_device.dev.coherent_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
		.platform_data		= &ehcix_pdata,
	},
	.num_resources	= ARRAY_SIZE(ehci0_resources),
	.resource	= ehci0_resources,
};

static struct resource ehci1_resources[] = {
	[0] = {
		.start	= 0xfff70000,
		.end	= 0xfff70400 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_iid(0x4d),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ehci1_device = {
	.name	= "ehci-platform",
	.id	= 1,
	.dev	= {
		.dma_mask		= &ehci1_device.dev.coherent_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
		.platform_data		= &ehcix_pdata,
	},
	.num_resources	= ARRAY_SIZE(ehci1_resources),
	.resource	= ehci1_resources,
};

static struct usb_ohci_pdata ohcix_pdata = {
	.power_on	= usb_power_on,
	.power_off	= usb_power_off,
	.power_suspend	= usb_power_off,
};

static struct resource ohci0_resources[] = {
	[0] = {
		.start	= 0xffe70400,
		.end	= 0xffe70800 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_iid(0x4c),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ohci0_device = {
	.name	= "ohci-platform",
	.id	= 0,
	.dev	= {
		.dma_mask		= &ohci0_device.dev.coherent_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
		.platform_data		= &ohcix_pdata,
	},
	.num_resources	= ARRAY_SIZE(ohci0_resources),
	.resource	= ohci0_resources,
};

static struct resource ohci1_resources[] = {
	[0] = {
		.start	= 0xfff70400,
		.end	= 0xfff70800 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_iid(0x4d),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ohci1_device = {
	.name	= "ohci-platform",
	.id	= 1,
	.dev	= {
		.dma_mask		= &ohci1_device.dev.coherent_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
		.platform_data		= &ohcix_pdata,
	},
	.num_resources	= ARRAY_SIZE(ohci1_resources),
	.resource	= ohci1_resources,
};

/* Ether */
static struct resource ether_resources[] __initdata = {
	{
		.start	= 0xfde00000,
		.end	= 0xfde003ff,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= gic_iid(0xb4),
		.flags	= IORESOURCE_IRQ,
	},
};

#define R8A7779_VIN(idx) \
static struct resource vin##idx##_resources[] __initdata = {		\
	DEFINE_RES_MEM(0xffc50000 + 0x1000 * (idx), 0x1000),		\
	DEFINE_RES_IRQ(gic_iid(0x5f + (idx))),				\
};									\
									\
static struct platform_device_info vin##idx##_info __initdata = {	\
	.parent		= &platform_bus,				\
	.name		= "r8a7779-vin",				\
	.id		= idx,						\
	.res		= vin##idx##_resources,				\
	.num_res	= ARRAY_SIZE(vin##idx##_resources),		\
	.dma_mask	= DMA_BIT_MASK(32),				\
}

R8A7779_VIN(0);
R8A7779_VIN(1);
R8A7779_VIN(2);
R8A7779_VIN(3);

static struct platform_device_info *vin_info_table[] __initdata = {
	&vin0_info,
	&vin1_info,
	&vin2_info,
	&vin3_info,
};

static struct platform_device *r8a7779_devices_dt[] __initdata = {
	&scif0_device,
	&scif1_device,
	&scif2_device,
	&scif3_device,
	&scif4_device,
	&scif5_device,
	&tmu00_device,
	&tmu01_device,
};

static struct platform_device *r8a7779_standard_devices[] __initdata = {
	&i2c0_device,
	&i2c1_device,
	&i2c2_device,
	&i2c3_device,
	&sata_device,
};

void __init r8a7779_add_standard_devices(void)
{
#ifdef CONFIG_CACHE_L2X0
	/* Early BRESP enable, Shared attribute override enable, 64K*16way */
	l2x0_init(IOMEM(0xf0100000), 0x40470000, 0x82000fff);
#endif
	r8a7779_pm_init();

	r8a7779_init_pm_domains();

	platform_add_devices(r8a7779_devices_dt,
			    ARRAY_SIZE(r8a7779_devices_dt));
	platform_add_devices(r8a7779_standard_devices,
			    ARRAY_SIZE(r8a7779_standard_devices));
}

void __init r8a7779_add_ether_device(struct sh_eth_plat_data *pdata)
{
	platform_device_register_resndata(&platform_bus, "r8a777x-ether", -1,
					  ether_resources,
					  ARRAY_SIZE(ether_resources),
					  pdata, sizeof(*pdata));
}

void __init r8a7779_add_vin_device(int id, struct rcar_vin_platform_data *pdata)
{
	BUG_ON(id < 0 || id > 3);

	vin_info_table[id]->data = pdata;
	vin_info_table[id]->size_data = sizeof(*pdata);

	platform_device_register_full(vin_info_table[id]);
}

/* do nothing for !CONFIG_SMP or !CONFIG_HAVE_TWD */
void __init __weak r8a7779_register_twd(void) { }

void __init r8a7779_earlytimer_init(void)
{
	r8a7779_clock_init();
	r8a7779_register_twd();
	shmobile_earlytimer_init();
}

void __init r8a7779_add_early_devices(void)
{
	early_platform_add_devices(r8a7779_devices_dt,
				   ARRAY_SIZE(r8a7779_devices_dt));

	/* Early serial console setup is not included here due to
	 * memory map collisions. The SCIF serial ports in r8a7779
	 * are difficult to entity map 1:1 due to collision with the
	 * virtual memory range used by the coherent DMA code on ARM.
	 *
	 * Anyone wanting to debug early can remove UPF_IOREMAP from
	 * the sh-sci serial console platform data, adjust mapbase
	 * to a static M:N virt:phys mapping that needs to be added to
	 * the mappings passed with iotable_init() above.
	 *
	 * Then add a call to shmobile_setup_console() from this function.
	 *
	 * As a final step pass earlyprint=sh-sci.2,115200 on the kernel
	 * command line in case of the marzen board.
	 */
}

static struct platform_device *r8a7779_late_devices[] __initdata = {
	&ehci0_device,
	&ehci1_device,
	&ohci0_device,
	&ohci1_device,
};

void __init r8a7779_init_late(void)
{
	/* get USB PHY */
	phy = usb_get_phy(USB_PHY_TYPE_USB2);

	shmobile_init_late();
	platform_add_devices(r8a7779_late_devices,
			     ARRAY_SIZE(r8a7779_late_devices));
}

#ifdef CONFIG_USE_OF
static int r8a7779_set_wake(struct irq_data *data, unsigned int on)
{
	return 0; /* always allow wakeup */
}

void __init r8a7779_init_irq_dt(void)
{
	gic_arch_extn.irq_set_wake = r8a7779_set_wake;

	irqchip_init();

	/* route all interrupts to ARM */
	__raw_writel(0xffffffff, INT2NTSR0);
	__raw_writel(0x3fffffff, INT2NTSR1);

	/* unmask all known interrupts in INTCS2 */
	__raw_writel(0xfffffff0, INT2SMSKCR0);
	__raw_writel(0xfff7ffff, INT2SMSKCR1);
	__raw_writel(0xfffbffdf, INT2SMSKCR2);
	__raw_writel(0xbffffffc, INT2SMSKCR3);
	__raw_writel(0x003fee3f, INT2SMSKCR4);
}

void __init r8a7779_init_delay(void)
{
	shmobile_setup_delay(1000, 2, 4); /* Cortex-A9 @ 1000MHz */
}

void __init r8a7779_add_standard_devices_dt(void)
{
	/* clocks are setup late during boot in the case of DT */
	r8a7779_clock_init();

	platform_add_devices(r8a7779_devices_dt,
			     ARRAY_SIZE(r8a7779_devices_dt));
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static const char *r8a7779_compat_dt[] __initdata = {
	"renesas,r8a7779",
	NULL,
};

DT_MACHINE_START(R8A7779_DT, "Generic R8A7779 (Flattened Device Tree)")
	.map_io		= r8a7779_map_io,
	.init_early	= r8a7779_init_delay,
	.nr_irqs	= NR_IRQS_LEGACY,
	.init_irq	= r8a7779_init_irq_dt,
	.init_machine	= r8a7779_add_standard_devices_dt,
	.init_late	= r8a7779_init_late,
	.dt_compat	= r8a7779_compat_dt,
MACHINE_END
#endif /* CONFIG_USE_OF */
