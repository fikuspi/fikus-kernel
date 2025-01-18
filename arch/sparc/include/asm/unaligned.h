#ifndef _ASM_SPARC_UNALIGNED_H
#define _ASM_SPARC_UNALIGNED_H

#include <fikus/unaligned/be_struct.h>
#include <fikus/unaligned/le_byteshift.h>
#include <fikus/unaligned/generic.h>
#define get_unaligned	__get_unaligned_be
#define put_unaligned	__put_unaligned_be

#endif /* _ASM_SPARC_UNALIGNED_H */
