/* sonet.h - SONET/SHD physical layer control */
#ifndef FIKUS_SONET_H
#define FIKUS_SONET_H


#include <fikus/atomic.h>
#include <uapi/fikus/sonet.h>

struct k_sonet_stats {
#define __HANDLE_ITEM(i) atomic_t i
	__SONET_ITEMS
#undef __HANDLE_ITEM
};

extern void sonet_copy_stats(struct k_sonet_stats *from,struct sonet_stats *to);
extern void sonet_subtract_stats(struct k_sonet_stats *from,
    struct sonet_stats *to);

#endif
