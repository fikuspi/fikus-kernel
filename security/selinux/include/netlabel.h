/*
 * SEFikus interface to the NetLabel subsystem
 *
 * Author: Paul Moore <paul@paul-moore.com>
 *
 */

/*
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006
 *
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef _SEFIKUS_NETLABEL_H_
#define _SEFIKUS_NETLABEL_H_

#include <fikus/types.h>
#include <fikus/fs.h>
#include <fikus/net.h>
#include <fikus/skbuff.h>
#include <net/sock.h>
#include <net/request_sock.h>

#include "avc.h"
#include "objsec.h"

#ifdef CONFIG_NETLABEL
void sefikus_netlbl_cache_invalidate(void);

void sefikus_netlbl_err(struct sk_buff *skb, int error, int gateway);

void sefikus_netlbl_sk_security_free(struct sk_security_struct *sksec);
void sefikus_netlbl_sk_security_reset(struct sk_security_struct *sksec);

int sefikus_netlbl_skbuff_getsid(struct sk_buff *skb,
				 u16 family,
				 u32 *type,
				 u32 *sid);
int sefikus_netlbl_skbuff_setsid(struct sk_buff *skb,
				 u16 family,
				 u32 sid);

int sefikus_netlbl_inet_conn_request(struct request_sock *req, u16 family);
void sefikus_netlbl_inet_csk_clone(struct sock *sk, u16 family);
int sefikus_netlbl_socket_post_create(struct sock *sk, u16 family);
int sefikus_netlbl_sock_rcv_skb(struct sk_security_struct *sksec,
				struct sk_buff *skb,
				u16 family,
				struct common_audit_data *ad);
int sefikus_netlbl_socket_setsockopt(struct socket *sock,
				     int level,
				     int optname);
int sefikus_netlbl_socket_connect(struct sock *sk, struct sockaddr *addr);

#else
static inline void sefikus_netlbl_cache_invalidate(void)
{
	return;
}

static inline void sefikus_netlbl_err(struct sk_buff *skb,
				      int error,
				      int gateway)
{
	return;
}

static inline void sefikus_netlbl_sk_security_free(
					       struct sk_security_struct *sksec)
{
	return;
}

static inline void sefikus_netlbl_sk_security_reset(
					       struct sk_security_struct *sksec)
{
	return;
}

static inline int sefikus_netlbl_skbuff_getsid(struct sk_buff *skb,
					       u16 family,
					       u32 *type,
					       u32 *sid)
{
	*type = NETLBL_NLTYPE_NONE;
	*sid = SECSID_NULL;
	return 0;
}
static inline int sefikus_netlbl_skbuff_setsid(struct sk_buff *skb,
					       u16 family,
					       u32 sid)
{
	return 0;
}

static inline int sefikus_netlbl_conn_setsid(struct sock *sk,
					     struct sockaddr *addr)
{
	return 0;
}

static inline int sefikus_netlbl_inet_conn_request(struct request_sock *req,
						   u16 family)
{
	return 0;
}
static inline void sefikus_netlbl_inet_csk_clone(struct sock *sk, u16 family)
{
	return;
}
static inline int sefikus_netlbl_socket_post_create(struct sock *sk,
						    u16 family)
{
	return 0;
}
static inline int sefikus_netlbl_sock_rcv_skb(struct sk_security_struct *sksec,
					      struct sk_buff *skb,
					      u16 family,
					      struct common_audit_data *ad)
{
	return 0;
}
static inline int sefikus_netlbl_socket_setsockopt(struct socket *sock,
						   int level,
						   int optname)
{
	return 0;
}
static inline int sefikus_netlbl_socket_connect(struct sock *sk,
						struct sockaddr *addr)
{
	return 0;
}
#endif /* CONFIG_NETLABEL */

#endif
