/*
 * net/sched/ipt.c     iptables target interface
 *
 *TODO: Add other tables. For now we only support the ipv4 table targets
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Copyright:	Jamal Hadi Salim (2002-13)
 */

#include <fikus/types.h>
#include <fikus/kernel.h>
#include <fikus/string.h>
#include <fikus/errno.h>
#include <fikus/skbuff.h>
#include <fikus/rtnetlink.h>
#include <fikus/module.h>
#include <fikus/init.h>
#include <fikus/slab.h>
#include <net/netlink.h>
#include <net/pkt_sched.h>
#include <fikus/tc_act/tc_ipt.h>
#include <net/tc_act/tc_ipt.h>

#include <fikus/netfilter_ipv4/ip_tables.h>


#define IPT_TAB_MASK     15
static struct tcf_common *tcf_ipt_ht[IPT_TAB_MASK + 1];
static u32 ipt_idx_gen;
static DEFINE_RWLOCK(ipt_lock);

static struct tcf_hashinfo ipt_hash_info = {
	.htab	=	tcf_ipt_ht,
	.hmask	=	IPT_TAB_MASK,
	.lock	=	&ipt_lock,
};

static int ipt_init_target(struct xt_entry_target *t, char *table, unsigned int hook)
{
	struct xt_tgchk_param par;
	struct xt_target *target;
	int ret = 0;

	target = xt_request_find_target(AF_INET, t->u.user.name,
					t->u.user.revision);
	if (IS_ERR(target))
		return PTR_ERR(target);

	t->u.kernel.target = target;
	par.table     = table;
	par.entryinfo = NULL;
	par.target    = target;
	par.targinfo  = t->data;
	par.hook_mask = hook;
	par.family    = NFPROTO_IPV4;

	ret = xt_check_target(&par, t->u.target_size - sizeof(*t), 0, false);
	if (ret < 0) {
		module_put(t->u.kernel.target->me);
		return ret;
	}
	return 0;
}

static void ipt_destroy_target(struct xt_entry_target *t)
{
	struct xt_tgdtor_param par = {
		.target   = t->u.kernel.target,
		.targinfo = t->data,
	};
	if (par.target->destroy != NULL)
		par.target->destroy(&par);
	module_put(par.target->me);
}

static int tcf_ipt_release(struct tcf_ipt *ipt, int bind)
{
	int ret = 0;
	if (ipt) {
		if (bind)
			ipt->tcf_bindcnt--;
		ipt->tcf_refcnt--;
		if (ipt->tcf_bindcnt <= 0 && ipt->tcf_refcnt <= 0) {
			ipt_destroy_target(ipt->tcfi_t);
			kfree(ipt->tcfi_tname);
			kfree(ipt->tcfi_t);
			tcf_hash_destroy(&ipt->common, &ipt_hash_info);
			ret = ACT_P_DELETED;
		}
	}
	return ret;
}

static const struct nla_policy ipt_policy[TCA_IPT_MAX + 1] = {
	[TCA_IPT_TABLE]	= { .type = NLA_STRING, .len = IFNAMSIZ },
	[TCA_IPT_HOOK]	= { .type = NLA_U32 },
	[TCA_IPT_INDEX]	= { .type = NLA_U32 },
	[TCA_IPT_TARG]	= { .len = sizeof(struct xt_entry_target) },
};

static int tcf_ipt_init(struct net *net, struct nlattr *nla, struct nlattr *est,
			struct tc_action *a, int ovr, int bind)
{
	struct nlattr *tb[TCA_IPT_MAX + 1];
	struct tcf_ipt *ipt;
	struct tcf_common *pc;
	struct xt_entry_target *td, *t;
	char *tname;
	int ret = 0, err;
	u32 hook = 0;
	u32 index = 0;

	if (nla == NULL)
		return -EINVAL;

	err = nla_parse_nested(tb, TCA_IPT_MAX, nla, ipt_policy);
	if (err < 0)
		return err;

	if (tb[TCA_IPT_HOOK] == NULL)
		return -EINVAL;
	if (tb[TCA_IPT_TARG] == NULL)
		return -EINVAL;

	td = (struct xt_entry_target *)nla_data(tb[TCA_IPT_TARG]);
	if (nla_len(tb[TCA_IPT_TARG]) < td->u.target_size)
		return -EINVAL;

	if (tb[TCA_IPT_INDEX] != NULL)
		index = nla_get_u32(tb[TCA_IPT_INDEX]);

	pc = tcf_hash_check(index, a, bind, &ipt_hash_info);
	if (!pc) {
		pc = tcf_hash_create(index, est, a, sizeof(*ipt), bind,
				     &ipt_idx_gen, &ipt_hash_info);
		if (IS_ERR(pc))
			return PTR_ERR(pc);
		ret = ACT_P_CREATED;
	} else {
		if (!ovr) {
			tcf_ipt_release(to_ipt(pc), bind);
			return -EEXIST;
		}
	}
	ipt = to_ipt(pc);

	hook = nla_get_u32(tb[TCA_IPT_HOOK]);

	err = -ENOMEM;
	tname = kmalloc(IFNAMSIZ, GFP_KERNEL);
	if (unlikely(!tname))
		goto err1;
	if (tb[TCA_IPT_TABLE] == NULL ||
	    nla_strlcpy(tname, tb[TCA_IPT_TABLE], IFNAMSIZ) >= IFNAMSIZ)
		strcpy(tname, "mangle");

	t = kmemdup(td, td->u.target_size, GFP_KERNEL);
	if (unlikely(!t))
		goto err2;

	err = ipt_init_target(t, tname, hook);
	if (err < 0)
		goto err3;

	spin_lock_bh(&ipt->tcf_lock);
	if (ret != ACT_P_CREATED) {
		ipt_destroy_target(ipt->tcfi_t);
		kfree(ipt->tcfi_tname);
		kfree(ipt->tcfi_t);
	}
	ipt->tcfi_tname = tname;
	ipt->tcfi_t     = t;
	ipt->tcfi_hook  = hook;
	spin_unlock_bh(&ipt->tcf_lock);
	if (ret == ACT_P_CREATED)
		tcf_hash_insert(pc, &ipt_hash_info);
	return ret;

err3:
	kfree(t);
err2:
	kfree(tname);
err1:
	if (ret == ACT_P_CREATED) {
		if (est)
			gen_kill_estimator(&pc->tcfc_bstats,
					   &pc->tcfc_rate_est);
		kfree_rcu(pc, tcfc_rcu);
	}
	return err;
}

static int tcf_ipt_cleanup(struct tc_action *a, int bind)
{
	struct tcf_ipt *ipt = a->priv;
	return tcf_ipt_release(ipt, bind);
}

static int tcf_ipt(struct sk_buff *skb, const struct tc_action *a,
		   struct tcf_result *res)
{
	int ret = 0, result = 0;
	struct tcf_ipt *ipt = a->priv;
	struct xt_action_param par;

	if (skb_unclone(skb, GFP_ATOMIC))
		return TC_ACT_UNSPEC;

	spin_lock(&ipt->tcf_lock);

	ipt->tcf_tm.lastuse = jiffies;
	bstats_update(&ipt->tcf_bstats, skb);

	/* yes, we have to worry about both in and out dev
	 * worry later - danger - this API seems to have changed
	 * from earlier kernels
	 */
	par.in       = skb->dev;
	par.out      = NULL;
	par.hooknum  = ipt->tcfi_hook;
	par.target   = ipt->tcfi_t->u.kernel.target;
	par.targinfo = ipt->tcfi_t->data;
	ret = par.target->target(skb, &par);

	switch (ret) {
	case NF_ACCEPT:
		result = TC_ACT_OK;
		break;
	case NF_DROP:
		result = TC_ACT_SHOT;
		ipt->tcf_qstats.drops++;
		break;
	case XT_CONTINUE:
		result = TC_ACT_PIPE;
		break;
	default:
		net_notice_ratelimited("tc filter: Bogus netfilter code %d assume ACCEPT\n",
				       ret);
		result = TC_POLICE_OK;
		break;
	}
	spin_unlock(&ipt->tcf_lock);
	return result;

}

static int tcf_ipt_dump(struct sk_buff *skb, struct tc_action *a, int bind, int ref)
{
	unsigned char *b = skb_tail_pointer(skb);
	struct tcf_ipt *ipt = a->priv;
	struct xt_entry_target *t;
	struct tcf_t tm;
	struct tc_cnt c;

	/* for simple targets kernel size == user size
	 * user name = target name
	 * for foolproof you need to not assume this
	 */

	t = kmemdup(ipt->tcfi_t, ipt->tcfi_t->u.user.target_size, GFP_ATOMIC);
	if (unlikely(!t))
		goto nla_put_failure;

	c.bindcnt = ipt->tcf_bindcnt - bind;
	c.refcnt = ipt->tcf_refcnt - ref;
	strcpy(t->u.user.name, ipt->tcfi_t->u.kernel.target->name);

	if (nla_put(skb, TCA_IPT_TARG, ipt->tcfi_t->u.user.target_size, t) ||
	    nla_put_u32(skb, TCA_IPT_INDEX, ipt->tcf_index) ||
	    nla_put_u32(skb, TCA_IPT_HOOK, ipt->tcfi_hook) ||
	    nla_put(skb, TCA_IPT_CNT, sizeof(struct tc_cnt), &c) ||
	    nla_put_string(skb, TCA_IPT_TABLE, ipt->tcfi_tname))
		goto nla_put_failure;
	tm.install = jiffies_to_clock_t(jiffies - ipt->tcf_tm.install);
	tm.lastuse = jiffies_to_clock_t(jiffies - ipt->tcf_tm.lastuse);
	tm.expires = jiffies_to_clock_t(ipt->tcf_tm.expires);
	if (nla_put(skb, TCA_IPT_TM, sizeof (tm), &tm))
		goto nla_put_failure;
	kfree(t);
	return skb->len;

nla_put_failure:
	nlmsg_trim(skb, b);
	kfree(t);
	return -1;
}

static struct tc_action_ops act_ipt_ops = {
	.kind		=	"ipt",
	.hinfo		=	&ipt_hash_info,
	.type		=	TCA_ACT_IPT,
	.capab		=	TCA_CAP_NONE,
	.owner		=	THIS_MODULE,
	.act		=	tcf_ipt,
	.dump		=	tcf_ipt_dump,
	.cleanup	=	tcf_ipt_cleanup,
	.lookup		=	tcf_hash_search,
	.init		=	tcf_ipt_init,
	.walk		=	tcf_generic_walker
};

static struct tc_action_ops act_xt_ops = {
	.kind		=	"xt",
	.hinfo		=	&ipt_hash_info,
	.type		=	TCA_ACT_IPT,
	.capab		=	TCA_CAP_NONE,
	.owner		=	THIS_MODULE,
	.act		=	tcf_ipt,
	.dump		=	tcf_ipt_dump,
	.cleanup	=	tcf_ipt_cleanup,
	.lookup		=	tcf_hash_search,
	.init		=	tcf_ipt_init,
	.walk		=	tcf_generic_walker
};

MODULE_AUTHOR("Jamal Hadi Salim(2002-13)");
MODULE_DESCRIPTION("Iptables target actions");
MODULE_LICENSE("GPL");
MODULE_ALIAS("act_xt");

static int __init ipt_init_module(void)
{
	int ret1, ret2;
	ret1 = tcf_register_action(&act_xt_ops);
	if (ret1 < 0)
		printk("Failed to load xt action\n");
	ret2 = tcf_register_action(&act_ipt_ops);
	if (ret2 < 0)
		printk("Failed to load ipt action\n");

	if (ret1 < 0 && ret2 < 0)
		return ret1;
	else
		return 0;
}

static void __exit ipt_cleanup_module(void)
{
	tcf_unregister_action(&act_xt_ops);
	tcf_unregister_action(&act_ipt_ops);
}

module_init(ipt_init_module);
module_exit(ipt_cleanup_module);
