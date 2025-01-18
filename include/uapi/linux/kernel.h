#ifndef _UAPI_FIKUS_KERNEL_H
#define _UAPI_FIKUS_KERNEL_H

#include <fikus/sysinfo.h>

/*
 * 'kernel.h' contains some often-used function prototypes etc
 */
#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))


#endif /* _UAPI_FIKUS_KERNEL_H */
