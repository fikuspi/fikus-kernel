/*
 * Based on arch/arm/kernel/armksyms.c
 *
 * Copyright (C) 2000 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fikus/export.h>
#include <fikus/sched.h>
#include <fikus/string.h>
#include <fikus/cryptohash.h>
#include <fikus/delay.h>
#include <fikus/in6.h>
#include <fikus/syscalls.h>
#include <fikus/uaccess.h>
#include <fikus/io.h>

#include <asm/checksum.h>

	/* user mem (segment) */
EXPORT_SYMBOL(__strnlen_user);
EXPORT_SYMBOL(__strncpy_from_user);

EXPORT_SYMBOL(copy_page);
EXPORT_SYMBOL(clear_page);

EXPORT_SYMBOL(__copy_from_user);
EXPORT_SYMBOL(__copy_to_user);
EXPORT_SYMBOL(__clear_user);

	/* physical memory */
EXPORT_SYMBOL(memstart_addr);

	/* string / mem functions */
EXPORT_SYMBOL(strchr);
EXPORT_SYMBOL(strrchr);
EXPORT_SYMBOL(memset);
EXPORT_SYMBOL(memcpy);
EXPORT_SYMBOL(memmove);
EXPORT_SYMBOL(memchr);

	/* atomic bitops */
EXPORT_SYMBOL(set_bit);
EXPORT_SYMBOL(test_and_set_bit);
EXPORT_SYMBOL(clear_bit);
EXPORT_SYMBOL(test_and_clear_bit);
EXPORT_SYMBOL(change_bit);
EXPORT_SYMBOL(test_and_change_bit);
