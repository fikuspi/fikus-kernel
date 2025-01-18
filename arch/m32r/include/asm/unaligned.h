#ifndef _ASM_M32R_UNALIGNED_H
#define _ASM_M32R_UNALIGNED_H

#if defined(__LITTLE_ENDIAN__)
# include <fikus/unaligned/le_memmove.h>
# include <fikus/unaligned/be_byteshift.h>
# include <fikus/unaligned/generic.h>
# define get_unaligned	__get_unaligned_le
# define put_unaligned	__put_unaligned_le
#else
# include <fikus/unaligned/be_memmove.h>
# include <fikus/unaligned/le_byteshift.h>
# include <fikus/unaligned/generic.h>
# define get_unaligned	__get_unaligned_be
# define put_unaligned	__put_unaligned_be
#endif

#endif /* _ASM_M32R_UNALIGNED_H */
