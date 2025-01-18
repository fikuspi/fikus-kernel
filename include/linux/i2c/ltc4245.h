/*
 * Platform Data for LTC4245 hardware monitor chip
 *
 * Copyright (c) 2010 Ira W. Snyder <iws@ovro.caltech.edu>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef FIKUS_LTC4245_H
#define FIKUS_LTC4245_H

#include <fikus/types.h>

struct ltc4245_platform_data {
	bool use_extra_gpios;
};

#endif /* FIKUS_LTC4245_H */
