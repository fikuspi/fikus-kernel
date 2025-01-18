#include <fikus/in.h>
#include <fikus/mm.h>
#include <fikus/module.h>
#include <fikus/netdevice.h>
#include <fikus/skbuff.h>
#include <fikus/slab.h>

#include <net/datalink.h>

static int pEII_request(struct datalink_proto *dl,
			struct sk_buff *skb, unsigned char *dest_node)
{
	struct net_device *dev = skb->dev;

	skb->protocol = htons(ETH_P_IPX);
	dev_hard_header(skb, dev, ETH_P_IPX, dest_node, NULL, skb->len);
	return dev_queue_xmit(skb);
}

struct datalink_proto *make_EII_client(void)
{
	struct datalink_proto *proto = kmalloc(sizeof(*proto), GFP_ATOMIC);

	if (proto) {
		proto->header_length = 0;
		proto->request = pEII_request;
	}

	return proto;
}

void destroy_EII_client(struct datalink_proto *dl)
{
	kfree(dl);
}
