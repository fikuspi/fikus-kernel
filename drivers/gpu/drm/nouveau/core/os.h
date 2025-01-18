#ifndef __NOUVEAU_OS_H__
#define __NOUVEAU_OS_H__

#include <fikus/types.h>
#include <fikus/slab.h>
#include <fikus/mutex.h>
#include <fikus/pci.h>
#include <fikus/printk.h>
#include <fikus/bitops.h>
#include <fikus/firmware.h>
#include <fikus/module.h>
#include <fikus/i2c.h>
#include <fikus/i2c-algo-bit.h>
#include <fikus/delay.h>
#include <fikus/io-mapping.h>
#include <fikus/acpi.h>
#include <fikus/vmalloc.h>
#include <fikus/dmi.h>
#include <fikus/reboot.h>
#include <fikus/interrupt.h>
#include <fikus/log2.h>
#include <fikus/pm_runtime.h>

#include <asm/unaligned.h>

static inline int
ffsll(u64 mask)
{
	int i;
	for (i = 0; i < 64; i++) {
		if (mask & (1ULL << i))
			return i + 1;
	}
	return 0;
}

#ifndef ioread32_native
#ifdef __BIG_ENDIAN
#define ioread16_native ioread16be
#define iowrite16_native iowrite16be
#define ioread32_native  ioread32be
#define iowrite32_native iowrite32be
#else /* def __BIG_ENDIAN */
#define ioread16_native ioread16
#define iowrite16_native iowrite16
#define ioread32_native  ioread32
#define iowrite32_native iowrite32
#endif /* def __BIG_ENDIAN else */
#endif /* !ioread32_native */

#endif
