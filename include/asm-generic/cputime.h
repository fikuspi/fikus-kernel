#ifndef _ASM_GENERIC_CPUTIME_H
#define _ASM_GENERIC_CPUTIME_H

#include <fikus/time.h>
#include <fikus/jiffies.h>

#ifndef CONFIG_VIRT_CPU_ACCOUNTING
# include <asm-generic/cputime_jiffies.h>
#endif

#ifdef CONFIG_VIRT_CPU_ACCOUNTING_GEN
# include <asm-generic/cputime_nsecs.h>
#endif

#endif
