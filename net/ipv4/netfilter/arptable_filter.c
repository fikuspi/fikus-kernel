/*
 * Filtering ARP tables module.
 *
 * Copyright (C) 2002 David S. Miller (davem@redhat.com)
 *
 */

#include <fikus/module.h>
#include <fikus/netfilter/x_tables.h>
#include <fikus/netfilter_arp/arp_tables.h>
#include <fikus/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David S. Miller <davem@redhat.com>");
MODULE_DESCRIPTION("arptables filter table");

#define FILTER_VALID_HOOKS ((1 << NF_ARP_IN) | (1 << NF_ARP_OUT) | \
			   (1 << NF_ARP_FORWARD))

static const struct xt_table packet_filter = {
	.name		= "filter",
	.valid_hooks	= FILTER_VALID_HOOKS,
	.me		= THIS_MODULE,
	.af		= NFPROTO_ARP,
	.priority	= NF_IP_PRI_FILTER,
};

/* The work comes in here from netfilter.c */
static unsigned int
arptable_filter_hook(unsigned int hook, struct sk_buff *skb,
		     const struct net_device *in, const struct net_device *out,
		     int (*okfn)(struct sk_buff *))
{
	const struct net *net = dev_net((in != NULL) ? in : out);

	return arpt_do_table(skb, hook, in, out, net->ipv4.arptable_filter);
}

static struct nf_hook_ops *arpfilter_ops __read_mostly;

static int __net_init arptable_filter_net_init(struct net *net)
{
	struct arpt_replace *repl;
	
	repl = arpt_alloc_initial_table(&packet_filter);
	if (repl == NULL)
		return -ENOMEM;
	net->ipv4.arptable_filter =
		arpt_register_table(net, &packet_filter, repl);
	kfree(repl);
	return PTR_ERR_OR_ZERO(net->ipv4.arptable_filter);
}

static void __net_exit arptable_filter_net_exit(struct net *net)
{
	arpt_unregister_table(net->ipv4.arptable_filter);
}

static struct pernet_operations arptable_filter_net_ops = {
	.init = arptable_filter_net_init,
	.exit = arptable_filter_net_exit,
};

static int __init arptable_filter_init(void)
{
	int ret;

	ret = register_pernet_subsys(&arptable_filter_net_ops);
	if (ret < 0)
		return ret;

	arpfilter_ops = xt_hook_link(&packet_filter, arptable_filter_hook);
	if (IS_ERR(arpfilter_ops)) {
		ret = PTR_ERR(arpfilter_ops);
		goto cleanup_table;
	}
	return ret;

cleanup_table:
	unregister_pernet_subsys(&arptable_filter_net_ops);
	return ret;
}

static void __exit arptable_filter_fini(void)
{
	xt_hook_unlink(&packet_filter, arpfilter_ops);
	unregister_pernet_subsys(&arptable_filter_net_ops);
}

module_init(arptable_filter_init);
module_exit(arptable_filter_fini);
