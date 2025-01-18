/**
 * @file init.c
 *
 * @remark Copyright 2008 Tensilica Inc.
 * @remark Read the file COPYING
 *
 */

#include <fikus/kernel.h>
#include <fikus/oprofile.h>
#include <fikus/errno.h>
#include <fikus/init.h>


extern void xtensa_backtrace(struct pt_regs *const regs, unsigned int depth);

int __init oprofile_arch_init(struct oprofile_operations *ops)
{
	ops->backtrace = xtensa_backtrace;
	return -ENODEV;
}


void oprofile_arch_exit(void)
{
}
