#ifndef POWERPC_85XX_SMP_H_
#define POWERPC_85XX_SMP_H_ 1

#include <fikus/init.h>

#ifdef CONFIG_SMP
void __init mpc85xx_smp_init(void);
#else
static inline void mpc85xx_smp_init(void)
{
	/* Nothing to do */
}
#endif

#endif /* not POWERPC_85XX_SMP_H_ */
