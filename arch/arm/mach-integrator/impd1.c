/*
 *  fikus/arch/arm/mach-integrator/impd1.c
 *
 *  Copyright (C) 2003 Deep Blue Solutions Ltd, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file provides the core support for the IM-PD1 module.
 *
 * Module / boot parameters.
 *   lmid=n   impd1.lmid=n - set the logic module position in stack to 'n'
 */
#include <fikus/module.h>
#include <fikus/moduleparam.h>
#include <fikus/init.h>
#include <fikus/device.h>
#include <fikus/errno.h>
#include <fikus/mm.h>
#include <fikus/amba/bus.h>
#include <fikus/amba/clcd.h>
#include <fikus/io.h>
#include <fikus/platform_data/clk-integrator.h>
#include <fikus/slab.h>

#include <mach/lm.h>
#include <mach/impd1.h>
#include <asm/sizes.h>

static int module_id;

module_param_named(lmid, module_id, int, 0444);
MODULE_PARM_DESC(lmid, "logic module stack position");

struct impd1_module {
	void __iomem	*base;
};

void impd1_tweak_control(struct device *dev, u32 mask, u32 val)
{
	struct impd1_module *impd1 = dev_get_drvdata(dev);
	u32 cur;

	val &= mask;
	cur = readl(impd1->base + IMPD1_CTRL) & ~mask;
	writel(cur | val, impd1->base + IMPD1_CTRL);
}

EXPORT_SYMBOL(impd1_tweak_control);

/*
 * CLCD support
 */
#define PANEL		PROSPECTOR

#define LTM10C209		1
#define PROSPECTOR		2
#define SVGA			3
#define VGA			4

#if PANEL == VGA
#define PANELTYPE	vga
static struct clcd_panel vga = {
	.mode		= {
		.name		= "VGA",
		.refresh	= 60,
		.xres		= 640,
		.yres		= 480,
		.pixclock	= 39721,
		.left_margin	= 40,
		.right_margin	= 24,
		.upper_margin	= 32,
		.lower_margin	= 11,
		.hsync_len	= 96,
		.vsync_len	= 2,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD | TIM2_IPC,
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1),
	.caps		= CLCD_CAP_5551,
	.connector	= IMPD1_CTRL_DISP_VGA,
	.bpp		= 16,
	.grayscale	= 0,
};

#elif PANEL == SVGA
#define PANELTYPE	svga
static struct clcd_panel svga = {
	.mode		= {
		.name		= "SVGA",
		.refresh	= 0,
		.xres		= 800,
		.yres		= 600,
		.pixclock	= 27778,
		.left_margin	= 20,
		.right_margin	= 20,
		.upper_margin	= 5,
		.lower_margin	= 5,
		.hsync_len	= 164,
		.vsync_len	= 62,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD,
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1),
	.connector	= IMPD1_CTRL_DISP_VGA,
	.caps		= CLCD_CAP_5551,
	.bpp		= 16,
	.grayscale	= 0,
};

#elif PANEL == PROSPECTOR
#define PANELTYPE	prospector
static struct clcd_panel prospector = {
	.mode		= {
		.name		= "PROSPECTOR",
		.refresh	= 0,
		.xres		= 640,
		.yres		= 480,
		.pixclock	= 40000,
		.left_margin	= 33,
		.right_margin	= 64,
		.upper_margin	= 36,
		.lower_margin	= 7,
		.hsync_len	= 64,
		.vsync_len	= 25,
		.sync		= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD,
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1),
	.caps		= CLCD_CAP_5551,
	.fixedtimings	= 1,
	.connector	= IMPD1_CTRL_DISP_LCD,
	.bpp		= 16,
	.grayscale	= 0,
};

#elif PANEL == LTM10C209
#define PANELTYPE	ltm10c209
/*
 * Untested.
 */
static struct clcd_panel ltm10c209 = {
	.mode		= {
		.name		= "LTM10C209",
		.refresh	= 0,
		.xres		= 640,
		.yres		= 480,
		.pixclock	= 40000,
		.left_margin	= 20,
		.right_margin	= 20,
		.upper_margin	= 19,
		.lower_margin	= 19,
		.hsync_len	= 20,
		.vsync_len	= 10,
		.sync		= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD,
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1),
	.caps		= CLCD_CAP_5551,
	.fixedtimings	= 1,
	.connector	= IMPD1_CTRL_DISP_LCD,
	.bpp		= 16,
	.grayscale	= 0,
};
#endif

/*
 * Disable all display connectors on the interface module.
 */
static void impd1fb_clcd_disable(struct clcd_fb *fb)
{
	impd1_tweak_control(fb->dev->dev.parent, IMPD1_CTRL_DISP_MASK, 0);
}

/*
 * Enable the relevant connector on the interface module.
 */
static void impd1fb_clcd_enable(struct clcd_fb *fb)
{
	impd1_tweak_control(fb->dev->dev.parent, IMPD1_CTRL_DISP_MASK,
			fb->panel->connector | IMPD1_CTRL_DISP_ENABLE);
}

static int impd1fb_clcd_setup(struct clcd_fb *fb)
{
	unsigned long framebase = fb->dev->res.start + 0x01000000;
	unsigned long framesize = SZ_1M;
	int ret = 0;

	fb->panel = &PANELTYPE;

	if (!request_mem_region(framebase, framesize, "clcd framebuffer")) {
		printk(KERN_ERR "IM-PD1: unable to reserve framebuffer\n");
		return -EBUSY;
	}

	fb->fb.screen_base = ioremap(framebase, framesize);
	if (!fb->fb.screen_base) {
		printk(KERN_ERR "IM-PD1: unable to map framebuffer\n");
		ret = -ENOMEM;
		goto free_buffer;
	}

	fb->fb.fix.smem_start	= framebase;
	fb->fb.fix.smem_len	= framesize;

	return 0;

 free_buffer:
	release_mem_region(framebase, framesize);
	return ret;
}

static int impd1fb_clcd_mmap(struct clcd_fb *fb, struct vm_area_struct *vma)
{
	unsigned long start, size;

	start = vma->vm_pgoff + (fb->fb.fix.smem_start >> PAGE_SHIFT);
	size = vma->vm_end - vma->vm_start;

	return remap_pfn_range(vma, vma->vm_start, start, size,
			       vma->vm_page_prot);
}

static void impd1fb_clcd_remove(struct clcd_fb *fb)
{
	iounmap(fb->fb.screen_base);
	release_mem_region(fb->fb.fix.smem_start, fb->fb.fix.smem_len);
}

static struct clcd_board impd1_clcd_data = {
	.name		= "IM-PD/1",
	.caps		= CLCD_CAP_5551 | CLCD_CAP_888,
	.check		= clcdfb_check,
	.decode		= clcdfb_decode,
	.disable	= impd1fb_clcd_disable,
	.enable		= impd1fb_clcd_enable,
	.setup		= impd1fb_clcd_setup,
	.mmap		= impd1fb_clcd_mmap,
	.remove		= impd1fb_clcd_remove,
};

struct impd1_device {
	unsigned long	offset;
	unsigned int	irq[2];
	unsigned int	id;
	void		*platform_data;
};

static struct impd1_device impd1_devs[] = {
	{
		.offset	= 0x03000000,
		.id	= 0x00041190,
	}, {
		.offset	= 0x00100000,
		.irq	= { 1 },
		.id	= 0x00141011,
	}, {
		.offset	= 0x00200000,
		.irq	= { 2 },
		.id	= 0x00141011,
	}, {
		.offset	= 0x00300000,
		.irq	= { 3 },
		.id	= 0x00041022,
	}, {
		.offset	= 0x00400000,
		.irq	= { 4 },
		.id	= 0x00041061,
	}, {
		.offset	= 0x00500000,
		.irq	= { 5 },
		.id	= 0x00041061,
	}, {
		.offset	= 0x00600000,
		.irq	= { 6 },
		.id	= 0x00041130,
	}, {
		.offset	= 0x00700000,
		.irq	= { 7, 8 },
		.id	= 0x00041181,
	}, {
		.offset	= 0x00800000,
		.irq	= { 9 },
		.id	= 0x00041041,
	}, {
		.offset	= 0x01000000,
		.irq	= { 11 },
		.id	= 0x00041110,
		.platform_data = &impd1_clcd_data,
	}
};

static int impd1_probe(struct lm_device *dev)
{
	struct impd1_module *impd1;
	int i, ret;

	if (dev->id != module_id)
		return -EINVAL;

	if (!request_mem_region(dev->resource.start, SZ_4K, "LM registers"))
		return -EBUSY;

	impd1 = kzalloc(sizeof(struct impd1_module), GFP_KERNEL);
	if (!impd1) {
		ret = -ENOMEM;
		goto release_lm;
	}

	impd1->base = ioremap(dev->resource.start, SZ_4K);
	if (!impd1->base) {
		ret = -ENOMEM;
		goto free_impd1;
	}

	lm_set_drvdata(dev, impd1);

	printk("IM-PD1 found at 0x%08lx\n",
		(unsigned long)dev->resource.start);

	integrator_impd1_clk_init(impd1->base, dev->id);

	for (i = 0; i < ARRAY_SIZE(impd1_devs); i++) {
		struct impd1_device *idev = impd1_devs + i;
		struct amba_device *d;
		unsigned long pc_base;
		char devname[32];

		pc_base = dev->resource.start + idev->offset;
		snprintf(devname, 32, "lm%x:%5.5lx", dev->id, idev->offset >> 12);
		d = amba_ahb_device_add_res(&dev->dev, devname, pc_base, SZ_4K,
					    dev->irq, dev->irq,
					    idev->platform_data, idev->id,
					    &dev->resource);
		if (IS_ERR(d)) {
			dev_err(&dev->dev, "unable to register device: %ld\n", PTR_ERR(d));
			continue;
		}
	}

	return 0;

 free_impd1:
	if (impd1 && impd1->base)
		iounmap(impd1->base);
	kfree(impd1);
 release_lm:
	release_mem_region(dev->resource.start, SZ_4K);
	return ret;
}

static int impd1_remove_one(struct device *dev, void *data)
{
	device_unregister(dev);
	return 0;
}

static void impd1_remove(struct lm_device *dev)
{
	struct impd1_module *impd1 = lm_get_drvdata(dev);

	device_for_each_child(&dev->dev, NULL, impd1_remove_one);
	integrator_impd1_clk_exit(dev->id);

	lm_set_drvdata(dev, NULL);

	iounmap(impd1->base);
	kfree(impd1);
	release_mem_region(dev->resource.start, SZ_4K);
}

static struct lm_driver impd1_driver = {
	.drv = {
		.name	= "impd1",
	},
	.probe		= impd1_probe,
	.remove		= impd1_remove,
};

static int __init impd1_init(void)
{
	return lm_driver_register(&impd1_driver);
}

static void __exit impd1_exit(void)
{
	lm_driver_unregister(&impd1_driver);
}

module_init(impd1_init);
module_exit(impd1_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Integrator/IM-PD1 logic module core driver");
MODULE_AUTHOR("Deep Blue Solutions Ltd");
