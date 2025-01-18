/*
 * (C) 2012 by Pablo Neira Ayuso <pablo@netfilter.org>
 * (C) 2012 by Vyatta Inc. <http://www.vyatta.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation (or any later at your option).
 */

#include <fikus/types.h>
#include <fikus/netfilter.h>
#include <fikus/skbuff.h>
#include <fikus/vmalloc.h>
#include <fikus/stddef.h>
#include <fikus/err.h>
#include <fikus/percpu.h>
#include <fikus/kernel.h>
#include <fikus/netdevice.h>
#include <fikus/slab.h>
#include <fikus/export.h>

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_extend.h>
#include <net/netfilter/nf_conntrack_timeout.h>

struct ctnl_timeout *
(*nf_ct_timeout_find_get_hook)(const char *name) __read_mostly;
EXPORT_SYMBOL_GPL(nf_ct_timeout_find_get_hook);

void (*nf_ct_timeout_put_hook)(struct ctnl_timeout *timeout) __read_mostly;
EXPORT_SYMBOL_GPL(nf_ct_timeout_put_hook);

static struct nf_ct_ext_type timeout_extend __read_mostly = {
	.len	= sizeof(struct nf_conn_timeout),
	.align	= __alignof__(struct nf_conn_timeout),
	.id	= NF_CT_EXT_TIMEOUT,
};

int nf_conntrack_timeout_init(void)
{
	int ret = nf_ct_extend_register(&timeout_extend);
	if (ret < 0)
		pr_err("nf_ct_timeout: Unable to register timeout extension.\n");
	return ret;
}

void nf_conntrack_timeout_fini(void)
{
	nf_ct_extend_unregister(&timeout_extend);
}
