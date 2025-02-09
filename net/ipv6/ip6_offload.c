/*
 *	IPV6 GSO/GRO offload support
 *	Fikus INET6 implementation
 *
 *	This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */

#include <fikus/kernel.h>
#include <fikus/socket.h>
#include <fikus/netdevice.h>
#include <fikus/skbuff.h>
#include <fikus/printk.h>

#include <net/protocol.h>
#include <net/ipv6.h>

#include "ip6_offload.h"

static int ipv6_gso_pull_exthdrs(struct sk_buff *skb, int proto)
{
	const struct net_offload *ops = NULL;

	for (;;) {
		struct ipv6_opt_hdr *opth;
		int len;

		if (proto != NEXTHDR_HOP) {
			ops = rcu_dereference(inet6_offloads[proto]);

			if (unlikely(!ops))
				break;

			if (!(ops->flags & INET6_PROTO_GSO_EXTHDR))
				break;
		}

		if (unlikely(!pskb_may_pull(skb, 8)))
			break;

		opth = (void *)skb->data;
		len = ipv6_optlen(opth);

		if (unlikely(!pskb_may_pull(skb, len)))
			break;

		proto = opth->nexthdr;
		__skb_pull(skb, len);
	}

	return proto;
}

static int ipv6_gso_send_check(struct sk_buff *skb)
{
	const struct ipv6hdr *ipv6h;
	const struct net_offload *ops;
	int err = -EINVAL;

	if (unlikely(!pskb_may_pull(skb, sizeof(*ipv6h))))
		goto out;

	ipv6h = ipv6_hdr(skb);
	__skb_pull(skb, sizeof(*ipv6h));
	err = -EPROTONOSUPPORT;

	rcu_read_lock();
	ops = rcu_dereference(inet6_offloads[
		ipv6_gso_pull_exthdrs(skb, ipv6h->nexthdr)]);

	if (likely(ops && ops->callbacks.gso_send_check)) {
		skb_reset_transport_header(skb);
		err = ops->callbacks.gso_send_check(skb);
	}
	rcu_read_unlock();

out:
	return err;
}

static struct sk_buff *ipv6_gso_segment(struct sk_buff *skb,
	netdev_features_t features)
{
	struct sk_buff *segs = ERR_PTR(-EINVAL);
	struct ipv6hdr *ipv6h;
	const struct net_offload *ops;
	int proto;
	struct frag_hdr *fptr;
	unsigned int unfrag_ip6hlen;
	u8 *prevhdr;
	int offset = 0;
	bool tunnel;

	if (unlikely(skb_shinfo(skb)->gso_type &
		     ~(SKB_GSO_UDP |
		       SKB_GSO_DODGY |
		       SKB_GSO_TCP_ECN |
		       SKB_GSO_GRE |
		       SKB_GSO_UDP_TUNNEL |
		       SKB_GSO_MPLS |
		       SKB_GSO_TCPV6 |
		       0)))
		goto out;

	if (unlikely(!pskb_may_pull(skb, sizeof(*ipv6h))))
		goto out;

	tunnel = skb->encapsulation;
	ipv6h = ipv6_hdr(skb);
	__skb_pull(skb, sizeof(*ipv6h));
	segs = ERR_PTR(-EPROTONOSUPPORT);

	proto = ipv6_gso_pull_exthdrs(skb, ipv6h->nexthdr);
	rcu_read_lock();
	ops = rcu_dereference(inet6_offloads[proto]);
	if (likely(ops && ops->callbacks.gso_segment)) {
		skb_reset_transport_header(skb);
		segs = ops->callbacks.gso_segment(skb, features);
	}
	rcu_read_unlock();

	if (IS_ERR(segs))
		goto out;

	for (skb = segs; skb; skb = skb->next) {
		ipv6h = ipv6_hdr(skb);
		ipv6h->payload_len = htons(skb->len - skb->mac_len -
					   sizeof(*ipv6h));
		if (!tunnel && proto == IPPROTO_UDP) {
			unfrag_ip6hlen = ip6_find_1stfragopt(skb, &prevhdr);
			fptr = (struct frag_hdr *)(skb_network_header(skb) +
				unfrag_ip6hlen);
			fptr->frag_off = htons(offset);
			if (skb->next != NULL)
				fptr->frag_off |= htons(IP6_MF);
			offset += (ntohs(ipv6h->payload_len) -
				   sizeof(struct frag_hdr));
		}
	}

out:
	return segs;
}

static struct sk_buff **ipv6_gro_receive(struct sk_buff **head,
					 struct sk_buff *skb)
{
	const struct net_offload *ops;
	struct sk_buff **pp = NULL;
	struct sk_buff *p;
	struct ipv6hdr *iph;
	unsigned int nlen;
	unsigned int hlen;
	unsigned int off;
	int flush = 1;
	int proto;
	__wsum csum;

	off = skb_gro_offset(skb);
	hlen = off + sizeof(*iph);
	iph = skb_gro_header_fast(skb, off);
	if (skb_gro_header_hard(skb, hlen)) {
		iph = skb_gro_header_slow(skb, hlen, off);
		if (unlikely(!iph))
			goto out;
	}

	skb_gro_pull(skb, sizeof(*iph));
	skb_set_transport_header(skb, skb_gro_offset(skb));

	flush += ntohs(iph->payload_len) != skb_gro_len(skb);

	rcu_read_lock();
	proto = iph->nexthdr;
	ops = rcu_dereference(inet6_offloads[proto]);
	if (!ops || !ops->callbacks.gro_receive) {
		__pskb_pull(skb, skb_gro_offset(skb));
		proto = ipv6_gso_pull_exthdrs(skb, proto);
		skb_gro_pull(skb, -skb_transport_offset(skb));
		skb_reset_transport_header(skb);
		__skb_push(skb, skb_gro_offset(skb));

		ops = rcu_dereference(inet6_offloads[proto]);
		if (!ops || !ops->callbacks.gro_receive)
			goto out_unlock;

		iph = ipv6_hdr(skb);
	}

	NAPI_GRO_CB(skb)->proto = proto;

	flush--;
	nlen = skb_network_header_len(skb);

	for (p = *head; p; p = p->next) {
		const struct ipv6hdr *iph2;
		__be32 first_word; /* <Version:4><Traffic_Class:8><Flow_Label:20> */

		if (!NAPI_GRO_CB(p)->same_flow)
			continue;

		iph2 = ipv6_hdr(p);
		first_word = *(__be32 *)iph ^ *(__be32 *)iph2 ;

		/* All fields must match except length and Traffic Class. */
		if (nlen != skb_network_header_len(p) ||
		    (first_word & htonl(0xF00FFFFF)) ||
		    memcmp(&iph->nexthdr, &iph2->nexthdr,
			   nlen - offsetof(struct ipv6hdr, nexthdr))) {
			NAPI_GRO_CB(p)->same_flow = 0;
			continue;
		}
		/* flush if Traffic Class fields are different */
		NAPI_GRO_CB(p)->flush |= !!(first_word & htonl(0x0FF00000));
		NAPI_GRO_CB(p)->flush |= flush;
	}

	NAPI_GRO_CB(skb)->flush |= flush;

	csum = skb->csum;
	skb_postpull_rcsum(skb, iph, skb_network_header_len(skb));

	pp = ops->callbacks.gro_receive(head, skb);

	skb->csum = csum;

out_unlock:
	rcu_read_unlock();

out:
	NAPI_GRO_CB(skb)->flush |= flush;

	return pp;
}

static int ipv6_gro_complete(struct sk_buff *skb)
{
	const struct net_offload *ops;
	struct ipv6hdr *iph = ipv6_hdr(skb);
	int err = -ENOSYS;

	iph->payload_len = htons(skb->len - skb_network_offset(skb) -
				 sizeof(*iph));

	rcu_read_lock();
	ops = rcu_dereference(inet6_offloads[NAPI_GRO_CB(skb)->proto]);
	if (WARN_ON(!ops || !ops->callbacks.gro_complete))
		goto out_unlock;

	err = ops->callbacks.gro_complete(skb);

out_unlock:
	rcu_read_unlock();

	return err;
}

static struct packet_offload ipv6_packet_offload __read_mostly = {
	.type = cpu_to_be16(ETH_P_IPV6),
	.callbacks = {
		.gso_send_check = ipv6_gso_send_check,
		.gso_segment = ipv6_gso_segment,
		.gro_receive = ipv6_gro_receive,
		.gro_complete = ipv6_gro_complete,
	},
};

static int __init ipv6_offload_init(void)
{

	if (tcpv6_offload_init() < 0)
		pr_crit("%s: Cannot add TCP protocol offload\n", __func__);
	if (udp_offload_init() < 0)
		pr_crit("%s: Cannot add UDP protocol offload\n", __func__);
	if (ipv6_exthdrs_offload_init() < 0)
		pr_crit("%s: Cannot add EXTHDRS protocol offload\n", __func__);

	dev_add_offload(&ipv6_packet_offload);
	return 0;
}

fs_initcall(ipv6_offload_init);
