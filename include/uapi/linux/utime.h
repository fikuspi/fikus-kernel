#ifndef _FIKUS_UTIME_H
#define _FIKUS_UTIME_H

#include <fikus/types.h>

struct utimbuf {
	__kernel_time_t actime;
	__kernel_time_t modtime;
};

#endif
