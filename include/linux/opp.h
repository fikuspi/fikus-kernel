/*
 * Generic OPP Interface
 *
 * Copyright (C) 2009-2010 Texas Instruments Incorporated.
 *	Nishanth Menon
 *	Romit Dasgupta
 *	Kevin Hilman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __FIKUS_OPP_H__
#define __FIKUS_OPP_H__

#include <fikus/err.h>
#include <fikus/cpufreq.h>
#include <fikus/notifier.h>

struct opp;
struct device;

enum opp_event {
	OPP_EVENT_ADD, OPP_EVENT_ENABLE, OPP_EVENT_DISABLE,
};

#if defined(CONFIG_PM_OPP)

unsigned long opp_get_voltage(struct opp *opp);

unsigned long opp_get_freq(struct opp *opp);

int opp_get_opp_count(struct device *dev);

struct opp *opp_find_freq_exact(struct device *dev, unsigned long freq,
				bool available);

struct opp *opp_find_freq_floor(struct device *dev, unsigned long *freq);

struct opp *opp_find_freq_ceil(struct device *dev, unsigned long *freq);

int opp_add(struct device *dev, unsigned long freq, unsigned long u_volt);

int opp_enable(struct device *dev, unsigned long freq);

int opp_disable(struct device *dev, unsigned long freq);

struct srcu_notifier_head *opp_get_notifier(struct device *dev);
#else
static inline unsigned long opp_get_voltage(struct opp *opp)
{
	return 0;
}

static inline unsigned long opp_get_freq(struct opp *opp)
{
	return 0;
}

static inline int opp_get_opp_count(struct device *dev)
{
	return 0;
}

static inline struct opp *opp_find_freq_exact(struct device *dev,
					unsigned long freq, bool available)
{
	return ERR_PTR(-EINVAL);
}

static inline struct opp *opp_find_freq_floor(struct device *dev,
					unsigned long *freq)
{
	return ERR_PTR(-EINVAL);
}

static inline struct opp *opp_find_freq_ceil(struct device *dev,
					unsigned long *freq)
{
	return ERR_PTR(-EINVAL);
}

static inline int opp_add(struct device *dev, unsigned long freq,
					unsigned long u_volt)
{
	return -EINVAL;
}

static inline int opp_enable(struct device *dev, unsigned long freq)
{
	return 0;
}

static inline int opp_disable(struct device *dev, unsigned long freq)
{
	return 0;
}

static inline struct srcu_notifier_head *opp_get_notifier(struct device *dev)
{
	return ERR_PTR(-EINVAL);
}
#endif		/* CONFIG_PM_OPP */

#if defined(CONFIG_PM_OPP) && defined(CONFIG_OF)
int of_init_opp_table(struct device *dev);
#else
static inline int of_init_opp_table(struct device *dev)
{
	return -EINVAL;
}
#endif

#if defined(CONFIG_CPU_FREQ) && defined(CONFIG_PM_OPP)
int opp_init_cpufreq_table(struct device *dev,
			    struct cpufreq_frequency_table **table);
void opp_free_cpufreq_table(struct device *dev,
				struct cpufreq_frequency_table **table);
#else
static inline int opp_init_cpufreq_table(struct device *dev,
			    struct cpufreq_frequency_table **table)
{
	return -EINVAL;
}

static inline
void opp_free_cpufreq_table(struct device *dev,
				struct cpufreq_frequency_table **table)
{
}
#endif		/* CONFIG_CPU_FREQ */

#endif		/* __FIKUS_OPP_H__ */
