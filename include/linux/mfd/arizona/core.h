/*
 * Arizona MFD internals
 *
 * Copyright 2012 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _WM_ARIZONA_CORE_H
#define _WM_ARIZONA_CORE_H

#include <fikus/interrupt.h>
#include <fikus/regmap.h>
#include <fikus/regulator/consumer.h>
#include <fikus/mfd/arizona/pdata.h>

#define ARIZONA_MAX_CORE_SUPPLIES 3

enum arizona_type {
	WM5102 = 1,
	WM5110 = 2,
	WM8997 = 3,
};

#define ARIZONA_IRQ_GP1                    0
#define ARIZONA_IRQ_GP2                    1
#define ARIZONA_IRQ_GP3                    2
#define ARIZONA_IRQ_GP4                    3
#define ARIZONA_IRQ_GP5_FALL               4
#define ARIZONA_IRQ_GP5_RISE               5
#define ARIZONA_IRQ_JD_FALL                6
#define ARIZONA_IRQ_JD_RISE                7
#define ARIZONA_IRQ_DSP1_RAM_RDY           8
#define ARIZONA_IRQ_DSP2_RAM_RDY           9
#define ARIZONA_IRQ_DSP3_RAM_RDY          10
#define ARIZONA_IRQ_DSP4_RAM_RDY          11
#define ARIZONA_IRQ_DSP_IRQ1              12
#define ARIZONA_IRQ_DSP_IRQ2              13
#define ARIZONA_IRQ_DSP_IRQ3              14
#define ARIZONA_IRQ_DSP_IRQ4              15
#define ARIZONA_IRQ_DSP_IRQ5              16
#define ARIZONA_IRQ_DSP_IRQ6              17
#define ARIZONA_IRQ_DSP_IRQ7              18
#define ARIZONA_IRQ_DSP_IRQ8              19
#define ARIZONA_IRQ_SPK_SHUTDOWN_WARN     20
#define ARIZONA_IRQ_SPK_SHUTDOWN          21
#define ARIZONA_IRQ_MICDET                22
#define ARIZONA_IRQ_HPDET                 23
#define ARIZONA_IRQ_WSEQ_DONE             24
#define ARIZONA_IRQ_DRC2_SIG_DET          25
#define ARIZONA_IRQ_DRC1_SIG_DET          26
#define ARIZONA_IRQ_ASRC2_LOCK            27
#define ARIZONA_IRQ_ASRC1_LOCK            28
#define ARIZONA_IRQ_UNDERCLOCKED          29
#define ARIZONA_IRQ_OVERCLOCKED           30
#define ARIZONA_IRQ_FLL2_LOCK             31
#define ARIZONA_IRQ_FLL1_LOCK             32
#define ARIZONA_IRQ_CLKGEN_ERR            33
#define ARIZONA_IRQ_CLKGEN_ERR_ASYNC      34
#define ARIZONA_IRQ_ASRC_CFG_ERR          35
#define ARIZONA_IRQ_AIF3_ERR              36
#define ARIZONA_IRQ_AIF2_ERR              37
#define ARIZONA_IRQ_AIF1_ERR              38
#define ARIZONA_IRQ_CTRLIF_ERR            39
#define ARIZONA_IRQ_MIXER_DROPPED_SAMPLES 40
#define ARIZONA_IRQ_ASYNC_CLK_ENA_LOW     41
#define ARIZONA_IRQ_SYSCLK_ENA_LOW        42
#define ARIZONA_IRQ_ISRC1_CFG_ERR         43
#define ARIZONA_IRQ_ISRC2_CFG_ERR         44
#define ARIZONA_IRQ_BOOT_DONE             45
#define ARIZONA_IRQ_DCS_DAC_DONE          46
#define ARIZONA_IRQ_DCS_HP_DONE           47
#define ARIZONA_IRQ_FLL2_CLOCK_OK         48
#define ARIZONA_IRQ_FLL1_CLOCK_OK         49
#define ARIZONA_IRQ_MICD_CLAMP_RISE	  50
#define ARIZONA_IRQ_MICD_CLAMP_FALL	  51

#define ARIZONA_NUM_IRQ                   52

struct snd_soc_dapm_context;

struct arizona {
	struct regmap *regmap;
	struct device *dev;

	enum arizona_type type;
	unsigned int rev;

	int num_core_supplies;
	struct regulator_bulk_data core_supplies[ARIZONA_MAX_CORE_SUPPLIES];
	struct regulator *dcvdd;

	struct arizona_pdata pdata;

	unsigned int external_dcvdd:1;

	int irq;
	struct irq_domain *virq;
	struct regmap_irq_chip_data *aod_irq_chip;
	struct regmap_irq_chip_data *irq_chip;

	bool hpdet_magic;
	unsigned int hp_ena;

	struct mutex clk_lock;
	int clk32k_ref;

	struct snd_soc_dapm_context *dapm;
};

int arizona_clk32k_enable(struct arizona *arizona);
int arizona_clk32k_disable(struct arizona *arizona);

int arizona_request_irq(struct arizona *arizona, int irq, char *name,
			irq_handler_t handler, void *data);
void arizona_free_irq(struct arizona *arizona, int irq, void *data);
int arizona_set_irq_wake(struct arizona *arizona, int irq, int on);

int wm5102_patch(struct arizona *arizona);
int wm5110_patch(struct arizona *arizona);
int wm8997_patch(struct arizona *arizona);

#endif
