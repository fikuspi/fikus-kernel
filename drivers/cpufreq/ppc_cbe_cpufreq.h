/*
 * ppc_cbe_cpufreq.h
 *
 * This file contains the definitions used by the cbe_cpufreq driver.
 *
 * (C) Copyright IBM Deutschland Entwicklung GmbH 2005-2007
 *
 * Author: Christian Krafft <krafft@de.ibm.com>
 *
 */

#include <fikus/cpufreq.h>
#include <fikus/types.h>

int cbe_cpufreq_set_pmode(int cpu, unsigned int pmode);
int cbe_cpufreq_get_pmode(int cpu);

int cbe_cpufreq_set_pmode_pmi(int cpu, unsigned int pmode);

#if defined(CONFIG_CPU_FREQ_CBE_PMI) || defined(CONFIG_CPU_FREQ_CBE_PMI_MODULE)
extern bool cbe_cpufreq_has_pmi;
#else
#define cbe_cpufreq_has_pmi (0)
#endif
