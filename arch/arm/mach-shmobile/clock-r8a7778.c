/*
 * r8a7778 clock framework support
 *
 * Copyright (C) 2013  Renesas Solutions Corp.
 * Copyright (C) 2013  Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * based on r8a7779
 *
 * Copyright (C) 2011  Renesas Solutions Corp.
 * Copyright (C) 2011  Magnus Damm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *     MD      MD      MD      MD       PLLA   PLLB    EXTAL   clki    clkz
 *     19      18      12      11                      (HMz)   (MHz)   (MHz)
 *----------------------------------------------------------------------------
 *     1       0       0       0       x21     x21     38.00   800     800
 *     1       0       0       1       x24     x24     33.33   800     800
 *     1       0       1       0       x28     x28     28.50   800     800
 *     1       0       1       1       x32     x32     25.00   800     800
 *     1       1       0       1       x24     x21     33.33   800     700
 *     1       1       1       0       x28     x21     28.50   800     600
 *     1       1       1       1       x32     x24     25.00   800     600
 */

#include <fikus/io.h>
#include <fikus/sh_clk.h>
#include <fikus/clkdev.h>
#include <mach/clock.h>
#include <mach/common.h>

#define MSTPCR0		IOMEM(0xffc80030)
#define MSTPCR1		IOMEM(0xffc80034)
#define MSTPCR3		IOMEM(0xffc8003c)
#define MSTPSR1		IOMEM(0xffc80044)
#define MSTPSR4		IOMEM(0xffc80048)
#define MSTPSR6		IOMEM(0xffc8004c)
#define MSTPCR4		IOMEM(0xffc80050)
#define MSTPCR5		IOMEM(0xffc80054)
#define MSTPCR6		IOMEM(0xffc80058)
#define MODEMR		0xFFCC0020

#define MD(nr)	BIT(nr)

/* ioremap() through clock mapping mandatory to avoid
 * collision with ARM coherent DMA virtual memory range.
 */

static struct clk_mapping cpg_mapping = {
	.phys	= 0xffc80000,
	.len	= 0x80,
};

static struct clk extal_clk = {
	/* .rate will be updated on r8a7778_clock_init() */
	.mapping = &cpg_mapping,
};

/*
 * clock ratio of these clock will be updated
 * on r8a7778_clock_init()
 */
SH_FIXED_RATIO_CLK_SET(plla_clk,	extal_clk, 1, 1);
SH_FIXED_RATIO_CLK_SET(pllb_clk,	extal_clk, 1, 1);
SH_FIXED_RATIO_CLK_SET(i_clk,		plla_clk,  1, 1);
SH_FIXED_RATIO_CLK_SET(s_clk,		plla_clk,  1, 1);
SH_FIXED_RATIO_CLK_SET(s1_clk,		plla_clk,  1, 1);
SH_FIXED_RATIO_CLK_SET(s3_clk,		plla_clk,  1, 1);
SH_FIXED_RATIO_CLK_SET(s4_clk,		plla_clk,  1, 1);
SH_FIXED_RATIO_CLK_SET(b_clk,		plla_clk,  1, 1);
SH_FIXED_RATIO_CLK_SET(out_clk,		plla_clk,  1, 1);
SH_FIXED_RATIO_CLK_SET(p_clk,		plla_clk,  1, 1);
SH_FIXED_RATIO_CLK_SET(g_clk,		plla_clk,  1, 1);
SH_FIXED_RATIO_CLK_SET(z_clk,		pllb_clk,  1, 1);

static struct clk *main_clks[] = {
	&extal_clk,
	&plla_clk,
	&pllb_clk,
	&i_clk,
	&s_clk,
	&s1_clk,
	&s3_clk,
	&s4_clk,
	&b_clk,
	&out_clk,
	&p_clk,
	&g_clk,
	&z_clk,
};

enum {
	MSTP331,
	MSTP323, MSTP322, MSTP321,
	MSTP114,
	MSTP110, MSTP109,
	MSTP100,
	MSTP030,
	MSTP029, MSTP028, MSTP027, MSTP026, MSTP025, MSTP024, MSTP023, MSTP022, MSTP021,
	MSTP016, MSTP015,
	MSTP007,
	MSTP_NR };

static struct clk mstp_clks[MSTP_NR] = {
	[MSTP331] = SH_CLK_MSTP32(&s4_clk, MSTPCR3, 31, 0), /* MMC */
	[MSTP323] = SH_CLK_MSTP32(&p_clk, MSTPCR3, 23, 0), /* SDHI0 */
	[MSTP322] = SH_CLK_MSTP32(&p_clk, MSTPCR3, 22, 0), /* SDHI1 */
	[MSTP321] = SH_CLK_MSTP32(&p_clk, MSTPCR3, 21, 0), /* SDHI2 */
	[MSTP114] = SH_CLK_MSTP32(&p_clk, MSTPCR1, 14, 0), /* Ether */
	[MSTP110] = SH_CLK_MSTP32(&s_clk, MSTPCR1, 10, 0), /* VIN0 */
	[MSTP109] = SH_CLK_MSTP32(&s_clk, MSTPCR1,  9, 0), /* VIN1 */
	[MSTP100] = SH_CLK_MSTP32(&p_clk, MSTPCR1,  0, 0), /* USB0/1 */
	[MSTP030] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 30, 0), /* I2C0 */
	[MSTP029] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 29, 0), /* I2C1 */
	[MSTP028] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 28, 0), /* I2C2 */
	[MSTP027] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 27, 0), /* I2C3 */
	[MSTP026] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 26, 0), /* SCIF0 */
	[MSTP025] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 25, 0), /* SCIF1 */
	[MSTP024] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 24, 0), /* SCIF2 */
	[MSTP023] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 23, 0), /* SCIF3 */
	[MSTP022] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 22, 0), /* SCIF4 */
	[MSTP021] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 21, 0), /* SCIF5 */
	[MSTP016] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 16, 0), /* TMU0 */
	[MSTP015] = SH_CLK_MSTP32(&p_clk, MSTPCR0, 15, 0), /* TMU1 */
	[MSTP007] = SH_CLK_MSTP32(&p_clk, MSTPCR0,  7, 0), /* HSPI */
};

static struct clk_lookup lookups[] = {
	/* main */
	CLKDEV_CON_ID("shyway_clk",	&s_clk),
	CLKDEV_CON_ID("peripheral_clk",	&p_clk),

	/* MSTP32 clocks */
	CLKDEV_DEV_ID("sh_mmcif", &mstp_clks[MSTP331]), /* MMC */
	CLKDEV_DEV_ID("sh_mobile_sdhi.0", &mstp_clks[MSTP323]), /* SDHI0 */
	CLKDEV_DEV_ID("sh_mobile_sdhi.1", &mstp_clks[MSTP322]), /* SDHI1 */
	CLKDEV_DEV_ID("sh_mobile_sdhi.2", &mstp_clks[MSTP321]), /* SDHI2 */
	CLKDEV_DEV_ID("r8a777x-ether", &mstp_clks[MSTP114]), /* Ether */
	CLKDEV_DEV_ID("r8a7778-vin.0", &mstp_clks[MSTP110]), /* VIN0 */
	CLKDEV_DEV_ID("r8a7778-vin.1", &mstp_clks[MSTP109]), /* VIN1 */
	CLKDEV_DEV_ID("ehci-platform", &mstp_clks[MSTP100]), /* USB EHCI port0/1 */
	CLKDEV_DEV_ID("ohci-platform", &mstp_clks[MSTP100]), /* USB OHCI port0/1 */
	CLKDEV_DEV_ID("i2c-rcar.0", &mstp_clks[MSTP030]), /* I2C0 */
	CLKDEV_DEV_ID("i2c-rcar.1", &mstp_clks[MSTP029]), /* I2C1 */
	CLKDEV_DEV_ID("i2c-rcar.2", &mstp_clks[MSTP028]), /* I2C2 */
	CLKDEV_DEV_ID("i2c-rcar.3", &mstp_clks[MSTP027]), /* I2C3 */
	CLKDEV_DEV_ID("sh-sci.0", &mstp_clks[MSTP026]), /* SCIF0 */
	CLKDEV_DEV_ID("sh-sci.1", &mstp_clks[MSTP025]), /* SCIF1 */
	CLKDEV_DEV_ID("sh-sci.2", &mstp_clks[MSTP024]), /* SCIF2 */
	CLKDEV_DEV_ID("sh-sci.3", &mstp_clks[MSTP023]), /* SCIF3 */
	CLKDEV_DEV_ID("sh-sci.4", &mstp_clks[MSTP022]), /* SCIF4 */
	CLKDEV_DEV_ID("sh-sci.5", &mstp_clks[MSTP021]), /* SCIF6 */
	CLKDEV_DEV_ID("sh_tmu.0", &mstp_clks[MSTP016]), /* TMU00 */
	CLKDEV_DEV_ID("sh_tmu.1", &mstp_clks[MSTP015]), /* TMU01 */
	CLKDEV_DEV_ID("sh-hspi.0", &mstp_clks[MSTP007]), /* HSPI0 */
	CLKDEV_DEV_ID("sh-hspi.1", &mstp_clks[MSTP007]), /* HSPI1 */
	CLKDEV_DEV_ID("sh-hspi.2", &mstp_clks[MSTP007]), /* HSPI2 */
};

void __init r8a7778_clock_init(void)
{
	void __iomem *modemr = ioremap_nocache(MODEMR, PAGE_SIZE);
	u32 mode;
	int k, ret = 0;

	BUG_ON(!modemr);
	mode = ioread32(modemr);
	iounmap(modemr);

	switch (mode & (MD(19) | MD(18) | MD(12) | MD(11))) {
	case MD(19):
		extal_clk.rate = 38000000;
		SH_CLK_SET_RATIO(&plla_clk_ratio,	21, 1);
		SH_CLK_SET_RATIO(&pllb_clk_ratio,	21, 1);
		break;
	case MD(19) | MD(11):
		extal_clk.rate = 33333333;
		SH_CLK_SET_RATIO(&plla_clk_ratio,	24, 1);
		SH_CLK_SET_RATIO(&pllb_clk_ratio,	24, 1);
		break;
	case MD(19) | MD(12):
		extal_clk.rate = 28500000;
		SH_CLK_SET_RATIO(&plla_clk_ratio,	28, 1);
		SH_CLK_SET_RATIO(&pllb_clk_ratio,	28, 1);
		break;
	case MD(19) | MD(12) | MD(11):
		extal_clk.rate = 25000000;
		SH_CLK_SET_RATIO(&plla_clk_ratio,	32, 1);
		SH_CLK_SET_RATIO(&pllb_clk_ratio,	32, 1);
		break;
	case MD(19) | MD(18) | MD(11):
		extal_clk.rate = 33333333;
		SH_CLK_SET_RATIO(&plla_clk_ratio,	24, 1);
		SH_CLK_SET_RATIO(&pllb_clk_ratio,	21, 1);
		break;
	case MD(19) | MD(18) | MD(12):
		extal_clk.rate = 28500000;
		SH_CLK_SET_RATIO(&plla_clk_ratio,	28, 1);
		SH_CLK_SET_RATIO(&pllb_clk_ratio,	21, 1);
		break;
	case MD(19) | MD(18) | MD(12) | MD(11):
		extal_clk.rate = 25000000;
		SH_CLK_SET_RATIO(&plla_clk_ratio,	32, 1);
		SH_CLK_SET_RATIO(&pllb_clk_ratio,	24, 1);
		break;
	default:
		BUG();
	}

	if (mode & MD(1)) {
		SH_CLK_SET_RATIO(&i_clk_ratio,	1, 1);
		SH_CLK_SET_RATIO(&s_clk_ratio,	1, 3);
		SH_CLK_SET_RATIO(&s1_clk_ratio,	1, 6);
		SH_CLK_SET_RATIO(&s3_clk_ratio,	1, 4);
		SH_CLK_SET_RATIO(&s4_clk_ratio,	1, 8);
		SH_CLK_SET_RATIO(&p_clk_ratio,	1, 12);
		SH_CLK_SET_RATIO(&g_clk_ratio,	1, 12);
		if (mode & MD(2)) {
			SH_CLK_SET_RATIO(&b_clk_ratio,		1, 18);
			SH_CLK_SET_RATIO(&out_clk_ratio,	1, 18);
		} else {
			SH_CLK_SET_RATIO(&b_clk_ratio,		1, 12);
			SH_CLK_SET_RATIO(&out_clk_ratio,	1, 12);
		}
	} else {
		SH_CLK_SET_RATIO(&i_clk_ratio,	1, 1);
		SH_CLK_SET_RATIO(&s_clk_ratio,	1, 4);
		SH_CLK_SET_RATIO(&s1_clk_ratio,	1, 8);
		SH_CLK_SET_RATIO(&s3_clk_ratio,	1, 4);
		SH_CLK_SET_RATIO(&s4_clk_ratio,	1, 8);
		SH_CLK_SET_RATIO(&p_clk_ratio,	1, 16);
		SH_CLK_SET_RATIO(&g_clk_ratio,	1, 12);
		if (mode & MD(2)) {
			SH_CLK_SET_RATIO(&b_clk_ratio,		1, 16);
			SH_CLK_SET_RATIO(&out_clk_ratio,	1, 16);
		} else {
			SH_CLK_SET_RATIO(&b_clk_ratio,		1, 12);
			SH_CLK_SET_RATIO(&out_clk_ratio,	1, 12);
		}
	}

	for (k = 0; !ret && (k < ARRAY_SIZE(main_clks)); k++)
		ret = clk_register(main_clks[k]);

	if (!ret)
		ret = sh_clk_mstp_register(mstp_clks, MSTP_NR);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	if (!ret)
		shmobile_clk_init();
	else
		panic("failed to setup r8a7778 clocks\n");
}
