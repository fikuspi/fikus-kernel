#ifndef _PERF_FIKUS_TYPES_H_
#define _PERF_FIKUS_TYPES_H_

#include <asm/types.h>

#ifndef __bitwise
#define __bitwise
#endif

#ifndef __le32
typedef __u32 __bitwise __le32;
#endif

#define DECLARE_BITMAP(name,bits) \
	unsigned long name[BITS_TO_LONGS(bits)]

struct list_head {
	struct list_head *next, *prev;
};

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

#endif
