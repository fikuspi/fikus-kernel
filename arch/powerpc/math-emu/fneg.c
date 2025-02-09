#include <fikus/types.h>
#include <fikus/errno.h>
#include <asm/uaccess.h>

int
fneg(u32 *frD, u32 *frB)
{
	frD[0] = frB[0] ^ 0x80000000;
	frD[1] = frB[1];

#ifdef DEBUG
	printk("%s: D %p, B %p: ", __func__, frD, frB);
	dump_double(frD);
	printk("\n");
#endif

	return 0;
}
