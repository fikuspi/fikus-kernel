#ifndef _ASM_IA64_UNALIGNED_H
#define _ASM_IA64_UNALIGNED_H

#include <fikus/unaligned/le_struct.h>
#include <fikus/unaligned/be_byteshift.h>
#include <fikus/unaligned/generic.h>

#define get_unaligned	__get_unaligned_le
#define put_unaligned	__put_unaligned_le

#endif /* _ASM_IA64_UNALIGNED_H */
