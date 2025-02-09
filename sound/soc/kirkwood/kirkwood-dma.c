/*
 * kirkwood-dma.c
 *
 * (c) 2010 Arnaud Patard <apatard@mandriva.com>
 * (c) 2010 Arnaud Patard <arnaud.patard@rtp-net.org>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <fikus/init.h>
#include <fikus/module.h>
#include <fikus/device.h>
#include <fikus/io.h>
#include <fikus/slab.h>
#include <fikus/interrupt.h>
#include <fikus/dma-mapping.h>
#include <fikus/mbus.h>
#include <sound/soc.h>
#include "kirkwood.h"

#define KIRKWOOD_RATES \
	(SNDRV_PCM_RATE_8000_192000 |		\
	 SNDRV_PCM_RATE_CONTINUOUS |		\
	 SNDRV_PCM_RATE_KNOT)

#define KIRKWOOD_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE | \
	 SNDRV_PCM_FMTBIT_S24_LE | \
	 SNDRV_PCM_FMTBIT_S32_LE | \
	 SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE | \
	 SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_BE)

static struct kirkwood_dma_data *kirkwood_priv(struct snd_pcm_substream *subs)
{
	struct snd_soc_pcm_runtime *soc_runtime = subs->private_data;
	return snd_soc_dai_get_drvdata(soc_runtime->cpu_dai);
}

static struct snd_pcm_hardware kirkwood_dma_snd_hw = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_PAUSE),
	.formats		= KIRKWOOD_FORMATS,
	.rates			= KIRKWOOD_RATES,
	.rate_min		= 8000,
	.rate_max		= 384000,
	.channels_min		= 1,
	.channels_max		= 8,
	.buffer_bytes_max	= KIRKWOOD_SND_MAX_BUFFER_BYTES,
	.period_bytes_min	= KIRKWOOD_SND_MIN_PERIOD_BYTES,
	.period_bytes_max	= KIRKWOOD_SND_MAX_PERIOD_BYTES,
	.periods_min		= KIRKWOOD_SND_MIN_PERIODS,
	.periods_max		= KIRKWOOD_SND_MAX_PERIODS,
	.fifo_size		= 0,
};

static u64 kirkwood_dma_dmamask = DMA_BIT_MASK(32);

static irqreturn_t kirkwood_dma_irq(int irq, void *dev_id)
{
	struct kirkwood_dma_data *priv = dev_id;
	unsigned long mask, status, cause;

	mask = readl(priv->io + KIRKWOOD_INT_MASK);
	status = readl(priv->io + KIRKWOOD_INT_CAUSE) & mask;

	cause = readl(priv->io + KIRKWOOD_ERR_CAUSE);
	if (unlikely(cause)) {
		printk(KERN_WARNING "%s: got err interrupt 0x%lx\n",
				__func__, cause);
		writel(cause, priv->io + KIRKWOOD_ERR_CAUSE);
	}

	/* we've enabled only bytes interrupts ... */
	if (status & ~(KIRKWOOD_INT_CAUSE_PLAY_BYTES | \
			KIRKWOOD_INT_CAUSE_REC_BYTES)) {
		printk(KERN_WARNING "%s: unexpected interrupt %lx\n",
			__func__, status);
		return IRQ_NONE;
	}

	/* ack int */
	writel(status, priv->io + KIRKWOOD_INT_CAUSE);

	if (status & KIRKWOOD_INT_CAUSE_PLAY_BYTES)
		snd_pcm_period_elapsed(priv->substream_play);

	if (status & KIRKWOOD_INT_CAUSE_REC_BYTES)
		snd_pcm_period_elapsed(priv->substream_rec);

	return IRQ_HANDLED;
}

static void
kirkwood_dma_conf_mbus_windows(void __iomem *base, int win,
			       unsigned long dma,
			       const struct mbus_dram_target_info *dram)
{
	int i;

	/* First disable and clear windows */
	writel(0, base + KIRKWOOD_AUDIO_WIN_CTRL_REG(win));
	writel(0, base + KIRKWOOD_AUDIO_WIN_BASE_REG(win));

	/* try to find matching cs for current dma address */
	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;
		if ((cs->base & 0xffff0000) < (dma & 0xffff0000)) {
			writel(cs->base & 0xffff0000,
				base + KIRKWOOD_AUDIO_WIN_BASE_REG(win));
			writel(((cs->size - 1) & 0xffff0000) |
				(cs->mbus_attr << 8) |
				(dram->mbus_dram_target_id << 4) | 1,
				base + KIRKWOOD_AUDIO_WIN_CTRL_REG(win));
		}
	}
}

static int kirkwood_dma_open(struct snd_pcm_substream *substream)
{
	int err;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct kirkwood_dma_data *priv = kirkwood_priv(substream);
	const struct mbus_dram_target_info *dram;
	unsigned long addr;

	snd_soc_set_runtime_hwparams(substream, &kirkwood_dma_snd_hw);

	/* Ensure that all constraints linked to dma burst are fulfilled */
	err = snd_pcm_hw_constraint_minmax(runtime,
			SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
			priv->burst * 2,
			KIRKWOOD_AUDIO_BUF_MAX-1);
	if (err < 0)
		return err;

	err = snd_pcm_hw_constraint_step(runtime, 0,
			SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
			priv->burst);
	if (err < 0)
		return err;

	err = snd_pcm_hw_constraint_step(substream->runtime, 0,
			 SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
			 priv->burst);
	if (err < 0)
		return err;

	if (!priv->substream_play && !priv->substream_rec) {
		err = request_irq(priv->irq, kirkwood_dma_irq, IRQF_SHARED,
				  "kirkwood-i2s", priv);
		if (err)
			return -EBUSY;

		/*
		 * Enable Error interrupts. We're only ack'ing them but
		 * it's useful for diagnostics
		 */
		writel((unsigned long)-1, priv->io + KIRKWOOD_ERR_MASK);
	}

	dram = mv_mbus_dram_info();
	addr = substream->dma_buffer.addr;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		priv->substream_play = substream;
		kirkwood_dma_conf_mbus_windows(priv->io,
			KIRKWOOD_PLAYBACK_WIN, addr, dram);
	} else {
		priv->substream_rec = substream;
		kirkwood_dma_conf_mbus_windows(priv->io,
			KIRKWOOD_RECORD_WIN, addr, dram);
	}

	return 0;
}

static int kirkwood_dma_close(struct snd_pcm_substream *substream)
{
	struct kirkwood_dma_data *priv = kirkwood_priv(substream);

	if (!priv)
		return 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		priv->substream_play = NULL;
	else
		priv->substream_rec = NULL;

	if (!priv->substream_play && !priv->substream_rec) {
		writel(0, priv->io + KIRKWOOD_ERR_MASK);
		free_irq(priv->irq, priv);
	}

	return 0;
}

static int kirkwood_dma_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = params_buffer_bytes(params);

	return 0;
}

static int kirkwood_dma_hw_free(struct snd_pcm_substream *substream)
{
	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int kirkwood_dma_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct kirkwood_dma_data *priv = kirkwood_priv(substream);
	unsigned long size, count;

	/* compute buffer size in term of "words" as requested in specs */
	size = frames_to_bytes(runtime, runtime->buffer_size);
	size = (size>>2)-1;
	count = snd_pcm_lib_period_bytes(substream);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		writel(count, priv->io + KIRKWOOD_PLAY_BYTE_INT_COUNT);
		writel(runtime->dma_addr, priv->io + KIRKWOOD_PLAY_BUF_ADDR);
		writel(size, priv->io + KIRKWOOD_PLAY_BUF_SIZE);
	} else {
		writel(count, priv->io + KIRKWOOD_REC_BYTE_INT_COUNT);
		writel(runtime->dma_addr, priv->io + KIRKWOOD_REC_BUF_ADDR);
		writel(size, priv->io + KIRKWOOD_REC_BUF_SIZE);
	}


	return 0;
}

static snd_pcm_uframes_t kirkwood_dma_pointer(struct snd_pcm_substream
						*substream)
{
	struct kirkwood_dma_data *priv = kirkwood_priv(substream);
	snd_pcm_uframes_t count;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		count = bytes_to_frames(substream->runtime,
			readl(priv->io + KIRKWOOD_PLAY_BYTE_COUNT));
	else
		count = bytes_to_frames(substream->runtime,
			readl(priv->io + KIRKWOOD_REC_BYTE_COUNT));

	return count;
}

static struct snd_pcm_ops kirkwood_dma_ops = {
	.open =		kirkwood_dma_open,
	.close =        kirkwood_dma_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	kirkwood_dma_hw_params,
	.hw_free =      kirkwood_dma_hw_free,
	.prepare =      kirkwood_dma_prepare,
	.pointer =	kirkwood_dma_pointer,
};

static int kirkwood_dma_preallocate_dma_buffer(struct snd_pcm *pcm,
		int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = kirkwood_dma_snd_hw.buffer_bytes_max;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->area = dma_alloc_coherent(pcm->card->dev, size,
			&buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;
	buf->bytes = size;
	buf->private_data = NULL;

	return 0;
}

static int kirkwood_dma_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &kirkwood_dma_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = kirkwood_dma_preallocate_dma_buffer(pcm,
				SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			return ret;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = kirkwood_dma_preallocate_dma_buffer(pcm,
				SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			return ret;
	}

	return 0;
}

static void kirkwood_dma_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;
		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_coherent(pcm->card->dev, buf->bytes,
				buf->area, buf->addr);
		buf->area = NULL;
	}
}

struct snd_soc_platform_driver kirkwood_soc_platform = {
	.ops		= &kirkwood_dma_ops,
	.pcm_new	= kirkwood_dma_new,
	.pcm_free	= kirkwood_dma_free_dma_buffers,
};
