#ifndef _FIKUS_STACKPROTECTOR_H
#define _FIKUS_STACKPROTECTOR_H 1

#include <fikus/compiler.h>
#include <fikus/sched.h>
#include <fikus/random.h>

#ifdef CONFIG_CC_STACKPROTECTOR
# include <asm/stackprotector.h>
#else
static inline void boot_init_stack_canary(void)
{
}
#endif

#endif
