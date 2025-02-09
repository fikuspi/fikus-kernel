/*
 * Copyright (c) 2003-2006 Simtec Electronics <fikus@simtec.co.uk>
 *	http://armfikus.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ARCH_ARM_MACH_S3C24XX_S3C2412_POWER_H
#define __ARCH_ARM_MACH_S3C24XX_S3C2412_POWER_H __FILE__

#define S3C24XX_PWRREG(x)			((x) + S3C24XX_VA_CLKPWR)

#define S3C2412_PWRMODECON			S3C24XX_PWRREG(0x20)
#define S3C2412_PWRCFG				S3C24XX_PWRREG(0x24)

#define S3C2412_INFORM0				S3C24XX_PWRREG(0x70)
#define S3C2412_INFORM1				S3C24XX_PWRREG(0x74)
#define S3C2412_INFORM2				S3C24XX_PWRREG(0x78)
#define S3C2412_INFORM3				S3C24XX_PWRREG(0x7C)

#define S3C2412_PWRCFG_BATF_IRQ			(1 << 0)
#define S3C2412_PWRCFG_BATF_IGNORE		(2 << 0)
#define S3C2412_PWRCFG_BATF_SLEEP		(3 << 0)
#define S3C2412_PWRCFG_BATF_MASK		(3 << 0)

#define S3C2412_PWRCFG_STANDBYWFI_IGNORE	(0 << 6)
#define S3C2412_PWRCFG_STANDBYWFI_IDLE		(1 << 6)
#define S3C2412_PWRCFG_STANDBYWFI_STOP		(2 << 6)
#define S3C2412_PWRCFG_STANDBYWFI_SLEEP		(3 << 6)
#define S3C2412_PWRCFG_STANDBYWFI_MASK		(3 << 6)

#define S3C2412_PWRCFG_RTC_MASKIRQ		(1 << 8)
#define S3C2412_PWRCFG_NAND_NORST		(1 << 9)

#endif /* __ARCH_ARM_MACH_S3C24XX_S3C2412_POWER_H */
