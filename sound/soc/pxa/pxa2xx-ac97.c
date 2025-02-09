/*
 * fikus/sound/pxa2xx-ac97.c -- AC97 support for the Intel PXA2xx chip.
 *
 * Author:	Nicolas Pitre
 * Created:	Dec 02, 2004
 * Copyright:	MontaVista Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/init.h>
#include <fikus/io.h>
#include <fikus/module.h>
#include <fikus/platform_device.h>
#include <fikus/dmaengine.h>

#include <sound/core.h>
#include <sound/ac97_codec.h>
#include <sound/soc.h>
#include <sound/pxa2xx-lib.h>
#include <sound/dmaengine_pcm.h>

#include <mach/hardware.h>
#include <mach/regs-ac97.h>
#include <mach/audio.h>

#include "pxa2xx-ac97.h"

static void pxa2xx_ac97_warm_reset(struct snd_ac97 *ac97)
{
	pxa2xx_ac97_try_warm_reset(ac97);

	pxa2xx_ac97_finish_reset(ac97);
}

static void pxa2xx_ac97_cold_reset(struct snd_ac97 *ac97)
{
	pxa2xx_ac97_try_cold_reset(ac97);

	pxa2xx_ac97_finish_reset(ac97);
}

static struct snd_ac97_bus_ops pxa2xx_ac97_ops = {
	.read	= pxa2xx_ac97_read,
	.write	= pxa2xx_ac97_write,
	.warm_reset	= pxa2xx_ac97_warm_reset,
	.reset	= pxa2xx_ac97_cold_reset,
};

static unsigned long pxa2xx_ac97_pcm_stereo_in_req = 12;
static struct snd_dmaengine_dai_dma_data pxa2xx_ac97_pcm_stereo_in = {
	.addr		= __PREG(PCDR),
	.addr_width	= DMA_SLAVE_BUSWIDTH_4_BYTES,
	.maxburst	= 32,
	.filter_data	= &pxa2xx_ac97_pcm_stereo_in_req,
};

static unsigned long pxa2xx_ac97_pcm_stereo_out_req = 11;
static struct snd_dmaengine_dai_dma_data pxa2xx_ac97_pcm_stereo_out = {
	.addr		= __PREG(PCDR),
	.addr_width	= DMA_SLAVE_BUSWIDTH_4_BYTES,
	.maxburst	= 32,
	.filter_data	= &pxa2xx_ac97_pcm_stereo_out_req,
};

static unsigned long pxa2xx_ac97_pcm_aux_mono_out_req = 10;
static struct snd_dmaengine_dai_dma_data pxa2xx_ac97_pcm_aux_mono_out = {
	.addr		= __PREG(MODR),
	.addr_width	= DMA_SLAVE_BUSWIDTH_2_BYTES,
	.maxburst	= 16,
	.filter_data	= &pxa2xx_ac97_pcm_aux_mono_out_req,
};

static unsigned long pxa2xx_ac97_pcm_aux_mono_in_req = 9;
static struct snd_dmaengine_dai_dma_data pxa2xx_ac97_pcm_aux_mono_in = {
	.addr		= __PREG(MODR),
	.addr_width	= DMA_SLAVE_BUSWIDTH_2_BYTES,
	.maxburst	= 16,
	.filter_data	= &pxa2xx_ac97_pcm_aux_mono_in_req,
};

static unsigned long pxa2xx_ac97_pcm_aux_mic_mono_req = 8;
static struct snd_dmaengine_dai_dma_data pxa2xx_ac97_pcm_mic_mono_in = {
	.addr		= __PREG(MCDR),
	.addr_width	= DMA_SLAVE_BUSWIDTH_2_BYTES,
	.maxburst	= 16,
	.filter_data	= &pxa2xx_ac97_pcm_aux_mic_mono_req,
};

#ifdef CONFIG_PM
static int pxa2xx_ac97_suspend(struct snd_soc_dai *dai)
{
	return pxa2xx_ac97_hw_suspend();
}

static int pxa2xx_ac97_resume(struct snd_soc_dai *dai)
{
	return pxa2xx_ac97_hw_resume();
}

#else
#define pxa2xx_ac97_suspend	NULL
#define pxa2xx_ac97_resume	NULL
#endif

static int pxa2xx_ac97_probe(struct snd_soc_dai *dai)
{
	return pxa2xx_ac97_hw_probe(to_platform_device(dai->dev));
}

static int pxa2xx_ac97_remove(struct snd_soc_dai *dai)
{
	pxa2xx_ac97_hw_remove(to_platform_device(dai->dev));
	return 0;
}

static int pxa2xx_ac97_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *cpu_dai)
{
	struct snd_dmaengine_dai_dma_data *dma_data;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &pxa2xx_ac97_pcm_stereo_out;
	else
		dma_data = &pxa2xx_ac97_pcm_stereo_in;

	snd_soc_dai_set_dma_data(cpu_dai, substream, dma_data);

	return 0;
}

static int pxa2xx_ac97_hw_aux_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *cpu_dai)
{
	struct snd_dmaengine_dai_dma_data *dma_data;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &pxa2xx_ac97_pcm_aux_mono_out;
	else
		dma_data = &pxa2xx_ac97_pcm_aux_mono_in;

	snd_soc_dai_set_dma_data(cpu_dai, substream, dma_data);

	return 0;
}

static int pxa2xx_ac97_hw_mic_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *cpu_dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return -ENODEV;
	else
		snd_soc_dai_set_dma_data(cpu_dai, substream,
					 &pxa2xx_ac97_pcm_mic_mono_in);

	return 0;
}

#define PXA2XX_AC97_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_48000)

static const struct snd_soc_dai_ops pxa_ac97_hifi_dai_ops = {
	.hw_params	= pxa2xx_ac97_hw_params,
};

static const struct snd_soc_dai_ops pxa_ac97_aux_dai_ops = {
	.hw_params	= pxa2xx_ac97_hw_aux_params,
};

static const struct snd_soc_dai_ops pxa_ac97_mic_dai_ops = {
	.hw_params	= pxa2xx_ac97_hw_mic_params,
};

/*
 * There is only 1 physical AC97 interface for pxa2xx, but it
 * has extra fifo's that can be used for aux DACs and ADCs.
 */
static struct snd_soc_dai_driver pxa_ac97_dai_driver[] = {
{
	.name = "pxa2xx-ac97",
	.ac97_control = 1,
	.probe = pxa2xx_ac97_probe,
	.remove = pxa2xx_ac97_remove,
	.suspend = pxa2xx_ac97_suspend,
	.resume = pxa2xx_ac97_resume,
	.playback = {
		.stream_name = "AC97 Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = PXA2XX_AC97_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.capture = {
		.stream_name = "AC97 Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = PXA2XX_AC97_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &pxa_ac97_hifi_dai_ops,
},
{
	.name = "pxa2xx-ac97-aux",
	.ac97_control = 1,
	.playback = {
		.stream_name = "AC97 Aux Playback",
		.channels_min = 1,
		.channels_max = 1,
		.rates = PXA2XX_AC97_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.capture = {
		.stream_name = "AC97 Aux Capture",
		.channels_min = 1,
		.channels_max = 1,
		.rates = PXA2XX_AC97_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &pxa_ac97_aux_dai_ops,
},
{
	.name = "pxa2xx-ac97-mic",
	.ac97_control = 1,
	.capture = {
		.stream_name = "AC97 Mic Capture",
		.channels_min = 1,
		.channels_max = 1,
		.rates = PXA2XX_AC97_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &pxa_ac97_mic_dai_ops,
},
};

static const struct snd_soc_component_driver pxa_ac97_component = {
	.name		= "pxa-ac97",
};

static int pxa2xx_ac97_dev_probe(struct platform_device *pdev)
{
	int ret;

	if (pdev->id != -1) {
		dev_err(&pdev->dev, "PXA2xx has only one AC97 port.\n");
		return -ENXIO;
	}

	ret = snd_soc_set_ac97_ops(&pxa2xx_ac97_ops);
	if (ret != 0)
		return ret;

	/* Punt most of the init to the SoC probe; we may need the machine
	 * driver to do interesting things with the clocking to get us up
	 * and running.
	 */
	return snd_soc_register_component(&pdev->dev, &pxa_ac97_component,
					  pxa_ac97_dai_driver, ARRAY_SIZE(pxa_ac97_dai_driver));
}

static int pxa2xx_ac97_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	snd_soc_set_ac97_ops(NULL);
	return 0;
}

static struct platform_driver pxa2xx_ac97_driver = {
	.probe		= pxa2xx_ac97_dev_probe,
	.remove		= pxa2xx_ac97_dev_remove,
	.driver		= {
		.name	= "pxa2xx-ac97",
		.owner	= THIS_MODULE,
	},
};

module_platform_driver(pxa2xx_ac97_driver);

MODULE_AUTHOR("Nicolas Pitre");
MODULE_DESCRIPTION("AC97 driver for the Intel PXA2xx chip");
MODULE_LICENSE("GPL");
