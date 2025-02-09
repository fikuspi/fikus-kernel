#ifndef __FS_CEPH_BUFFER_H
#define __FS_CEPH_BUFFER_H

#include <fikus/kref.h>
#include <fikus/mm.h>
#include <fikus/vmalloc.h>
#include <fikus/types.h>
#include <fikus/uio.h>

/*
 * a simple reference counted buffer.
 *
 * use kmalloc for small sizes (<= one page), vmalloc for larger
 * sizes.
 */
struct ceph_buffer {
	struct kref kref;
	struct kvec vec;
	size_t alloc_len;
	bool is_vmalloc;
};

extern struct ceph_buffer *ceph_buffer_new(size_t len, gfp_t gfp);
extern void ceph_buffer_release(struct kref *kref);

static inline struct ceph_buffer *ceph_buffer_get(struct ceph_buffer *b)
{
	kref_get(&b->kref);
	return b;
}

static inline void ceph_buffer_put(struct ceph_buffer *b)
{
	kref_put(&b->kref, ceph_buffer_release);
}

extern int ceph_decode_buffer(struct ceph_buffer **b, void **p, void *end);

#endif
