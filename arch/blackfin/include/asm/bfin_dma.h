/*
 * bfin_dma.h - Blackfin DMA defines/structures/etc...
 *
 * Copyright 2004-2010 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __ASM_BFIN_DMA_H__
#define __ASM_BFIN_DMA_H__

#include <fikus/types.h>

/* DMA_CONFIG Masks */
#define DMAEN			0x0001	/* DMA Channel Enable */
#define WNR				0x0002	/* Channel Direction (W/R*) */
#define WDSIZE_8		0x0000	/* Transfer Word Size = 8 */
#define PSIZE_8			0x00000000	/* Transfer Word Size = 16 */

#ifdef CONFIG_BF60x

#define PSIZE_16		0x00000010	/* Transfer Word Size = 16 */
#define PSIZE_32		0x00000020	/* Transfer Word Size = 32 */
#define PSIZE_64		0x00000030	/* Transfer Word Size = 32 */
#define WDSIZE_16		0x00000100	/* Transfer Word Size = 16 */
#define WDSIZE_32		0x00000200	/* Transfer Word Size = 32 */
#define WDSIZE_64		0x00000300	/* Transfer Word Size = 32 */
#define WDSIZE_128		0x00000400	/* Transfer Word Size = 32 */
#define WDSIZE_256		0x00000500	/* Transfer Word Size = 32 */
#define DMA2D			0x04000000	/* DMA Mode (2D/1D*) */
#define RESTART			0x00000004	/* DMA Buffer Clear SYNC */
#define DI_EN_X			0x00100000	/* Data Interrupt Enable in X count */
#define DI_EN_Y			0x00200000	/* Data Interrupt Enable in Y count */
#define DI_EN_P			0x00300000	/* Data Interrupt Enable in Peripheral */
#define DI_EN			DI_EN_X		/* Data Interrupt Enable */
#define NDSIZE_0		0x00000000	/* Next Descriptor Size = 1 */
#define NDSIZE_1		0x00010000	/* Next Descriptor Size = 2 */
#define NDSIZE_2		0x00020000	/* Next Descriptor Size = 3 */
#define NDSIZE_3		0x00030000	/* Next Descriptor Size = 4 */
#define NDSIZE_4		0x00040000	/* Next Descriptor Size = 5 */
#define NDSIZE_5		0x00050000	/* Next Descriptor Size = 6 */
#define NDSIZE_6		0x00060000	/* Next Descriptor Size = 7 */
#define NDSIZE			0x00070000	/* Next Descriptor Size */
#define NDSIZE_OFFSET		16		/* Next Descriptor Size Offset */
#define DMAFLOW_LIST		0x00004000	/* Descriptor List Mode */
#define DMAFLOW_LARGE		DMAFLOW_LIST
#define DMAFLOW_ARRAY		0x00005000	/* Descriptor Array Mode */
#define DMAFLOW_LIST_DEMAND	0x00006000	/* Descriptor Demand List Mode */
#define DMAFLOW_ARRAY_DEMAND	0x00007000	/* Descriptor Demand Array Mode */
#define DMA_RUN_DFETCH		0x00000100	/* DMA Channel Running Indicator (DFETCH) */
#define DMA_RUN			0x00000200	/* DMA Channel Running Indicator */
#define DMA_RUN_WAIT_TRIG	0x00000300	/* DMA Channel Running Indicator (WAIT TRIG) */
#define DMA_RUN_WAIT_ACK	0x00000400	/* DMA Channel Running Indicator (WAIT ACK) */

#else

#define PSIZE_16		0x0000	/* Transfer Word Size = 16 */
#define PSIZE_32		0x0000	/* Transfer Word Size = 32 */
#define WDSIZE_16		0x0004	/* Transfer Word Size = 16 */
#define WDSIZE_32		0x0008	/* Transfer Word Size = 32 */
#define DMA2D			0x0010	/* DMA Mode (2D/1D*) */
#define RESTART			0x0020	/* DMA Buffer Clear */
#define DI_SEL			0x0040	/* Data Interrupt Timing Select */
#define DI_EN			0x0080	/* Data Interrupt Enable */
#define DI_EN_X			0x00C0	/* Data Interrupt Enable in X count*/
#define DI_EN_Y			0x0080	/* Data Interrupt Enable in Y count*/
#define NDSIZE_0		0x0000	/* Next Descriptor Size = 0 (Stop/Autobuffer) */
#define NDSIZE_1		0x0100	/* Next Descriptor Size = 1 */
#define NDSIZE_2		0x0200	/* Next Descriptor Size = 2 */
#define NDSIZE_3		0x0300	/* Next Descriptor Size = 3 */
#define NDSIZE_4		0x0400	/* Next Descriptor Size = 4 */
#define NDSIZE_5		0x0500	/* Next Descriptor Size = 5 */
#define NDSIZE_6		0x0600	/* Next Descriptor Size = 6 */
#define NDSIZE_7		0x0700	/* Next Descriptor Size = 7 */
#define NDSIZE_8		0x0800	/* Next Descriptor Size = 8 */
#define NDSIZE_9		0x0900	/* Next Descriptor Size = 9 */
#define NDSIZE			0x0f00	/* Next Descriptor Size */
#define NDSIZE_OFFSET		8	/* Next Descriptor Size Offset */
#define DMAFLOW_ARRAY	0x4000	/* Descriptor Array Mode */
#define DMAFLOW_SMALL	0x6000	/* Small Model Descriptor List Mode */
#define DMAFLOW_LARGE	0x7000	/* Large Model Descriptor List Mode */
#define DFETCH			0x0004	/* DMA Descriptor Fetch Indicator */
#define DMA_RUN			0x0008	/* DMA Channel Running Indicator */

#endif
#define DMAFLOW			0x7000	/* Flow Control */
#define DMAFLOW_STOP	0x0000	/* Stop Mode */
#define DMAFLOW_AUTO	0x1000	/* Autobuffer Mode */

/* DMA_IRQ_STATUS Masks */
#define DMA_DONE		0x0001	/* DMA Completion Interrupt Status */
#define DMA_ERR			0x0002	/* DMA Error Interrupt Status */
#ifdef CONFIG_BF60x
#define DMA_PIRQ		0x0004	/* DMA Peripheral Error Interrupt Status */
#else
#define DMA_PIRQ		0
#endif

/*
 * All Blackfin system MMRs are padded to 32bits even if the register
 * itself is only 16bits.  So use a helper macro to streamline this.
 */
#define __BFP(m) u16 m; u16 __pad_##m

/*
 * bfin dma registers layout
 */
struct bfin_dma_regs {
	u32 next_desc_ptr;
	u32 start_addr;
#ifdef CONFIG_BF60x
	u32 cfg;
	u32 x_count;
	u32 x_modify;
	u32 y_count;
	u32 y_modify;
	u32 pad1;
	u32 pad2;
	u32 curr_desc_ptr;
	u32 prev_desc_ptr;
	u32 curr_addr;
	u32 irq_status;
	u32 curr_x_count;
	u32 curr_y_count;
	u32 pad3;
	u32 bw_limit_count;
	u32 curr_bw_limit_count;
	u32 bw_monitor_count;
	u32 curr_bw_monitor_count;
#else
	__BFP(config);
	u32 __pad0;
	__BFP(x_count);
	__BFP(x_modify);
	__BFP(y_count);
	__BFP(y_modify);
	u32 curr_desc_ptr;
	u32 curr_addr;
	__BFP(irq_status);
	__BFP(peripheral_map);
	__BFP(curr_x_count);
	u32 __pad1;
	__BFP(curr_y_count);
	u32 __pad2;
#endif
};

#ifndef CONFIG_BF60x
/*
 * bfin handshake mdma registers layout
 */
struct bfin_hmdma_regs {
	__BFP(control);
	__BFP(ecinit);
	__BFP(bcinit);
	__BFP(ecurgent);
	__BFP(ecoverflow);
	__BFP(ecount);
	__BFP(bcount);
};
#endif

#undef __BFP

#endif
