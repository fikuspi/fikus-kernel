/*
 * fikus/net/sunrpc/sunrpc_syms.c
 *
 * Symbols exported by the sunrpc module.
 *
 * Copyright (C) 1997 Olaf Kirch <okir@monad.swb.de>
 */

#include <fikus/module.h>

#include <fikus/types.h>
#include <fikus/uio.h>
#include <fikus/unistd.h>
#include <fikus/init.h>

#include <fikus/sunrpc/sched.h>
#include <fikus/sunrpc/clnt.h>
#include <fikus/sunrpc/svc.h>
#include <fikus/sunrpc/svcsock.h>
#include <fikus/sunrpc/auth.h>
#include <fikus/workqueue.h>
#include <fikus/sunrpc/rpc_pipe_fs.h>
#include <fikus/sunrpc/xprtsock.h>

#include "netns.h"

int sunrpc_net_id;
EXPORT_SYMBOL_GPL(sunrpc_net_id);

static __net_init int sunrpc_init_net(struct net *net)
{
	int err;
	struct sunrpc_net *sn = net_generic(net, sunrpc_net_id);

	err = rpc_proc_init(net);
	if (err)
		goto err_proc;

	err = ip_map_cache_create(net);
	if (err)
		goto err_ipmap;

	err = unix_gid_cache_create(net);
	if (err)
		goto err_unixgid;

	rpc_pipefs_init_net(net);
	INIT_LIST_HEAD(&sn->all_clients);
	spin_lock_init(&sn->rpc_client_lock);
	spin_lock_init(&sn->rpcb_clnt_lock);
	return 0;

err_unixgid:
	ip_map_cache_destroy(net);
err_ipmap:
	rpc_proc_exit(net);
err_proc:
	return err;
}

static __net_exit void sunrpc_exit_net(struct net *net)
{
	unix_gid_cache_destroy(net);
	ip_map_cache_destroy(net);
	rpc_proc_exit(net);
}

static struct pernet_operations sunrpc_net_ops = {
	.init = sunrpc_init_net,
	.exit = sunrpc_exit_net,
	.id = &sunrpc_net_id,
	.size = sizeof(struct sunrpc_net),
};

static int __init
init_sunrpc(void)
{
	int err = rpc_init_mempool();
	if (err)
		goto out;
	err = rpcauth_init_module();
	if (err)
		goto out2;

	cache_initialize();

	err = register_pernet_subsys(&sunrpc_net_ops);
	if (err)
		goto out3;

	err = register_rpc_pipefs();
	if (err)
		goto out4;
#ifdef RPC_DEBUG
	rpc_register_sysctl();
#endif
	svc_init_xprt_sock();	/* svc sock transport */
	init_socket_xprt();	/* clnt sock transport */
	return 0;

out4:
	unregister_pernet_subsys(&sunrpc_net_ops);
out3:
	rpcauth_remove_module();
out2:
	rpc_destroy_mempool();
out:
	return err;
}

static void __exit
cleanup_sunrpc(void)
{
	rpcauth_remove_module();
	cleanup_socket_xprt();
	svc_cleanup_xprt_sock();
	unregister_rpc_pipefs();
	rpc_destroy_mempool();
	unregister_pernet_subsys(&sunrpc_net_ops);
#ifdef RPC_DEBUG
	rpc_unregister_sysctl();
#endif
	rcu_barrier(); /* Wait for completion of call_rcu()'s */
}
MODULE_LICENSE("GPL");
fs_initcall(init_sunrpc); /* Ensure we're initialised before nfs */
module_exit(cleanup_sunrpc);
