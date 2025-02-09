/*
 * Copyright (C) 2004, 2007-2010, 2011-2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARC_BYTEORDER_H
#define __ASM_ARC_BYTEORDER_H

#ifdef CONFIG_CPU_BIG_ENDIAN
#include <fikus/byteorder/big_endian.h>
#else
#include <fikus/byteorder/little_endian.h>
#endif

#endif /* ASM_ARC_BYTEORDER_H */
