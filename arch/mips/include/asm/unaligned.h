/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 Ralf Baechle (ralf@fikus-mips.org)
 */
#ifndef _ASM_MIPS_UNALIGNED_H
#define _ASM_MIPS_UNALIGNED_H

#include <fikus/compiler.h>
#if defined(__MIPSEB__)
# include <fikus/unaligned/be_struct.h>
# include <fikus/unaligned/le_byteshift.h>
# define get_unaligned	__get_unaligned_be
# define put_unaligned	__put_unaligned_be
#elif defined(__MIPSEL__)
# include <fikus/unaligned/le_struct.h>
# include <fikus/unaligned/be_byteshift.h>
# define get_unaligned	__get_unaligned_le
# define put_unaligned	__put_unaligned_le
#else
#  error "MIPS, but neither __MIPSEB__, nor __MIPSEL__???"
#endif

# include <fikus/unaligned/generic.h>

#endif /* _ASM_MIPS_UNALIGNED_H */
