/*
 * ALSA SoC McASP Audio Layer for TI DAVINCI processor
 *
 * Multi-channel Audio Serial Port Driver
 *
 * Author: Nirmal Pandey <n-pandey@ti.com>,
 *         Suresh Rajashekara <suresh.r@ti.com>
 *         Steve Chen <schen@.mvista.com>
 *
 * Copyright:   (C) 2009 MontaVista Software, Inc., <source@mvista.com>
 * Copyright:   (C) 2009  Texas Instruments, India
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/init.h>
#include <fikus/module.h>
#include <fikus/device.h>
#include <fikus/slab.h>
#include <fikus/delay.h>
#include <fikus/io.h>
#include <fikus/pm_runtime.h>
#include <fikus/of.h>
#include <fikus/of_platform.h>
#include <fikus/of_device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "davinci-pcm.h"
#include "davinci-mcasp.h"

/*
 * McASP register definitions
 */
#define DAVINCI_MCASP_PID_REG		0x00
#define DAVINCI_MCASP_PWREMUMGT_REG	0x04

#define DAVINCI_MCASP_PFUNC_REG		0x10
#define DAVINCI_MCASP_PDIR_REG		0x14
#define DAVINCI_MCASP_PDOUT_REG		0x18
#define DAVINCI_MCASP_PDSET_REG		0x1c

#define DAVINCI_MCASP_PDCLR_REG		0x20

#define DAVINCI_MCASP_TLGC_REG		0x30
#define DAVINCI_MCASP_TLMR_REG		0x34

#define DAVINCI_MCASP_GBLCTL_REG	0x44
#define DAVINCI_MCASP_AMUTE_REG		0x48
#define DAVINCI_MCASP_LBCTL_REG		0x4c

#define DAVINCI_MCASP_TXDITCTL_REG	0x50

#define DAVINCI_MCASP_GBLCTLR_REG	0x60
#define DAVINCI_MCASP_RXMASK_REG	0x64
#define DAVINCI_MCASP_RXFMT_REG		0x68
#define DAVINCI_MCASP_RXFMCTL_REG	0x6c

#define DAVINCI_MCASP_ACLKRCTL_REG	0x70
#define DAVINCI_MCASP_AHCLKRCTL_REG	0x74
#define DAVINCI_MCASP_RXTDM_REG		0x78
#define DAVINCI_MCASP_EVTCTLR_REG	0x7c

#define DAVINCI_MCASP_RXSTAT_REG	0x80
#define DAVINCI_MCASP_RXTDMSLOT_REG	0x84
#define DAVINCI_MCASP_RXCLKCHK_REG	0x88
#define DAVINCI_MCASP_REVTCTL_REG	0x8c

#define DAVINCI_MCASP_GBLCTLX_REG	0xa0
#define DAVINCI_MCASP_TXMASK_REG	0xa4
#define DAVINCI_MCASP_TXFMT_REG		0xa8
#define DAVINCI_MCASP_TXFMCTL_REG	0xac

#define DAVINCI_MCASP_ACLKXCTL_REG	0xb0
#define DAVINCI_MCASP_AHCLKXCTL_REG	0xb4
#define DAVINCI_MCASP_TXTDM_REG		0xb8
#define DAVINCI_MCASP_EVTCTLX_REG	0xbc

#define DAVINCI_MCASP_TXSTAT_REG	0xc0
#define DAVINCI_MCASP_TXTDMSLOT_REG	0xc4
#define DAVINCI_MCASP_TXCLKCHK_REG	0xc8
#define DAVINCI_MCASP_XEVTCTL_REG	0xcc

/* Left(even TDM Slot) Channel Status Register File */
#define DAVINCI_MCASP_DITCSRA_REG	0x100
/* Right(odd TDM slot) Channel Status Register File */
#define DAVINCI_MCASP_DITCSRB_REG	0x118
/* Left(even TDM slot) User Data Register File */
#define DAVINCI_MCASP_DITUDRA_REG	0x130
/* Right(odd TDM Slot) User Data Register File */
#define DAVINCI_MCASP_DITUDRB_REG	0x148

/* Serializer n Control Register */
#define DAVINCI_MCASP_XRSRCTL_BASE_REG	0x180
#define DAVINCI_MCASP_XRSRCTL_REG(n)	(DAVINCI_MCASP_XRSRCTL_BASE_REG + \
						(n << 2))

/* Transmit Buffer for Serializer n */
#define DAVINCI_MCASP_TXBUF_REG		0x200
/* Receive Buffer for Serializer n */
#define DAVINCI_MCASP_RXBUF_REG		0x280

/* McASP FIFO Registers */
#define DAVINCI_MCASP_WFIFOCTL		(0x1010)
#define DAVINCI_MCASP_WFIFOSTS		(0x1014)
#define DAVINCI_MCASP_RFIFOCTL		(0x1018)
#define DAVINCI_MCASP_RFIFOSTS		(0x101C)
#define MCASP_VER3_WFIFOCTL		(0x1000)
#define MCASP_VER3_WFIFOSTS		(0x1004)
#define MCASP_VER3_RFIFOCTL		(0x1008)
#define MCASP_VER3_RFIFOSTS		(0x100C)

/*
 * DAVINCI_MCASP_PWREMUMGT_REG - Power Down and Emulation Management
 *     Register Bits
 */
#define MCASP_FREE	BIT(0)
#define MCASP_SOFT	BIT(1)

/*
 * DAVINCI_MCASP_PFUNC_REG - Pin Function / GPIO Enable Register Bits
 */
#define AXR(n)		(1<<n)
#define PFUNC_AMUTE	BIT(25)
#define ACLKX		BIT(26)
#define AHCLKX		BIT(27)
#define AFSX		BIT(28)
#define ACLKR		BIT(29)
#define AHCLKR		BIT(30)
#define AFSR		BIT(31)

/*
 * DAVINCI_MCASP_PDIR_REG - Pin Direction Register Bits
 */
#define AXR(n)		(1<<n)
#define PDIR_AMUTE	BIT(25)
#define ACLKX		BIT(26)
#define AHCLKX		BIT(27)
#define AFSX		BIT(28)
#define ACLKR		BIT(29)
#define AHCLKR		BIT(30)
#define AFSR		BIT(31)

/*
 * DAVINCI_MCASP_TXDITCTL_REG - Transmit DIT Control Register Bits
 */
#define DITEN	BIT(0)	/* Transmit DIT mode enable/disable */
#define VA	BIT(2)
#define VB	BIT(3)

/*
 * DAVINCI_MCASP_TXFMT_REG - Transmit Bitstream Format Register Bits
 */
#define TXROT(val)	(val)
#define TXSEL		BIT(3)
#define TXSSZ(val)	(val<<4)
#define TXPBIT(val)	(val<<8)
#define TXPAD(val)	(val<<13)
#define TXORD		BIT(15)
#define FSXDLY(val)	(val<<16)

/*
 * DAVINCI_MCASP_RXFMT_REG - Receive Bitstream Format Register Bits
 */
#define RXROT(val)	(val)
#define RXSEL		BIT(3)
#define RXSSZ(val)	(val<<4)
#define RXPBIT(val)	(val<<8)
#define RXPAD(val)	(val<<13)
#define RXORD		BIT(15)
#define FSRDLY(val)	(val<<16)

/*
 * DAVINCI_MCASP_TXFMCTL_REG -  Transmit Frame Control Register Bits
 */
#define FSXPOL		BIT(0)
#define AFSXE		BIT(1)
#define FSXDUR		BIT(4)
#define FSXMOD(val)	(val<<7)

/*
 * DAVINCI_MCASP_RXFMCTL_REG - Receive Frame Control Register Bits
 */
#define FSRPOL		BIT(0)
#define AFSRE		BIT(1)
#define FSRDUR		BIT(4)
#define FSRMOD(val)	(val<<7)

/*
 * DAVINCI_MCASP_ACLKXCTL_REG - Transmit Clock Control Register Bits
 */
#define ACLKXDIV(val)	(val)
#define ACLKXE		BIT(5)
#define TX_ASYNC	BIT(6)
#define ACLKXPOL	BIT(7)
#define ACLKXDIV_MASK	0x1f

/*
 * DAVINCI_MCASP_ACLKRCTL_REG Receive Clock Control Register Bits
 */
#define ACLKRDIV(val)	(val)
#define ACLKRE		BIT(5)
#define RX_ASYNC	BIT(6)
#define ACLKRPOL	BIT(7)
#define ACLKRDIV_MASK	0x1f

/*
 * DAVINCI_MCASP_AHCLKXCTL_REG - High Frequency Transmit Clock Control
 *     Register Bits
 */
#define AHCLKXDIV(val)	(val)
#define AHCLKXPOL	BIT(14)
#define AHCLKXE		BIT(15)
#define AHCLKXDIV_MASK	0xfff

/*
 * DAVINCI_MCASP_AHCLKRCTL_REG - High Frequency Receive Clock Control
 *     Register Bits
 */
#define AHCLKRDIV(val)	(val)
#define AHCLKRPOL	BIT(14)
#define AHCLKRE		BIT(15)
#define AHCLKRDIV_MASK	0xfff

/*
 * DAVINCI_MCASP_XRSRCTL_BASE_REG -  Serializer Control Register Bits
 */
#define MODE(val)	(val)
#define DISMOD		(val)(val<<2)
#define TXSTATE		BIT(4)
#define RXSTATE		BIT(5)
#define SRMOD_MASK	3
#define SRMOD_INACTIVE	0

/*
 * DAVINCI_MCASP_LBCTL_REG - Loop Back Control Register Bits
 */
#define LBEN		BIT(0)
#define LBORD		BIT(1)
#define LBGENMODE(val)	(val<<2)

/*
 * DAVINCI_MCASP_TXTDMSLOT_REG - Transmit TDM Slot Register configuration
 */
#define TXTDMS(n)	(1<<n)

/*
 * DAVINCI_MCASP_RXTDMSLOT_REG - Receive TDM Slot Register configuration
 */
#define RXTDMS(n)	(1<<n)

/*
 * DAVINCI_MCASP_GBLCTL_REG -  Global Control Register Bits
 */
#define RXCLKRST	BIT(0)	/* Receiver Clock Divider Reset */
#define RXHCLKRST	BIT(1)	/* Receiver High Frequency Clock Divider */
#define RXSERCLR	BIT(2)	/* Receiver Serializer Clear */
#define RXSMRST		BIT(3)	/* Receiver State Machine Reset */
#define RXFSRST		BIT(4)	/* Frame Sync Generator Reset */
#define TXCLKRST	BIT(8)	/* Transmitter Clock Divider Reset */
#define TXHCLKRST	BIT(9)	/* Transmitter High Frequency Clock Divider*/
#define TXSERCLR	BIT(10)	/* Transmit Serializer Clear */
#define TXSMRST		BIT(11)	/* Transmitter State Machine Reset */
#define TXFSRST		BIT(12)	/* Frame Sync Generator Reset */

/*
 * DAVINCI_MCASP_AMUTE_REG -  Mute Control Register Bits
 */
#define MUTENA(val)	(val)
#define MUTEINPOL	BIT(2)
#define MUTEINENA	BIT(3)
#define MUTEIN		BIT(4)
#define MUTER		BIT(5)
#define MUTEX		BIT(6)
#define MUTEFSR		BIT(7)
#define MUTEFSX		BIT(8)
#define MUTEBADCLKR	BIT(9)
#define MUTEBADCLKX	BIT(10)
#define MUTERXDMAERR	BIT(11)
#define MUTETXDMAERR	BIT(12)

/*
 * DAVINCI_MCASP_REVTCTL_REG - Receiver DMA Event Control Register bits
 */
#define RXDATADMADIS	BIT(0)

/*
 * DAVINCI_MCASP_XEVTCTL_REG - Transmitter DMA Event Control Register bits
 */
#define TXDATADMADIS	BIT(0)

/*
 * DAVINCI_MCASP_W[R]FIFOCTL - Write/Read FIFO Control Register bits
 */
#define FIFO_ENABLE	BIT(16)
#define NUMEVT_MASK	(0xFF << 8)
#define NUMDMA_MASK	(0xFF)

#define DAVINCI_MCASP_NUM_SERIALIZER	16

static inline void mcasp_set_bits(void __iomem *reg, u32 val)
{
	__raw_writel(__raw_readl(reg) | val, reg);
}

static inline void mcasp_clr_bits(void __iomem *reg, u32 val)
{
	__raw_writel((__raw_readl(reg) & ~(val)), reg);
}

static inline void mcasp_mod_bits(void __iomem *reg, u32 val, u32 mask)
{
	__raw_writel((__raw_readl(reg) & ~mask) | val, reg);
}

static inline void mcasp_set_reg(void __iomem *reg, u32 val)
{
	__raw_writel(val, reg);
}

static inline u32 mcasp_get_reg(void __iomem *reg)
{
	return (unsigned int)__raw_readl(reg);
}

static inline void mcasp_set_ctl_reg(void __iomem *regs, u32 val)
{
	int i = 0;

	mcasp_set_bits(regs, val);

	/* programming GBLCTL needs to read back from GBLCTL and verfiy */
	/* loop count is to avoid the lock-up */
	for (i = 0; i < 1000; i++) {
		if ((mcasp_get_reg(regs) & val) == val)
			break;
	}

	if (i == 1000 && ((mcasp_get_reg(regs) & val) != val))
		printk(KERN_ERR "GBLCTL write error\n");
}

static void mcasp_start_rx(struct davinci_audio_dev *dev)
{
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXHCLKRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXCLKRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXSERCLR);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_RXBUF_REG, 0);

	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXSMRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXFSRST);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_RXBUF_REG, 0);

	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXSMRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXFSRST);
}

static void mcasp_start_tx(struct davinci_audio_dev *dev)
{
	u8 offset = 0, i;
	u32 cnt;

	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXHCLKRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXCLKRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXSERCLR);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXBUF_REG, 0);

	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXSMRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXFSRST);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXBUF_REG, 0);
	for (i = 0; i < dev->num_serializer; i++) {
		if (dev->serial_dir[i] == TX_MODE) {
			offset = i;
			break;
		}
	}

	/* wait for TX ready */
	cnt = 0;
	while (!(mcasp_get_reg(dev->base + DAVINCI_MCASP_XRSRCTL_REG(offset)) &
		 TXSTATE) && (cnt < 100000))
		cnt++;

	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXBUF_REG, 0);
}

static void davinci_mcasp_start(struct davinci_audio_dev *dev, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (dev->txnumevt) {	/* enable FIFO */
			switch (dev->version) {
			case MCASP_VERSION_3:
				mcasp_clr_bits(dev->base + MCASP_VER3_WFIFOCTL,
								FIFO_ENABLE);
				mcasp_set_bits(dev->base + MCASP_VER3_WFIFOCTL,
								FIFO_ENABLE);
				break;
			default:
				mcasp_clr_bits(dev->base +
					DAVINCI_MCASP_WFIFOCTL,	FIFO_ENABLE);
				mcasp_set_bits(dev->base +
					DAVINCI_MCASP_WFIFOCTL,	FIFO_ENABLE);
			}
		}
		mcasp_start_tx(dev);
	} else {
		if (dev->rxnumevt) {	/* enable FIFO */
			switch (dev->version) {
			case MCASP_VERSION_3:
				mcasp_clr_bits(dev->base + MCASP_VER3_RFIFOCTL,
								FIFO_ENABLE);
				mcasp_set_bits(dev->base + MCASP_VER3_RFIFOCTL,
								FIFO_ENABLE);
				break;
			default:
				mcasp_clr_bits(dev->base +
					DAVINCI_MCASP_RFIFOCTL,	FIFO_ENABLE);
				mcasp_set_bits(dev->base +
					DAVINCI_MCASP_RFIFOCTL,	FIFO_ENABLE);
			}
		}
		mcasp_start_rx(dev);
	}
}

static void mcasp_stop_rx(struct davinci_audio_dev *dev)
{
	mcasp_set_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, 0);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_RXSTAT_REG, 0xFFFFFFFF);
}

static void mcasp_stop_tx(struct davinci_audio_dev *dev)
{
	mcasp_set_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, 0);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXSTAT_REG, 0xFFFFFFFF);
}

static void davinci_mcasp_stop(struct davinci_audio_dev *dev, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (dev->txnumevt) {	/* disable FIFO */
			switch (dev->version) {
			case MCASP_VERSION_3:
				mcasp_clr_bits(dev->base + MCASP_VER3_WFIFOCTL,
								FIFO_ENABLE);
				break;
			default:
				mcasp_clr_bits(dev->base +
					DAVINCI_MCASP_WFIFOCTL,	FIFO_ENABLE);
			}
		}
		mcasp_stop_tx(dev);
	} else {
		if (dev->rxnumevt) {	/* disable FIFO */
			switch (dev->version) {
			case MCASP_VERSION_3:
				mcasp_clr_bits(dev->base + MCASP_VER3_RFIFOCTL,
								FIFO_ENABLE);
			break;

			default:
				mcasp_clr_bits(dev->base +
					DAVINCI_MCASP_RFIFOCTL,	FIFO_ENABLE);
			}
		}
		mcasp_stop_rx(dev);
	}
}

static int davinci_mcasp_set_dai_fmt(struct snd_soc_dai *cpu_dai,
					 unsigned int fmt)
{
	struct davinci_audio_dev *dev = snd_soc_dai_get_drvdata(cpu_dai);
	void __iomem *base = dev->base;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_B:
	case SND_SOC_DAIFMT_AC97:
		mcasp_clr_bits(dev->base + DAVINCI_MCASP_TXFMCTL_REG, FSXDUR);
		mcasp_clr_bits(dev->base + DAVINCI_MCASP_RXFMCTL_REG, FSRDUR);
		break;
	default:
		/* configure a full-word SYNC pulse (LRCLK) */
		mcasp_set_bits(dev->base + DAVINCI_MCASP_TXFMCTL_REG, FSXDUR);
		mcasp_set_bits(dev->base + DAVINCI_MCASP_RXFMCTL_REG, FSRDUR);

		/* make 1st data bit occur one ACLK cycle after the frame sync */
		mcasp_set_bits(dev->base + DAVINCI_MCASP_TXFMT_REG, FSXDLY(1));
		mcasp_set_bits(dev->base + DAVINCI_MCASP_RXFMT_REG, FSRDLY(1));
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		/* codec is clock and frame slave */
		mcasp_set_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXE);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMCTL_REG, AFSXE);

		mcasp_set_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRE);
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMCTL_REG, AFSRE);

		mcasp_set_bits(base + DAVINCI_MCASP_PDIR_REG,
				ACLKX | ACLKR);
		mcasp_set_bits(base + DAVINCI_MCASP_PDIR_REG,
				AFSX | AFSR);
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		/* codec is clock master and frame slave */
		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXE);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMCTL_REG, AFSXE);

		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRE);
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMCTL_REG, AFSRE);

		mcasp_clr_bits(base + DAVINCI_MCASP_PDIR_REG,
				ACLKX | ACLKR);
		mcasp_set_bits(base + DAVINCI_MCASP_PDIR_REG,
				AFSX | AFSR);
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		/* codec is clock and frame master */
		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXE);
		mcasp_clr_bits(base + DAVINCI_MCASP_TXFMCTL_REG, AFSXE);

		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRE);
		mcasp_clr_bits(base + DAVINCI_MCASP_RXFMCTL_REG, AFSRE);

		mcasp_clr_bits(base + DAVINCI_MCASP_PDIR_REG,
				ACLKX | AHCLKX | AFSX | ACLKR | AHCLKR | AFSR);
		break;

	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_IB_NF:
		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXPOL);
		mcasp_clr_bits(base + DAVINCI_MCASP_TXFMCTL_REG, FSXPOL);

		mcasp_set_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRPOL);
		mcasp_clr_bits(base + DAVINCI_MCASP_RXFMCTL_REG, FSRPOL);
		break;

	case SND_SOC_DAIFMT_NB_IF:
		mcasp_set_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXPOL);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMCTL_REG, FSXPOL);

		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRPOL);
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMCTL_REG, FSRPOL);
		break;

	case SND_SOC_DAIFMT_IB_IF:
		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXPOL);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMCTL_REG, FSXPOL);

		mcasp_set_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRPOL);
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMCTL_REG, FSRPOL);
		break;

	case SND_SOC_DAIFMT_NB_NF:
		mcasp_set_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXPOL);
		mcasp_clr_bits(base + DAVINCI_MCASP_TXFMCTL_REG, FSXPOL);

		mcasp_set_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRPOL);
		mcasp_clr_bits(base + DAVINCI_MCASP_RXFMCTL_REG, FSRPOL);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int davinci_mcasp_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	struct davinci_audio_dev *dev = snd_soc_dai_get_drvdata(dai);

	switch (div_id) {
	case 0:		/* MCLK divider */
		mcasp_mod_bits(dev->base + DAVINCI_MCASP_AHCLKXCTL_REG,
			       AHCLKXDIV(div - 1), AHCLKXDIV_MASK);
		mcasp_mod_bits(dev->base + DAVINCI_MCASP_AHCLKRCTL_REG,
			       AHCLKRDIV(div - 1), AHCLKRDIV_MASK);
		break;

	case 1:		/* BCLK divider */
		mcasp_mod_bits(dev->base + DAVINCI_MCASP_ACLKXCTL_REG,
			       ACLKXDIV(div - 1), ACLKXDIV_MASK);
		mcasp_mod_bits(dev->base + DAVINCI_MCASP_ACLKRCTL_REG,
			       ACLKRDIV(div - 1), ACLKRDIV_MASK);
		break;

	case 2:		/* BCLK/LRCLK ratio */
		dev->bclk_lrclk_ratio = div;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int davinci_mcasp_set_sysclk(struct snd_soc_dai *dai, int clk_id,
				    unsigned int freq, int dir)
{
	struct davinci_audio_dev *dev = snd_soc_dai_get_drvdata(dai);

	if (dir == SND_SOC_CLOCK_OUT) {
		mcasp_set_bits(dev->base + DAVINCI_MCASP_AHCLKXCTL_REG, AHCLKXE);
		mcasp_set_bits(dev->base + DAVINCI_MCASP_AHCLKRCTL_REG, AHCLKRE);
		mcasp_set_bits(dev->base + DAVINCI_MCASP_PDIR_REG, AHCLKX);
	} else {
		mcasp_clr_bits(dev->base + DAVINCI_MCASP_AHCLKXCTL_REG, AHCLKXE);
		mcasp_clr_bits(dev->base + DAVINCI_MCASP_AHCLKRCTL_REG, AHCLKRE);
		mcasp_clr_bits(dev->base + DAVINCI_MCASP_PDIR_REG, AHCLKX);
	}

	return 0;
}

static int davinci_config_channel_size(struct davinci_audio_dev *dev,
				       int word_length)
{
	u32 fmt;
	u32 tx_rotate = (word_length / 4) & 0x7;
	u32 rx_rotate = (32 - word_length) / 4;
	u32 mask = (1ULL << word_length) - 1;

	/*
	 * if s BCLK-to-LRCLK ratio has been configured via the set_clkdiv()
	 * callback, take it into account here. That allows us to for example
	 * send 32 bits per channel to the codec, while only 16 of them carry
	 * audio payload.
	 * The clock ratio is given for a full period of data (for I2S format
	 * both left and right channels), so it has to be divided by number of
	 * tdm-slots (for I2S - divided by 2).
	 */
	if (dev->bclk_lrclk_ratio)
		word_length = dev->bclk_lrclk_ratio / dev->tdm_slots;

	/* mapping of the XSSZ bit-field as described in the datasheet */
	fmt = (word_length >> 1) - 1;

	if (dev->op_mode != DAVINCI_MCASP_DIT_MODE) {
		mcasp_mod_bits(dev->base + DAVINCI_MCASP_RXFMT_REG,
				RXSSZ(fmt), RXSSZ(0x0F));
		mcasp_mod_bits(dev->base + DAVINCI_MCASP_TXFMT_REG,
				TXSSZ(fmt), TXSSZ(0x0F));
		mcasp_mod_bits(dev->base + DAVINCI_MCASP_TXFMT_REG,
				TXROT(tx_rotate), TXROT(7));
		mcasp_mod_bits(dev->base + DAVINCI_MCASP_RXFMT_REG,
				RXROT(rx_rotate), RXROT(7));
		mcasp_set_reg(dev->base + DAVINCI_MCASP_RXMASK_REG,
				mask);
	}

	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXMASK_REG, mask);

	return 0;
}

static int davinci_hw_common_param(struct davinci_audio_dev *dev, int stream,
				    int channels)
{
	int i;
	u8 tx_ser = 0;
	u8 rx_ser = 0;
	u8 ser;
	u8 slots = dev->tdm_slots;
	u8 max_active_serializers = (channels + slots - 1) / slots;
	/* Default configuration */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_PWREMUMGT_REG, MCASP_SOFT);

	/* All PINS as McASP */
	mcasp_set_reg(dev->base + DAVINCI_MCASP_PFUNC_REG, 0x00000000);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		mcasp_set_reg(dev->base + DAVINCI_MCASP_TXSTAT_REG, 0xFFFFFFFF);
		mcasp_clr_bits(dev->base + DAVINCI_MCASP_XEVTCTL_REG,
				TXDATADMADIS);
	} else {
		mcasp_set_reg(dev->base + DAVINCI_MCASP_RXSTAT_REG, 0xFFFFFFFF);
		mcasp_clr_bits(dev->base + DAVINCI_MCASP_REVTCTL_REG,
				RXDATADMADIS);
	}

	for (i = 0; i < dev->num_serializer; i++) {
		mcasp_set_bits(dev->base + DAVINCI_MCASP_XRSRCTL_REG(i),
					dev->serial_dir[i]);
		if (dev->serial_dir[i] == TX_MODE &&
					tx_ser < max_active_serializers) {
			mcasp_set_bits(dev->base + DAVINCI_MCASP_PDIR_REG,
					AXR(i));
			tx_ser++;
		} else if (dev->serial_dir[i] == RX_MODE &&
					rx_ser < max_active_serializers) {
			mcasp_clr_bits(dev->base + DAVINCI_MCASP_PDIR_REG,
					AXR(i));
			rx_ser++;
		} else {
			mcasp_mod_bits(dev->base + DAVINCI_MCASP_XRSRCTL_REG(i),
					SRMOD_INACTIVE, SRMOD_MASK);
		}
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		ser = tx_ser;
	else
		ser = rx_ser;

	if (ser < max_active_serializers) {
		dev_warn(dev->dev, "stream has more channels (%d) than are "
			"enabled in mcasp (%d)\n", channels, ser * slots);
		return -EINVAL;
	}

	if (dev->txnumevt && stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (dev->txnumevt * tx_ser > 64)
			dev->txnumevt = 1;

		switch (dev->version) {
		case MCASP_VERSION_3:
			mcasp_mod_bits(dev->base + MCASP_VER3_WFIFOCTL, tx_ser,
								NUMDMA_MASK);
			mcasp_mod_bits(dev->base + MCASP_VER3_WFIFOCTL,
				((dev->txnumevt * tx_ser) << 8), NUMEVT_MASK);
			break;
		default:
			mcasp_mod_bits(dev->base + DAVINCI_MCASP_WFIFOCTL,
							tx_ser,	NUMDMA_MASK);
			mcasp_mod_bits(dev->base + DAVINCI_MCASP_WFIFOCTL,
				((dev->txnumevt * tx_ser) << 8), NUMEVT_MASK);
		}
	}

	if (dev->rxnumevt && stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (dev->rxnumevt * rx_ser > 64)
			dev->rxnumevt = 1;
		switch (dev->version) {
		case MCASP_VERSION_3:
			mcasp_mod_bits(dev->base + MCASP_VER3_RFIFOCTL, rx_ser,
								NUMDMA_MASK);
			mcasp_mod_bits(dev->base + MCASP_VER3_RFIFOCTL,
				((dev->rxnumevt * rx_ser) << 8), NUMEVT_MASK);
			break;
		default:
			mcasp_mod_bits(dev->base + DAVINCI_MCASP_RFIFOCTL,
							rx_ser,	NUMDMA_MASK);
			mcasp_mod_bits(dev->base + DAVINCI_MCASP_RFIFOCTL,
				((dev->rxnumevt * rx_ser) << 8), NUMEVT_MASK);
		}
	}

	return 0;
}

static void davinci_hw_param(struct davinci_audio_dev *dev, int stream)
{
	int i, active_slots;
	u32 mask = 0;

	active_slots = (dev->tdm_slots > 31) ? 32 : dev->tdm_slots;
	for (i = 0; i < active_slots; i++)
		mask |= (1 << i);

	mcasp_clr_bits(dev->base + DAVINCI_MCASP_ACLKXCTL_REG, TX_ASYNC);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* bit stream is MSB first  with no delay */
		/* DSP_B mode */
		mcasp_set_reg(dev->base + DAVINCI_MCASP_TXTDM_REG, mask);
		mcasp_set_bits(dev->base + DAVINCI_MCASP_TXFMT_REG, TXORD);

		if ((dev->tdm_slots >= 2) && (dev->tdm_slots <= 32))
			mcasp_mod_bits(dev->base + DAVINCI_MCASP_TXFMCTL_REG,
					FSXMOD(dev->tdm_slots), FSXMOD(0x1FF));
		else
			printk(KERN_ERR "playback tdm slot %d not supported\n",
				dev->tdm_slots);
	} else {
		/* bit stream is MSB first with no delay */
		/* DSP_B mode */
		mcasp_set_bits(dev->base + DAVINCI_MCASP_RXFMT_REG, RXORD);
		mcasp_set_reg(dev->base + DAVINCI_MCASP_RXTDM_REG, mask);

		if ((dev->tdm_slots >= 2) && (dev->tdm_slots <= 32))
			mcasp_mod_bits(dev->base + DAVINCI_MCASP_RXFMCTL_REG,
					FSRMOD(dev->tdm_slots), FSRMOD(0x1FF));
		else
			printk(KERN_ERR "capture tdm slot %d not supported\n",
				dev->tdm_slots);
	}
}

/* S/PDIF */
static void davinci_hw_dit_param(struct davinci_audio_dev *dev)
{
	/* Set the TX format : 24 bit right rotation, 32 bit slot, Pad 0
	   and LSB first */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_TXFMT_REG,
						TXROT(6) | TXSSZ(15));

	/* Set TX frame synch : DIT Mode, 1 bit width, internal, rising edge */
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXFMCTL_REG,
						AFSXE | FSXMOD(0x180));

	/* Set the TX tdm : for all the slots */
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXTDM_REG, 0xFFFFFFFF);

	/* Set the TX clock controls : div = 1 and internal */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_ACLKXCTL_REG,
						ACLKXE | TX_ASYNC);

	mcasp_clr_bits(dev->base + DAVINCI_MCASP_XEVTCTL_REG, TXDATADMADIS);

	/* Only 44100 and 48000 are valid, both have the same setting */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_AHCLKXCTL_REG, AHCLKXDIV(3));

	/* Enable the DIT */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_TXDITCTL_REG, DITEN);
}

static int davinci_mcasp_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params,
					struct snd_soc_dai *cpu_dai)
{
	struct davinci_audio_dev *dev = snd_soc_dai_get_drvdata(cpu_dai);
	struct davinci_pcm_dma_params *dma_params =
					&dev->dma_params[substream->stream];
	int word_length;
	u8 fifo_level;
	u8 slots = dev->tdm_slots;
	u8 active_serializers;
	int channels;
	struct snd_interval *pcm_channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);
	channels = pcm_channels->min;

	active_serializers = (channels + slots - 1) / slots;

	if (davinci_hw_common_param(dev, substream->stream, channels) == -EINVAL)
		return -EINVAL;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		fifo_level = dev->txnumevt * active_serializers;
	else
		fifo_level = dev->rxnumevt * active_serializers;

	if (dev->op_mode == DAVINCI_MCASP_DIT_MODE)
		davinci_hw_dit_param(dev);
	else
		davinci_hw_param(dev, substream->stream);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_U8:
	case SNDRV_PCM_FORMAT_S8:
		dma_params->data_type = 1;
		word_length = 8;
		break;

	case SNDRV_PCM_FORMAT_U16_LE:
	case SNDRV_PCM_FORMAT_S16_LE:
		dma_params->data_type = 2;
		word_length = 16;
		break;

	case SNDRV_PCM_FORMAT_U24_3LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
		dma_params->data_type = 3;
		word_length = 24;
		break;

	case SNDRV_PCM_FORMAT_U24_LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_U32_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		dma_params->data_type = 4;
		word_length = 32;
		break;

	default:
		printk(KERN_WARNING "davinci-mcasp: unsupported PCM format");
		return -EINVAL;
	}

	if (dev->version == MCASP_VERSION_2 && !fifo_level)
		dma_params->acnt = 4;
	else
		dma_params->acnt = dma_params->data_type;

	dma_params->fifo_level = fifo_level;
	davinci_config_channel_size(dev, word_length);

	return 0;
}

static int davinci_mcasp_trigger(struct snd_pcm_substream *substream,
				     int cmd, struct snd_soc_dai *cpu_dai)
{
	struct davinci_audio_dev *dev = snd_soc_dai_get_drvdata(cpu_dai);
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ret = pm_runtime_get_sync(dev->dev);
		if (IS_ERR_VALUE(ret))
			dev_err(dev->dev, "pm_runtime_get_sync() failed\n");
		davinci_mcasp_start(dev, substream->stream);
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
		davinci_mcasp_stop(dev, substream->stream);
		ret = pm_runtime_put_sync(dev->dev);
		if (IS_ERR_VALUE(ret))
			dev_err(dev->dev, "pm_runtime_put_sync() failed\n");
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		davinci_mcasp_stop(dev, substream->stream);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static int davinci_mcasp_startup(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct davinci_audio_dev *dev = snd_soc_dai_get_drvdata(dai);

	snd_soc_dai_set_dma_data(dai, substream, dev->dma_params);
	return 0;
}

static const struct snd_soc_dai_ops davinci_mcasp_dai_ops = {
	.startup	= davinci_mcasp_startup,
	.trigger	= davinci_mcasp_trigger,
	.hw_params	= davinci_mcasp_hw_params,
	.set_fmt	= davinci_mcasp_set_dai_fmt,
	.set_clkdiv	= davinci_mcasp_set_clkdiv,
	.set_sysclk	= davinci_mcasp_set_sysclk,
};

#define DAVINCI_MCASP_PCM_FMTS (SNDRV_PCM_FMTBIT_S8 | \
				SNDRV_PCM_FMTBIT_U8 | \
				SNDRV_PCM_FMTBIT_S16_LE | \
				SNDRV_PCM_FMTBIT_U16_LE | \
				SNDRV_PCM_FMTBIT_S24_LE | \
				SNDRV_PCM_FMTBIT_U24_LE | \
				SNDRV_PCM_FMTBIT_S24_3LE | \
				SNDRV_PCM_FMTBIT_U24_3LE | \
				SNDRV_PCM_FMTBIT_S32_LE | \
				SNDRV_PCM_FMTBIT_U32_LE)

static struct snd_soc_dai_driver davinci_mcasp_dai[] = {
	{
		.name		= "davinci-mcasp.0",
		.playback	= {
			.channels_min	= 2,
			.channels_max	= 32 * 16,
			.rates 		= DAVINCI_MCASP_RATES,
			.formats	= DAVINCI_MCASP_PCM_FMTS,
		},
		.capture 	= {
			.channels_min 	= 2,
			.channels_max	= 32 * 16,
			.rates 		= DAVINCI_MCASP_RATES,
			.formats	= DAVINCI_MCASP_PCM_FMTS,
		},
		.ops 		= &davinci_mcasp_dai_ops,

	},
	{
		"davinci-mcasp.1",
		.playback 	= {
			.channels_min	= 1,
			.channels_max	= 384,
			.rates		= DAVINCI_MCASP_RATES,
			.formats	= DAVINCI_MCASP_PCM_FMTS,
		},
		.ops 		= &davinci_mcasp_dai_ops,
	},

};

static const struct snd_soc_component_driver davinci_mcasp_component = {
	.name		= "davinci-mcasp",
};

static const struct of_device_id mcasp_dt_ids[] = {
	{
		.compatible = "ti,dm646x-mcasp-audio",
		.data = (void *)MCASP_VERSION_1,
	},
	{
		.compatible = "ti,da830-mcasp-audio",
		.data = (void *)MCASP_VERSION_2,
	},
	{
		.compatible = "ti,omap2-mcasp-audio",
		.data = (void *)MCASP_VERSION_3,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mcasp_dt_ids);

static struct snd_platform_data *davinci_mcasp_set_pdata_from_of(
						struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct snd_platform_data *pdata = NULL;
	const struct of_device_id *match =
			of_match_device(mcasp_dt_ids, &pdev->dev);

	const u32 *of_serial_dir32;
	u8 *of_serial_dir;
	u32 val;
	int i, ret = 0;

	if (pdev->dev.platform_data) {
		pdata = pdev->dev.platform_data;
		return pdata;
	} else if (match) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			ret = -ENOMEM;
			goto nodata;
		}
	} else {
		/* control shouldn't reach here. something is wrong */
		ret = -EINVAL;
		goto nodata;
	}

	if (match->data)
		pdata->version = (u8)((int)match->data);

	ret = of_property_read_u32(np, "op-mode", &val);
	if (ret >= 0)
		pdata->op_mode = val;

	ret = of_property_read_u32(np, "tdm-slots", &val);
	if (ret >= 0) {
		if (val < 2 || val > 32) {
			dev_err(&pdev->dev,
				"tdm-slots must be in rage [2-32]\n");
			ret = -EINVAL;
			goto nodata;
		}

		pdata->tdm_slots = val;
	}

	ret = of_property_read_u32(np, "num-serializer", &val);
	if (ret >= 0)
		pdata->num_serializer = val;

	of_serial_dir32 = of_get_property(np, "serial-dir", &val);
	val /= sizeof(u32);
	if (val != pdata->num_serializer) {
		dev_err(&pdev->dev,
				"num-serializer(%d) != serial-dir size(%d)\n",
				pdata->num_serializer, val);
		ret = -EINVAL;
		goto nodata;
	}

	if (of_serial_dir32) {
		of_serial_dir = devm_kzalloc(&pdev->dev,
						(sizeof(*of_serial_dir) * val),
						GFP_KERNEL);
		if (!of_serial_dir) {
			ret = -ENOMEM;
			goto nodata;
		}

		for (i = 0; i < pdata->num_serializer; i++)
			of_serial_dir[i] = be32_to_cpup(&of_serial_dir32[i]);

		pdata->serial_dir = of_serial_dir;
	}

	ret = of_property_read_u32(np, "tx-num-evt", &val);
	if (ret >= 0)
		pdata->txnumevt = val;

	ret = of_property_read_u32(np, "rx-num-evt", &val);
	if (ret >= 0)
		pdata->rxnumevt = val;

	ret = of_property_read_u32(np, "sram-size-playback", &val);
	if (ret >= 0)
		pdata->sram_size_playback = val;

	ret = of_property_read_u32(np, "sram-size-capture", &val);
	if (ret >= 0)
		pdata->sram_size_capture = val;

	return  pdata;

nodata:
	if (ret < 0) {
		dev_err(&pdev->dev, "Error populating platform data, err %d\n",
			ret);
		pdata = NULL;
	}
	return  pdata;
}

static int davinci_mcasp_probe(struct platform_device *pdev)
{
	struct davinci_pcm_dma_params *dma_data;
	struct resource *mem, *ioarea, *res;
	struct snd_platform_data *pdata;
	struct davinci_audio_dev *dev;
	int ret;

	if (!pdev->dev.platform_data && !pdev->dev.of_node) {
		dev_err(&pdev->dev, "No platform data supplied\n");
		return -EINVAL;
	}

	dev = devm_kzalloc(&pdev->dev, sizeof(struct davinci_audio_dev),
			   GFP_KERNEL);
	if (!dev)
		return	-ENOMEM;

	pdata = davinci_mcasp_set_pdata_from_of(pdev);
	if (!pdata) {
		dev_err(&pdev->dev, "no platform data\n");
		return -EINVAL;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	ioarea = devm_request_mem_region(&pdev->dev, mem->start,
			resource_size(mem), pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "Audio region already claimed\n");
		return -EBUSY;
	}

	pm_runtime_enable(&pdev->dev);

	ret = pm_runtime_get_sync(&pdev->dev);
	if (IS_ERR_VALUE(ret)) {
		dev_err(&pdev->dev, "pm_runtime_get_sync() failed\n");
		return ret;
	}

	dev->base = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
	if (!dev->base) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto err_release_clk;
	}

	dev->op_mode = pdata->op_mode;
	dev->tdm_slots = pdata->tdm_slots;
	dev->num_serializer = pdata->num_serializer;
	dev->serial_dir = pdata->serial_dir;
	dev->version = pdata->version;
	dev->txnumevt = pdata->txnumevt;
	dev->rxnumevt = pdata->rxnumevt;
	dev->dev = &pdev->dev;

	dma_data = &dev->dma_params[SNDRV_PCM_STREAM_PLAYBACK];
	dma_data->asp_chan_q = pdata->asp_chan_q;
	dma_data->ram_chan_q = pdata->ram_chan_q;
	dma_data->sram_pool = pdata->sram_pool;
	dma_data->sram_size = pdata->sram_size_playback;
	dma_data->dma_addr = (dma_addr_t) (pdata->tx_dma_offset +
							mem->start);

	/* first TX, then RX */
	res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!res) {
		dev_err(&pdev->dev, "no DMA resource\n");
		ret = -ENODEV;
		goto err_release_clk;
	}

	dma_data->channel = res->start;

	dma_data = &dev->dma_params[SNDRV_PCM_STREAM_CAPTURE];
	dma_data->asp_chan_q = pdata->asp_chan_q;
	dma_data->ram_chan_q = pdata->ram_chan_q;
	dma_data->sram_pool = pdata->sram_pool;
	dma_data->sram_size = pdata->sram_size_capture;
	dma_data->dma_addr = (dma_addr_t)(pdata->rx_dma_offset +
							mem->start);

	res = platform_get_resource(pdev, IORESOURCE_DMA, 1);
	if (!res) {
		dev_err(&pdev->dev, "no DMA resource\n");
		ret = -ENODEV;
		goto err_release_clk;
	}

	dma_data->channel = res->start;
	dev_set_drvdata(&pdev->dev, dev);
	ret = snd_soc_register_component(&pdev->dev, &davinci_mcasp_component,
					 &davinci_mcasp_dai[pdata->op_mode], 1);

	if (ret != 0)
		goto err_release_clk;

	ret = davinci_soc_platform_register(&pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "register PCM failed: %d\n", ret);
		goto err_unregister_component;
	}

	return 0;

err_unregister_component:
	snd_soc_unregister_component(&pdev->dev);
err_release_clk:
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return ret;
}

static int davinci_mcasp_remove(struct platform_device *pdev)
{

	snd_soc_unregister_component(&pdev->dev);
	davinci_soc_platform_unregister(&pdev->dev);

	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static struct platform_driver davinci_mcasp_driver = {
	.probe		= davinci_mcasp_probe,
	.remove		= davinci_mcasp_remove,
	.driver		= {
		.name	= "davinci-mcasp",
		.owner	= THIS_MODULE,
		.of_match_table = mcasp_dt_ids,
	},
};

module_platform_driver(davinci_mcasp_driver);

MODULE_AUTHOR("Steve Chen");
MODULE_DESCRIPTION("TI DAVINCI McASP SoC Interface");
MODULE_LICENSE("GPL");

