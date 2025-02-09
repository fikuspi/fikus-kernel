/*
 * ip_vs_proto_ah_esp.c:	AH/ESP IPSec load balancing support for IPVS
 *
 * Authors:	Julian Anastasov <ja@ssi.bg>, February 2002
 *		Wensong Zhang <wensong@fikusvirtualserver.org>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		version 2 as published by the Free Software Foundation;
 *
 */

#define KMSG_COMPONENT "IPVS"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#include <fikus/in.h>
#include <fikus/ip.h>
#include <fikus/module.h>
#include <fikus/kernel.h>
#include <fikus/netfilter.h>
#include <fikus/netfilter_ipv4.h>

#include <net/ip_vs.h>


/* TODO:

struct isakmp_hdr {
	__u8		icookie[8];
	__u8		rcookie[8];
	__u8		np;
	__u8		version;
	__u8		xchgtype;
	__u8		flags;
	__u32		msgid;
	__u32		length;
};

*/

#define PORT_ISAKMP	500

static void
ah_esp_conn_fill_param_proto(struct net *net, int af,
			     const struct ip_vs_iphdr *iph, int inverse,
			     struct ip_vs_conn_param *p)
{
	if (likely(!inverse))
		ip_vs_conn_fill_param(net, af, IPPROTO_UDP,
				      &iph->saddr, htons(PORT_ISAKMP),
				      &iph->daddr, htons(PORT_ISAKMP), p);
	else
		ip_vs_conn_fill_param(net, af, IPPROTO_UDP,
				      &iph->daddr, htons(PORT_ISAKMP),
				      &iph->saddr, htons(PORT_ISAKMP), p);
}

static struct ip_vs_conn *
ah_esp_conn_in_get(int af, const struct sk_buff *skb,
		   const struct ip_vs_iphdr *iph,
		   int inverse)
{
	struct ip_vs_conn *cp;
	struct ip_vs_conn_param p;
	struct net *net = skb_net(skb);

	ah_esp_conn_fill_param_proto(net, af, iph, inverse, &p);
	cp = ip_vs_conn_in_get(&p);
	if (!cp) {
		/*
		 * We are not sure if the packet is from our
		 * service, so our conn_schedule hook should return NF_ACCEPT
		 */
		IP_VS_DBG_BUF(12, "Unknown ISAKMP entry for outin packet "
			      "%s%s %s->%s\n",
			      inverse ? "ICMP+" : "",
			      ip_vs_proto_get(iph->protocol)->name,
			      IP_VS_DBG_ADDR(af, &iph->saddr),
			      IP_VS_DBG_ADDR(af, &iph->daddr));
	}

	return cp;
}


static struct ip_vs_conn *
ah_esp_conn_out_get(int af, const struct sk_buff *skb,
		    const struct ip_vs_iphdr *iph, int inverse)
{
	struct ip_vs_conn *cp;
	struct ip_vs_conn_param p;
	struct net *net = skb_net(skb);

	ah_esp_conn_fill_param_proto(net, af, iph, inverse, &p);
	cp = ip_vs_conn_out_get(&p);
	if (!cp) {
		IP_VS_DBG_BUF(12, "Unknown ISAKMP entry for inout packet "
			      "%s%s %s->%s\n",
			      inverse ? "ICMP+" : "",
			      ip_vs_proto_get(iph->protocol)->name,
			      IP_VS_DBG_ADDR(af, &iph->saddr),
			      IP_VS_DBG_ADDR(af, &iph->daddr));
	}

	return cp;
}


static int
ah_esp_conn_schedule(int af, struct sk_buff *skb, struct ip_vs_proto_data *pd,
		     int *verdict, struct ip_vs_conn **cpp,
		     struct ip_vs_iphdr *iph)
{
	/*
	 * AH/ESP is only related traffic. Pass the packet to IP stack.
	 */
	*verdict = NF_ACCEPT;
	return 0;
}

#ifdef CONFIG_IP_VS_PROTO_AH
struct ip_vs_protocol ip_vs_protocol_ah = {
	.name =			"AH",
	.protocol =		IPPROTO_AH,
	.num_states =		1,
	.dont_defrag =		1,
	.init =			NULL,
	.exit =			NULL,
	.conn_schedule =	ah_esp_conn_schedule,
	.conn_in_get =		ah_esp_conn_in_get,
	.conn_out_get =		ah_esp_conn_out_get,
	.snat_handler =		NULL,
	.dnat_handler =		NULL,
	.csum_check =		NULL,
	.state_transition =	NULL,
	.register_app =		NULL,
	.unregister_app =	NULL,
	.app_conn_bind =	NULL,
	.debug_packet =		ip_vs_tcpudp_debug_packet,
	.timeout_change =	NULL,		/* ISAKMP */
};
#endif

#ifdef CONFIG_IP_VS_PROTO_ESP
struct ip_vs_protocol ip_vs_protocol_esp = {
	.name =			"ESP",
	.protocol =		IPPROTO_ESP,
	.num_states =		1,
	.dont_defrag =		1,
	.init =			NULL,
	.exit =			NULL,
	.conn_schedule =	ah_esp_conn_schedule,
	.conn_in_get =		ah_esp_conn_in_get,
	.conn_out_get =		ah_esp_conn_out_get,
	.snat_handler =		NULL,
	.dnat_handler =		NULL,
	.csum_check =		NULL,
	.state_transition =	NULL,
	.register_app =		NULL,
	.unregister_app =	NULL,
	.app_conn_bind =	NULL,
	.debug_packet =		ip_vs_tcpudp_debug_packet,
	.timeout_change =	NULL,		/* ISAKMP */
};
#endif
