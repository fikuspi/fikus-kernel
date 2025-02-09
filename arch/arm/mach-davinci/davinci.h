/*
 * This file contains the processor specific definitions
 * of the TI DM644x, DM355, DM365, and DM646x.
 *
 * Copyright (C) 2011 Texas Instruments Incorporated
 * Copyright (c) 2007 Deep Root Systems, LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __DAVINCI_H
#define __DAVINCI_H

#include <fikus/clk.h>
#include <fikus/videodev2.h>
#include <fikus/davinci_emac.h>
#include <fikus/platform_device.h>
#include <fikus/spi/spi.h>
#include <fikus/platform_data/davinci_asp.h>
#include <fikus/platform_data/edma.h>
#include <fikus/platform_data/keyscan-davinci.h>
#include <mach/hardware.h>

#include <media/davinci/vpfe_capture.h>
#include <media/davinci/vpif_types.h>
#include <media/davinci/vpss.h>
#include <media/davinci/vpbe_types.h>
#include <media/davinci/vpbe_venc.h>
#include <media/davinci/vpbe.h>
#include <media/davinci/vpbe_osd.h>

#define DAVINCI_SYSTEM_MODULE_BASE	0x01c40000
#define SYSMOD_VDAC_CONFIG		0x2c
#define SYSMOD_VIDCLKCTL		0x38
#define SYSMOD_VPSS_CLKCTL		0x44
#define SYSMOD_VDD3P3VPWDN		0x48
#define SYSMOD_VSCLKDIS			0x6c
#define SYSMOD_PUPDCTL1			0x7c

/* VPSS CLKCTL bit definitions */
#define VPSS_MUXSEL_EXTCLK_ENABLE	BIT(1)
#define VPSS_VENCCLKEN_ENABLE		BIT(3)
#define VPSS_DACCLKEN_ENABLE		BIT(4)
#define VPSS_PLLC2SYSCLK5_ENABLE	BIT(5)

extern void __iomem *davinci_sysmod_base;
#define DAVINCI_SYSMOD_VIRT(x)	(davinci_sysmod_base + (x))
void davinci_map_sysmod(void);

/* DM355 base addresses */
#define DM355_ASYNC_EMIF_CONTROL_BASE	0x01e10000
#define DM355_ASYNC_EMIF_DATA_CE0_BASE	0x02000000

#define ASP1_TX_EVT_EN	1
#define ASP1_RX_EVT_EN	2

/* DM365 base addresses */
#define DM365_ASYNC_EMIF_CONTROL_BASE	0x01d10000
#define DM365_ASYNC_EMIF_DATA_CE0_BASE	0x02000000
#define DM365_ASYNC_EMIF_DATA_CE1_BASE	0x04000000

/* DM644x base addresses */
#define DM644X_ASYNC_EMIF_CONTROL_BASE	0x01e00000
#define DM644X_ASYNC_EMIF_DATA_CE0_BASE 0x02000000
#define DM644X_ASYNC_EMIF_DATA_CE1_BASE 0x04000000
#define DM644X_ASYNC_EMIF_DATA_CE2_BASE 0x06000000
#define DM644X_ASYNC_EMIF_DATA_CE3_BASE 0x08000000

/* DM646x base addresses */
#define DM646X_ASYNC_EMIF_CONTROL_BASE	0x20008000
#define DM646X_ASYNC_EMIF_CS2_SPACE_BASE 0x42000000

/* DM355 function declarations */
void dm355_init(void);
void dm355_init_spi0(unsigned chipselect_mask,
		const struct spi_board_info *info, unsigned len);
void dm355_init_asp1(u32 evt_enable, struct snd_platform_data *pdata);
int dm355_init_video(struct vpfe_config *, struct vpbe_config *);

/* DM365 function declarations */
void dm365_init(void);
void dm365_init_asp(struct snd_platform_data *pdata);
void dm365_init_vc(struct snd_platform_data *pdata);
void dm365_init_ks(struct davinci_ks_platform_data *pdata);
void dm365_init_rtc(void);
void dm365_init_spi0(unsigned chipselect_mask,
			const struct spi_board_info *info, unsigned len);
int dm365_init_video(struct vpfe_config *, struct vpbe_config *);

/* DM644x function declarations */
void dm644x_init(void);
void dm644x_init_asp(struct snd_platform_data *pdata);
int dm644x_init_video(struct vpfe_config *, struct vpbe_config *);

/* DM646x function declarations */
void dm646x_init(void);
void dm646x_init_mcasp0(struct snd_platform_data *pdata);
void dm646x_init_mcasp1(struct snd_platform_data *pdata);
int dm646x_init_edma(struct edma_rsv_info *rsv);
void dm646x_video_init(void);
void dm646x_setup_vpif(struct vpif_display_config *,
		       struct vpif_capture_config *);

extern struct platform_device dm365_serial_device[];
extern struct platform_device dm355_serial_device[];
extern struct platform_device dm644x_serial_device[];
extern struct platform_device dm646x_serial_device[];
#endif /*__DAVINCI_H */
