/*
 * Generic PWM backlight driver data - see drivers/video/backlight/pwm_bl.c
 */
#ifndef __FIKUS_PWM_BACKLIGHT_H
#define __FIKUS_PWM_BACKLIGHT_H

#include <fikus/backlight.h>

struct platform_pwm_backlight_data {
	int pwm_id;
	unsigned int max_brightness;
	unsigned int dft_brightness;
	unsigned int lth_brightness;
	unsigned int pwm_period_ns;
	unsigned int *levels;
	int (*init)(struct device *dev);
	int (*notify)(struct device *dev, int brightness);
	void (*notify_after)(struct device *dev, int brightness);
	void (*exit)(struct device *dev);
	int (*check_fb)(struct device *dev, struct fb_info *info);
};

#endif
