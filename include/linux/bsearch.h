#ifndef _FIKUS_BSEARCH_H
#define _FIKUS_BSEARCH_H

#include <fikus/types.h>

void *bsearch(const void *key, const void *base, size_t num, size_t size,
	      int (*cmp)(const void *key, const void *elt));

#endif /* _FIKUS_BSEARCH_H */
