/*
 * arch/arm/plat-orion/include/plat/mv_xor.h
 *
 * Marvell XOR platform device data definition file.
 */

#ifndef __PLAT_MV_XOR_H
#define __PLAT_MV_XOR_H

#include <fikus/dmaengine.h>
#include <fikus/mbus.h>

#define MV_XOR_NAME	"mv_xor"

struct mv_xor_channel_data {
	dma_cap_mask_t			cap_mask;
};

struct mv_xor_platform_data {
	struct mv_xor_channel_data    *channels;
};

#endif
