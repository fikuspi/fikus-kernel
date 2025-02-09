/*
 * imx-pcm-dma-mx2.c  --  ALSA Soc Audio Layer
 *
 * Copyright 2009 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This code is based on code copyrighted by Freescale,
 * Liam Girdwood, Javier Martin and probably others.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <fikus/platform_device.h>
#include <fikus/dmaengine.h>
#include <fikus/types.h>
#include <fikus/module.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>

#include "imx-pcm.h"

static bool filter(struct dma_chan *chan, void *param)
{
	struct snd_dmaengine_dai_dma_data *dma_data = param;

	if (!imx_dma_is_general_purpose(chan))
		return false;

	chan->private = dma_data->filter_data;

	return true;
}

static const struct snd_pcm_hardware imx_pcm_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_PAUSE |
		SNDRV_PCM_INFO_RESUME,
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rate_min = 8000,
	.channels_min = 2,
	.channels_max = 2,
	.buffer_bytes_max = IMX_SSI_DMABUF_SIZE,
	.period_bytes_min = 128,
	.period_bytes_max = 65535, /* Limited by SDMA engine */
	.periods_min = 2,
	.periods_max = 255,
	.fifo_size = 0,
};

static const struct snd_dmaengine_pcm_config imx_dmaengine_pcm_config = {
	.pcm_hardware = &imx_pcm_hardware,
	.prepare_slave_config = snd_dmaengine_pcm_prepare_slave_config,
	.compat_filter_fn = filter,
	.prealloc_buffer_size = IMX_SSI_DMABUF_SIZE,
};

int imx_pcm_dma_init(struct platform_device *pdev)
{
	return snd_dmaengine_pcm_register(&pdev->dev, &imx_dmaengine_pcm_config,
		SND_DMAENGINE_PCM_FLAG_NO_RESIDUE |
		SND_DMAENGINE_PCM_FLAG_COMPAT);
}
EXPORT_SYMBOL_GPL(imx_pcm_dma_init);

void imx_pcm_dma_exit(struct platform_device *pdev)
{
	snd_dmaengine_pcm_unregister(&pdev->dev);
}
EXPORT_SYMBOL_GPL(imx_pcm_dma_exit);

MODULE_LICENSE("GPL");
