#ifndef _X25DEVICE_H
#define _X25DEVICE_H

#include <fikus/if_ether.h>
#include <fikus/if_packet.h>
#include <fikus/if_x25.h>
#include <fikus/skbuff.h>

static inline __be16 x25_type_trans(struct sk_buff *skb, struct net_device *dev)
{
	skb->dev = dev;
	skb_reset_mac_header(skb);
	skb->pkt_type = PACKET_HOST;
	
	return htons(ETH_P_X25);
}
#endif
