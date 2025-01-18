#ifndef __FIKUS_BRIDGE_EBT_802_3_H
#define __FIKUS_BRIDGE_EBT_802_3_H

#include <fikus/skbuff.h>
#include <uapi/fikus/netfilter_bridge/ebt_802_3.h>

static inline struct ebt_802_3_hdr *ebt_802_3_hdr(const struct sk_buff *skb)
{
	return (struct ebt_802_3_hdr *)skb_mac_header(skb);
}
#endif
