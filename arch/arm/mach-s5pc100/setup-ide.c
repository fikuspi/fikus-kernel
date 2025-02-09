/* fikus/arch/arm/mach-s5pc100/setup-ide.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5PC100 setup information for IDE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <fikus/kernel.h>
#include <fikus/gpio.h>
#include <fikus/io.h>

#include <mach/regs-clock.h>
#include <plat/gpio-cfg.h>

static void s5pc100_ide_cfg_gpios(unsigned int base, unsigned int nr)
{
	s3c_gpio_cfgrange_nopull(base, nr, S3C_GPIO_SFN(4));

	for (; nr > 0; nr--, base++)
		s5p_gpio_set_drvstr(base, S5P_GPIO_DRVSTR_LV4);
}

void s5pc100_ide_setup_gpio(void)
{
	u32 reg;

	/* Independent CF interface, CF chip select configuration */
	reg = readl(S5PC100_MEM_SYS_CFG) & (~0x3f);
	writel(reg | MEM_SYS_CFG_EBI_FIX_PRI_CFCON, S5PC100_MEM_SYS_CFG);

	/* CF_Add[0 - 2], CF_IORDY, CF_INTRQ, CF_DMARQ, CF_DMARST, CF_DMACK */
	s5pc100_ide_cfg_gpios(S5PC100_GPJ0(0), 8);

	/*CF_Data[0 - 7] */
	s5pc100_ide_cfg_gpios(S5PC100_GPJ2(0), 8);

	/* CF_Data[8 - 15] */
	s5pc100_ide_cfg_gpios(S5PC100_GPJ3(0), 8);

	/* CF_CS0, CF_CS1, CF_IORD, CF_IOWR */
	s5pc100_ide_cfg_gpios(S5PC100_GPJ4(0), 4);

	/* EBI_OE, EBI_WE */
	s3c_gpio_cfgpin_range(S5PC100_GPK0(6), 2, S3C_GPIO_SFN(0));

	/* CF_OE, CF_WE */
	s3c_gpio_cfgrange_nopull(S5PC100_GPK1(6), 8, S3C_GPIO_SFN(2));

	/* CF_CD */
	s3c_gpio_cfgpin(S5PC100_GPK3(5), S3C_GPIO_SFN(2));
	s3c_gpio_setpull(S5PC100_GPK3(5), S3C_GPIO_PULL_NONE);
}
