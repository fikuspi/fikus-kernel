/*
 * ppp_defs.h - PPP definitions.
 *
 * Copyright 1994-2000 Paul Mackerras.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */
#ifndef _PPP_DEFS_H_
#define _PPP_DEFS_H_

#include <fikus/crc-ccitt.h>
#include <uapi/fikus/ppp_defs.h>

#define PPP_FCS(fcs, c) crc_ccitt_byte(fcs, c)
#endif /* _PPP_DEFS_H_ */
