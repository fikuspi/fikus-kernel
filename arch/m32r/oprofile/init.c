/**
 * @file init.c
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon <levon@movementarian.org>
 */

#include <fikus/kernel.h>
#include <fikus/oprofile.h>
#include <fikus/errno.h>
#include <fikus/init.h>

int __init oprofile_arch_init(struct oprofile_operations *ops)
{
	return -ENODEV;
}

void oprofile_arch_exit(void)
{
}
