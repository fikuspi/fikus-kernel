#include <fikus/types.h>
#include <fikus/errno.h>
#include <asm/uaccess.h>

int frsqrtes(void *frD, void *frB)
{
#ifdef DEBUG
	printk("%s: %p %p\n", __func__, frD, frB);
#endif
	return 0;
}
