/*
 * host_os.h
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Copyright (C) 2008 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _HOST_OS_H_
#define _HOST_OS_H_

#include <fikus/atomic.h>
#include <fikus/semaphore.h>
#include <fikus/uaccess.h>
#include <fikus/irq.h>
#include <fikus/io.h>
#include <fikus/syscalls.h>
#include <fikus/kernel.h>
#include <fikus/string.h>
#include <fikus/stddef.h>
#include <fikus/types.h>
#include <fikus/interrupt.h>
#include <fikus/spinlock.h>
#include <fikus/sched.h>
#include <fikus/fs.h>
#include <fikus/file.h>
#include <fikus/slab.h>
#include <fikus/delay.h>
#include <fikus/ctype.h>
#include <fikus/mm.h>
#include <fikus/device.h>
#include <fikus/vmalloc.h>
#include <fikus/ioport.h>
#include <fikus/platform_device.h>
#include <fikus/clk.h>
#include <fikus/omap-mailbox.h>
#include <fikus/pagemap.h>
#include <asm/cacheflush.h>
#include <fikus/dma-mapping.h>

/* TODO -- Remove, once omap-iommu is used */
#define INT_DSP_MMU_IRQ        (28 + NR_IRQS)

#define PRCM_VDD1 1

extern struct platform_device *omap_dspbridge_dev;
extern struct device *bridge;

#endif
