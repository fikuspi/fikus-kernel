/* fikus/arch/arm/plat-samsung/devs.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Base SAMSUNG platform device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <fikus/amba/pl330.h>
#include <fikus/kernel.h>
#include <fikus/types.h>
#include <fikus/interrupt.h>
#include <fikus/list.h>
#include <fikus/timer.h>
#include <fikus/init.h>
#include <fikus/serial_core.h>
#include <fikus/platform_device.h>
#include <fikus/io.h>
#include <fikus/slab.h>
#include <fikus/string.h>
#include <fikus/dma-mapping.h>
#include <fikus/fb.h>
#include <fikus/gfp.h>
#include <fikus/mtd/mtd.h>
#include <fikus/mtd/onenand.h>
#include <fikus/mtd/partitions.h>
#include <fikus/mmc/host.h>
#include <fikus/ioport.h>
#include <fikus/platform_data/s3c-hsudc.h>
#include <fikus/platform_data/s3c-hsotg.h>

#include <media/s5p_hdmi.h>

#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/dma.h>
#include <mach/irqs.h>
#include <mach/map.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/adc.h>
#include <fikus/platform_data/ata-samsung_cf.h>
#include <fikus/platform_data/usb-ehci-s5p.h>
#include <plat/fb.h>
#include <plat/fb-s3c2410.h>
#include <plat/hdmi.h>
#include <fikus/platform_data/hwmon-s3c.h>
#include <fikus/platform_data/i2c-s3c2410.h>
#include <plat/keypad.h>
#include <fikus/platform_data/mmc-s3cmci.h>
#include <fikus/platform_data/mtd-nand-s3c2410.h>
#include <plat/pwm-core.h>
#include <plat/sdhci.h>
#include <fikus/platform_data/touchscreen-s3c2410.h>
#include <fikus/platform_data/usb-s3c2410_udc.h>
#include <fikus/platform_data/usb-ohci-s3c2410.h>
#include <plat/usb-phy.h>
#include <plat/regs-serial.h>
#include <plat/regs-spi.h>
#include <fikus/platform_data/spi-s3c64xx.h>

static u64 samsung_device_dma_mask = DMA_BIT_MASK(32);

/* AC97 */
#ifdef CONFIG_CPU_S3C2440
static struct resource s3c_ac97_resource[] = {
	[0] = DEFINE_RES_MEM(S3C2440_PA_AC97, S3C2440_SZ_AC97),
	[1] = DEFINE_RES_IRQ(IRQ_S3C244X_AC97),
	[2] = DEFINE_RES_DMA_NAMED(DMACH_PCM_OUT, "PCM out"),
	[3] = DEFINE_RES_DMA_NAMED(DMACH_PCM_IN, "PCM in"),
	[4] = DEFINE_RES_DMA_NAMED(DMACH_MIC_IN, "Mic in"),
};

struct platform_device s3c_device_ac97 = {
	.name		= "samsung-ac97",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_ac97_resource),
	.resource	= s3c_ac97_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif /* CONFIG_CPU_S3C2440 */

/* ADC */

#ifdef CONFIG_PLAT_S3C24XX
static struct resource s3c_adc_resource[] = {
	[0] = DEFINE_RES_MEM(S3C24XX_PA_ADC, S3C24XX_SZ_ADC),
	[1] = DEFINE_RES_IRQ(IRQ_TC),
	[2] = DEFINE_RES_IRQ(IRQ_ADC),
};

struct platform_device s3c_device_adc = {
	.name		= "s3c24xx-adc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_adc_resource),
	.resource	= s3c_adc_resource,
};
#endif /* CONFIG_PLAT_S3C24XX */

#if defined(CONFIG_SAMSUNG_DEV_ADC)
static struct resource s3c_adc_resource[] = {
	[0] = DEFINE_RES_MEM(SAMSUNG_PA_ADC, SZ_256),
	[1] = DEFINE_RES_IRQ(IRQ_TC),
	[2] = DEFINE_RES_IRQ(IRQ_ADC),
};

struct platform_device s3c_device_adc = {
	.name		= "samsung-adc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_adc_resource),
	.resource	= s3c_adc_resource,
};
#endif /* CONFIG_SAMSUNG_DEV_ADC */

/* Camif Controller */

#ifdef CONFIG_CPU_S3C2440
static struct resource s3c_camif_resource[] = {
	[0] = DEFINE_RES_MEM(S3C2440_PA_CAMIF, S3C2440_SZ_CAMIF),
	[1] = DEFINE_RES_IRQ(IRQ_S3C2440_CAM_C),
	[2] = DEFINE_RES_IRQ(IRQ_S3C2440_CAM_P),
};

struct platform_device s3c_device_camif = {
	.name		= "s3c2440-camif",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_camif_resource),
	.resource	= s3c_camif_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif /* CONFIG_CPU_S3C2440 */

/* ASOC DMA */

#ifdef CONFIG_PLAT_S5P 
static struct resource samsung_asoc_idma_resource = DEFINE_RES_IRQ(IRQ_I2S0);

struct platform_device samsung_asoc_idma = {
	.name		= "samsung-idma",
	.id		= -1,
	.num_resources	= 1,
	.resource	= &samsung_asoc_idma_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif

/* FB */

#ifdef CONFIG_S3C_DEV_FB
static struct resource s3c_fb_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_FB, SZ_16K),
	[1] = DEFINE_RES_IRQ(IRQ_LCD_VSYNC),
	[2] = DEFINE_RES_IRQ(IRQ_LCD_FIFO),
	[3] = DEFINE_RES_IRQ(IRQ_LCD_SYSTEM),
};

struct platform_device s3c_device_fb = {
	.name		= "s3c-fb",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_fb_resource),
	.resource	= s3c_fb_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

void __init s3c_fb_set_platdata(struct s3c_fb_platdata *pd)
{
	s3c_set_platdata(pd, sizeof(struct s3c_fb_platdata),
			 &s3c_device_fb);
}
#endif /* CONFIG_S3C_DEV_FB */

/* FIMC */

#ifdef CONFIG_S5P_DEV_FIMC0
static struct resource s5p_fimc0_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_FIMC0, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_FIMC0),
};

struct platform_device s5p_device_fimc0 = {
	.name		= "s5p-fimc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s5p_fimc0_resource),
	.resource	= s5p_fimc0_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

struct platform_device s5p_device_fimc_md = {
	.name	= "s5p-fimc-md",
	.id	= -1,
};
#endif /* CONFIG_S5P_DEV_FIMC0 */

#ifdef CONFIG_S5P_DEV_FIMC1
static struct resource s5p_fimc1_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_FIMC1, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_FIMC1),
};

struct platform_device s5p_device_fimc1 = {
	.name		= "s5p-fimc",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s5p_fimc1_resource),
	.resource	= s5p_fimc1_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};
#endif /* CONFIG_S5P_DEV_FIMC1 */

#ifdef CONFIG_S5P_DEV_FIMC2
static struct resource s5p_fimc2_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_FIMC2, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_FIMC2),
};

struct platform_device s5p_device_fimc2 = {
	.name		= "s5p-fimc",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s5p_fimc2_resource),
	.resource	= s5p_fimc2_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};
#endif /* CONFIG_S5P_DEV_FIMC2 */

#ifdef CONFIG_S5P_DEV_FIMC3
static struct resource s5p_fimc3_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_FIMC3, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_FIMC3),
};

struct platform_device s5p_device_fimc3 = {
	.name		= "s5p-fimc",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(s5p_fimc3_resource),
	.resource	= s5p_fimc3_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};
#endif /* CONFIG_S5P_DEV_FIMC3 */

/* G2D */

#ifdef CONFIG_S5P_DEV_G2D
static struct resource s5p_g2d_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_G2D, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_2D),
};

struct platform_device s5p_device_g2d = {
	.name		= "s5p-g2d",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s5p_g2d_resource),
	.resource	= s5p_g2d_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};
#endif /* CONFIG_S5P_DEV_G2D */

#ifdef CONFIG_S5P_DEV_JPEG
static struct resource s5p_jpeg_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_JPEG, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_JPEG),
};

struct platform_device s5p_device_jpeg = {
	.name		= "s5p-jpeg",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s5p_jpeg_resource),
	.resource	= s5p_jpeg_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};
#endif /*  CONFIG_S5P_DEV_JPEG */

/* FIMD0 */

#ifdef CONFIG_S5P_DEV_FIMD0
static struct resource s5p_fimd0_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_FIMD0, SZ_32K),
	[1] = DEFINE_RES_IRQ_NAMED(IRQ_FIMD0_VSYNC, "vsync"),
	[2] = DEFINE_RES_IRQ_NAMED(IRQ_FIMD0_FIFO, "fifo"),
	[3] = DEFINE_RES_IRQ_NAMED(IRQ_FIMD0_SYSTEM, "lcd_sys"),
};

struct platform_device s5p_device_fimd0 = {
	.name		= "s5p-fb",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s5p_fimd0_resource),
	.resource	= s5p_fimd0_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

void __init s5p_fimd0_set_platdata(struct s3c_fb_platdata *pd)
{
	s3c_set_platdata(pd, sizeof(struct s3c_fb_platdata),
			 &s5p_device_fimd0);
}
#endif /* CONFIG_S5P_DEV_FIMD0 */

/* HWMON */

#ifdef CONFIG_S3C_DEV_HWMON
struct platform_device s3c_device_hwmon = {
	.name		= "s3c-hwmon",
	.id		= -1,
	.dev.parent	= &s3c_device_adc.dev,
};

void __init s3c_hwmon_set_platdata(struct s3c_hwmon_pdata *pd)
{
	s3c_set_platdata(pd, sizeof(struct s3c_hwmon_pdata),
			 &s3c_device_hwmon);
}
#endif /* CONFIG_S3C_DEV_HWMON */

/* HSMMC */

#ifdef CONFIG_S3C_DEV_HSMMC
static struct resource s3c_hsmmc_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_HSMMC0, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_HSMMC0),
};

struct s3c_sdhci_platdata s3c_hsmmc0_def_platdata = {
	.max_width	= 4,
	.host_caps	= (MMC_CAP_4_BIT_DATA |
			   MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED),
};

struct platform_device s3c_device_hsmmc0 = {
	.name		= "s3c-sdhci",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_hsmmc_resource),
	.resource	= s3c_hsmmc_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &s3c_hsmmc0_def_platdata,
	},
};

void s3c_sdhci0_set_platdata(struct s3c_sdhci_platdata *pd)
{
	s3c_sdhci_set_platdata(pd, &s3c_hsmmc0_def_platdata);
}
#endif /* CONFIG_S3C_DEV_HSMMC */

#ifdef CONFIG_S3C_DEV_HSMMC1
static struct resource s3c_hsmmc1_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_HSMMC1, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_HSMMC1),
};

struct s3c_sdhci_platdata s3c_hsmmc1_def_platdata = {
	.max_width	= 4,
	.host_caps	= (MMC_CAP_4_BIT_DATA |
			   MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED),
};

struct platform_device s3c_device_hsmmc1 = {
	.name		= "s3c-sdhci",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_hsmmc1_resource),
	.resource	= s3c_hsmmc1_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &s3c_hsmmc1_def_platdata,
	},
};

void s3c_sdhci1_set_platdata(struct s3c_sdhci_platdata *pd)
{
	s3c_sdhci_set_platdata(pd, &s3c_hsmmc1_def_platdata);
}
#endif /* CONFIG_S3C_DEV_HSMMC1 */

/* HSMMC2 */

#ifdef CONFIG_S3C_DEV_HSMMC2
static struct resource s3c_hsmmc2_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_HSMMC2, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_HSMMC2),
};

struct s3c_sdhci_platdata s3c_hsmmc2_def_platdata = {
	.max_width	= 4,
	.host_caps	= (MMC_CAP_4_BIT_DATA |
			   MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED),
};

struct platform_device s3c_device_hsmmc2 = {
	.name		= "s3c-sdhci",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c_hsmmc2_resource),
	.resource	= s3c_hsmmc2_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &s3c_hsmmc2_def_platdata,
	},
};

void s3c_sdhci2_set_platdata(struct s3c_sdhci_platdata *pd)
{
	s3c_sdhci_set_platdata(pd, &s3c_hsmmc2_def_platdata);
}
#endif /* CONFIG_S3C_DEV_HSMMC2 */

#ifdef CONFIG_S3C_DEV_HSMMC3
static struct resource s3c_hsmmc3_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_HSMMC3, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_HSMMC3),
};

struct s3c_sdhci_platdata s3c_hsmmc3_def_platdata = {
	.max_width	= 4,
	.host_caps	= (MMC_CAP_4_BIT_DATA |
			   MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED),
};

struct platform_device s3c_device_hsmmc3 = {
	.name		= "s3c-sdhci",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(s3c_hsmmc3_resource),
	.resource	= s3c_hsmmc3_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &s3c_hsmmc3_def_platdata,
	},
};

void s3c_sdhci3_set_platdata(struct s3c_sdhci_platdata *pd)
{
	s3c_sdhci_set_platdata(pd, &s3c_hsmmc3_def_platdata);
}
#endif /* CONFIG_S3C_DEV_HSMMC3 */

/* I2C */

static struct resource s3c_i2c0_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_IIC, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_IIC),
};

struct platform_device s3c_device_i2c0 = {
	.name		= "s3c2410-i2c",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_i2c0_resource),
	.resource	= s3c_i2c0_resource,
};

struct s3c2410_platform_i2c default_i2c_data __initdata = {
	.flags		= 0,
	.slave_addr	= 0x10,
	.frequency	= 100*1000,
	.sda_delay	= 100,
};

void __init s3c_i2c0_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;
		pd->bus_num = 0;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c0);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c0_cfg_gpio;
}

#ifdef CONFIG_S3C_DEV_I2C1
static struct resource s3c_i2c1_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_IIC1, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_IIC1),
};

struct platform_device s3c_device_i2c1 = {
	.name		= "s3c2410-i2c",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_i2c1_resource),
	.resource	= s3c_i2c1_resource,
};

void __init s3c_i2c1_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;
		pd->bus_num = 1;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c1);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c1_cfg_gpio;
}
#endif /* CONFIG_S3C_DEV_I2C1 */

#ifdef CONFIG_S3C_DEV_I2C2
static struct resource s3c_i2c2_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_IIC2, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_IIC2),
};

struct platform_device s3c_device_i2c2 = {
	.name		= "s3c2410-i2c",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c_i2c2_resource),
	.resource	= s3c_i2c2_resource,
};

void __init s3c_i2c2_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;
		pd->bus_num = 2;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c2);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c2_cfg_gpio;
}
#endif /* CONFIG_S3C_DEV_I2C2 */

#ifdef CONFIG_S3C_DEV_I2C3
static struct resource s3c_i2c3_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_IIC3, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_IIC3),
};

struct platform_device s3c_device_i2c3 = {
	.name		= "s3c2440-i2c",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(s3c_i2c3_resource),
	.resource	= s3c_i2c3_resource,
};

void __init s3c_i2c3_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;
		pd->bus_num = 3;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c3);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c3_cfg_gpio;
}
#endif /*CONFIG_S3C_DEV_I2C3 */

#ifdef CONFIG_S3C_DEV_I2C4
static struct resource s3c_i2c4_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_IIC4, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_IIC4),
};

struct platform_device s3c_device_i2c4 = {
	.name		= "s3c2440-i2c",
	.id		= 4,
	.num_resources	= ARRAY_SIZE(s3c_i2c4_resource),
	.resource	= s3c_i2c4_resource,
};

void __init s3c_i2c4_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;
		pd->bus_num = 4;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c4);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c4_cfg_gpio;
}
#endif /*CONFIG_S3C_DEV_I2C4 */

#ifdef CONFIG_S3C_DEV_I2C5
static struct resource s3c_i2c5_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_IIC5, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_IIC5),
};

struct platform_device s3c_device_i2c5 = {
	.name		= "s3c2440-i2c",
	.id		= 5,
	.num_resources	= ARRAY_SIZE(s3c_i2c5_resource),
	.resource	= s3c_i2c5_resource,
};

void __init s3c_i2c5_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;
		pd->bus_num = 5;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c5);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c5_cfg_gpio;
}
#endif /*CONFIG_S3C_DEV_I2C5 */

#ifdef CONFIG_S3C_DEV_I2C6
static struct resource s3c_i2c6_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_IIC6, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_IIC6),
};

struct platform_device s3c_device_i2c6 = {
	.name		= "s3c2440-i2c",
	.id		= 6,
	.num_resources	= ARRAY_SIZE(s3c_i2c6_resource),
	.resource	= s3c_i2c6_resource,
};

void __init s3c_i2c6_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;
		pd->bus_num = 6;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c6);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c6_cfg_gpio;
}
#endif /* CONFIG_S3C_DEV_I2C6 */

#ifdef CONFIG_S3C_DEV_I2C7
static struct resource s3c_i2c7_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_IIC7, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_IIC7),
};

struct platform_device s3c_device_i2c7 = {
	.name		= "s3c2440-i2c",
	.id		= 7,
	.num_resources	= ARRAY_SIZE(s3c_i2c7_resource),
	.resource	= s3c_i2c7_resource,
};

void __init s3c_i2c7_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;
		pd->bus_num = 7;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c7);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c7_cfg_gpio;
}
#endif /* CONFIG_S3C_DEV_I2C7 */

/* I2C HDMIPHY */

#ifdef CONFIG_S5P_DEV_I2C_HDMIPHY
static struct resource s5p_i2c_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_IIC_HDMIPHY, SZ_4K),
	[1] = DEFINE_RES_IRQ(IRQ_IIC_HDMIPHY),
};

struct platform_device s5p_device_i2c_hdmiphy = {
	.name		= "s3c2440-hdmiphy-i2c",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_i2c_resource),
	.resource	= s5p_i2c_resource,
};

void __init s5p_i2c_hdmiphy_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;

		if (soc_is_exynos4210() ||
		    soc_is_exynos4212() || soc_is_exynos4412())
			pd->bus_num = 8;
		else if (soc_is_s5pv210())
			pd->bus_num = 3;
		else
			pd->bus_num = 0;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s5p_device_i2c_hdmiphy);
}

static struct s5p_hdmi_platform_data s5p_hdmi_def_platdata;

void __init s5p_hdmi_set_platdata(struct i2c_board_info *hdmiphy_info,
				  struct i2c_board_info *mhl_info, int mhl_bus)
{
	struct s5p_hdmi_platform_data *pd = &s5p_hdmi_def_platdata;

	if (soc_is_exynos4210() ||
	    soc_is_exynos4212() || soc_is_exynos4412())
		pd->hdmiphy_bus = 8;
	else if (soc_is_s5pv210())
		pd->hdmiphy_bus = 3;
	else
		pd->hdmiphy_bus = 0;

	pd->hdmiphy_info = hdmiphy_info;
	pd->mhl_info = mhl_info;
	pd->mhl_bus = mhl_bus;

	s3c_set_platdata(pd, sizeof(struct s5p_hdmi_platform_data),
			 &s5p_device_hdmi);
}

#endif /* CONFIG_S5P_DEV_I2C_HDMIPHY */

/* I2S */

#ifdef CONFIG_PLAT_S3C24XX
static struct resource s3c_iis_resource[] = {
	[0] = DEFINE_RES_MEM(S3C24XX_PA_IIS, S3C24XX_SZ_IIS),
};

struct platform_device s3c_device_iis = {
	.name		= "s3c24xx-iis",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_iis_resource),
	.resource	= s3c_iis_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif /* CONFIG_PLAT_S3C24XX */

/* IDE CFCON */

#ifdef CONFIG_SAMSUNG_DEV_IDE
static struct resource s3c_cfcon_resource[] = {
	[0] = DEFINE_RES_MEM(SAMSUNG_PA_CFCON, SZ_16K),
	[1] = DEFINE_RES_IRQ(IRQ_CFCON),
};

struct platform_device s3c_device_cfcon = {
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_cfcon_resource),
	.resource	= s3c_cfcon_resource,
};

void __init s3c_ide_set_platdata(struct s3c_ide_platdata *pdata)
{
	s3c_set_platdata(pdata, sizeof(struct s3c_ide_platdata),
			 &s3c_device_cfcon);
}
#endif /* CONFIG_SAMSUNG_DEV_IDE */

/* KEYPAD */

#ifdef CONFIG_SAMSUNG_DEV_KEYPAD
static struct resource samsung_keypad_resources[] = {
	[0] = DEFINE_RES_MEM(SAMSUNG_PA_KEYPAD, SZ_32),
	[1] = DEFINE_RES_IRQ(IRQ_KEYPAD),
};

struct platform_device samsung_device_keypad = {
	.name		= "samsung-keypad",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(samsung_keypad_resources),
	.resource	= samsung_keypad_resources,
};

void __init samsung_keypad_set_platdata(struct samsung_keypad_platdata *pd)
{
	struct samsung_keypad_platdata *npd;

	npd = s3c_set_platdata(pd, sizeof(struct samsung_keypad_platdata),
			&samsung_device_keypad);

	if (!npd->cfg_gpio)
		npd->cfg_gpio = samsung_keypad_cfg_gpio;
}
#endif /* CONFIG_SAMSUNG_DEV_KEYPAD */

/* LCD Controller */

#ifdef CONFIG_PLAT_S3C24XX
static struct resource s3c_lcd_resource[] = {
	[0] = DEFINE_RES_MEM(S3C24XX_PA_LCD, S3C24XX_SZ_LCD),
	[1] = DEFINE_RES_IRQ(IRQ_LCD),
};

struct platform_device s3c_device_lcd = {
	.name		= "s3c2410-lcd",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_lcd_resource),
	.resource	= s3c_lcd_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

void __init s3c24xx_fb_set_platdata(struct s3c2410fb_mach_info *pd)
{
	struct s3c2410fb_mach_info *npd;

	npd = s3c_set_platdata(pd, sizeof(*npd), &s3c_device_lcd);
	if (npd) {
		npd->displays = kmemdup(pd->displays,
			sizeof(struct s3c2410fb_display) * npd->num_displays,
			GFP_KERNEL);
		if (!npd->displays)
			printk(KERN_ERR "no memory for LCD display data\n");
	} else {
		printk(KERN_ERR "no memory for LCD platform data\n");
	}
}
#endif /* CONFIG_PLAT_S3C24XX */

/* MIPI CSIS */

#ifdef CONFIG_S5P_DEV_CSIS0
static struct resource s5p_mipi_csis0_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_MIPI_CSIS0, SZ_16K),
	[1] = DEFINE_RES_IRQ(IRQ_MIPI_CSIS0),
};

struct platform_device s5p_device_mipi_csis0 = {
	.name		= "s5p-mipi-csis",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s5p_mipi_csis0_resource),
	.resource	= s5p_mipi_csis0_resource,
};
#endif /* CONFIG_S5P_DEV_CSIS0 */

#ifdef CONFIG_S5P_DEV_CSIS1
static struct resource s5p_mipi_csis1_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_MIPI_CSIS1, SZ_16K),
	[1] = DEFINE_RES_IRQ(IRQ_MIPI_CSIS1),
};

struct platform_device s5p_device_mipi_csis1 = {
	.name		= "s5p-mipi-csis",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s5p_mipi_csis1_resource),
	.resource	= s5p_mipi_csis1_resource,
};
#endif

/* NAND */

#ifdef CONFIG_S3C_DEV_NAND
static struct resource s3c_nand_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_NAND, SZ_1M),
};

struct platform_device s3c_device_nand = {
	.name		= "s3c2410-nand",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_nand_resource),
	.resource	= s3c_nand_resource,
};

/*
 * s3c_nand_copy_set() - copy nand set data
 * @set: The new structure, directly copied from the old.
 *
 * Copy all the fields from the NAND set field from what is probably __initdata
 * to new kernel memory. The code returns 0 if the copy happened correctly or
 * an error code for the calling function to display.
 *
 * Note, we currently do not try and look to see if we've already copied the
 * data in a previous set.
 */
static int __init s3c_nand_copy_set(struct s3c2410_nand_set *set)
{
	void *ptr;
	int size;

	size = sizeof(struct mtd_partition) * set->nr_partitions;
	if (size) {
		ptr = kmemdup(set->partitions, size, GFP_KERNEL);
		set->partitions = ptr;

		if (!ptr)
			return -ENOMEM;
	}

	if (set->nr_map && set->nr_chips) {
		size = sizeof(int) * set->nr_chips;
		ptr = kmemdup(set->nr_map, size, GFP_KERNEL);
		set->nr_map = ptr;

		if (!ptr)
			return -ENOMEM;
	}

	if (set->ecc_layout) {
		ptr = kmemdup(set->ecc_layout,
			      sizeof(struct nand_ecclayout), GFP_KERNEL);
		set->ecc_layout = ptr;

		if (!ptr)
			return -ENOMEM;
	}

	return 0;
}

void __init s3c_nand_set_platdata(struct s3c2410_platform_nand *nand)
{
	struct s3c2410_platform_nand *npd;
	int size;
	int ret;

	/* note, if we get a failure in allocation, we simply drop out of the
	 * function. If there is so little memory available at initialisation
	 * time then there is little chance the system is going to run.
	 */

	npd = s3c_set_platdata(nand, sizeof(struct s3c2410_platform_nand),
				&s3c_device_nand);
	if (!npd)
		return;

	/* now see if we need to copy any of the nand set data */

	size = sizeof(struct s3c2410_nand_set) * npd->nr_sets;
	if (size) {
		struct s3c2410_nand_set *from = npd->sets;
		struct s3c2410_nand_set *to;
		int i;

		to = kmemdup(from, size, GFP_KERNEL);
		npd->sets = to;	/* set, even if we failed */

		if (!to) {
			printk(KERN_ERR "%s: no memory for sets\n", __func__);
			return;
		}

		for (i = 0; i < npd->nr_sets; i++) {
			ret = s3c_nand_copy_set(to);
			if (ret) {
				printk(KERN_ERR "%s: failed to copy set %d\n",
				__func__, i);
				return;
			}
			to++;
		}
	}
}
#endif /* CONFIG_S3C_DEV_NAND */

/* ONENAND */

#ifdef CONFIG_S3C_DEV_ONENAND
static struct resource s3c_onenand_resources[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_ONENAND, SZ_1K),
	[1] = DEFINE_RES_MEM(S3C_PA_ONENAND_BUF, S3C_SZ_ONENAND_BUF),
	[2] = DEFINE_RES_IRQ(IRQ_ONENAND),
};

struct platform_device s3c_device_onenand = {
	.name		= "samsung-onenand",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_onenand_resources),
	.resource	= s3c_onenand_resources,
};
#endif /* CONFIG_S3C_DEV_ONENAND */

#ifdef CONFIG_S3C64XX_DEV_ONENAND1
static struct resource s3c64xx_onenand1_resources[] = {
	[0] = DEFINE_RES_MEM(S3C64XX_PA_ONENAND1, SZ_1K),
	[1] = DEFINE_RES_MEM(S3C64XX_PA_ONENAND1_BUF, S3C64XX_SZ_ONENAND1_BUF),
	[2] = DEFINE_RES_IRQ(IRQ_ONENAND1),
};

struct platform_device s3c64xx_device_onenand1 = {
	.name		= "samsung-onenand",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c64xx_onenand1_resources),
	.resource	= s3c64xx_onenand1_resources,
};

void __init s3c64xx_onenand1_set_platdata(struct onenand_platform_data *pdata)
{
	s3c_set_platdata(pdata, sizeof(struct onenand_platform_data),
			 &s3c64xx_device_onenand1);
}
#endif /* CONFIG_S3C64XX_DEV_ONENAND1 */

#ifdef CONFIG_S5P_DEV_ONENAND
static struct resource s5p_onenand_resources[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_ONENAND, SZ_128K),
	[1] = DEFINE_RES_MEM(S5P_PA_ONENAND_DMA, SZ_8K),
	[2] = DEFINE_RES_IRQ(IRQ_ONENAND_AUDI),
};

struct platform_device s5p_device_onenand = {
	.name		= "s5pc110-onenand",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_onenand_resources),
	.resource	= s5p_onenand_resources,
};
#endif /* CONFIG_S5P_DEV_ONENAND */

/* PMU */

#if defined(CONFIG_PLAT_S5P) && !defined(CONFIG_ARCH_EXYNOS)
static struct resource s5p_pmu_resource[] = {
	DEFINE_RES_IRQ(IRQ_PMU)
};

static struct platform_device s5p_device_pmu = {
	.name		= "arm-pmu",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_pmu_resource),
	.resource	= s5p_pmu_resource,
};

static int __init s5p_pmu_init(void)
{
	platform_device_register(&s5p_device_pmu);
	return 0;
}
arch_initcall(s5p_pmu_init);
#endif /* CONFIG_PLAT_S5P */

/* PWM Timer */

#ifdef CONFIG_SAMSUNG_DEV_PWM
static struct resource samsung_pwm_resource[] = {
	DEFINE_RES_MEM(SAMSUNG_PA_TIMER, SZ_4K),
};

struct platform_device samsung_device_pwm = {
	.name		= "samsung-pwm",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(samsung_pwm_resource),
	.resource	= samsung_pwm_resource,
};

void __init samsung_pwm_set_platdata(struct samsung_pwm_variant *pd)
{
	samsung_device_pwm.dev.platform_data = pd;
}
#endif /* CONFIG_SAMSUNG_DEV_PWM */

/* RTC */

#ifdef CONFIG_PLAT_S3C24XX
static struct resource s3c_rtc_resource[] = {
	[0] = DEFINE_RES_MEM(S3C24XX_PA_RTC, SZ_256),
	[1] = DEFINE_RES_IRQ(IRQ_RTC),
	[2] = DEFINE_RES_IRQ(IRQ_TICK),
};

struct platform_device s3c_device_rtc = {
	.name		= "s3c2410-rtc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_rtc_resource),
	.resource	= s3c_rtc_resource,
};
#endif /* CONFIG_PLAT_S3C24XX */

#ifdef CONFIG_S3C_DEV_RTC
static struct resource s3c_rtc_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_RTC, SZ_256),
	[1] = DEFINE_RES_IRQ(IRQ_RTC_ALARM),
	[2] = DEFINE_RES_IRQ(IRQ_RTC_TIC),
};

struct platform_device s3c_device_rtc = {
	.name		= "s3c64xx-rtc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_rtc_resource),
	.resource	= s3c_rtc_resource,
};
#endif /* CONFIG_S3C_DEV_RTC */

/* SDI */

#ifdef CONFIG_PLAT_S3C24XX
static struct resource s3c_sdi_resource[] = {
	[0] = DEFINE_RES_MEM(S3C24XX_PA_SDI, S3C24XX_SZ_SDI),
	[1] = DEFINE_RES_IRQ(IRQ_SDI),
};

struct platform_device s3c_device_sdi = {
	.name		= "s3c2410-sdi",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_sdi_resource),
	.resource	= s3c_sdi_resource,
};

void __init s3c24xx_mci_set_platdata(struct s3c24xx_mci_pdata *pdata)
{
	s3c_set_platdata(pdata, sizeof(struct s3c24xx_mci_pdata),
			 &s3c_device_sdi);
}
#endif /* CONFIG_PLAT_S3C24XX */

/* SPI */

#ifdef CONFIG_PLAT_S3C24XX
static struct resource s3c_spi0_resource[] = {
	[0] = DEFINE_RES_MEM(S3C24XX_PA_SPI, SZ_32),
	[1] = DEFINE_RES_IRQ(IRQ_SPI0),
};

struct platform_device s3c_device_spi0 = {
	.name		= "s3c2410-spi",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_spi0_resource),
	.resource	= s3c_spi0_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

static struct resource s3c_spi1_resource[] = {
	[0] = DEFINE_RES_MEM(S3C24XX_PA_SPI1, SZ_32),
	[1] = DEFINE_RES_IRQ(IRQ_SPI1),
};

struct platform_device s3c_device_spi1 = {
	.name		= "s3c2410-spi",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_spi1_resource),
	.resource	= s3c_spi1_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif /* CONFIG_PLAT_S3C24XX */

/* Touchscreen */

#ifdef CONFIG_PLAT_S3C24XX
static struct resource s3c_ts_resource[] = {
	[0] = DEFINE_RES_MEM(S3C24XX_PA_ADC, S3C24XX_SZ_ADC),
	[1] = DEFINE_RES_IRQ(IRQ_TC),
};

struct platform_device s3c_device_ts = {
	.name		= "s3c2410-ts",
	.id		= -1,
	.dev.parent	= &s3c_device_adc.dev,
	.num_resources	= ARRAY_SIZE(s3c_ts_resource),
	.resource	= s3c_ts_resource,
};

void __init s3c24xx_ts_set_platdata(struct s3c2410_ts_mach_info *hard_s3c2410ts_info)
{
	s3c_set_platdata(hard_s3c2410ts_info,
			 sizeof(struct s3c2410_ts_mach_info), &s3c_device_ts);
}
#endif /* CONFIG_PLAT_S3C24XX */

#ifdef CONFIG_SAMSUNG_DEV_TS
static struct resource s3c_ts_resource[] = {
	[0] = DEFINE_RES_MEM(SAMSUNG_PA_ADC, SZ_256),
	[1] = DEFINE_RES_IRQ(IRQ_TC),
};

static struct s3c2410_ts_mach_info default_ts_data __initdata = {
	.delay			= 10000,
	.presc			= 49,
	.oversampling_shift	= 2,
};

struct platform_device s3c_device_ts = {
	.name		= "s3c64xx-ts",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_ts_resource),
	.resource	= s3c_ts_resource,
};

void __init s3c24xx_ts_set_platdata(struct s3c2410_ts_mach_info *pd)
{
	if (!pd)
		pd = &default_ts_data;

	s3c_set_platdata(pd, sizeof(struct s3c2410_ts_mach_info),
			 &s3c_device_ts);
}
#endif /* CONFIG_SAMSUNG_DEV_TS */

/* TV */

#ifdef CONFIG_S5P_DEV_TV

static struct resource s5p_hdmi_resources[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_HDMI, SZ_1M),
	[1] = DEFINE_RES_IRQ(IRQ_HDMI),
};

struct platform_device s5p_device_hdmi = {
	.name		= "s5p-hdmi",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_hdmi_resources),
	.resource	= s5p_hdmi_resources,
};

static struct resource s5p_sdo_resources[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_SDO, SZ_64K),
	[1] = DEFINE_RES_IRQ(IRQ_SDO),
};

struct platform_device s5p_device_sdo = {
	.name		= "s5p-sdo",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_sdo_resources),
	.resource	= s5p_sdo_resources,
};

static struct resource s5p_mixer_resources[] = {
	[0] = DEFINE_RES_MEM_NAMED(S5P_PA_MIXER, SZ_64K, "mxr"),
	[1] = DEFINE_RES_MEM_NAMED(S5P_PA_VP, SZ_64K, "vp"),
	[2] = DEFINE_RES_IRQ_NAMED(IRQ_MIXER, "irq"),
};

struct platform_device s5p_device_mixer = {
	.name		= "s5p-mixer",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_mixer_resources),
	.resource	= s5p_mixer_resources,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif /* CONFIG_S5P_DEV_TV */

/* USB */

#ifdef CONFIG_S3C_DEV_USB_HOST
static struct resource s3c_usb_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_USBHOST, SZ_256),
	[1] = DEFINE_RES_IRQ(IRQ_USBH),
};

struct platform_device s3c_device_ohci = {
	.name		= "s3c2410-ohci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb_resource),
	.resource	= s3c_usb_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

/*
 * s3c_ohci_set_platdata - initialise OHCI device platform data
 * @info: The platform data.
 *
 * This call copies the @info passed in and sets the device .platform_data
 * field to that copy. The @info is copied so that the original can be marked
 * __initdata.
 */

void __init s3c_ohci_set_platdata(struct s3c2410_hcd_info *info)
{
	s3c_set_platdata(info, sizeof(struct s3c2410_hcd_info),
			 &s3c_device_ohci);
}
#endif /* CONFIG_S3C_DEV_USB_HOST */

/* USB Device (Gadget) */

#ifdef CONFIG_PLAT_S3C24XX
static struct resource s3c_usbgadget_resource[] = {
	[0] = DEFINE_RES_MEM(S3C24XX_PA_USBDEV, S3C24XX_SZ_USBDEV),
	[1] = DEFINE_RES_IRQ(IRQ_USBD),
};

struct platform_device s3c_device_usbgadget = {
	.name		= "s3c2410-usbgadget",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usbgadget_resource),
	.resource	= s3c_usbgadget_resource,
};

void __init s3c24xx_udc_set_platdata(struct s3c2410_udc_mach_info *pd)
{
	s3c_set_platdata(pd, sizeof(*pd), &s3c_device_usbgadget);
}
#endif /* CONFIG_PLAT_S3C24XX */

/* USB EHCI Host Controller */

#ifdef CONFIG_S5P_DEV_USB_EHCI
static struct resource s5p_ehci_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_EHCI, SZ_256),
	[1] = DEFINE_RES_IRQ(IRQ_USB_HOST),
};

struct platform_device s5p_device_ehci = {
	.name		= "s5p-ehci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_ehci_resource),
	.resource	= s5p_ehci_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

void __init s5p_ehci_set_platdata(struct s5p_ehci_platdata *pd)
{
	struct s5p_ehci_platdata *npd;

	npd = s3c_set_platdata(pd, sizeof(struct s5p_ehci_platdata),
			&s5p_device_ehci);

	if (!npd->phy_init)
		npd->phy_init = s5p_usb_phy_init;
	if (!npd->phy_exit)
		npd->phy_exit = s5p_usb_phy_exit;
}
#endif /* CONFIG_S5P_DEV_USB_EHCI */

/* USB HSOTG */

#ifdef CONFIG_S3C_DEV_USB_HSOTG
static struct resource s3c_usb_hsotg_resources[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_USB_HSOTG, SZ_128K),
	[1] = DEFINE_RES_IRQ(IRQ_OTG),
};

struct platform_device s3c_device_usb_hsotg = {
	.name		= "s3c-hsotg",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb_hsotg_resources),
	.resource	= s3c_usb_hsotg_resources,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

void __init s3c_hsotg_set_platdata(struct s3c_hsotg_plat *pd)
{
	struct s3c_hsotg_plat *npd;

	npd = s3c_set_platdata(pd, sizeof(struct s3c_hsotg_plat),
			&s3c_device_usb_hsotg);

	if (!npd->phy_init)
		npd->phy_init = s5p_usb_phy_init;
	if (!npd->phy_exit)
		npd->phy_exit = s5p_usb_phy_exit;
}
#endif /* CONFIG_S3C_DEV_USB_HSOTG */

/* USB High Spped 2.0 Device (Gadget) */

#ifdef CONFIG_PLAT_S3C24XX
static struct resource s3c_hsudc_resource[] = {
	[0] = DEFINE_RES_MEM(S3C2416_PA_HSUDC, S3C2416_SZ_HSUDC),
	[1] = DEFINE_RES_IRQ(IRQ_USBD),
};

struct platform_device s3c_device_usb_hsudc = {
	.name		= "s3c-hsudc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_hsudc_resource),
	.resource	= s3c_hsudc_resource,
	.dev		= {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

void __init s3c24xx_hsudc_set_platdata(struct s3c24xx_hsudc_platdata *pd)
{
	s3c_set_platdata(pd, sizeof(*pd), &s3c_device_usb_hsudc);
}
#endif /* CONFIG_PLAT_S3C24XX */

/* WDT */

#ifdef CONFIG_S3C_DEV_WDT
static struct resource s3c_wdt_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_WDT, SZ_1K),
	[1] = DEFINE_RES_IRQ(IRQ_WDT),
};

struct platform_device s3c_device_wdt = {
	.name		= "s3c2410-wdt",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_wdt_resource),
	.resource	= s3c_wdt_resource,
};
#endif /* CONFIG_S3C_DEV_WDT */

#ifdef CONFIG_S3C64XX_DEV_SPI0
static struct resource s3c64xx_spi0_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_SPI0, SZ_256),
	[1] = DEFINE_RES_DMA(DMACH_SPI0_TX),
	[2] = DEFINE_RES_DMA(DMACH_SPI0_RX),
	[3] = DEFINE_RES_IRQ(IRQ_SPI0),
};

struct platform_device s3c64xx_device_spi0 = {
	.name		= "s3c6410-spi",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c64xx_spi0_resource),
	.resource	= s3c64xx_spi0_resource,
	.dev = {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

void __init s3c64xx_spi0_set_platdata(int (*cfg_gpio)(void), int src_clk_nr,
						int num_cs)
{
	struct s3c64xx_spi_info pd;

	/* Reject invalid configuration */
	if (!num_cs || src_clk_nr < 0) {
		pr_err("%s: Invalid SPI configuration\n", __func__);
		return;
	}

	pd.num_cs = num_cs;
	pd.src_clk_nr = src_clk_nr;
	pd.cfg_gpio = (cfg_gpio) ? cfg_gpio : s3c64xx_spi0_cfg_gpio;
#ifdef CONFIG_PL330_DMA
	pd.filter = pl330_filter;
#endif

	s3c_set_platdata(&pd, sizeof(pd), &s3c64xx_device_spi0);
}
#endif /* CONFIG_S3C64XX_DEV_SPI0 */

#ifdef CONFIG_S3C64XX_DEV_SPI1
static struct resource s3c64xx_spi1_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_SPI1, SZ_256),
	[1] = DEFINE_RES_DMA(DMACH_SPI1_TX),
	[2] = DEFINE_RES_DMA(DMACH_SPI1_RX),
	[3] = DEFINE_RES_IRQ(IRQ_SPI1),
};

struct platform_device s3c64xx_device_spi1 = {
	.name		= "s3c6410-spi",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c64xx_spi1_resource),
	.resource	= s3c64xx_spi1_resource,
	.dev = {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

void __init s3c64xx_spi1_set_platdata(int (*cfg_gpio)(void), int src_clk_nr,
						int num_cs)
{
	struct s3c64xx_spi_info pd;

	/* Reject invalid configuration */
	if (!num_cs || src_clk_nr < 0) {
		pr_err("%s: Invalid SPI configuration\n", __func__);
		return;
	}

	pd.num_cs = num_cs;
	pd.src_clk_nr = src_clk_nr;
	pd.cfg_gpio = (cfg_gpio) ? cfg_gpio : s3c64xx_spi1_cfg_gpio;
#ifdef CONFIG_PL330_DMA
	pd.filter = pl330_filter;
#endif

	s3c_set_platdata(&pd, sizeof(pd), &s3c64xx_device_spi1);
}
#endif /* CONFIG_S3C64XX_DEV_SPI1 */

#ifdef CONFIG_S3C64XX_DEV_SPI2
static struct resource s3c64xx_spi2_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_SPI2, SZ_256),
	[1] = DEFINE_RES_DMA(DMACH_SPI2_TX),
	[2] = DEFINE_RES_DMA(DMACH_SPI2_RX),
	[3] = DEFINE_RES_IRQ(IRQ_SPI2),
};

struct platform_device s3c64xx_device_spi2 = {
	.name		= "s3c6410-spi",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c64xx_spi2_resource),
	.resource	= s3c64xx_spi2_resource,
	.dev = {
		.dma_mask		= &samsung_device_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

void __init s3c64xx_spi2_set_platdata(int (*cfg_gpio)(void), int src_clk_nr,
						int num_cs)
{
	struct s3c64xx_spi_info pd;

	/* Reject invalid configuration */
	if (!num_cs || src_clk_nr < 0) {
		pr_err("%s: Invalid SPI configuration\n", __func__);
		return;
	}

	pd.num_cs = num_cs;
	pd.src_clk_nr = src_clk_nr;
	pd.cfg_gpio = (cfg_gpio) ? cfg_gpio : s3c64xx_spi2_cfg_gpio;
#ifdef CONFIG_PL330_DMA
	pd.filter = pl330_filter;
#endif

	s3c_set_platdata(&pd, sizeof(pd), &s3c64xx_device_spi2);
}
#endif /* CONFIG_S3C64XX_DEV_SPI2 */
