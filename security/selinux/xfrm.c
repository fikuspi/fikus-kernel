/*
 *  NSA Security-Enhanced Fikus (SEFikus) security module
 *
 *  This file contains the SEFikus XFRM hook function implementations.
 *
 *  Authors:  Serge Hallyn <sergeh@us.ibm.com>
 *	      Trent Jaeger <jaegert@us.ibm.com>
 *
 *  Updated: Venkat Yekkirala <vyekkirala@TrustedCS.com>
 *
 *           Granular IPSec Associations for use in MLS environments.
 *
 *  Copyright (C) 2005 International Business Machines Corporation
 *  Copyright (C) 2006 Trusted Computer Solutions, Inc.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2,
 *	as published by the Free Software Foundation.
 */

/*
 * USAGE:
 * NOTES:
 *   1. Make sure to enable the following options in your kernel config:
 *	CONFIG_SECURITY=y
 *	CONFIG_SECURITY_NETWORK=y
 *	CONFIG_SECURITY_NETWORK_XFRM=y
 *	CONFIG_SECURITY_SEFIKUS=m/y
 * ISSUES:
 *   1. Caching packets, so they are not dropped during negotiation
 *   2. Emulating a reasonable SO_PEERSEC across machines
 *   3. Testing addition of sk_policy's with security context via setsockopt
 */
#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/security.h>
#include <fikus/types.h>
#include <fikus/netfilter.h>
#include <fikus/netfilter_ipv4.h>
#include <fikus/netfilter_ipv6.h>
#include <fikus/slab.h>
#include <fikus/ip.h>
#include <fikus/tcp.h>
#include <fikus/skbuff.h>
#include <fikus/xfrm.h>
#include <net/xfrm.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <fikus/atomic.h>

#include "avc.h"
#include "objsec.h"
#include "xfrm.h"

/* Labeled XFRM instance counter */
atomic_t sefikus_xfrm_refcount = ATOMIC_INIT(0);

/*
 * Returns true if an LSM/SEFikus context
 */
static inline int sefikus_authorizable_ctx(struct xfrm_sec_ctx *ctx)
{
	return (ctx &&
		(ctx->ctx_doi == XFRM_SC_DOI_LSM) &&
		(ctx->ctx_alg == XFRM_SC_ALG_SEFIKUS));
}

/*
 * Returns true if the xfrm contains a security blob for SEFikus
 */
static inline int sefikus_authorizable_xfrm(struct xfrm_state *x)
{
	return sefikus_authorizable_ctx(x->security);
}

/*
 * LSM hook implementation that authorizes that a flow can use
 * a xfrm policy rule.
 */
int sefikus_xfrm_policy_lookup(struct xfrm_sec_ctx *ctx, u32 fl_secid, u8 dir)
{
	int rc;
	u32 sel_sid;

	/* Context sid is either set to label or ANY_ASSOC */
	if (ctx) {
		if (!sefikus_authorizable_ctx(ctx))
			return -EINVAL;

		sel_sid = ctx->ctx_sid;
	} else
		/*
		 * All flows should be treated as polmatch'ing an
		 * otherwise applicable "non-labeled" policy. This
		 * would prevent inadvertent "leaks".
		 */
		return 0;

	rc = avc_has_perm(fl_secid, sel_sid, SECCLASS_ASSOCIATION,
			  ASSOCIATION__POLMATCH,
			  NULL);

	if (rc == -EACCES)
		return -ESRCH;

	return rc;
}

/*
 * LSM hook implementation that authorizes that a state matches
 * the given policy, flow combo.
 */

int sefikus_xfrm_state_pol_flow_match(struct xfrm_state *x, struct xfrm_policy *xp,
			const struct flowi *fl)
{
	u32 state_sid;
	int rc;

	if (!xp->security)
		if (x->security)
			/* unlabeled policy and labeled SA can't match */
			return 0;
		else
			/* unlabeled policy and unlabeled SA match all flows */
			return 1;
	else
		if (!x->security)
			/* unlabeled SA and labeled policy can't match */
			return 0;
		else
			if (!sefikus_authorizable_xfrm(x))
				/* Not a SEFikus-labeled SA */
				return 0;

	state_sid = x->security->ctx_sid;

	if (fl->flowi_secid != state_sid)
		return 0;

	rc = avc_has_perm(fl->flowi_secid, state_sid, SECCLASS_ASSOCIATION,
			  ASSOCIATION__SENDTO,
			  NULL)? 0:1;

	/*
	 * We don't need a separate SA Vs. policy polmatch check
	 * since the SA is now of the same label as the flow and
	 * a flow Vs. policy polmatch check had already happened
	 * in sefikus_xfrm_policy_lookup() above.
	 */

	return rc;
}

static int sefikus_xfrm_skb_sid_ingress(struct sk_buff *skb,
					u32 *sid, int ckall)
{
	struct sec_path *sp = skb->sp;

	*sid = SECSID_NULL;

	if (sp) {
		int i, sid_set = 0;

		for (i = sp->len-1; i >= 0; i--) {
			struct xfrm_state *x = sp->xvec[i];
			if (sefikus_authorizable_xfrm(x)) {
				struct xfrm_sec_ctx *ctx = x->security;

				if (!sid_set) {
					*sid = ctx->ctx_sid;
					sid_set = 1;

					if (!ckall)
						break;
				} else if (*sid != ctx->ctx_sid)
					return -EINVAL;
			}
		}
	}

	return 0;
}

static u32 sefikus_xfrm_skb_sid_egress(struct sk_buff *skb)
{
	struct dst_entry *dst = skb_dst(skb);
	struct xfrm_state *x;

	if (dst == NULL)
		return SECSID_NULL;
	x = dst->xfrm;
	if (x == NULL || !sefikus_authorizable_xfrm(x))
		return SECSID_NULL;

	return x->security->ctx_sid;
}

/*
 * LSM hook implementation that checks and/or returns the xfrm sid for the
 * incoming packet.
 */

int sefikus_xfrm_decode_session(struct sk_buff *skb, u32 *sid, int ckall)
{
	if (skb == NULL) {
		*sid = SECSID_NULL;
		return 0;
	}
	return sefikus_xfrm_skb_sid_ingress(skb, sid, ckall);
}

int sefikus_xfrm_skb_sid(struct sk_buff *skb, u32 *sid)
{
	int rc;

	rc = sefikus_xfrm_skb_sid_ingress(skb, sid, 0);
	if (rc == 0 && *sid == SECSID_NULL)
		*sid = sefikus_xfrm_skb_sid_egress(skb);

	return rc;
}

/*
 * Security blob allocation for xfrm_policy and xfrm_state
 * CTX does not have a meaningful value on input
 */
static int sefikus_xfrm_sec_ctx_alloc(struct xfrm_sec_ctx **ctxp,
	struct xfrm_user_sec_ctx *uctx, u32 sid)
{
	int rc = 0;
	const struct task_security_struct *tsec = current_security();
	struct xfrm_sec_ctx *ctx = NULL;
	char *ctx_str = NULL;
	u32 str_len;

	BUG_ON(uctx && sid);

	if (!uctx)
		goto not_from_user;

	if (uctx->ctx_alg != XFRM_SC_ALG_SEFIKUS)
		return -EINVAL;

	str_len = uctx->ctx_len;
	if (str_len >= PAGE_SIZE)
		return -ENOMEM;

	*ctxp = ctx = kmalloc(sizeof(*ctx) +
			      str_len + 1,
			      GFP_KERNEL);

	if (!ctx)
		return -ENOMEM;

	ctx->ctx_doi = uctx->ctx_doi;
	ctx->ctx_len = str_len;
	ctx->ctx_alg = uctx->ctx_alg;

	memcpy(ctx->ctx_str,
	       uctx+1,
	       str_len);
	ctx->ctx_str[str_len] = 0;
	rc = security_context_to_sid(ctx->ctx_str,
				     str_len,
				     &ctx->ctx_sid);

	if (rc)
		goto out;

	/*
	 * Does the subject have permission to set security context?
	 */
	rc = avc_has_perm(tsec->sid, ctx->ctx_sid,
			  SECCLASS_ASSOCIATION,
			  ASSOCIATION__SETCONTEXT, NULL);
	if (rc)
		goto out;

	return rc;

not_from_user:
	rc = security_sid_to_context(sid, &ctx_str, &str_len);
	if (rc)
		goto out;

	*ctxp = ctx = kmalloc(sizeof(*ctx) +
			      str_len,
			      GFP_ATOMIC);

	if (!ctx) {
		rc = -ENOMEM;
		goto out;
	}

	ctx->ctx_doi = XFRM_SC_DOI_LSM;
	ctx->ctx_alg = XFRM_SC_ALG_SEFIKUS;
	ctx->ctx_sid = sid;
	ctx->ctx_len = str_len;
	memcpy(ctx->ctx_str,
	       ctx_str,
	       str_len);

	goto out2;

out:
	*ctxp = NULL;
	kfree(ctx);
out2:
	kfree(ctx_str);
	return rc;
}

/*
 * LSM hook implementation that allocs and transfers uctx spec to
 * xfrm_policy.
 */
int sefikus_xfrm_policy_alloc(struct xfrm_sec_ctx **ctxp,
			      struct xfrm_user_sec_ctx *uctx)
{
	int err;

	BUG_ON(!uctx);

	err = sefikus_xfrm_sec_ctx_alloc(ctxp, uctx, 0);
	if (err == 0)
		atomic_inc(&sefikus_xfrm_refcount);

	return err;
}


/*
 * LSM hook implementation that copies security data structure from old to
 * new for policy cloning.
 */
int sefikus_xfrm_policy_clone(struct xfrm_sec_ctx *old_ctx,
			      struct xfrm_sec_ctx **new_ctxp)
{
	struct xfrm_sec_ctx *new_ctx;

	if (old_ctx) {
		new_ctx = kmalloc(sizeof(*old_ctx) + old_ctx->ctx_len,
				  GFP_ATOMIC);
		if (!new_ctx)
			return -ENOMEM;

		memcpy(new_ctx, old_ctx, sizeof(*new_ctx));
		memcpy(new_ctx->ctx_str, old_ctx->ctx_str, new_ctx->ctx_len);
		atomic_inc(&sefikus_xfrm_refcount);
		*new_ctxp = new_ctx;
	}
	return 0;
}

/*
 * LSM hook implementation that frees xfrm_sec_ctx security information.
 */
void sefikus_xfrm_policy_free(struct xfrm_sec_ctx *ctx)
{
	atomic_dec(&sefikus_xfrm_refcount);
	kfree(ctx);
}

/*
 * LSM hook implementation that authorizes deletion of labeled policies.
 */
int sefikus_xfrm_policy_delete(struct xfrm_sec_ctx *ctx)
{
	const struct task_security_struct *tsec = current_security();

	if (!ctx)
		return 0;

	return avc_has_perm(tsec->sid, ctx->ctx_sid,
			    SECCLASS_ASSOCIATION, ASSOCIATION__SETCONTEXT,
			    NULL);
}

/*
 * LSM hook implementation that allocs and transfers sec_ctx spec to
 * xfrm_state.
 */
int sefikus_xfrm_state_alloc(struct xfrm_state *x, struct xfrm_user_sec_ctx *uctx,
		u32 secid)
{
	int err;

	BUG_ON(!x);

	err = sefikus_xfrm_sec_ctx_alloc(&x->security, uctx, secid);
	if (err == 0)
		atomic_inc(&sefikus_xfrm_refcount);
	return err;
}

/*
 * LSM hook implementation that frees xfrm_state security information.
 */
void sefikus_xfrm_state_free(struct xfrm_state *x)
{
	atomic_dec(&sefikus_xfrm_refcount);
	kfree(x->security);
}

 /*
  * LSM hook implementation that authorizes deletion of labeled SAs.
  */
int sefikus_xfrm_state_delete(struct xfrm_state *x)
{
	const struct task_security_struct *tsec = current_security();
	struct xfrm_sec_ctx *ctx = x->security;

	if (!ctx)
		return 0;

	return avc_has_perm(tsec->sid, ctx->ctx_sid,
			    SECCLASS_ASSOCIATION, ASSOCIATION__SETCONTEXT,
			    NULL);
}

/*
 * LSM hook that controls access to unlabelled packets.  If
 * a xfrm_state is authorizable (defined by macro) then it was
 * already authorized by the IPSec process.  If not, then
 * we need to check for unlabelled access since this may not have
 * gone thru the IPSec process.
 */
int sefikus_xfrm_sock_rcv_skb(u32 isec_sid, struct sk_buff *skb,
				struct common_audit_data *ad)
{
	int i, rc = 0;
	struct sec_path *sp;
	u32 sel_sid = SECINITSID_UNLABELED;

	sp = skb->sp;

	if (sp) {
		for (i = 0; i < sp->len; i++) {
			struct xfrm_state *x = sp->xvec[i];

			if (x && sefikus_authorizable_xfrm(x)) {
				struct xfrm_sec_ctx *ctx = x->security;
				sel_sid = ctx->ctx_sid;
				break;
			}
		}
	}

	/*
	 * This check even when there's no association involved is
	 * intended, according to Trent Jaeger, to make sure a
	 * process can't engage in non-ipsec communication unless
	 * explicitly allowed by policy.
	 */

	rc = avc_has_perm(isec_sid, sel_sid, SECCLASS_ASSOCIATION,
			  ASSOCIATION__RECVFROM, ad);

	return rc;
}

/*
 * POSTROUTE_LAST hook's XFRM processing:
 * If we have no security association, then we need to determine
 * whether the socket is allowed to send to an unlabelled destination.
 * If we do have a authorizable security association, then it has already been
 * checked in the sefikus_xfrm_state_pol_flow_match hook above.
 */
int sefikus_xfrm_postroute_last(u32 isec_sid, struct sk_buff *skb,
					struct common_audit_data *ad, u8 proto)
{
	struct dst_entry *dst;
	int rc = 0;

	dst = skb_dst(skb);

	if (dst) {
		struct dst_entry *dst_test;

		for (dst_test = dst; dst_test != NULL;
		     dst_test = dst_test->child) {
			struct xfrm_state *x = dst_test->xfrm;

			if (x && sefikus_authorizable_xfrm(x))
				goto out;
		}
	}

	switch (proto) {
	case IPPROTO_AH:
	case IPPROTO_ESP:
	case IPPROTO_COMP:
		/*
		 * We should have already seen this packet once before
		 * it underwent xfrm(s). No need to subject it to the
		 * unlabeled check.
		 */
		goto out;
	default:
		break;
	}

	/*
	 * This check even when there's no association involved is
	 * intended, according to Trent Jaeger, to make sure a
	 * process can't engage in non-ipsec communication unless
	 * explicitly allowed by policy.
	 */

	rc = avc_has_perm(isec_sid, SECINITSID_UNLABELED, SECCLASS_ASSOCIATION,
			  ASSOCIATION__SENDTO, ad);
out:
	return rc;
}
