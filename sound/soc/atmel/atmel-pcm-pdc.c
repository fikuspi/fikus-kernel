/*
 * atmel-pcm.c  --  ALSA PCM interface for the Atmel atmel SoC.
 *
 *  Copyright (C) 2005 SAN People
 *  Copyright (C) 2008 Atmel
 *
 * Authors: Sedji Gaouaou <sedji.gaouaou@atmel.com>
 *
 * Based on at91-pcm. by:
 * Frank Mandarino <fmandarino@endrelia.com>
 * Copyright 2006 Endrelia Technologies Inc.
 *
 * Based on pxa2xx-pcm.c by:
 *
 * Author:	Nicolas Pitre
 * Created:	Nov 30, 2004
 * Copyright:	(C) 2004 MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include <fikus/module.h>
#include <fikus/init.h>
#include <fikus/platform_device.h>
#include <fikus/slab.h>
#include <fikus/dma-mapping.h>
#include <fikus/atmel_pdc.h>
#include <fikus/atmel-ssc.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "atmel-pcm.h"


/*--------------------------------------------------------------------------*\
 * Hardware definition
\*--------------------------------------------------------------------------*/
/* TODO: These values were taken from the AT91 platform driver, check
 *	 them against real values for AT32
 */
static const struct snd_pcm_hardware atmel_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_INTERLEAVED |
				  SNDRV_PCM_INFO_PAUSE,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min	= 32,
	.period_bytes_max	= 8192,
	.periods_min		= 2,
	.periods_max		= 1024,
	.buffer_bytes_max	= ATMEL_SSC_DMABUF_SIZE,
};


/*--------------------------------------------------------------------------*\
 * Data types
\*--------------------------------------------------------------------------*/
struct atmel_runtime_data {
	struct atmel_pcm_dma_params *params;
	dma_addr_t dma_buffer;		/* physical address of dma buffer */
	dma_addr_t dma_buffer_end;	/* first address beyond DMA buffer */
	size_t period_size;

	dma_addr_t period_ptr;		/* physical address of next period */

	/* PDC register save */
	u32 pdc_xpr_save;
	u32 pdc_xcr_save;
	u32 pdc_xnpr_save;
	u32 pdc_xncr_save;
};

/*--------------------------------------------------------------------------*\
 * ISR
\*--------------------------------------------------------------------------*/
static void atmel_pcm_dma_irq(u32 ssc_sr,
	struct snd_pcm_substream *substream)
{
	struct atmel_runtime_data *prtd = substream->runtime->private_data;
	struct atmel_pcm_dma_params *params = prtd->params;
	static int count;

	count++;

	if (ssc_sr & params->mask->ssc_endbuf) {
		pr_warn("atmel-pcm: buffer %s on %s (SSC_SR=%#x, count=%d)\n",
				substream->stream == SNDRV_PCM_STREAM_PLAYBACK
				? "underrun" : "overrun",
				params->name, ssc_sr, count);

		/* re-start the PDC */
		ssc_writex(params->ssc->regs, ATMEL_PDC_PTCR,
			   params->mask->pdc_disable);
		prtd->period_ptr += prtd->period_size;
		if (prtd->period_ptr >= prtd->dma_buffer_end)
			prtd->period_ptr = prtd->dma_buffer;

		ssc_writex(params->ssc->regs, params->pdc->xpr,
			   prtd->period_ptr);
		ssc_writex(params->ssc->regs, params->pdc->xcr,
			   prtd->period_size / params->pdc_xfer_size);
		ssc_writex(params->ssc->regs, ATMEL_PDC_PTCR,
			   params->mask->pdc_enable);
	}

	if (ssc_sr & params->mask->ssc_endx) {
		/* Load the PDC next pointer and counter registers */
		prtd->period_ptr += prtd->period_size;
		if (prtd->period_ptr >= prtd->dma_buffer_end)
			prtd->period_ptr = prtd->dma_buffer;

		ssc_writex(params->ssc->regs, params->pdc->xnpr,
			   prtd->period_ptr);
		ssc_writex(params->ssc->regs, params->pdc->xncr,
			   prtd->period_size / params->pdc_xfer_size);
	}

	snd_pcm_period_elapsed(substream);
}


/*--------------------------------------------------------------------------*\
 * PCM operations
\*--------------------------------------------------------------------------*/
static int atmel_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct atmel_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;

	/* this may get called several times by oss emulation
	 * with different params */

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = params_buffer_bytes(params);

	prtd->params = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);
	prtd->params->dma_intr_handler = atmel_pcm_dma_irq;

	prtd->dma_buffer = runtime->dma_addr;
	prtd->dma_buffer_end = runtime->dma_addr + runtime->dma_bytes;
	prtd->period_size = params_period_bytes(params);

	pr_debug("atmel-pcm: "
		"hw_params: DMA for %s initialized "
		"(dma_bytes=%zu, period_size=%zu)\n",
		prtd->params->name,
		runtime->dma_bytes,
		prtd->period_size);
	return 0;
}

static int atmel_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct atmel_runtime_data *prtd = substream->runtime->private_data;
	struct atmel_pcm_dma_params *params = prtd->params;

	if (params != NULL) {
		ssc_writex(params->ssc->regs, SSC_PDC_PTCR,
			   params->mask->pdc_disable);
		prtd->params->dma_intr_handler = NULL;
	}

	return 0;
}

static int atmel_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct atmel_runtime_data *prtd = substream->runtime->private_data;
	struct atmel_pcm_dma_params *params = prtd->params;

	ssc_writex(params->ssc->regs, SSC_IDR,
		   params->mask->ssc_endx | params->mask->ssc_endbuf);
	ssc_writex(params->ssc->regs, ATMEL_PDC_PTCR,
		   params->mask->pdc_disable);
	return 0;
}

static int atmel_pcm_trigger(struct snd_pcm_substream *substream,
	int cmd)
{
	struct snd_pcm_runtime *rtd = substream->runtime;
	struct atmel_runtime_data *prtd = rtd->private_data;
	struct atmel_pcm_dma_params *params = prtd->params;
	int ret = 0;

	pr_debug("atmel-pcm:buffer_size = %ld,"
		"dma_area = %p, dma_bytes = %zu\n",
		rtd->buffer_size, rtd->dma_area, rtd->dma_bytes);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		prtd->period_ptr = prtd->dma_buffer;

		ssc_writex(params->ssc->regs, params->pdc->xpr,
			   prtd->period_ptr);
		ssc_writex(params->ssc->regs, params->pdc->xcr,
			   prtd->period_size / params->pdc_xfer_size);

		prtd->period_ptr += prtd->period_size;
		ssc_writex(params->ssc->regs, params->pdc->xnpr,
			   prtd->period_ptr);
		ssc_writex(params->ssc->regs, params->pdc->xncr,
			   prtd->period_size / params->pdc_xfer_size);

		pr_debug("atmel-pcm: trigger: "
			"period_ptr=%lx, xpr=%u, "
			"xcr=%u, xnpr=%u, xncr=%u\n",
			(unsigned long)prtd->period_ptr,
			ssc_readx(params->ssc->regs, params->pdc->xpr),
			ssc_readx(params->ssc->regs, params->pdc->xcr),
			ssc_readx(params->ssc->regs, params->pdc->xnpr),
			ssc_readx(params->ssc->regs, params->pdc->xncr));

		ssc_writex(params->ssc->regs, SSC_IER,
			   params->mask->ssc_endx | params->mask->ssc_endbuf);
		ssc_writex(params->ssc->regs, SSC_PDC_PTCR,
			   params->mask->pdc_enable);

		pr_debug("sr=%u imr=%u\n",
			ssc_readx(params->ssc->regs, SSC_SR),
			ssc_readx(params->ssc->regs, SSC_IER));
		break;		/* SNDRV_PCM_TRIGGER_START */

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		ssc_writex(params->ssc->regs, ATMEL_PDC_PTCR,
			   params->mask->pdc_disable);
		break;

	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ssc_writex(params->ssc->regs, ATMEL_PDC_PTCR,
			   params->mask->pdc_enable);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static snd_pcm_uframes_t atmel_pcm_pointer(
	struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct atmel_runtime_data *prtd = runtime->private_data;
	struct atmel_pcm_dma_params *params = prtd->params;
	dma_addr_t ptr;
	snd_pcm_uframes_t x;

	ptr = (dma_addr_t) ssc_readx(params->ssc->regs, params->pdc->xpr);
	x = bytes_to_frames(runtime, ptr - prtd->dma_buffer);

	if (x == runtime->buffer_size)
		x = 0;

	return x;
}

static int atmel_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct atmel_runtime_data *prtd;
	int ret = 0;

	snd_soc_set_runtime_hwparams(substream, &atmel_pcm_hardware);

	/* ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime,
						SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		goto out;

	prtd = kzalloc(sizeof(struct atmel_runtime_data), GFP_KERNEL);
	if (prtd == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	runtime->private_data = prtd;

 out:
	return ret;
}

static int atmel_pcm_close(struct snd_pcm_substream *substream)
{
	struct atmel_runtime_data *prtd = substream->runtime->private_data;

	kfree(prtd);
	return 0;
}

static struct snd_pcm_ops atmel_pcm_ops = {
	.open		= atmel_pcm_open,
	.close		= atmel_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= atmel_pcm_hw_params,
	.hw_free	= atmel_pcm_hw_free,
	.prepare	= atmel_pcm_prepare,
	.trigger	= atmel_pcm_trigger,
	.pointer	= atmel_pcm_pointer,
	.mmap		= atmel_pcm_mmap,
};


/*--------------------------------------------------------------------------*\
 * ASoC platform driver
\*--------------------------------------------------------------------------*/
#ifdef CONFIG_PM
static int atmel_pcm_suspend(struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = dai->runtime;
	struct atmel_runtime_data *prtd;
	struct atmel_pcm_dma_params *params;

	if (!runtime)
		return 0;

	prtd = runtime->private_data;
	params = prtd->params;

	/* disable the PDC and save the PDC registers */

	ssc_writel(params->ssc->regs, PDC_PTCR, params->mask->pdc_disable);

	prtd->pdc_xpr_save = ssc_readx(params->ssc->regs, params->pdc->xpr);
	prtd->pdc_xcr_save = ssc_readx(params->ssc->regs, params->pdc->xcr);
	prtd->pdc_xnpr_save = ssc_readx(params->ssc->regs, params->pdc->xnpr);
	prtd->pdc_xncr_save = ssc_readx(params->ssc->regs, params->pdc->xncr);

	return 0;
}

static int atmel_pcm_resume(struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = dai->runtime;
	struct atmel_runtime_data *prtd;
	struct atmel_pcm_dma_params *params;

	if (!runtime)
		return 0;

	prtd = runtime->private_data;
	params = prtd->params;

	/* restore the PDC registers and enable the PDC */
	ssc_writex(params->ssc->regs, params->pdc->xpr, prtd->pdc_xpr_save);
	ssc_writex(params->ssc->regs, params->pdc->xcr, prtd->pdc_xcr_save);
	ssc_writex(params->ssc->regs, params->pdc->xnpr, prtd->pdc_xnpr_save);
	ssc_writex(params->ssc->regs, params->pdc->xncr, prtd->pdc_xncr_save);

	ssc_writel(params->ssc->regs, PDC_PTCR, params->mask->pdc_enable);
	return 0;
}
#else
#define atmel_pcm_suspend	NULL
#define atmel_pcm_resume	NULL
#endif

static struct snd_soc_platform_driver atmel_soc_platform = {
	.ops		= &atmel_pcm_ops,
	.pcm_new	= atmel_pcm_new,
	.pcm_free	= atmel_pcm_free,
	.suspend	= atmel_pcm_suspend,
	.resume		= atmel_pcm_resume,
};

int atmel_pcm_pdc_platform_register(struct device *dev)
{
	return snd_soc_register_platform(dev, &atmel_soc_platform);
}
EXPORT_SYMBOL(atmel_pcm_pdc_platform_register);

void atmel_pcm_pdc_platform_unregister(struct device *dev)
{
	snd_soc_unregister_platform(dev);
}
EXPORT_SYMBOL(atmel_pcm_pdc_platform_unregister);

MODULE_AUTHOR("Sedji Gaouaou <sedji.gaouaou@atmel.com>");
MODULE_DESCRIPTION("Atmel PCM module");
MODULE_LICENSE("GPL");
