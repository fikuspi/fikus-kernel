/* fikus/arch/arm/plat-s3c64xx/setup-sdhci-gpio.c
 *
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armfikus.simtec.co.uk/
 *
 * S3C64XX - Helper functions for setting up SDHCI device(s) GPIO (HSMMC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <fikus/kernel.h>
#include <fikus/types.h>
#include <fikus/interrupt.h>
#include <fikus/platform_device.h>
#include <fikus/io.h>
#include <fikus/gpio.h>

#include <plat/gpio-cfg.h>
#include <plat/sdhci.h>

void s3c64xx_setup_sdhci0_cfg_gpio(struct platform_device *dev, int width)
{
	struct s3c_sdhci_platdata *pdata = dev->dev.platform_data;

	/* Set all the necessary GPG pins to special-function 2 */
	s3c_gpio_cfgrange_nopull(S3C64XX_GPG(0), 2 + width, S3C_GPIO_SFN(2));

	if (pdata->cd_type == S3C_SDHCI_CD_INTERNAL) {
		s3c_gpio_setpull(S3C64XX_GPG(6), S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(S3C64XX_GPG(6), S3C_GPIO_SFN(2));
	}
}

void s3c64xx_setup_sdhci1_cfg_gpio(struct platform_device *dev, int width)
{
	struct s3c_sdhci_platdata *pdata = dev->dev.platform_data;

	/* Set all the necessary GPH pins to special-function 2 */
	s3c_gpio_cfgrange_nopull(S3C64XX_GPH(0), 2 + width, S3C_GPIO_SFN(2));

	if (pdata->cd_type == S3C_SDHCI_CD_INTERNAL) {
		s3c_gpio_setpull(S3C64XX_GPG(6), S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(S3C64XX_GPG(6), S3C_GPIO_SFN(3));
	}
}

void s3c64xx_setup_sdhci2_cfg_gpio(struct platform_device *dev, int width)
{
	/* Set all the necessary GPH pins to special-function 3 */
	s3c_gpio_cfgrange_nopull(S3C64XX_GPH(6), width, S3C_GPIO_SFN(3));

	/* Set all the necessary GPC pins to special-function 3 */
	s3c_gpio_cfgrange_nopull(S3C64XX_GPC(4), 2, S3C_GPIO_SFN(3));
}
