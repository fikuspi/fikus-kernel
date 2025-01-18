/* include/fikus/fec.h
 *
 * Copyright (c) 2009 Orex Computed Radiography
 *   Baruch Siach <baruch@tkos.co.il>
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * Header file for the FEC platform data
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __FIKUS_FEC_H__
#define __FIKUS_FEC_H__

#include <fikus/phy.h>

struct fec_platform_data {
	phy_interface_t phy;
	unsigned char mac[ETH_ALEN];
};

#endif
