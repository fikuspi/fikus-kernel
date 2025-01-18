/*
 * Copyright (C) 2008-2009 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2008-2009 PetaLogix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/export.h>
#include <fikus/string.h>
#include <fikus/cryptohash.h>
#include <fikus/delay.h>
#include <fikus/in6.h>
#include <fikus/syscalls.h>

#include <asm/checksum.h>
#include <asm/cacheflush.h>
#include <fikus/io.h>
#include <asm/page.h>
#include <fikus/ftrace.h>
#include <fikus/uaccess.h>

#ifdef CONFIG_FUNCTION_TRACER
extern void _mcount(void);
EXPORT_SYMBOL(_mcount);
#endif

/*
 * Assembly functions that may be used (directly or indirectly) by modules
 */
EXPORT_SYMBOL(__copy_tofrom_user);
EXPORT_SYMBOL(__strncpy_user);

#ifdef CONFIG_OPT_LIB_ASM
EXPORT_SYMBOL(memcpy);
EXPORT_SYMBOL(memmove);
#endif

#ifdef CONFIG_MMU
EXPORT_SYMBOL(empty_zero_page);
#endif

EXPORT_SYMBOL(mbc);

extern void __divsi3(void);
EXPORT_SYMBOL(__divsi3);
extern void __modsi3(void);
EXPORT_SYMBOL(__modsi3);
extern void __mulsi3(void);
EXPORT_SYMBOL(__mulsi3);
extern void __udivsi3(void);
EXPORT_SYMBOL(__udivsi3);
extern void __umodsi3(void);
EXPORT_SYMBOL(__umodsi3);
