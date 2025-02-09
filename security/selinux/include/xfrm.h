/*
 * SEFikus support for the XFRM LSM hooks
 *
 * Author : Trent Jaeger, <jaegert@us.ibm.com>
 * Updated : Venkat Yekkirala, <vyekkirala@TrustedCS.com>
 */
#ifndef _SEFIKUS_XFRM_H_
#define _SEFIKUS_XFRM_H_

#include <net/flow.h>

int sefikus_xfrm_policy_alloc(struct xfrm_sec_ctx **ctxp,
			      struct xfrm_user_sec_ctx *sec_ctx);
int sefikus_xfrm_policy_clone(struct xfrm_sec_ctx *old_ctx,
			      struct xfrm_sec_ctx **new_ctxp);
void sefikus_xfrm_policy_free(struct xfrm_sec_ctx *ctx);
int sefikus_xfrm_policy_delete(struct xfrm_sec_ctx *ctx);
int sefikus_xfrm_state_alloc(struct xfrm_state *x,
	struct xfrm_user_sec_ctx *sec_ctx, u32 secid);
void sefikus_xfrm_state_free(struct xfrm_state *x);
int sefikus_xfrm_state_delete(struct xfrm_state *x);
int sefikus_xfrm_policy_lookup(struct xfrm_sec_ctx *ctx, u32 fl_secid, u8 dir);
int sefikus_xfrm_state_pol_flow_match(struct xfrm_state *x,
			struct xfrm_policy *xp, const struct flowi *fl);

/*
 * Extract the security blob from the sock (it's actually on the socket)
 */
static inline struct inode_security_struct *get_sock_isec(struct sock *sk)
{
	if (!sk->sk_socket)
		return NULL;

	return SOCK_INODE(sk->sk_socket)->i_security;
}

#ifdef CONFIG_SECURITY_NETWORK_XFRM
extern atomic_t sefikus_xfrm_refcount;

static inline int sefikus_xfrm_enabled(void)
{
	return (atomic_read(&sefikus_xfrm_refcount) > 0);
}

int sefikus_xfrm_sock_rcv_skb(u32 sid, struct sk_buff *skb,
			struct common_audit_data *ad);
int sefikus_xfrm_postroute_last(u32 isec_sid, struct sk_buff *skb,
			struct common_audit_data *ad, u8 proto);
int sefikus_xfrm_decode_session(struct sk_buff *skb, u32 *sid, int ckall);
int sefikus_xfrm_skb_sid(struct sk_buff *skb, u32 *sid);

static inline void sefikus_xfrm_notify_policyload(void)
{
	struct net *net;

	atomic_inc(&flow_cache_genid);
	rtnl_lock();
	for_each_net(net)
		rt_genid_bump_all(net);
	rtnl_unlock();
}
#else
static inline int sefikus_xfrm_enabled(void)
{
	return 0;
}

static inline int sefikus_xfrm_sock_rcv_skb(u32 isec_sid, struct sk_buff *skb,
			struct common_audit_data *ad)
{
	return 0;
}

static inline int sefikus_xfrm_postroute_last(u32 isec_sid, struct sk_buff *skb,
			struct common_audit_data *ad, u8 proto)
{
	return 0;
}

static inline int sefikus_xfrm_decode_session(struct sk_buff *skb, u32 *sid, int ckall)
{
	*sid = SECSID_NULL;
	return 0;
}

static inline void sefikus_xfrm_notify_policyload(void)
{
}

static inline int sefikus_xfrm_skb_sid(struct sk_buff *skb, u32 *sid)
{
	*sid = SECSID_NULL;
	return 0;
}
#endif

#endif /* _SEFIKUS_XFRM_H_ */
