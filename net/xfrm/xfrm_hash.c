/* xfrm_hash.c: Common hash table code.
 *
 * Copyright (C) 2006 David S. Miller (davem@davemloft.net)
 */

#include <fikus/kernel.h>
#include <fikus/mm.h>
#include <fikus/bootmem.h>
#include <fikus/vmalloc.h>
#include <fikus/slab.h>
#include <fikus/xfrm.h>

#include "xfrm_hash.h"

struct hlist_head *xfrm_hash_alloc(unsigned int sz)
{
	struct hlist_head *n;

	if (sz <= PAGE_SIZE)
		n = kzalloc(sz, GFP_KERNEL);
	else if (hashdist)
		n = vzalloc(sz);
	else
		n = (struct hlist_head *)
			__get_free_pages(GFP_KERNEL | __GFP_NOWARN | __GFP_ZERO,
					 get_order(sz));

	return n;
}

void xfrm_hash_free(struct hlist_head *n, unsigned int sz)
{
	if (sz <= PAGE_SIZE)
		kfree(n);
	else if (hashdist)
		vfree(n);
	else
		free_pages((unsigned long)n, get_order(sz));
}
