/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/types.h>
#include <fikus/init.h>
#include <fikus/export.h>
#include <fikus/ip.h>
#include <fikus/icmp.h>

#include <fikus/netfilter.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_core.h>
#include <net/netfilter/nf_nat_l4proto.h>

static bool
icmp_in_range(const struct nf_conntrack_tuple *tuple,
	      enum nf_nat_manip_type maniptype,
	      const union nf_conntrack_man_proto *min,
	      const union nf_conntrack_man_proto *max)
{
	return ntohs(tuple->src.u.icmp.id) >= ntohs(min->icmp.id) &&
	       ntohs(tuple->src.u.icmp.id) <= ntohs(max->icmp.id);
}

static void
icmp_unique_tuple(const struct nf_nat_l3proto *l3proto,
		  struct nf_conntrack_tuple *tuple,
		  const struct nf_nat_range *range,
		  enum nf_nat_manip_type maniptype,
		  const struct nf_conn *ct)
{
	static u_int16_t id;
	unsigned int range_size;
	unsigned int i;

	range_size = ntohs(range->max_proto.icmp.id) -
		     ntohs(range->min_proto.icmp.id) + 1;
	/* If no range specified... */
	if (!(range->flags & NF_NAT_RANGE_PROTO_SPECIFIED))
		range_size = 0xFFFF;

	for (i = 0; ; ++id) {
		tuple->src.u.icmp.id = htons(ntohs(range->min_proto.icmp.id) +
					     (id % range_size));
		if (++i == range_size || !nf_nat_used_tuple(tuple, ct))
			return;
	}
	return;
}

static bool
icmp_manip_pkt(struct sk_buff *skb,
	       const struct nf_nat_l3proto *l3proto,
	       unsigned int iphdroff, unsigned int hdroff,
	       const struct nf_conntrack_tuple *tuple,
	       enum nf_nat_manip_type maniptype)
{
	struct icmphdr *hdr;

	if (!skb_make_writable(skb, hdroff + sizeof(*hdr)))
		return false;

	hdr = (struct icmphdr *)(skb->data + hdroff);
	inet_proto_csum_replace2(&hdr->checksum, skb,
				 hdr->un.echo.id, tuple->src.u.icmp.id, 0);
	hdr->un.echo.id = tuple->src.u.icmp.id;
	return true;
}

const struct nf_nat_l4proto nf_nat_l4proto_icmp = {
	.l4proto		= IPPROTO_ICMP,
	.manip_pkt		= icmp_manip_pkt,
	.in_range		= icmp_in_range,
	.unique_tuple		= icmp_unique_tuple,
#if defined(CONFIG_NF_CT_NETLINK) || defined(CONFIG_NF_CT_NETLINK_MODULE)
	.nlattr_to_range	= nf_nat_l4proto_nlattr_to_range,
#endif
};
