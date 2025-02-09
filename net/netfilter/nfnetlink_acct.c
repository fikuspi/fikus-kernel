/*
 * (C) 2011 Pablo Neira Ayuso <pablo@netfilter.org>
 * (C) 2011 Intra2net AG <http://www.intra2net.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation (or any later at your option).
 */
#include <fikus/init.h>
#include <fikus/module.h>
#include <fikus/kernel.h>
#include <fikus/skbuff.h>
#include <fikus/atomic.h>
#include <fikus/netlink.h>
#include <fikus/rculist.h>
#include <fikus/slab.h>
#include <fikus/types.h>
#include <fikus/errno.h>
#include <net/netlink.h>
#include <net/sock.h>

#include <fikus/netfilter.h>
#include <fikus/netfilter/nfnetlink.h>
#include <fikus/netfilter/nfnetlink_acct.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pablo Neira Ayuso <pablo@netfilter.org>");
MODULE_DESCRIPTION("nfacct: Extended Netfilter accounting infrastructure");

static LIST_HEAD(nfnl_acct_list);

struct nf_acct {
	atomic64_t		pkts;
	atomic64_t		bytes;
	struct list_head	head;
	atomic_t		refcnt;
	char			name[NFACCT_NAME_MAX];
	struct rcu_head		rcu_head;
};

static int
nfnl_acct_new(struct sock *nfnl, struct sk_buff *skb,
	     const struct nlmsghdr *nlh, const struct nlattr * const tb[])
{
	struct nf_acct *nfacct, *matching = NULL;
	char *acct_name;

	if (!tb[NFACCT_NAME])
		return -EINVAL;

	acct_name = nla_data(tb[NFACCT_NAME]);
	if (strlen(acct_name) == 0)
		return -EINVAL;

	list_for_each_entry(nfacct, &nfnl_acct_list, head) {
		if (strncmp(nfacct->name, acct_name, NFACCT_NAME_MAX) != 0)
			continue;

                if (nlh->nlmsg_flags & NLM_F_EXCL)
			return -EEXIST;

		matching = nfacct;
		break;
        }

	if (matching) {
		if (nlh->nlmsg_flags & NLM_F_REPLACE) {
			/* reset counters if you request a replacement. */
			atomic64_set(&matching->pkts, 0);
			atomic64_set(&matching->bytes, 0);
			return 0;
		}
		return -EBUSY;
	}

	nfacct = kzalloc(sizeof(struct nf_acct), GFP_KERNEL);
	if (nfacct == NULL)
		return -ENOMEM;

	strncpy(nfacct->name, nla_data(tb[NFACCT_NAME]), NFACCT_NAME_MAX);

	if (tb[NFACCT_BYTES]) {
		atomic64_set(&nfacct->bytes,
			     be64_to_cpu(nla_get_be64(tb[NFACCT_BYTES])));
	}
	if (tb[NFACCT_PKTS]) {
		atomic64_set(&nfacct->pkts,
			     be64_to_cpu(nla_get_be64(tb[NFACCT_PKTS])));
	}
	atomic_set(&nfacct->refcnt, 1);
	list_add_tail_rcu(&nfacct->head, &nfnl_acct_list);
	return 0;
}

static int
nfnl_acct_fill_info(struct sk_buff *skb, u32 portid, u32 seq, u32 type,
		   int event, struct nf_acct *acct)
{
	struct nlmsghdr *nlh;
	struct nfgenmsg *nfmsg;
	unsigned int flags = portid ? NLM_F_MULTI : 0;
	u64 pkts, bytes;

	event |= NFNL_SUBSYS_ACCT << 8;
	nlh = nlmsg_put(skb, portid, seq, event, sizeof(*nfmsg), flags);
	if (nlh == NULL)
		goto nlmsg_failure;

	nfmsg = nlmsg_data(nlh);
	nfmsg->nfgen_family = AF_UNSPEC;
	nfmsg->version = NFNETLINK_V0;
	nfmsg->res_id = 0;

	if (nla_put_string(skb, NFACCT_NAME, acct->name))
		goto nla_put_failure;

	if (type == NFNL_MSG_ACCT_GET_CTRZERO) {
		pkts = atomic64_xchg(&acct->pkts, 0);
		bytes = atomic64_xchg(&acct->bytes, 0);
	} else {
		pkts = atomic64_read(&acct->pkts);
		bytes = atomic64_read(&acct->bytes);
	}
	if (nla_put_be64(skb, NFACCT_PKTS, cpu_to_be64(pkts)) ||
	    nla_put_be64(skb, NFACCT_BYTES, cpu_to_be64(bytes)) ||
	    nla_put_be32(skb, NFACCT_USE, htonl(atomic_read(&acct->refcnt))))
		goto nla_put_failure;

	nlmsg_end(skb, nlh);
	return skb->len;

nlmsg_failure:
nla_put_failure:
	nlmsg_cancel(skb, nlh);
	return -1;
}

static int
nfnl_acct_dump(struct sk_buff *skb, struct netlink_callback *cb)
{
	struct nf_acct *cur, *last;

	if (cb->args[2])
		return 0;

	last = (struct nf_acct *)cb->args[1];
	if (cb->args[1])
		cb->args[1] = 0;

	rcu_read_lock();
	list_for_each_entry_rcu(cur, &nfnl_acct_list, head) {
		if (last) {
			if (cur != last)
				continue;

			last = NULL;
		}
		if (nfnl_acct_fill_info(skb, NETLINK_CB(cb->skb).portid,
				       cb->nlh->nlmsg_seq,
				       NFNL_MSG_TYPE(cb->nlh->nlmsg_type),
				       NFNL_MSG_ACCT_NEW, cur) < 0) {
			cb->args[1] = (unsigned long)cur;
			break;
		}
	}
	if (!cb->args[1])
		cb->args[2] = 1;
	rcu_read_unlock();
	return skb->len;
}

static int
nfnl_acct_get(struct sock *nfnl, struct sk_buff *skb,
	     const struct nlmsghdr *nlh, const struct nlattr * const tb[])
{
	int ret = -ENOENT;
	struct nf_acct *cur;
	char *acct_name;

	if (nlh->nlmsg_flags & NLM_F_DUMP) {
		struct netlink_dump_control c = {
			.dump = nfnl_acct_dump,
		};
		return netlink_dump_start(nfnl, skb, nlh, &c);
	}

	if (!tb[NFACCT_NAME])
		return -EINVAL;
	acct_name = nla_data(tb[NFACCT_NAME]);

	list_for_each_entry(cur, &nfnl_acct_list, head) {
		struct sk_buff *skb2;

		if (strncmp(cur->name, acct_name, NFACCT_NAME_MAX)!= 0)
			continue;

		skb2 = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
		if (skb2 == NULL) {
			ret = -ENOMEM;
			break;
		}

		ret = nfnl_acct_fill_info(skb2, NETLINK_CB(skb).portid,
					 nlh->nlmsg_seq,
					 NFNL_MSG_TYPE(nlh->nlmsg_type),
					 NFNL_MSG_ACCT_NEW, cur);
		if (ret <= 0) {
			kfree_skb(skb2);
			break;
		}
		ret = netlink_unicast(nfnl, skb2, NETLINK_CB(skb).portid,
					MSG_DONTWAIT);
		if (ret > 0)
			ret = 0;

		/* this avoids a loop in nfnetlink. */
		return ret == -EAGAIN ? -ENOBUFS : ret;
	}
	return ret;
}

/* try to delete object, fail if it is still in use. */
static int nfnl_acct_try_del(struct nf_acct *cur)
{
	int ret = 0;

	/* we want to avoid races with nfnl_acct_find_get. */
	if (atomic_dec_and_test(&cur->refcnt)) {
		/* We are protected by nfnl mutex. */
		list_del_rcu(&cur->head);
		kfree_rcu(cur, rcu_head);
	} else {
		/* still in use, restore reference counter. */
		atomic_inc(&cur->refcnt);
		ret = -EBUSY;
	}
	return ret;
}

static int
nfnl_acct_del(struct sock *nfnl, struct sk_buff *skb,
	     const struct nlmsghdr *nlh, const struct nlattr * const tb[])
{
	char *acct_name;
	struct nf_acct *cur;
	int ret = -ENOENT;

	if (!tb[NFACCT_NAME]) {
		list_for_each_entry(cur, &nfnl_acct_list, head)
			nfnl_acct_try_del(cur);

		return 0;
	}
	acct_name = nla_data(tb[NFACCT_NAME]);

	list_for_each_entry(cur, &nfnl_acct_list, head) {
		if (strncmp(cur->name, acct_name, NFACCT_NAME_MAX) != 0)
			continue;

		ret = nfnl_acct_try_del(cur);
		if (ret < 0)
			return ret;

		break;
	}
	return ret;
}

static const struct nla_policy nfnl_acct_policy[NFACCT_MAX+1] = {
	[NFACCT_NAME] = { .type = NLA_NUL_STRING, .len = NFACCT_NAME_MAX-1 },
	[NFACCT_BYTES] = { .type = NLA_U64 },
	[NFACCT_PKTS] = { .type = NLA_U64 },
};

static const struct nfnl_callback nfnl_acct_cb[NFNL_MSG_ACCT_MAX] = {
	[NFNL_MSG_ACCT_NEW]		= { .call = nfnl_acct_new,
					    .attr_count = NFACCT_MAX,
					    .policy = nfnl_acct_policy },
	[NFNL_MSG_ACCT_GET] 		= { .call = nfnl_acct_get,
					    .attr_count = NFACCT_MAX,
					    .policy = nfnl_acct_policy },
	[NFNL_MSG_ACCT_GET_CTRZERO] 	= { .call = nfnl_acct_get,
					    .attr_count = NFACCT_MAX,
					    .policy = nfnl_acct_policy },
	[NFNL_MSG_ACCT_DEL]		= { .call = nfnl_acct_del,
					    .attr_count = NFACCT_MAX,
					    .policy = nfnl_acct_policy },
};

static const struct nfnetlink_subsystem nfnl_acct_subsys = {
	.name				= "acct",
	.subsys_id			= NFNL_SUBSYS_ACCT,
	.cb_count			= NFNL_MSG_ACCT_MAX,
	.cb				= nfnl_acct_cb,
};

MODULE_ALIAS_NFNL_SUBSYS(NFNL_SUBSYS_ACCT);

struct nf_acct *nfnl_acct_find_get(const char *acct_name)
{
	struct nf_acct *cur, *acct = NULL;

	rcu_read_lock();
	list_for_each_entry_rcu(cur, &nfnl_acct_list, head) {
		if (strncmp(cur->name, acct_name, NFACCT_NAME_MAX)!= 0)
			continue;

		if (!try_module_get(THIS_MODULE))
			goto err;

		if (!atomic_inc_not_zero(&cur->refcnt)) {
			module_put(THIS_MODULE);
			goto err;
		}

		acct = cur;
		break;
	}
err:
	rcu_read_unlock();
	return acct;
}
EXPORT_SYMBOL_GPL(nfnl_acct_find_get);

void nfnl_acct_put(struct nf_acct *acct)
{
	atomic_dec(&acct->refcnt);
	module_put(THIS_MODULE);
}
EXPORT_SYMBOL_GPL(nfnl_acct_put);

void nfnl_acct_update(const struct sk_buff *skb, struct nf_acct *nfacct)
{
	atomic64_inc(&nfacct->pkts);
	atomic64_add(skb->len, &nfacct->bytes);
}
EXPORT_SYMBOL_GPL(nfnl_acct_update);

static int __init nfnl_acct_init(void)
{
	int ret;

	pr_info("nfnl_acct: registering with nfnetlink.\n");
	ret = nfnetlink_subsys_register(&nfnl_acct_subsys);
	if (ret < 0) {
		pr_err("nfnl_acct_init: cannot register with nfnetlink.\n");
		goto err_out;
	}
	return 0;
err_out:
	return ret;
}

static void __exit nfnl_acct_exit(void)
{
	struct nf_acct *cur, *tmp;

	pr_info("nfnl_acct: unregistering from nfnetlink.\n");
	nfnetlink_subsys_unregister(&nfnl_acct_subsys);

	list_for_each_entry_safe(cur, tmp, &nfnl_acct_list, head) {
		list_del_rcu(&cur->head);
		/* We are sure that our objects have no clients at this point,
		 * it's safe to release them all without checking refcnt. */
		kfree_rcu(cur, rcu_head);
	}
}

module_init(nfnl_acct_init);
module_exit(nfnl_acct_exit);
