#ifndef _FIKUS_VIRTIO_RING_H
#define _FIKUS_VIRTIO_RING_H

#include <asm/barrier.h>
#include <fikus/irqreturn.h>
#include <uapi/fikus/virtio_ring.h>

/*
 * Barriers in virtio are tricky.  Non-SMP virtio guests can't assume
 * they're not on an SMP host system, so they need to assume real
 * barriers.  Non-SMP virtio hosts could skip the barriers, but does
 * anyone care?
 *
 * For virtio_pci on SMP, we don't need to order with respect to MMIO
 * accesses through relaxed memory I/O windows, so smp_mb() et al are
 * sufficient.
 *
 * For using virtio to talk to real devices (eg. other heterogeneous
 * CPUs) we do need real barriers.  In theory, we could be using both
 * kinds of virtio, so it's a runtime decision, and the branch is
 * actually quite cheap.
 */

#ifdef CONFIG_SMP
static inline void virtio_mb(bool weak_barriers)
{
	if (weak_barriers)
		smp_mb();
	else
		mb();
}

static inline void virtio_rmb(bool weak_barriers)
{
	if (weak_barriers)
		smp_rmb();
	else
		rmb();
}

static inline void virtio_wmb(bool weak_barriers)
{
	if (weak_barriers)
		smp_wmb();
	else
		wmb();
}
#else
static inline void virtio_mb(bool weak_barriers)
{
	mb();
}

static inline void virtio_rmb(bool weak_barriers)
{
	rmb();
}

static inline void virtio_wmb(bool weak_barriers)
{
	wmb();
}
#endif

struct virtio_device;
struct virtqueue;

struct virtqueue *vring_new_virtqueue(unsigned int index,
				      unsigned int num,
				      unsigned int vring_align,
				      struct virtio_device *vdev,
				      bool weak_barriers,
				      void *pages,
				      void (*notify)(struct virtqueue *vq),
				      void (*callback)(struct virtqueue *vq),
				      const char *name);
void vring_del_virtqueue(struct virtqueue *vq);
/* Filter out transport-specific feature bits. */
void vring_transport_features(struct virtio_device *vdev);

irqreturn_t vring_interrupt(int irq, void *_vq);
#endif /* _FIKUS_VIRTIO_RING_H */
