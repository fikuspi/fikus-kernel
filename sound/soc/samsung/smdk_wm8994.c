/*
 *  smdk_wm8994.c
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include "../codecs/wm8994.h"
#include <sound/pcm_params.h>
#include <fikus/module.h>
#include <fikus/of.h>
#include <fikus/of_device.h>

 /*
  * Default CFG switch settings to use this driver:
  *	SMDKV310: CFG5-1000, CFG7-111111
  */

 /*
  * Configure audio route as :-
  * $ amixer sset 'DAC1' on,on
  * $ amixer sset 'Right Headphone Mux' 'DAC'
  * $ amixer sset 'Left Headphone Mux' 'DAC'
  * $ amixer sset 'DAC1R Mixer AIF1.1' on
  * $ amixer sset 'DAC1L Mixer AIF1.1' on
  * $ amixer sset 'IN2L' on
  * $ amixer sset 'IN2L PGA IN2LN' on
  * $ amixer sset 'MIXINL IN2L' on
  * $ amixer sset 'AIF1ADC1L Mixer ADC/DMIC' on
  * $ amixer sset 'IN2R' on
  * $ amixer sset 'IN2R PGA IN2RN' on
  * $ amixer sset 'MIXINR IN2R' on
  * $ amixer sset 'AIF1ADC1R Mixer ADC/DMIC' on
  */

/* SMDK has a 16.934MHZ crystal attached to WM8994 */
#define SMDK_WM8994_FREQ 16934000

struct smdk_wm8994_data {
	int mclk1_rate;
};

/* Default SMDKs */
static struct smdk_wm8994_data smdk_board_data = {
	.mclk1_rate = SMDK_WM8994_FREQ,
};

static int smdk_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned int pll_out;
	int ret;

	/* AIF1CLK should be >=3MHz for optimal performance */
	if (params_format(params) == SNDRV_PCM_FORMAT_S24_LE)
		pll_out = params_rate(params) * 384;
	else if (params_rate(params) == 8000 || params_rate(params) == 11025)
		pll_out = params_rate(params) * 512;
	else
		pll_out = params_rate(params) * 256;

	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1, WM8994_FLL_SRC_MCLK1,
					SMDK_WM8994_FREQ, pll_out);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1,
					pll_out, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * SMDK WM8994 DAI operations.
 */
static struct snd_soc_ops smdk_ops = {
	.hw_params = smdk_hw_params,
};

static int smdk_wm8994_init_paiftx(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	/* HeadPhone */
	snd_soc_dapm_enable_pin(dapm, "HPOUT1R");
	snd_soc_dapm_enable_pin(dapm, "HPOUT1L");

	/* MicIn */
	snd_soc_dapm_enable_pin(dapm, "IN1LN");
	snd_soc_dapm_enable_pin(dapm, "IN1RN");

	/* LineIn */
	snd_soc_dapm_enable_pin(dapm, "IN2LN");
	snd_soc_dapm_enable_pin(dapm, "IN2RN");

	/* Other pins NC */
	snd_soc_dapm_nc_pin(dapm, "HPOUT2P");
	snd_soc_dapm_nc_pin(dapm, "HPOUT2N");
	snd_soc_dapm_nc_pin(dapm, "SPKOUTLN");
	snd_soc_dapm_nc_pin(dapm, "SPKOUTLP");
	snd_soc_dapm_nc_pin(dapm, "SPKOUTRP");
	snd_soc_dapm_nc_pin(dapm, "SPKOUTRN");
	snd_soc_dapm_nc_pin(dapm, "LINEOUT1N");
	snd_soc_dapm_nc_pin(dapm, "LINEOUT1P");
	snd_soc_dapm_nc_pin(dapm, "LINEOUT2N");
	snd_soc_dapm_nc_pin(dapm, "LINEOUT2P");
	snd_soc_dapm_nc_pin(dapm, "IN1LP");
	snd_soc_dapm_nc_pin(dapm, "IN2LP:VXRN");
	snd_soc_dapm_nc_pin(dapm, "IN1RP");
	snd_soc_dapm_nc_pin(dapm, "IN2RP:VXRP");

	return 0;
}

static struct snd_soc_dai_link smdk_dai[] = {
	{ /* Primary DAI i/f */
		.name = "WM8994 AIF1",
		.stream_name = "Pri_Dai",
		.cpu_dai_name = "samsung-i2s.0",
		.codec_dai_name = "wm8994-aif1",
		.platform_name = "samsung-i2s.0",
		.codec_name = "wm8994-codec",
		.init = smdk_wm8994_init_paiftx,
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBM_CFM,
		.ops = &smdk_ops,
	}, { /* Sec_Fifo Playback i/f */
		.name = "Sec_FIFO TX",
		.stream_name = "Sec_Dai",
		.cpu_dai_name = "samsung-i2s-sec",
		.codec_dai_name = "wm8994-aif1",
		.platform_name = "samsung-i2s-sec",
		.codec_name = "wm8994-codec",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBM_CFM,
		.ops = &smdk_ops,
	},
};

static struct snd_soc_card smdk = {
	.name = "SMDK-I2S",
	.owner = THIS_MODULE,
	.dai_link = smdk_dai,
	.num_links = ARRAY_SIZE(smdk_dai),
};

#ifdef CONFIG_OF
static const struct of_device_id samsung_wm8994_of_match[] = {
	{ .compatible = "samsung,smdk-wm8994", .data = &smdk_board_data },
	{},
};
MODULE_DEVICE_TABLE(of, samsung_wm8994_of_match);
#endif /* CONFIG_OF */

static int smdk_audio_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &smdk;
	struct smdk_wm8994_data *board;
	const struct of_device_id *id;

	card->dev = &pdev->dev;

	board = devm_kzalloc(&pdev->dev, sizeof(*board), GFP_KERNEL);
	if (!board)
		return -ENOMEM;

	if (np) {
		smdk_dai[0].cpu_dai_name = NULL;
		smdk_dai[0].cpu_of_node = of_parse_phandle(np,
				"samsung,i2s-controller", 0);
		if (!smdk_dai[0].cpu_of_node) {
			dev_err(&pdev->dev,
			   "Property 'samsung,i2s-controller' missing or invalid\n");
			ret = -EINVAL;
		}

		smdk_dai[0].platform_name = NULL;
		smdk_dai[0].platform_of_node = smdk_dai[0].cpu_of_node;
	}

	id = of_match_device(samsung_wm8994_of_match, &pdev->dev);
	if (id)
		*board = *((struct smdk_wm8994_data *)id->data);

	platform_set_drvdata(pdev, board);

	ret = snd_soc_register_card(card);

	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card() failed:%d\n", ret);

	return ret;
}

static int smdk_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_soc_unregister_card(card);

	return 0;
}

static struct platform_driver smdk_audio_driver = {
	.driver		= {
		.name	= "smdk-audio-wm8894",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_wm8994_of_match),
	},
	.probe		= smdk_audio_probe,
	.remove		= smdk_audio_remove,
};

module_platform_driver(smdk_audio_driver);

MODULE_DESCRIPTION("ALSA SoC SMDK WM8994");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:smdk-audio-wm8994");
