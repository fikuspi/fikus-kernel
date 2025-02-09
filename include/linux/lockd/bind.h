/*
 * fikus/include/fikus/lockd/bind.h
 *
 * This is the part of lockd visible to nfsd and the nfs client.
 *
 * Copyright (C) 1996, Olaf Kirch <okir@monad.swb.de>
 */

#ifndef FIKUS_LOCKD_BIND_H
#define FIKUS_LOCKD_BIND_H

#include <fikus/lockd/nlm.h>
/* need xdr-encoded error codes too, so... */
#include <fikus/lockd/xdr.h>
#ifdef CONFIG_LOCKD_V4
#include <fikus/lockd/xdr4.h>
#endif

/* Dummy declarations */
struct svc_rqst;

/*
 * This is the set of functions for lockd->nfsd communication
 */
struct nlmsvc_binding {
	__be32			(*fopen)(struct svc_rqst *,
						struct nfs_fh *,
						struct file **);
	void			(*fclose)(struct file *);
};

extern struct nlmsvc_binding *	nlmsvc_ops;

/*
 * Similar to nfs_client_initdata, but without the NFS-specific
 * rpc_ops field.
 */
struct nlmclnt_initdata {
	const char		*hostname;
	const struct sockaddr	*address;
	size_t			addrlen;
	unsigned short		protocol;
	u32			nfs_version;
	int			noresvport;
	struct net		*net;
};

/*
 * Functions exported by the lockd module
 */

extern struct nlm_host *nlmclnt_init(const struct nlmclnt_initdata *nlm_init);
extern void	nlmclnt_done(struct nlm_host *host);

extern int	nlmclnt_proc(struct nlm_host *host, int cmd,
					struct file_lock *fl);
extern int	lockd_up(struct net *net);
extern void	lockd_down(struct net *net);

#endif /* FIKUS_LOCKD_BIND_H */
