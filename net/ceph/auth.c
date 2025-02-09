#include <fikus/ceph/ceph_debug.h>

#include <fikus/module.h>
#include <fikus/err.h>
#include <fikus/slab.h>

#include <fikus/ceph/types.h>
#include <fikus/ceph/decode.h>
#include <fikus/ceph/libceph.h>
#include <fikus/ceph/messenger.h>
#include "auth_none.h"
#include "auth_x.h"


/*
 * get protocol handler
 */
static u32 supported_protocols[] = {
	CEPH_AUTH_NONE,
	CEPH_AUTH_CEPHX
};

static int ceph_auth_init_protocol(struct ceph_auth_client *ac, int protocol)
{
	switch (protocol) {
	case CEPH_AUTH_NONE:
		return ceph_auth_none_init(ac);
	case CEPH_AUTH_CEPHX:
		return ceph_x_init(ac);
	default:
		return -ENOENT;
	}
}

/*
 * setup, teardown.
 */
struct ceph_auth_client *ceph_auth_init(const char *name, const struct ceph_crypto_key *key)
{
	struct ceph_auth_client *ac;
	int ret;

	dout("auth_init name '%s'\n", name);

	ret = -ENOMEM;
	ac = kzalloc(sizeof(*ac), GFP_NOFS);
	if (!ac)
		goto out;

	mutex_init(&ac->mutex);
	ac->negotiating = true;
	if (name)
		ac->name = name;
	else
		ac->name = CEPH_AUTH_NAME_DEFAULT;
	dout("auth_init name %s\n", ac->name);
	ac->key = key;
	return ac;

out:
	return ERR_PTR(ret);
}

void ceph_auth_destroy(struct ceph_auth_client *ac)
{
	dout("auth_destroy %p\n", ac);
	if (ac->ops)
		ac->ops->destroy(ac);
	kfree(ac);
}

/*
 * Reset occurs when reconnecting to the monitor.
 */
void ceph_auth_reset(struct ceph_auth_client *ac)
{
	mutex_lock(&ac->mutex);
	dout("auth_reset %p\n", ac);
	if (ac->ops && !ac->negotiating)
		ac->ops->reset(ac);
	ac->negotiating = true;
	mutex_unlock(&ac->mutex);
}

int ceph_entity_name_encode(const char *name, void **p, void *end)
{
	int len = strlen(name);

	if (*p + 2*sizeof(u32) + len > end)
		return -ERANGE;
	ceph_encode_32(p, CEPH_ENTITY_TYPE_CLIENT);
	ceph_encode_32(p, len);
	ceph_encode_copy(p, name, len);
	return 0;
}

/*
 * Initiate protocol negotiation with monitor.  Include entity name
 * and list supported protocols.
 */
int ceph_auth_build_hello(struct ceph_auth_client *ac, void *buf, size_t len)
{
	struct ceph_mon_request_header *monhdr = buf;
	void *p = monhdr + 1, *end = buf + len, *lenp;
	int i, num;
	int ret;

	mutex_lock(&ac->mutex);
	dout("auth_build_hello\n");
	monhdr->have_version = 0;
	monhdr->session_mon = cpu_to_le16(-1);
	monhdr->session_mon_tid = 0;

	ceph_encode_32(&p, 0);  /* no protocol, yet */

	lenp = p;
	p += sizeof(u32);

	ceph_decode_need(&p, end, 1 + sizeof(u32), bad);
	ceph_encode_8(&p, 1);
	num = ARRAY_SIZE(supported_protocols);
	ceph_encode_32(&p, num);
	ceph_decode_need(&p, end, num * sizeof(u32), bad);
	for (i = 0; i < num; i++)
		ceph_encode_32(&p, supported_protocols[i]);

	ret = ceph_entity_name_encode(ac->name, &p, end);
	if (ret < 0)
		goto out;
	ceph_decode_need(&p, end, sizeof(u64), bad);
	ceph_encode_64(&p, ac->global_id);

	ceph_encode_32(&lenp, p - lenp - sizeof(u32));
	ret = p - buf;
out:
	mutex_unlock(&ac->mutex);
	return ret;

bad:
	ret = -ERANGE;
	goto out;
}

static int ceph_build_auth_request(struct ceph_auth_client *ac,
				   void *msg_buf, size_t msg_len)
{
	struct ceph_mon_request_header *monhdr = msg_buf;
	void *p = monhdr + 1;
	void *end = msg_buf + msg_len;
	int ret;

	monhdr->have_version = 0;
	monhdr->session_mon = cpu_to_le16(-1);
	monhdr->session_mon_tid = 0;

	ceph_encode_32(&p, ac->protocol);

	ret = ac->ops->build_request(ac, p + sizeof(u32), end);
	if (ret < 0) {
		pr_err("error %d building auth method %s request\n", ret,
		       ac->ops->name);
		goto out;
	}
	dout(" built request %d bytes\n", ret);
	ceph_encode_32(&p, ret);
	ret = p + ret - msg_buf;
out:
	return ret;
}

/*
 * Handle auth message from monitor.
 */
int ceph_handle_auth_reply(struct ceph_auth_client *ac,
			   void *buf, size_t len,
			   void *reply_buf, size_t reply_len)
{
	void *p = buf;
	void *end = buf + len;
	int protocol;
	s32 result;
	u64 global_id;
	void *payload, *payload_end;
	int payload_len;
	char *result_msg;
	int result_msg_len;
	int ret = -EINVAL;

	mutex_lock(&ac->mutex);
	dout("handle_auth_reply %p %p\n", p, end);
	ceph_decode_need(&p, end, sizeof(u32) * 3 + sizeof(u64), bad);
	protocol = ceph_decode_32(&p);
	result = ceph_decode_32(&p);
	global_id = ceph_decode_64(&p);
	payload_len = ceph_decode_32(&p);
	payload = p;
	p += payload_len;
	ceph_decode_need(&p, end, sizeof(u32), bad);
	result_msg_len = ceph_decode_32(&p);
	result_msg = p;
	p += result_msg_len;
	if (p != end)
		goto bad;

	dout(" result %d '%.*s' gid %llu len %d\n", result, result_msg_len,
	     result_msg, global_id, payload_len);

	payload_end = payload + payload_len;

	if (global_id && ac->global_id != global_id) {
		dout(" set global_id %lld -> %lld\n", ac->global_id, global_id);
		ac->global_id = global_id;
	}

	if (ac->negotiating) {
		/* server does not support our protocols? */
		if (!protocol && result < 0) {
			ret = result;
			goto out;
		}
		/* set up (new) protocol handler? */
		if (ac->protocol && ac->protocol != protocol) {
			ac->ops->destroy(ac);
			ac->protocol = 0;
			ac->ops = NULL;
		}
		if (ac->protocol != protocol) {
			ret = ceph_auth_init_protocol(ac, protocol);
			if (ret) {
				pr_err("error %d on auth protocol %d init\n",
				       ret, protocol);
				goto out;
			}
		}

		ac->negotiating = false;
	}

	ret = ac->ops->handle_reply(ac, result, payload, payload_end);
	if (ret == -EAGAIN) {
		ret = ceph_build_auth_request(ac, reply_buf, reply_len);
	} else if (ret) {
		pr_err("auth method '%s' error %d\n", ac->ops->name, ret);
	}

out:
	mutex_unlock(&ac->mutex);
	return ret;

bad:
	pr_err("failed to decode auth msg\n");
	ret = -EINVAL;
	goto out;
}

int ceph_build_auth(struct ceph_auth_client *ac,
		    void *msg_buf, size_t msg_len)
{
	int ret = 0;

	mutex_lock(&ac->mutex);
	if (!ac->protocol)
		ret = ceph_auth_build_hello(ac, msg_buf, msg_len);
	else if (ac->ops->should_authenticate(ac))
		ret = ceph_build_auth_request(ac, msg_buf, msg_len);
	mutex_unlock(&ac->mutex);
	return ret;
}

int ceph_auth_is_authenticated(struct ceph_auth_client *ac)
{
	int ret = 0;

	mutex_lock(&ac->mutex);
	if (ac->ops)
		ret = ac->ops->is_authenticated(ac);
	mutex_unlock(&ac->mutex);
	return ret;
}
EXPORT_SYMBOL(ceph_auth_is_authenticated);

int ceph_auth_create_authorizer(struct ceph_auth_client *ac,
				int peer_type,
				struct ceph_auth_handshake *auth)
{
	int ret = 0;

	mutex_lock(&ac->mutex);
	if (ac->ops && ac->ops->create_authorizer)
		ret = ac->ops->create_authorizer(ac, peer_type, auth);
	mutex_unlock(&ac->mutex);
	return ret;
}
EXPORT_SYMBOL(ceph_auth_create_authorizer);

void ceph_auth_destroy_authorizer(struct ceph_auth_client *ac,
				  struct ceph_authorizer *a)
{
	mutex_lock(&ac->mutex);
	if (ac->ops && ac->ops->destroy_authorizer)
		ac->ops->destroy_authorizer(ac, a);
	mutex_unlock(&ac->mutex);
}
EXPORT_SYMBOL(ceph_auth_destroy_authorizer);

int ceph_auth_update_authorizer(struct ceph_auth_client *ac,
				int peer_type,
				struct ceph_auth_handshake *a)
{
	int ret = 0;

	mutex_lock(&ac->mutex);
	if (ac->ops && ac->ops->update_authorizer)
		ret = ac->ops->update_authorizer(ac, peer_type, a);
	mutex_unlock(&ac->mutex);
	return ret;
}
EXPORT_SYMBOL(ceph_auth_update_authorizer);

int ceph_auth_verify_authorizer_reply(struct ceph_auth_client *ac,
				      struct ceph_authorizer *a, size_t len)
{
	int ret = 0;

	mutex_lock(&ac->mutex);
	if (ac->ops && ac->ops->verify_authorizer_reply)
		ret = ac->ops->verify_authorizer_reply(ac, a, len);
	mutex_unlock(&ac->mutex);
	return ret;
}
EXPORT_SYMBOL(ceph_auth_verify_authorizer_reply);

void ceph_auth_invalidate_authorizer(struct ceph_auth_client *ac, int peer_type)
{
	mutex_lock(&ac->mutex);
	if (ac->ops && ac->ops->invalidate_authorizer)
		ac->ops->invalidate_authorizer(ac, peer_type);
	mutex_unlock(&ac->mutex);
}
EXPORT_SYMBOL(ceph_auth_invalidate_authorizer);
