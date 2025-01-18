/* Null profiling driver
 *
 * Copyright (C) 2003  Paul Mundt
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * Licence.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <fikus/kernel.h>
#include <fikus/oprofile.h>
#include <fikus/init.h>
#include <fikus/errno.h>

int __init oprofile_arch_init(struct oprofile_operations *ops)
{
	return -ENODEV;
}

void oprofile_arch_exit(void)
{
}

