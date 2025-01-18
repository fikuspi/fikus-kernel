/*
 * fikus/include/fikus/sunrpc/svcauth_gss.h
 *
 * Bruce Fields <bfields@umich.edu>
 * Copyright (c) 2002 The Regents of the University of Michigan
 */

#ifndef _FIKUS_SUNRPC_SVCAUTH_GSS_H
#define _FIKUS_SUNRPC_SVCAUTH_GSS_H

#ifdef __KERNEL__
#include <fikus/sched.h>
#include <fikus/sunrpc/types.h>
#include <fikus/sunrpc/xdr.h>
#include <fikus/sunrpc/svcauth.h>
#include <fikus/sunrpc/svcsock.h>
#include <fikus/sunrpc/auth_gss.h>

int gss_svc_init(void);
void gss_svc_shutdown(void);
int gss_svc_init_net(struct net *net);
void gss_svc_shutdown_net(struct net *net);
int svcauth_gss_register_pseudoflavor(u32 pseudoflavor, char * name);
u32 svcauth_gss_flavor(struct auth_domain *dom);

#endif /* __KERNEL__ */
#endif /* _FIKUS_SUNRPC_SVCAUTH_GSS_H */
