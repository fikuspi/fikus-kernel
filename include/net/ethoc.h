/*
 * fikus/include/net/ethoc.h
 *
 * Copyright (C) 2008-2009 Avionic Design GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Written by Thierry Reding <thierry.reding@avionic-design.de>
 */

#ifndef FIKUS_NET_ETHOC_H
#define FIKUS_NET_ETHOC_H 1

struct ethoc_platform_data {
	u8 hwaddr[IFHWADDRLEN];
	s8 phy_id;
};

#endif /* !FIKUS_NET_ETHOC_H */

