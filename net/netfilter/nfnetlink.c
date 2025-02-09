/* Netfilter messages via netlink socket. Allows for user space
 * protocol helpers and general trouble making from userspace.
 *
 * (C) 2001 by Jay Schulist <jschlst@samba.org>,
 * (C) 2002-2005 by Harald Welte <laforge@gnumonks.org>
 * (C) 2005,2007 by Pablo Neira Ayuso <pablo@netfilter.org>
 *
 * Initial netfilter messages via netlink development funded and
 * generally made possible by Network Robots, Inc. (www.networkrobots.com)
 *
 * Further development of this code funded by Astaro AG (http://www.astaro.com)
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#include <fikus/module.h>
#include <fikus/types.h>
#include <fikus/socket.h>
#include <fikus/kernel.h>
#include <fikus/string.h>
#include <fikus/sockios.h>
#include <fikus/net.h>
#include <fikus/skbuff.h>
#include <asm/uaccess.h>
#include <net/sock.h>
#include <fikus/init.h>

#include <net/netlink.h>
#include <fikus/netfilter/nfnetlink.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harald Welte <laforge@netfilter.org>");
MODULE_ALIAS_NET_PF_PROTO(PF_NETLINK, NETLINK_NETFILTER);

static char __initdata nfversion[] = "0.30";

static struct {
	struct mutex				mutex;
	const struct nfnetlink_subsystem __rcu	*subsys;
} table[NFNL_SUBSYS_COUNT];

static const int nfnl_group2type[NFNLGRP_MAX+1] = {
	[NFNLGRP_CONNTRACK_NEW]		= NFNL_SUBSYS_CTNETLINK,
	[NFNLGRP_CONNTRACK_UPDATE]	= NFNL_SUBSYS_CTNETLINK,
	[NFNLGRP_CONNTRACK_DESTROY]	= NFNL_SUBSYS_CTNETLINK,
	[NFNLGRP_CONNTRACK_EXP_NEW]	= NFNL_SUBSYS_CTNETLINK_EXP,
	[NFNLGRP_CONNTRACK_EXP_UPDATE]	= NFNL_SUBSYS_CTNETLINK_EXP,
	[NFNLGRP_CONNTRACK_EXP_DESTROY] = NFNL_SUBSYS_CTNETLINK_EXP,
};

void nfnl_lock(__u8 subsys_id)
{
	mutex_lock(&table[subsys_id].mutex);
}
EXPORT_SYMBOL_GPL(nfnl_lock);

void nfnl_unlock(__u8 subsys_id)
{
	mutex_unlock(&table[subsys_id].mutex);
}
EXPORT_SYMBOL_GPL(nfnl_unlock);

int nfnetlink_subsys_register(const struct nfnetlink_subsystem *n)
{
	nfnl_lock(n->subsys_id);
	if (table[n->subsys_id].subsys) {
		nfnl_unlock(n->subsys_id);
		return -EBUSY;
	}
	rcu_assign_pointer(table[n->subsys_id].subsys, n);
	nfnl_unlock(n->subsys_id);

	return 0;
}
EXPORT_SYMBOL_GPL(nfnetlink_subsys_register);

int nfnetlink_subsys_unregister(const struct nfnetlink_subsystem *n)
{
	nfnl_lock(n->subsys_id);
	table[n->subsys_id].subsys = NULL;
	nfnl_unlock(n->subsys_id);
	synchronize_rcu();
	return 0;
}
EXPORT_SYMBOL_GPL(nfnetlink_subsys_unregister);

static inline const struct nfnetlink_subsystem *nfnetlink_get_subsys(u_int16_t type)
{
	u_int8_t subsys_id = NFNL_SUBSYS_ID(type);

	if (subsys_id >= NFNL_SUBSYS_COUNT)
		return NULL;

	return rcu_dereference(table[subsys_id].subsys);
}

static inline const struct nfnl_callback *
nfnetlink_find_client(u_int16_t type, const struct nfnetlink_subsystem *ss)
{
	u_int8_t cb_id = NFNL_MSG_TYPE(type);

	if (cb_id >= ss->cb_count)
		return NULL;

	return &ss->cb[cb_id];
}

int nfnetlink_has_listeners(struct net *net, unsigned int group)
{
	return netlink_has_listeners(net->nfnl, group);
}
EXPORT_SYMBOL_GPL(nfnetlink_has_listeners);

struct sk_buff *nfnetlink_alloc_skb(struct net *net, unsigned int size,
				    u32 dst_portid, gfp_t gfp_mask)
{
	return netlink_alloc_skb(net->nfnl, size, dst_portid, gfp_mask);
}
EXPORT_SYMBOL_GPL(nfnetlink_alloc_skb);

int nfnetlink_send(struct sk_buff *skb, struct net *net, u32 portid,
		   unsigned int group, int echo, gfp_t flags)
{
	return nlmsg_notify(net->nfnl, skb, portid, group, echo, flags);
}
EXPORT_SYMBOL_GPL(nfnetlink_send);

int nfnetlink_set_err(struct net *net, u32 portid, u32 group, int error)
{
	return netlink_set_err(net->nfnl, portid, group, error);
}
EXPORT_SYMBOL_GPL(nfnetlink_set_err);

int nfnetlink_unicast(struct sk_buff *skb, struct net *net, u32 portid,
		      int flags)
{
	return netlink_unicast(net->nfnl, skb, portid, flags);
}
EXPORT_SYMBOL_GPL(nfnetlink_unicast);

/* Process one complete nfnetlink message. */
static int nfnetlink_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh)
{
	struct net *net = sock_net(skb->sk);
	const struct nfnl_callback *nc;
	const struct nfnetlink_subsystem *ss;
	int type, err;

	if (!ns_capable(net->user_ns, CAP_NET_ADMIN))
		return -EPERM;

	/* All the messages must at least contain nfgenmsg */
	if (nlmsg_len(nlh) < sizeof(struct nfgenmsg))
		return 0;

	type = nlh->nlmsg_type;
replay:
	rcu_read_lock();
	ss = nfnetlink_get_subsys(type);
	if (!ss) {
#ifdef CONFIG_MODULES
		rcu_read_unlock();
		request_module("nfnetlink-subsys-%d", NFNL_SUBSYS_ID(type));
		rcu_read_lock();
		ss = nfnetlink_get_subsys(type);
		if (!ss)
#endif
		{
			rcu_read_unlock();
			return -EINVAL;
		}
	}

	nc = nfnetlink_find_client(type, ss);
	if (!nc) {
		rcu_read_unlock();
		return -EINVAL;
	}

	{
		int min_len = nlmsg_total_size(sizeof(struct nfgenmsg));
		u_int8_t cb_id = NFNL_MSG_TYPE(nlh->nlmsg_type);
		struct nlattr *cda[ss->cb[cb_id].attr_count + 1];
		struct nlattr *attr = (void *)nlh + min_len;
		int attrlen = nlh->nlmsg_len - min_len;
		__u8 subsys_id = NFNL_SUBSYS_ID(type);

		err = nla_parse(cda, ss->cb[cb_id].attr_count,
				attr, attrlen, ss->cb[cb_id].policy);
		if (err < 0) {
			rcu_read_unlock();
			return err;
		}

		if (nc->call_rcu) {
			err = nc->call_rcu(net->nfnl, skb, nlh,
					   (const struct nlattr **)cda);
			rcu_read_unlock();
		} else {
			rcu_read_unlock();
			nfnl_lock(subsys_id);
			if (rcu_dereference_protected(table[subsys_id].subsys,
				lockdep_is_held(&table[subsys_id].mutex)) != ss ||
			    nfnetlink_find_client(type, ss) != nc)
				err = -EAGAIN;
			else if (nc->call)
				err = nc->call(net->nfnl, skb, nlh,
						   (const struct nlattr **)cda);
			else
				err = -EINVAL;
			nfnl_unlock(subsys_id);
		}
		if (err == -EAGAIN)
			goto replay;
		return err;
	}
}

static void nfnetlink_rcv(struct sk_buff *skb)
{
	netlink_rcv_skb(skb, &nfnetlink_rcv_msg);
}

#ifdef CONFIG_MODULES
static void nfnetlink_bind(int group)
{
	const struct nfnetlink_subsystem *ss;
	int type = nfnl_group2type[group];

	rcu_read_lock();
	ss = nfnetlink_get_subsys(type);
	if (!ss) {
		rcu_read_unlock();
		request_module("nfnetlink-subsys-%d", type);
		return;
	}
	rcu_read_unlock();
}
#endif

static int __net_init nfnetlink_net_init(struct net *net)
{
	struct sock *nfnl;
	struct netlink_kernel_cfg cfg = {
		.groups	= NFNLGRP_MAX,
		.input	= nfnetlink_rcv,
#ifdef CONFIG_MODULES
		.bind	= nfnetlink_bind,
#endif
	};

	nfnl = netlink_kernel_create(net, NETLINK_NETFILTER, &cfg);
	if (!nfnl)
		return -ENOMEM;
	net->nfnl_stash = nfnl;
	rcu_assign_pointer(net->nfnl, nfnl);
	return 0;
}

static void __net_exit nfnetlink_net_exit_batch(struct list_head *net_exit_list)
{
	struct net *net;

	list_for_each_entry(net, net_exit_list, exit_list)
		RCU_INIT_POINTER(net->nfnl, NULL);
	synchronize_net();
	list_for_each_entry(net, net_exit_list, exit_list)
		netlink_kernel_release(net->nfnl_stash);
}

static struct pernet_operations nfnetlink_net_ops = {
	.init		= nfnetlink_net_init,
	.exit_batch	= nfnetlink_net_exit_batch,
};

static int __init nfnetlink_init(void)
{
	int i;

	for (i=0; i<NFNL_SUBSYS_COUNT; i++)
		mutex_init(&table[i].mutex);

	pr_info("Netfilter messages via NETLINK v%s.\n", nfversion);
	return register_pernet_subsys(&nfnetlink_net_ops);
}

static void __exit nfnetlink_exit(void)
{
	pr_info("Removing netfilter NETLINK layer.\n");
	unregister_pernet_subsys(&nfnetlink_net_ops);
}
module_init(nfnetlink_init);
module_exit(nfnetlink_exit);
