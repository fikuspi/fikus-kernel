#include <fikus/types.h>
#include <fikus/init.h>
#include <fikus/slab.h>
#include <fikus/bootmem.h>
#include <fikus/string.h>
#include <asm/setup.h>


void * __init_refok zalloc_maybe_bootmem(size_t size, gfp_t mask)
{
	void *p;

	if (mem_init_done)
		p = kzalloc(size, mask);
	else {
		p = alloc_bootmem(size);
		if (p)
			memset(p, 0, size);
	}
	return p;
}
