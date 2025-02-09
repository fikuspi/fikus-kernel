/* sunxvr1000.c: Sun XVR-1000 driver for sparc64 systems
 *
 * Copyright (C) 2010 David S. Miller (davem@davemloft.net)
 */

#include <fikus/module.h>
#include <fikus/kernel.h>
#include <fikus/fb.h>
#include <fikus/init.h>
#include <fikus/of_device.h>

struct gfb_info {
	struct fb_info		*info;

	char __iomem		*fb_base;
	unsigned long		fb_base_phys;

	struct device_node	*of_node;

	unsigned int		width;
	unsigned int		height;
	unsigned int		depth;
	unsigned int		fb_size;

	u32			pseudo_palette[16];
};

static int gfb_get_props(struct gfb_info *gp)
{
	gp->width = of_getintprop_default(gp->of_node, "width", 0);
	gp->height = of_getintprop_default(gp->of_node, "height", 0);
	gp->depth = of_getintprop_default(gp->of_node, "depth", 32);

	if (!gp->width || !gp->height) {
		printk(KERN_ERR "gfb: Critical properties missing for %s\n",
		       gp->of_node->full_name);
		return -EINVAL;
	}

	return 0;
}

static int gfb_setcolreg(unsigned regno,
			 unsigned red, unsigned green, unsigned blue,
			 unsigned transp, struct fb_info *info)
{
	u32 value;

	if (regno < 16) {
		red >>= 8;
		green >>= 8;
		blue >>= 8;

		value = (blue << 16) | (green << 8) | red;
		((u32 *)info->pseudo_palette)[regno] = value;
	}

	return 0;
}

static struct fb_ops gfb_ops = {
	.owner			= THIS_MODULE,
	.fb_setcolreg		= gfb_setcolreg,
	.fb_fillrect		= cfb_fillrect,
	.fb_copyarea		= cfb_copyarea,
	.fb_imageblit		= cfb_imageblit,
};

static int gfb_set_fbinfo(struct gfb_info *gp)
{
	struct fb_info *info = gp->info;
	struct fb_var_screeninfo *var = &info->var;

	info->flags = FBINFO_DEFAULT;
	info->fbops = &gfb_ops;
	info->screen_base = gp->fb_base;
	info->screen_size = gp->fb_size;

	info->pseudo_palette = gp->pseudo_palette;

	/* Fill fix common fields */
	strlcpy(info->fix.id, "gfb", sizeof(info->fix.id));
        info->fix.smem_start = gp->fb_base_phys;
        info->fix.smem_len = gp->fb_size;
        info->fix.type = FB_TYPE_PACKED_PIXELS;
	if (gp->depth == 32 || gp->depth == 24)
		info->fix.visual = FB_VISUAL_TRUECOLOR;
	else
		info->fix.visual = FB_VISUAL_PSEUDOCOLOR;

	var->xres = gp->width;
	var->yres = gp->height;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres;
	var->bits_per_pixel = gp->depth;

	var->red.offset = 0;
	var->red.length = 8;
	var->green.offset = 8;
	var->green.length = 8;
	var->blue.offset = 16;
	var->blue.length = 8;
	var->transp.offset = 0;
	var->transp.length = 0;

	if (fb_alloc_cmap(&info->cmap, 256, 0)) {
		printk(KERN_ERR "gfb: Cannot allocate color map.\n");
		return -ENOMEM;
	}

        return 0;
}

static int gfb_probe(struct platform_device *op)
{
	struct device_node *dp = op->dev.of_node;
	struct fb_info *info;
	struct gfb_info *gp;
	int err;

	info = framebuffer_alloc(sizeof(struct gfb_info), &op->dev);
	if (!info) {
		printk(KERN_ERR "gfb: Cannot allocate fb_info\n");
		err = -ENOMEM;
		goto err_out;
	}

	gp = info->par;
	gp->info = info;
	gp->of_node = dp;

	gp->fb_base_phys = op->resource[6].start;

	err = gfb_get_props(gp);
	if (err)
		goto err_release_fb;

	/* Framebuffer length is the same regardless of resolution. */
	info->fix.line_length = 16384;
	gp->fb_size = info->fix.line_length * gp->height;

	gp->fb_base = of_ioremap(&op->resource[6], 0,
				 gp->fb_size, "gfb fb");
	if (!gp->fb_base) {
		err = -ENOMEM;
		goto err_release_fb;
	}

	err = gfb_set_fbinfo(gp);
	if (err)
		goto err_unmap_fb;

	printk("gfb: Found device at %s\n", dp->full_name);

	err = register_framebuffer(info);
	if (err < 0) {
		printk(KERN_ERR "gfb: Could not register framebuffer %s\n",
		       dp->full_name);
		goto err_unmap_fb;
	}

	dev_set_drvdata(&op->dev, info);

	return 0;

err_unmap_fb:
	of_iounmap(&op->resource[6], gp->fb_base, gp->fb_size);

err_release_fb:
        framebuffer_release(info);

err_out:
	return err;
}

static int gfb_remove(struct platform_device *op)
{
	struct fb_info *info = dev_get_drvdata(&op->dev);
	struct gfb_info *gp = info->par;

	unregister_framebuffer(info);

	iounmap(gp->fb_base);

	of_iounmap(&op->resource[6], gp->fb_base, gp->fb_size);

        framebuffer_release(info);

	dev_set_drvdata(&op->dev, NULL);

	return 0;
}

static const struct of_device_id gfb_match[] = {
	{
		.name = "SUNW,gfb",
	},
	{},
};
MODULE_DEVICE_TABLE(of, ffb_match);

static struct platform_driver gfb_driver = {
	.probe		= gfb_probe,
	.remove		= gfb_remove,
	.driver = {
		.name		= "gfb",
		.owner		= THIS_MODULE,
		.of_match_table	= gfb_match,
	},
};

static int __init gfb_init(void)
{
	if (fb_get_options("gfb", NULL))
		return -ENODEV;

	return platform_driver_register(&gfb_driver);
}

static void __exit gfb_exit(void)
{
	platform_driver_unregister(&gfb_driver);
}

module_init(gfb_init);
module_exit(gfb_exit);

MODULE_DESCRIPTION("framebuffer driver for Sun XVR-1000 graphics");
MODULE_AUTHOR("David S. Miller <davem@davemloft.net>");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
