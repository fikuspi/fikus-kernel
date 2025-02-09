/*
 *  Copyright (C) 2010, Lars-Peter Clausen <lars@metafoo.de>
 *  PWM beeper driver
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <fikus/input.h>
#include <fikus/module.h>
#include <fikus/kernel.h>
#include <fikus/platform_device.h>
#include <fikus/pwm.h>
#include <fikus/slab.h>

struct pwm_beeper {
	struct input_dev *input;
	struct pwm_device *pwm;
	unsigned long period;
};

#define HZ_TO_NANOSECONDS(x) (1000000000UL/(x))

static int pwm_beeper_event(struct input_dev *input,
			    unsigned int type, unsigned int code, int value)
{
	int ret = 0;
	struct pwm_beeper *beeper = input_get_drvdata(input);
	unsigned long period;

	if (type != EV_SND || value < 0)
		return -EINVAL;

	switch (code) {
	case SND_BELL:
		value = value ? 1000 : 0;
		break;
	case SND_TONE:
		break;
	default:
		return -EINVAL;
	}

	if (value == 0) {
		pwm_config(beeper->pwm, 0, 0);
		pwm_disable(beeper->pwm);
	} else {
		period = HZ_TO_NANOSECONDS(value);
		ret = pwm_config(beeper->pwm, period / 2, period);
		if (ret)
			return ret;
		ret = pwm_enable(beeper->pwm);
		if (ret)
			return ret;
		beeper->period = period;
	}

	return 0;
}

static int pwm_beeper_probe(struct platform_device *pdev)
{
	unsigned long pwm_id = (unsigned long)pdev->dev.platform_data;
	struct pwm_beeper *beeper;
	int error;

	beeper = kzalloc(sizeof(*beeper), GFP_KERNEL);
	if (!beeper)
		return -ENOMEM;

	beeper->pwm = pwm_get(&pdev->dev, NULL);
	if (IS_ERR(beeper->pwm)) {
		dev_dbg(&pdev->dev, "unable to request PWM, trying legacy API\n");
		beeper->pwm = pwm_request(pwm_id, "pwm beeper");
	}

	if (IS_ERR(beeper->pwm)) {
		error = PTR_ERR(beeper->pwm);
		dev_err(&pdev->dev, "Failed to request pwm device: %d\n", error);
		goto err_free;
	}

	beeper->input = input_allocate_device();
	if (!beeper->input) {
		dev_err(&pdev->dev, "Failed to allocate input device\n");
		error = -ENOMEM;
		goto err_pwm_free;
	}
	beeper->input->dev.parent = &pdev->dev;

	beeper->input->name = "pwm-beeper";
	beeper->input->phys = "pwm/input0";
	beeper->input->id.bustype = BUS_HOST;
	beeper->input->id.vendor = 0x001f;
	beeper->input->id.product = 0x0001;
	beeper->input->id.version = 0x0100;

	beeper->input->evbit[0] = BIT(EV_SND);
	beeper->input->sndbit[0] = BIT(SND_TONE) | BIT(SND_BELL);

	beeper->input->event = pwm_beeper_event;

	input_set_drvdata(beeper->input, beeper);

	error = input_register_device(beeper->input);
	if (error) {
		dev_err(&pdev->dev, "Failed to register input device: %d\n", error);
		goto err_input_free;
	}

	platform_set_drvdata(pdev, beeper);

	return 0;

err_input_free:
	input_free_device(beeper->input);
err_pwm_free:
	pwm_free(beeper->pwm);
err_free:
	kfree(beeper);

	return error;
}

static int pwm_beeper_remove(struct platform_device *pdev)
{
	struct pwm_beeper *beeper = platform_get_drvdata(pdev);

	input_unregister_device(beeper->input);

	pwm_disable(beeper->pwm);
	pwm_free(beeper->pwm);

	kfree(beeper);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pwm_beeper_suspend(struct device *dev)
{
	struct pwm_beeper *beeper = dev_get_drvdata(dev);

	if (beeper->period)
		pwm_disable(beeper->pwm);

	return 0;
}

static int pwm_beeper_resume(struct device *dev)
{
	struct pwm_beeper *beeper = dev_get_drvdata(dev);

	if (beeper->period) {
		pwm_config(beeper->pwm, beeper->period / 2, beeper->period);
		pwm_enable(beeper->pwm);
	}

	return 0;
}

static SIMPLE_DEV_PM_OPS(pwm_beeper_pm_ops,
			 pwm_beeper_suspend, pwm_beeper_resume);

#define PWM_BEEPER_PM_OPS (&pwm_beeper_pm_ops)
#else
#define PWM_BEEPER_PM_OPS NULL
#endif

#ifdef CONFIG_OF
static const struct of_device_id pwm_beeper_match[] = {
	{ .compatible = "pwm-beeper", },
	{ },
};
#endif

static struct platform_driver pwm_beeper_driver = {
	.probe	= pwm_beeper_probe,
	.remove = pwm_beeper_remove,
	.driver = {
		.name	= "pwm-beeper",
		.owner	= THIS_MODULE,
		.pm	= PWM_BEEPER_PM_OPS,
		.of_match_table = of_match_ptr(pwm_beeper_match),
	},
};
module_platform_driver(pwm_beeper_driver);

MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_DESCRIPTION("PWM beeper driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pwm-beeper");
