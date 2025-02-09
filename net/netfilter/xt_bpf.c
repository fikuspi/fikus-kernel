/* Xtables module to match packets using a BPF filter.
 * Copyright 2013 Google Inc.
 * Written by Willem de Bruijn <willemb@google.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/module.h>
#include <fikus/skbuff.h>
#include <fikus/filter.h>

#include <fikus/netfilter/xt_bpf.h>
#include <fikus/netfilter/x_tables.h>

MODULE_AUTHOR("Willem de Bruijn <willemb@google.com>");
MODULE_DESCRIPTION("Xtables: BPF filter match");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_bpf");
MODULE_ALIAS("ip6t_bpf");

static int bpf_mt_check(const struct xt_mtchk_param *par)
{
	struct xt_bpf_info *info = par->matchinfo;
	struct sock_fprog program;

	program.len = info->bpf_program_num_elem;
	program.filter = (struct sock_filter __user *) info->bpf_program;
	if (sk_unattached_filter_create(&info->filter, &program)) {
		pr_info("bpf: check failed: parse error\n");
		return -EINVAL;
	}

	return 0;
}

static bool bpf_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_bpf_info *info = par->matchinfo;

	return SK_RUN_FILTER(info->filter, skb);
}

static void bpf_mt_destroy(const struct xt_mtdtor_param *par)
{
	const struct xt_bpf_info *info = par->matchinfo;
	sk_unattached_filter_destroy(info->filter);
}

static struct xt_match bpf_mt_reg __read_mostly = {
	.name		= "bpf",
	.revision	= 0,
	.family		= NFPROTO_UNSPEC,
	.checkentry	= bpf_mt_check,
	.match		= bpf_mt,
	.destroy	= bpf_mt_destroy,
	.matchsize	= sizeof(struct xt_bpf_info),
	.me		= THIS_MODULE,
};

static int __init bpf_mt_init(void)
{
	return xt_register_match(&bpf_mt_reg);
}

static void __exit bpf_mt_exit(void)
{
	xt_unregister_match(&bpf_mt_reg);
}

module_init(bpf_mt_init);
module_exit(bpf_mt_exit);
