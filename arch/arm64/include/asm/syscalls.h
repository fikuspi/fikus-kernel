/*
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
#ifndef __ASM_SYSCALLS_H
#define __ASM_SYSCALLS_H

#include <fikus/linkage.h>
#include <fikus/compiler.h>
#include <fikus/signal.h>

/*
 * System call wrappers implemented in kernel/entry.S.
 */
asmlinkage long sys_rt_sigreturn_wrapper(void);

#include <asm-generic/syscalls.h>

#endif	/* __ASM_SYSCALLS_H */
