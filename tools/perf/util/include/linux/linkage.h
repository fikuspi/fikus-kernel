
#ifndef PERF_FIKUS_LINKAGE_H_
#define PERF_FIKUS_LINKAGE_H_

/* linkage.h ... for including arch/x86/lib/memcpy_64.S */

#define ENTRY(name)				\
	.globl name;				\
	name:

#define ENDPROC(name)

#endif	/* PERF_FIKUS_LINKAGE_H_ */
