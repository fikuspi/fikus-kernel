/* fikus/arch/arm/mach-s3c64xx/include/mach/pm-core.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armfikus.simtec.co.uk/
 *
 * S3C64XX - PM core support for arch/arm/plat-s3c/pm.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MACH_S3C64XX_PM_CORE_H
#define __MACH_S3C64XX_PM_CORE_H __FILE__

#include <mach/regs-gpio.h>

static inline void s3c_pm_debug_init_uart(void)
{
	u32 tmp = __raw_readl(S3C_PCLK_GATE);

	/* As a note, since the S3C64XX UARTs generally have multiple
	 * clock sources, we simply enable PCLK at the moment and hope
	 * that the resume settings for the UART are suitable for the
	 * use with PCLK.
	 */

	tmp |= S3C_CLKCON_PCLK_UART0;
	tmp |= S3C_CLKCON_PCLK_UART1;
	tmp |= S3C_CLKCON_PCLK_UART2;
	tmp |= S3C_CLKCON_PCLK_UART3;

	__raw_writel(tmp, S3C_PCLK_GATE);
	udelay(10);
}

static inline void s3c_pm_arch_prepare_irqs(void)
{
	/* VIC should have already been taken care of */

	/* clear any pending EINT0 interrupts */
	__raw_writel(__raw_readl(S3C64XX_EINT0PEND), S3C64XX_EINT0PEND);
}

static inline void s3c_pm_arch_stop_clocks(void)
{
}

static inline void s3c_pm_arch_show_resume_irqs(void)
{
}

/* make these defines, we currently do not have any need to change
 * the IRQ wake controls depending on the CPU we are running on */

#define s3c_irqwake_eintallow	((1 << 28) - 1)
#define s3c_irqwake_intallow	(~0)

static inline void s3c_pm_arch_update_uart(void __iomem *regs,
					   struct pm_uart_save *save)
{
	u32 ucon = __raw_readl(regs + S3C2410_UCON);
	u32 ucon_clk = ucon & S3C6400_UCON_CLKMASK;
	u32 save_clk = save->ucon & S3C6400_UCON_CLKMASK;
	u32 new_ucon;
	u32 delta;

	/* S3C64XX UART blocks only support level interrupts, so ensure that
	 * when we restore unused UART blocks we force the level interrupt
	 * settigs. */
	save->ucon |= S3C2410_UCON_TXILEVEL | S3C2410_UCON_RXILEVEL;

	/* We have a constraint on changing the clock type of the UART
	 * between UCLKx and PCLK, so ensure that when we restore UCON
	 * that the CLK field is correctly modified if the bootloader
	 * has changed anything.
	 */
	if (ucon_clk != save_clk) {
		new_ucon = save->ucon;
		delta = ucon_clk ^ save_clk;

		/* change from UCLKx => wrong PCLK,
		 * either UCLK can be tested for by a bit-test
		 * with UCLK0 */
		if (ucon_clk & S3C6400_UCON_UCLK0 &&
		    !(save_clk & S3C6400_UCON_UCLK0) &&
		    delta & S3C6400_UCON_PCLK2) {
			new_ucon &= ~S3C6400_UCON_UCLK0;
		} else if (delta == S3C6400_UCON_PCLK2) {
			/* as an precaution, don't change from
			 * PCLK2 => PCLK or vice-versa */
			new_ucon ^= S3C6400_UCON_PCLK2;
		}

		S3C_PMDBG("ucon change %04x => %04x (save=%04x)\n",
			  ucon, new_ucon, save->ucon);
		save->ucon = new_ucon;
	}
}

static inline void s3c_pm_restored_gpios(void)
{
	/* ensure sleep mode has been cleared from the system */

	__raw_writel(0, S3C64XX_SLPEN);
}

static inline void samsung_pm_saved_gpios(void)
{
	/* turn on the sleep mode and keep it there, as it seems that during
	 * suspend the xCON registers get re-set and thus you can end up with
	 * problems between going to sleep and resuming.
	 */

	__raw_writel(S3C64XX_SLPEN_USE_xSLP, S3C64XX_SLPEN);
}
#endif /* __MACH_S3C64XX_PM_CORE_H */
