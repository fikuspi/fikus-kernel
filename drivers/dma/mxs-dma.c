/*
 * Copyright 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Refer to drivers/dma/imx-sdma.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/init.h>
#include <fikus/types.h>
#include <fikus/mm.h>
#include <fikus/interrupt.h>
#include <fikus/clk.h>
#include <fikus/wait.h>
#include <fikus/sched.h>
#include <fikus/semaphore.h>
#include <fikus/device.h>
#include <fikus/dma-mapping.h>
#include <fikus/slab.h>
#include <fikus/platform_device.h>
#include <fikus/dmaengine.h>
#include <fikus/delay.h>
#include <fikus/module.h>
#include <fikus/stmp_device.h>
#include <fikus/of.h>
#include <fikus/of_device.h>
#include <fikus/of_dma.h>

#include <asm/irq.h>

#include "dmaengine.h"

/*
 * NOTE: The term "PIO" throughout the mxs-dma implementation means
 * PIO mode of mxs apbh-dma and apbx-dma.  With this working mode,
 * dma can program the controller registers of peripheral devices.
 */

#define dma_is_apbh(mxs_dma)	((mxs_dma)->type == MXS_DMA_APBH)
#define apbh_is_old(mxs_dma)	((mxs_dma)->dev_id == IMX23_DMA)

#define HW_APBHX_CTRL0				0x000
#define BM_APBH_CTRL0_APB_BURST8_EN		(1 << 29)
#define BM_APBH_CTRL0_APB_BURST_EN		(1 << 28)
#define BP_APBH_CTRL0_RESET_CHANNEL		16
#define HW_APBHX_CTRL1				0x010
#define HW_APBHX_CTRL2				0x020
#define HW_APBHX_CHANNEL_CTRL			0x030
#define BP_APBHX_CHANNEL_CTRL_RESET_CHANNEL	16
/*
 * The offset of NXTCMDAR register is different per both dma type and version,
 * while stride for each channel is all the same 0x70.
 */
#define HW_APBHX_CHn_NXTCMDAR(d, n) \
	(((dma_is_apbh(d) && apbh_is_old(d)) ? 0x050 : 0x110) + (n) * 0x70)
#define HW_APBHX_CHn_SEMA(d, n) \
	(((dma_is_apbh(d) && apbh_is_old(d)) ? 0x080 : 0x140) + (n) * 0x70)

/*
 * ccw bits definitions
 *
 * COMMAND:		0..1	(2)
 * CHAIN:		2	(1)
 * IRQ:			3	(1)
 * NAND_LOCK:		4	(1) - not implemented
 * NAND_WAIT4READY:	5	(1) - not implemented
 * DEC_SEM:		6	(1)
 * WAIT4END:		7	(1)
 * HALT_ON_TERMINATE:	8	(1)
 * TERMINATE_FLUSH:	9	(1)
 * RESERVED:		10..11	(2)
 * PIO_NUM:		12..15	(4)
 */
#define BP_CCW_COMMAND		0
#define BM_CCW_COMMAND		(3 << 0)
#define CCW_CHAIN		(1 << 2)
#define CCW_IRQ			(1 << 3)
#define CCW_DEC_SEM		(1 << 6)
#define CCW_WAIT4END		(1 << 7)
#define CCW_HALT_ON_TERM	(1 << 8)
#define CCW_TERM_FLUSH		(1 << 9)
#define BP_CCW_PIO_NUM		12
#define BM_CCW_PIO_NUM		(0xf << 12)

#define BF_CCW(value, field)	(((value) << BP_CCW_##field) & BM_CCW_##field)

#define MXS_DMA_CMD_NO_XFER	0
#define MXS_DMA_CMD_WRITE	1
#define MXS_DMA_CMD_READ	2
#define MXS_DMA_CMD_DMA_SENSE	3	/* not implemented */

struct mxs_dma_ccw {
	u32		next;
	u16		bits;
	u16		xfer_bytes;
#define MAX_XFER_BYTES	0xff00
	u32		bufaddr;
#define MXS_PIO_WORDS	16
	u32		pio_words[MXS_PIO_WORDS];
};

#define CCW_BLOCK_SIZE	(4 * PAGE_SIZE)
#define NUM_CCW	(int)(CCW_BLOCK_SIZE / sizeof(struct mxs_dma_ccw))

struct mxs_dma_chan {
	struct mxs_dma_engine		*mxs_dma;
	struct dma_chan			chan;
	struct dma_async_tx_descriptor	desc;
	struct tasklet_struct		tasklet;
	unsigned int			chan_irq;
	struct mxs_dma_ccw		*ccw;
	dma_addr_t			ccw_phys;
	int				desc_count;
	enum dma_status			status;
	unsigned int			flags;
#define MXS_DMA_SG_LOOP			(1 << 0)
};

#define MXS_DMA_CHANNELS		16
#define MXS_DMA_CHANNELS_MASK		0xffff

enum mxs_dma_devtype {
	MXS_DMA_APBH,
	MXS_DMA_APBX,
};

enum mxs_dma_id {
	IMX23_DMA,
	IMX28_DMA,
};

struct mxs_dma_engine {
	enum mxs_dma_id			dev_id;
	enum mxs_dma_devtype		type;
	void __iomem			*base;
	struct clk			*clk;
	struct dma_device		dma_device;
	struct device_dma_parameters	dma_parms;
	struct mxs_dma_chan		mxs_chans[MXS_DMA_CHANNELS];
	struct platform_device		*pdev;
	unsigned int			nr_channels;
};

struct mxs_dma_type {
	enum mxs_dma_id id;
	enum mxs_dma_devtype type;
};

static struct mxs_dma_type mxs_dma_types[] = {
	{
		.id = IMX23_DMA,
		.type = MXS_DMA_APBH,
	}, {
		.id = IMX23_DMA,
		.type = MXS_DMA_APBX,
	}, {
		.id = IMX28_DMA,
		.type = MXS_DMA_APBH,
	}, {
		.id = IMX28_DMA,
		.type = MXS_DMA_APBX,
	}
};

static struct platform_device_id mxs_dma_ids[] = {
	{
		.name = "imx23-dma-apbh",
		.driver_data = (kernel_ulong_t) &mxs_dma_types[0],
	}, {
		.name = "imx23-dma-apbx",
		.driver_data = (kernel_ulong_t) &mxs_dma_types[1],
	}, {
		.name = "imx28-dma-apbh",
		.driver_data = (kernel_ulong_t) &mxs_dma_types[2],
	}, {
		.name = "imx28-dma-apbx",
		.driver_data = (kernel_ulong_t) &mxs_dma_types[3],
	}, {
		/* end of list */
	}
};

static const struct of_device_id mxs_dma_dt_ids[] = {
	{ .compatible = "fsl,imx23-dma-apbh", .data = &mxs_dma_ids[0], },
	{ .compatible = "fsl,imx23-dma-apbx", .data = &mxs_dma_ids[1], },
	{ .compatible = "fsl,imx28-dma-apbh", .data = &mxs_dma_ids[2], },
	{ .compatible = "fsl,imx28-dma-apbx", .data = &mxs_dma_ids[3], },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mxs_dma_dt_ids);

static struct mxs_dma_chan *to_mxs_dma_chan(struct dma_chan *chan)
{
	return container_of(chan, struct mxs_dma_chan, chan);
}

static void mxs_dma_reset_chan(struct mxs_dma_chan *mxs_chan)
{
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	int chan_id = mxs_chan->chan.chan_id;

	if (dma_is_apbh(mxs_dma) && apbh_is_old(mxs_dma))
		writel(1 << (chan_id + BP_APBH_CTRL0_RESET_CHANNEL),
			mxs_dma->base + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);
	else
		writel(1 << (chan_id + BP_APBHX_CHANNEL_CTRL_RESET_CHANNEL),
			mxs_dma->base + HW_APBHX_CHANNEL_CTRL + STMP_OFFSET_REG_SET);
}

static void mxs_dma_enable_chan(struct mxs_dma_chan *mxs_chan)
{
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	int chan_id = mxs_chan->chan.chan_id;

	/* set cmd_addr up */
	writel(mxs_chan->ccw_phys,
		mxs_dma->base + HW_APBHX_CHn_NXTCMDAR(mxs_dma, chan_id));

	/* write 1 to SEMA to kick off the channel */
	writel(1, mxs_dma->base + HW_APBHX_CHn_SEMA(mxs_dma, chan_id));
}

static void mxs_dma_disable_chan(struct mxs_dma_chan *mxs_chan)
{
	mxs_chan->status = DMA_SUCCESS;
}

static void mxs_dma_pause_chan(struct mxs_dma_chan *mxs_chan)
{
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	int chan_id = mxs_chan->chan.chan_id;

	/* freeze the channel */
	if (dma_is_apbh(mxs_dma) && apbh_is_old(mxs_dma))
		writel(1 << chan_id,
			mxs_dma->base + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);
	else
		writel(1 << chan_id,
			mxs_dma->base + HW_APBHX_CHANNEL_CTRL + STMP_OFFSET_REG_SET);

	mxs_chan->status = DMA_PAUSED;
}

static void mxs_dma_resume_chan(struct mxs_dma_chan *mxs_chan)
{
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	int chan_id = mxs_chan->chan.chan_id;

	/* unfreeze the channel */
	if (dma_is_apbh(mxs_dma) && apbh_is_old(mxs_dma))
		writel(1 << chan_id,
			mxs_dma->base + HW_APBHX_CTRL0 + STMP_OFFSET_REG_CLR);
	else
		writel(1 << chan_id,
			mxs_dma->base + HW_APBHX_CHANNEL_CTRL + STMP_OFFSET_REG_CLR);

	mxs_chan->status = DMA_IN_PROGRESS;
}

static dma_cookie_t mxs_dma_tx_submit(struct dma_async_tx_descriptor *tx)
{
	return dma_cookie_assign(tx);
}

static void mxs_dma_tasklet(unsigned long data)
{
	struct mxs_dma_chan *mxs_chan = (struct mxs_dma_chan *) data;

	if (mxs_chan->desc.callback)
		mxs_chan->desc.callback(mxs_chan->desc.callback_param);
}

static irqreturn_t mxs_dma_int_handler(int irq, void *dev_id)
{
	struct mxs_dma_engine *mxs_dma = dev_id;
	u32 stat1, stat2;

	/* completion status */
	stat1 = readl(mxs_dma->base + HW_APBHX_CTRL1);
	stat1 &= MXS_DMA_CHANNELS_MASK;
	writel(stat1, mxs_dma->base + HW_APBHX_CTRL1 + STMP_OFFSET_REG_CLR);

	/* error status */
	stat2 = readl(mxs_dma->base + HW_APBHX_CTRL2);
	writel(stat2, mxs_dma->base + HW_APBHX_CTRL2 + STMP_OFFSET_REG_CLR);

	/*
	 * When both completion and error of termination bits set at the
	 * same time, we do not take it as an error.  IOW, it only becomes
	 * an error we need to handle here in case of either it's (1) a bus
	 * error or (2) a termination error with no completion.
	 */
	stat2 = ((stat2 >> MXS_DMA_CHANNELS) & stat2) | /* (1) */
		(~(stat2 >> MXS_DMA_CHANNELS) & stat2 & ~stat1); /* (2) */

	/* combine error and completion status for checking */
	stat1 = (stat2 << MXS_DMA_CHANNELS) | stat1;
	while (stat1) {
		int channel = fls(stat1) - 1;
		struct mxs_dma_chan *mxs_chan =
			&mxs_dma->mxs_chans[channel % MXS_DMA_CHANNELS];

		if (channel >= MXS_DMA_CHANNELS) {
			dev_dbg(mxs_dma->dma_device.dev,
				"%s: error in channel %d\n", __func__,
				channel - MXS_DMA_CHANNELS);
			mxs_chan->status = DMA_ERROR;
			mxs_dma_reset_chan(mxs_chan);
		} else {
			if (mxs_chan->flags & MXS_DMA_SG_LOOP)
				mxs_chan->status = DMA_IN_PROGRESS;
			else
				mxs_chan->status = DMA_SUCCESS;
		}

		stat1 &= ~(1 << channel);

		if (mxs_chan->status == DMA_SUCCESS)
			dma_cookie_complete(&mxs_chan->desc);

		/* schedule tasklet on this channel */
		tasklet_schedule(&mxs_chan->tasklet);
	}

	return IRQ_HANDLED;
}

static int mxs_dma_alloc_chan_resources(struct dma_chan *chan)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	int ret;

	mxs_chan->ccw = dma_alloc_coherent(mxs_dma->dma_device.dev,
				CCW_BLOCK_SIZE, &mxs_chan->ccw_phys,
				GFP_KERNEL);
	if (!mxs_chan->ccw) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	memset(mxs_chan->ccw, 0, CCW_BLOCK_SIZE);

	if (mxs_chan->chan_irq != NO_IRQ) {
		ret = request_irq(mxs_chan->chan_irq, mxs_dma_int_handler,
					0, "mxs-dma", mxs_dma);
		if (ret)
			goto err_irq;
	}

	ret = clk_prepare_enable(mxs_dma->clk);
	if (ret)
		goto err_clk;

	mxs_dma_reset_chan(mxs_chan);

	dma_async_tx_descriptor_init(&mxs_chan->desc, chan);
	mxs_chan->desc.tx_submit = mxs_dma_tx_submit;

	/* the descriptor is ready */
	async_tx_ack(&mxs_chan->desc);

	return 0;

err_clk:
	free_irq(mxs_chan->chan_irq, mxs_dma);
err_irq:
	dma_free_coherent(mxs_dma->dma_device.dev, CCW_BLOCK_SIZE,
			mxs_chan->ccw, mxs_chan->ccw_phys);
err_alloc:
	return ret;
}

static void mxs_dma_free_chan_resources(struct dma_chan *chan)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;

	mxs_dma_disable_chan(mxs_chan);

	free_irq(mxs_chan->chan_irq, mxs_dma);

	dma_free_coherent(mxs_dma->dma_device.dev, CCW_BLOCK_SIZE,
			mxs_chan->ccw, mxs_chan->ccw_phys);

	clk_disable_unprepare(mxs_dma->clk);
}

/*
 * How to use the flags for ->device_prep_slave_sg() :
 *    [1] If there is only one DMA command in the DMA chain, the code should be:
 *            ......
 *            ->device_prep_slave_sg(DMA_CTRL_ACK);
 *            ......
 *    [2] If there are two DMA commands in the DMA chain, the code should be
 *            ......
 *            ->device_prep_slave_sg(0);
 *            ......
 *            ->device_prep_slave_sg(DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
 *            ......
 *    [3] If there are more than two DMA commands in the DMA chain, the code
 *        should be:
 *            ......
 *            ->device_prep_slave_sg(0);                                // First
 *            ......
 *            ->device_prep_slave_sg(DMA_PREP_INTERRUPT [| DMA_CTRL_ACK]);
 *            ......
 *            ->device_prep_slave_sg(DMA_PREP_INTERRUPT | DMA_CTRL_ACK); // Last
 *            ......
 */
static struct dma_async_tx_descriptor *mxs_dma_prep_slave_sg(
		struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	struct mxs_dma_ccw *ccw;
	struct scatterlist *sg;
	u32 i, j;
	u32 *pio;
	bool append = flags & DMA_PREP_INTERRUPT;
	int idx = append ? mxs_chan->desc_count : 0;

	if (mxs_chan->status == DMA_IN_PROGRESS && !append)
		return NULL;

	if (sg_len + (append ? idx : 0) > NUM_CCW) {
		dev_err(mxs_dma->dma_device.dev,
				"maximum number of sg exceeded: %d > %d\n",
				sg_len, NUM_CCW);
		goto err_out;
	}

	mxs_chan->status = DMA_IN_PROGRESS;
	mxs_chan->flags = 0;

	/*
	 * If the sg is prepared with append flag set, the sg
	 * will be appended to the last prepared sg.
	 */
	if (append) {
		BUG_ON(idx < 1);
		ccw = &mxs_chan->ccw[idx - 1];
		ccw->next = mxs_chan->ccw_phys + sizeof(*ccw) * idx;
		ccw->bits |= CCW_CHAIN;
		ccw->bits &= ~CCW_IRQ;
		ccw->bits &= ~CCW_DEC_SEM;
	} else {
		idx = 0;
	}

	if (direction == DMA_TRANS_NONE) {
		ccw = &mxs_chan->ccw[idx++];
		pio = (u32 *) sgl;

		for (j = 0; j < sg_len;)
			ccw->pio_words[j++] = *pio++;

		ccw->bits = 0;
		ccw->bits |= CCW_IRQ;
		ccw->bits |= CCW_DEC_SEM;
		if (flags & DMA_CTRL_ACK)
			ccw->bits |= CCW_WAIT4END;
		ccw->bits |= CCW_HALT_ON_TERM;
		ccw->bits |= CCW_TERM_FLUSH;
		ccw->bits |= BF_CCW(sg_len, PIO_NUM);
		ccw->bits |= BF_CCW(MXS_DMA_CMD_NO_XFER, COMMAND);
	} else {
		for_each_sg(sgl, sg, sg_len, i) {
			if (sg_dma_len(sg) > MAX_XFER_BYTES) {
				dev_err(mxs_dma->dma_device.dev, "maximum bytes for sg entry exceeded: %d > %d\n",
						sg_dma_len(sg), MAX_XFER_BYTES);
				goto err_out;
			}

			ccw = &mxs_chan->ccw[idx++];

			ccw->next = mxs_chan->ccw_phys + sizeof(*ccw) * idx;
			ccw->bufaddr = sg->dma_address;
			ccw->xfer_bytes = sg_dma_len(sg);

			ccw->bits = 0;
			ccw->bits |= CCW_CHAIN;
			ccw->bits |= CCW_HALT_ON_TERM;
			ccw->bits |= CCW_TERM_FLUSH;
			ccw->bits |= BF_CCW(direction == DMA_DEV_TO_MEM ?
					MXS_DMA_CMD_WRITE : MXS_DMA_CMD_READ,
					COMMAND);

			if (i + 1 == sg_len) {
				ccw->bits &= ~CCW_CHAIN;
				ccw->bits |= CCW_IRQ;
				ccw->bits |= CCW_DEC_SEM;
				if (flags & DMA_CTRL_ACK)
					ccw->bits |= CCW_WAIT4END;
			}
		}
	}
	mxs_chan->desc_count = idx;

	return &mxs_chan->desc;

err_out:
	mxs_chan->status = DMA_ERROR;
	return NULL;
}

static struct dma_async_tx_descriptor *mxs_dma_prep_dma_cyclic(
		struct dma_chan *chan, dma_addr_t dma_addr, size_t buf_len,
		size_t period_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	u32 num_periods = buf_len / period_len;
	u32 i = 0, buf = 0;

	if (mxs_chan->status == DMA_IN_PROGRESS)
		return NULL;

	mxs_chan->status = DMA_IN_PROGRESS;
	mxs_chan->flags |= MXS_DMA_SG_LOOP;

	if (num_periods > NUM_CCW) {
		dev_err(mxs_dma->dma_device.dev,
				"maximum number of sg exceeded: %d > %d\n",
				num_periods, NUM_CCW);
		goto err_out;
	}

	if (period_len > MAX_XFER_BYTES) {
		dev_err(mxs_dma->dma_device.dev,
				"maximum period size exceeded: %d > %d\n",
				period_len, MAX_XFER_BYTES);
		goto err_out;
	}

	while (buf < buf_len) {
		struct mxs_dma_ccw *ccw = &mxs_chan->ccw[i];

		if (i + 1 == num_periods)
			ccw->next = mxs_chan->ccw_phys;
		else
			ccw->next = mxs_chan->ccw_phys + sizeof(*ccw) * (i + 1);

		ccw->bufaddr = dma_addr;
		ccw->xfer_bytes = period_len;

		ccw->bits = 0;
		ccw->bits |= CCW_CHAIN;
		ccw->bits |= CCW_IRQ;
		ccw->bits |= CCW_HALT_ON_TERM;
		ccw->bits |= CCW_TERM_FLUSH;
		ccw->bits |= BF_CCW(direction == DMA_DEV_TO_MEM ?
				MXS_DMA_CMD_WRITE : MXS_DMA_CMD_READ, COMMAND);

		dma_addr += period_len;
		buf += period_len;

		i++;
	}
	mxs_chan->desc_count = i;

	return &mxs_chan->desc;

err_out:
	mxs_chan->status = DMA_ERROR;
	return NULL;
}

static int mxs_dma_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
		unsigned long arg)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	int ret = 0;

	switch (cmd) {
	case DMA_TERMINATE_ALL:
		mxs_dma_reset_chan(mxs_chan);
		mxs_dma_disable_chan(mxs_chan);
		break;
	case DMA_PAUSE:
		mxs_dma_pause_chan(mxs_chan);
		break;
	case DMA_RESUME:
		mxs_dma_resume_chan(mxs_chan);
		break;
	default:
		ret = -ENOSYS;
	}

	return ret;
}

static enum dma_status mxs_dma_tx_status(struct dma_chan *chan,
			dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);

	dma_set_tx_state(txstate, chan->completed_cookie, chan->cookie, 0);

	return mxs_chan->status;
}

static void mxs_dma_issue_pending(struct dma_chan *chan)
{
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);

	mxs_dma_enable_chan(mxs_chan);
}

static int __init mxs_dma_init(struct mxs_dma_engine *mxs_dma)
{
	int ret;

	ret = clk_prepare_enable(mxs_dma->clk);
	if (ret)
		return ret;

	ret = stmp_reset_block(mxs_dma->base);
	if (ret)
		goto err_out;

	/* enable apbh burst */
	if (dma_is_apbh(mxs_dma)) {
		writel(BM_APBH_CTRL0_APB_BURST_EN,
			mxs_dma->base + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);
		writel(BM_APBH_CTRL0_APB_BURST8_EN,
			mxs_dma->base + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);
	}

	/* enable irq for all the channels */
	writel(MXS_DMA_CHANNELS_MASK << MXS_DMA_CHANNELS,
		mxs_dma->base + HW_APBHX_CTRL1 + STMP_OFFSET_REG_SET);

err_out:
	clk_disable_unprepare(mxs_dma->clk);
	return ret;
}

struct mxs_dma_filter_param {
	struct device_node *of_node;
	unsigned int chan_id;
};

static bool mxs_dma_filter_fn(struct dma_chan *chan, void *fn_param)
{
	struct mxs_dma_filter_param *param = fn_param;
	struct mxs_dma_chan *mxs_chan = to_mxs_dma_chan(chan);
	struct mxs_dma_engine *mxs_dma = mxs_chan->mxs_dma;
	int chan_irq;

	if (mxs_dma->dma_device.dev->of_node != param->of_node)
		return false;

	if (chan->chan_id != param->chan_id)
		return false;

	chan_irq = platform_get_irq(mxs_dma->pdev, param->chan_id);
	if (chan_irq < 0)
		return false;

	mxs_chan->chan_irq = chan_irq;

	return true;
}

static struct dma_chan *mxs_dma_xlate(struct of_phandle_args *dma_spec,
			       struct of_dma *ofdma)
{
	struct mxs_dma_engine *mxs_dma = ofdma->of_dma_data;
	dma_cap_mask_t mask = mxs_dma->dma_device.cap_mask;
	struct mxs_dma_filter_param param;

	if (dma_spec->args_count != 1)
		return NULL;

	param.of_node = ofdma->of_node;
	param.chan_id = dma_spec->args[0];

	if (param.chan_id >= mxs_dma->nr_channels)
		return NULL;

	return dma_request_channel(mask, mxs_dma_filter_fn, &param);
}

static int __init mxs_dma_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const struct platform_device_id *id_entry;
	const struct of_device_id *of_id;
	const struct mxs_dma_type *dma_type;
	struct mxs_dma_engine *mxs_dma;
	struct resource *iores;
	int ret, i;

	mxs_dma = devm_kzalloc(&pdev->dev, sizeof(*mxs_dma), GFP_KERNEL);
	if (!mxs_dma)
		return -ENOMEM;

	ret = of_property_read_u32(np, "dma-channels", &mxs_dma->nr_channels);
	if (ret) {
		dev_err(&pdev->dev, "failed to read dma-channels\n");
		return ret;
	}

	of_id = of_match_device(mxs_dma_dt_ids, &pdev->dev);
	if (of_id)
		id_entry = of_id->data;
	else
		id_entry = platform_get_device_id(pdev);

	dma_type = (struct mxs_dma_type *)id_entry->driver_data;
	mxs_dma->type = dma_type->type;
	mxs_dma->dev_id = dma_type->id;

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mxs_dma->base = devm_ioremap_resource(&pdev->dev, iores);
	if (IS_ERR(mxs_dma->base))
		return PTR_ERR(mxs_dma->base);

	mxs_dma->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(mxs_dma->clk))
		return PTR_ERR(mxs_dma->clk);

	dma_cap_set(DMA_SLAVE, mxs_dma->dma_device.cap_mask);
	dma_cap_set(DMA_CYCLIC, mxs_dma->dma_device.cap_mask);

	INIT_LIST_HEAD(&mxs_dma->dma_device.channels);

	/* Initialize channel parameters */
	for (i = 0; i < MXS_DMA_CHANNELS; i++) {
		struct mxs_dma_chan *mxs_chan = &mxs_dma->mxs_chans[i];

		mxs_chan->mxs_dma = mxs_dma;
		mxs_chan->chan.device = &mxs_dma->dma_device;
		dma_cookie_init(&mxs_chan->chan);

		tasklet_init(&mxs_chan->tasklet, mxs_dma_tasklet,
			     (unsigned long) mxs_chan);


		/* Add the channel to mxs_chan list */
		list_add_tail(&mxs_chan->chan.device_node,
			&mxs_dma->dma_device.channels);
	}

	ret = mxs_dma_init(mxs_dma);
	if (ret)
		return ret;

	mxs_dma->pdev = pdev;
	mxs_dma->dma_device.dev = &pdev->dev;

	/* mxs_dma gets 65535 bytes maximum sg size */
	mxs_dma->dma_device.dev->dma_parms = &mxs_dma->dma_parms;
	dma_set_max_seg_size(mxs_dma->dma_device.dev, MAX_XFER_BYTES);

	mxs_dma->dma_device.device_alloc_chan_resources = mxs_dma_alloc_chan_resources;
	mxs_dma->dma_device.device_free_chan_resources = mxs_dma_free_chan_resources;
	mxs_dma->dma_device.device_tx_status = mxs_dma_tx_status;
	mxs_dma->dma_device.device_prep_slave_sg = mxs_dma_prep_slave_sg;
	mxs_dma->dma_device.device_prep_dma_cyclic = mxs_dma_prep_dma_cyclic;
	mxs_dma->dma_device.device_control = mxs_dma_control;
	mxs_dma->dma_device.device_issue_pending = mxs_dma_issue_pending;

	ret = dma_async_device_register(&mxs_dma->dma_device);
	if (ret) {
		dev_err(mxs_dma->dma_device.dev, "unable to register\n");
		return ret;
	}

	ret = of_dma_controller_register(np, mxs_dma_xlate, mxs_dma);
	if (ret) {
		dev_err(mxs_dma->dma_device.dev,
			"failed to register controller\n");
		dma_async_device_unregister(&mxs_dma->dma_device);
	}

	dev_info(mxs_dma->dma_device.dev, "initialized\n");

	return 0;
}

static struct platform_driver mxs_dma_driver = {
	.driver		= {
		.name	= "mxs-dma",
		.of_match_table = mxs_dma_dt_ids,
	},
	.id_table	= mxs_dma_ids,
};

static int __init mxs_dma_module_init(void)
{
	return platform_driver_probe(&mxs_dma_driver, mxs_dma_probe);
}
subsys_initcall(mxs_dma_module_init);
