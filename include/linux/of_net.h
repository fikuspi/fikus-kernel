/*
 * OF helpers for network devices.
 *
 * This file is released under the GPLv2
 */

#ifndef __FIKUS_OF_NET_H
#define __FIKUS_OF_NET_H

#ifdef CONFIG_OF_NET
#include <fikus/of.h>
extern int of_get_phy_mode(struct device_node *np);
extern const void *of_get_mac_address(struct device_node *np);
#else
static inline int of_get_phy_mode(struct device_node *np)
{
	return -ENODEV;
}

static inline const void *of_get_mac_address(struct device_node *np)
{
	return NULL;
}
#endif

#endif /* __FIKUS_OF_NET_H */
