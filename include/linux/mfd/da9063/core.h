/*
 * Definitions for DA9063 MFD driver
 *
 * Copyright 2012 Dialog Semiconductor Ltd.
 *
 * Author: Michal Hajduk <michal.hajduk@diasemi.com>
 *	   Krystian Garbaciak <krystian.garbaciak@diasemi.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __MFD_DA9063_CORE_H__
#define __MFD_DA9063_CORE_H__

#include <fikus/interrupt.h>
#include <fikus/mfd/da9063/registers.h>

/* DA9063 modules */
#define DA9063_DRVNAME_CORE		"da9063-core"
#define DA9063_DRVNAME_REGULATORS	"da9063-regulators"
#define DA9063_DRVNAME_LEDS		"da9063-leds"
#define DA9063_DRVNAME_WATCHDOG		"da9063-watchdog"
#define DA9063_DRVNAME_HWMON		"da9063-hwmon"
#define DA9063_DRVNAME_ONKEY		"da9063-onkey"
#define DA9063_DRVNAME_RTC		"da9063-rtc"
#define DA9063_DRVNAME_VIBRATION	"da9063-vibration"

enum da9063_models {
	PMIC_DA9063 = 0x61,
};

/* Interrupts */
enum da9063_irqs {
	DA9063_IRQ_ONKEY = 0,
	DA9063_IRQ_ALARM,
	DA9063_IRQ_TICK,
	DA9063_IRQ_ADC_RDY,
	DA9063_IRQ_SEQ_RDY,
	DA9063_IRQ_WAKE,
	DA9063_IRQ_TEMP,
	DA9063_IRQ_COMP_1V2,
	DA9063_IRQ_LDO_LIM,
	DA9063_IRQ_REG_UVOV,
	DA9063_IRQ_VDD_MON,
	DA9063_IRQ_WARN,
	DA9063_IRQ_GPI0,
	DA9063_IRQ_GPI1,
	DA9063_IRQ_GPI2,
	DA9063_IRQ_GPI3,
	DA9063_IRQ_GPI4,
	DA9063_IRQ_GPI5,
	DA9063_IRQ_GPI6,
	DA9063_IRQ_GPI7,
	DA9063_IRQ_GPI8,
	DA9063_IRQ_GPI9,
	DA9063_IRQ_GPI10,
	DA9063_IRQ_GPI11,
	DA9063_IRQ_GPI12,
	DA9063_IRQ_GPI13,
	DA9063_IRQ_GPI14,
	DA9063_IRQ_GPI15,
};

#define DA9063_IRQ_BASE_OFFSET	0
#define DA9063_NUM_IRQ		(DA9063_IRQ_GPI15 + 1 - DA9063_IRQ_BASE_OFFSET)

struct da9063 {
	/* Device */
	struct device	*dev;
	unsigned short	model;
	unsigned short	revision;
	unsigned int	flags;

	/* Control interface */
	struct regmap	*regmap;

	/* Interrupts */
	int		chip_irq;
	unsigned int	irq_base;
	struct regmap_irq_chip_data *regmap_irq;
};

int da9063_device_init(struct da9063 *da9063, unsigned int irq);
int da9063_irq_init(struct da9063 *da9063);

void da9063_device_exit(struct da9063 *da9063);
void da9063_irq_exit(struct da9063 *da9063);

#endif /* __MFD_DA9063_CORE_H__ */
