#ifndef _FIKUS_START_KERNEL_H
#define _FIKUS_START_KERNEL_H

#include <fikus/linkage.h>
#include <fikus/init.h>

/* Define the prototype for start_kernel here, rather than cluttering
   up something else. */

extern asmlinkage void __init start_kernel(void);

#endif /* _FIKUS_START_KERNEL_H */
