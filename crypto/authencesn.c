/*
 * authencesn.c - AEAD wrapper for IPsec with extended sequence numbers,
 *                 derived from authenc.c
 *
 * Copyright (C) 2010 secunet Security Networks AG
 * Copyright (C) 2010 Steffen Klassert <steffen.klassert@secunet.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include <crypto/aead.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/skcipher.h>
#include <crypto/authenc.h>
#include <crypto/scatterwalk.h>
#include <fikus/err.h>
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/rtnetlink.h>
#include <fikus/slab.h>
#include <fikus/spinlock.h>

struct authenc_esn_instance_ctx {
	struct crypto_ahash_spawn auth;
	struct crypto_skcipher_spawn enc;
};

struct crypto_authenc_esn_ctx {
	unsigned int reqoff;
	struct crypto_ahash *auth;
	struct crypto_ablkcipher *enc;
};

struct authenc_esn_request_ctx {
	unsigned int cryptlen;
	unsigned int headlen;
	unsigned int trailen;
	struct scatterlist *sg;
	struct scatterlist hsg[2];
	struct scatterlist tsg[1];
	struct scatterlist cipher[2];
	crypto_completion_t complete;
	crypto_completion_t update_complete;
	crypto_completion_t update_complete2;
	char tail[];
};

static void authenc_esn_request_complete(struct aead_request *req, int err)
{
	if (err != -EINPROGRESS)
		aead_request_complete(req, err);
}

static int crypto_authenc_esn_setkey(struct crypto_aead *authenc_esn, const u8 *key,
				     unsigned int keylen)
{
	unsigned int authkeylen;
	unsigned int enckeylen;
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct crypto_ahash *auth = ctx->auth;
	struct crypto_ablkcipher *enc = ctx->enc;
	struct rtattr *rta = (void *)key;
	struct crypto_authenc_key_param *param;
	int err = -EINVAL;

	if (!RTA_OK(rta, keylen))
		goto badkey;
	if (rta->rta_type != CRYPTO_AUTHENC_KEYA_PARAM)
		goto badkey;
	if (RTA_PAYLOAD(rta) < sizeof(*param))
		goto badkey;

	param = RTA_DATA(rta);
	enckeylen = be32_to_cpu(param->enckeylen);

	key += RTA_ALIGN(rta->rta_len);
	keylen -= RTA_ALIGN(rta->rta_len);

	if (keylen < enckeylen)
		goto badkey;

	authkeylen = keylen - enckeylen;

	crypto_ahash_clear_flags(auth, CRYPTO_TFM_REQ_MASK);
	crypto_ahash_set_flags(auth, crypto_aead_get_flags(authenc_esn) &
				     CRYPTO_TFM_REQ_MASK);
	err = crypto_ahash_setkey(auth, key, authkeylen);
	crypto_aead_set_flags(authenc_esn, crypto_ahash_get_flags(auth) &
					   CRYPTO_TFM_RES_MASK);

	if (err)
		goto out;

	crypto_ablkcipher_clear_flags(enc, CRYPTO_TFM_REQ_MASK);
	crypto_ablkcipher_set_flags(enc, crypto_aead_get_flags(authenc_esn) &
					 CRYPTO_TFM_REQ_MASK);
	err = crypto_ablkcipher_setkey(enc, key + authkeylen, enckeylen);
	crypto_aead_set_flags(authenc_esn, crypto_ablkcipher_get_flags(enc) &
					   CRYPTO_TFM_RES_MASK);

out:
	return err;

badkey:
	crypto_aead_set_flags(authenc_esn, CRYPTO_TFM_RES_BAD_KEY_LEN);
	goto out;
}

static void authenc_esn_geniv_ahash_update_done(struct crypto_async_request *areq,
						int err)
{
	struct aead_request *req = areq->data;
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct ahash_request *ahreq = (void *)(areq_ctx->tail + ctx->reqoff);

	if (err)
		goto out;

	ahash_request_set_crypt(ahreq, areq_ctx->sg, ahreq->result,
				areq_ctx->cryptlen);
	ahash_request_set_callback(ahreq, aead_request_flags(req) &
					  CRYPTO_TFM_REQ_MAY_SLEEP,
				   areq_ctx->update_complete2, req);

	err = crypto_ahash_update(ahreq);
	if (err)
		goto out;

	ahash_request_set_crypt(ahreq, areq_ctx->tsg, ahreq->result,
				areq_ctx->trailen);
	ahash_request_set_callback(ahreq, aead_request_flags(req) &
					  CRYPTO_TFM_REQ_MAY_SLEEP,
				   areq_ctx->complete, req);

	err = crypto_ahash_finup(ahreq);
	if (err)
		goto out;

	scatterwalk_map_and_copy(ahreq->result, areq_ctx->sg,
				 areq_ctx->cryptlen,
				 crypto_aead_authsize(authenc_esn), 1);

out:
	authenc_esn_request_complete(req, err);
}

static void authenc_esn_geniv_ahash_update_done2(struct crypto_async_request *areq,
						 int err)
{
	struct aead_request *req = areq->data;
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct ahash_request *ahreq = (void *)(areq_ctx->tail + ctx->reqoff);

	if (err)
		goto out;

	ahash_request_set_crypt(ahreq, areq_ctx->tsg, ahreq->result,
				areq_ctx->trailen);
	ahash_request_set_callback(ahreq, aead_request_flags(req) &
					  CRYPTO_TFM_REQ_MAY_SLEEP,
				   areq_ctx->complete, req);

	err = crypto_ahash_finup(ahreq);
	if (err)
		goto out;

	scatterwalk_map_and_copy(ahreq->result, areq_ctx->sg,
				 areq_ctx->cryptlen,
				 crypto_aead_authsize(authenc_esn), 1);

out:
	authenc_esn_request_complete(req, err);
}


static void authenc_esn_geniv_ahash_done(struct crypto_async_request *areq,
					 int err)
{
	struct aead_request *req = areq->data;
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct ahash_request *ahreq = (void *)(areq_ctx->tail + ctx->reqoff);

	if (err)
		goto out;

	scatterwalk_map_and_copy(ahreq->result, areq_ctx->sg,
				 areq_ctx->cryptlen,
				 crypto_aead_authsize(authenc_esn), 1);

out:
	aead_request_complete(req, err);
}


static void authenc_esn_verify_ahash_update_done(struct crypto_async_request *areq,
						 int err)
{
	u8 *ihash;
	unsigned int authsize;
	struct ablkcipher_request *abreq;
	struct aead_request *req = areq->data;
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct ahash_request *ahreq = (void *)(areq_ctx->tail + ctx->reqoff);
	unsigned int cryptlen = req->cryptlen;

	if (err)
		goto out;

	ahash_request_set_crypt(ahreq, areq_ctx->sg, ahreq->result,
				areq_ctx->cryptlen);

	ahash_request_set_callback(ahreq,
				   aead_request_flags(req) &
				   CRYPTO_TFM_REQ_MAY_SLEEP,
				   areq_ctx->update_complete2, req);

	err = crypto_ahash_update(ahreq);
	if (err)
		goto out;

	ahash_request_set_crypt(ahreq, areq_ctx->tsg, ahreq->result,
				areq_ctx->trailen);
	ahash_request_set_callback(ahreq, aead_request_flags(req) &
					  CRYPTO_TFM_REQ_MAY_SLEEP,
				   areq_ctx->complete, req);

	err = crypto_ahash_finup(ahreq);
	if (err)
		goto out;

	authsize = crypto_aead_authsize(authenc_esn);
	cryptlen -= authsize;
	ihash = ahreq->result + authsize;
	scatterwalk_map_and_copy(ihash, areq_ctx->sg, areq_ctx->cryptlen,
				 authsize, 0);

	err = memcmp(ihash, ahreq->result, authsize) ? -EBADMSG : 0;
	if (err)
		goto out;

	abreq = aead_request_ctx(req);
	ablkcipher_request_set_tfm(abreq, ctx->enc);
	ablkcipher_request_set_callback(abreq, aead_request_flags(req),
					req->base.complete, req->base.data);
	ablkcipher_request_set_crypt(abreq, req->src, req->dst,
				     cryptlen, req->iv);

	err = crypto_ablkcipher_decrypt(abreq);

out:
	authenc_esn_request_complete(req, err);
}

static void authenc_esn_verify_ahash_update_done2(struct crypto_async_request *areq,
						  int err)
{
	u8 *ihash;
	unsigned int authsize;
	struct ablkcipher_request *abreq;
	struct aead_request *req = areq->data;
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct ahash_request *ahreq = (void *)(areq_ctx->tail + ctx->reqoff);
	unsigned int cryptlen = req->cryptlen;

	if (err)
		goto out;

	ahash_request_set_crypt(ahreq, areq_ctx->tsg, ahreq->result,
				areq_ctx->trailen);
	ahash_request_set_callback(ahreq, aead_request_flags(req) &
					  CRYPTO_TFM_REQ_MAY_SLEEP,
				   areq_ctx->complete, req);

	err = crypto_ahash_finup(ahreq);
	if (err)
		goto out;

	authsize = crypto_aead_authsize(authenc_esn);
	cryptlen -= authsize;
	ihash = ahreq->result + authsize;
	scatterwalk_map_and_copy(ihash, areq_ctx->sg, areq_ctx->cryptlen,
				 authsize, 0);

	err = memcmp(ihash, ahreq->result, authsize) ? -EBADMSG : 0;
	if (err)
		goto out;

	abreq = aead_request_ctx(req);
	ablkcipher_request_set_tfm(abreq, ctx->enc);
	ablkcipher_request_set_callback(abreq, aead_request_flags(req),
					req->base.complete, req->base.data);
	ablkcipher_request_set_crypt(abreq, req->src, req->dst,
				     cryptlen, req->iv);

	err = crypto_ablkcipher_decrypt(abreq);

out:
	authenc_esn_request_complete(req, err);
}


static void authenc_esn_verify_ahash_done(struct crypto_async_request *areq,
					  int err)
{
	u8 *ihash;
	unsigned int authsize;
	struct ablkcipher_request *abreq;
	struct aead_request *req = areq->data;
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct ahash_request *ahreq = (void *)(areq_ctx->tail + ctx->reqoff);
	unsigned int cryptlen = req->cryptlen;

	if (err)
		goto out;

	authsize = crypto_aead_authsize(authenc_esn);
	cryptlen -= authsize;
	ihash = ahreq->result + authsize;
	scatterwalk_map_and_copy(ihash, areq_ctx->sg, areq_ctx->cryptlen,
				 authsize, 0);

	err = memcmp(ihash, ahreq->result, authsize) ? -EBADMSG : 0;
	if (err)
		goto out;

	abreq = aead_request_ctx(req);
	ablkcipher_request_set_tfm(abreq, ctx->enc);
	ablkcipher_request_set_callback(abreq, aead_request_flags(req),
					req->base.complete, req->base.data);
	ablkcipher_request_set_crypt(abreq, req->src, req->dst,
				     cryptlen, req->iv);

	err = crypto_ablkcipher_decrypt(abreq);

out:
	authenc_esn_request_complete(req, err);
}

static u8 *crypto_authenc_esn_ahash(struct aead_request *req,
				    unsigned int flags)
{
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct crypto_ahash *auth = ctx->auth;
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct ahash_request *ahreq = (void *)(areq_ctx->tail + ctx->reqoff);
	u8 *hash = areq_ctx->tail;
	int err;

	hash = (u8 *)ALIGN((unsigned long)hash + crypto_ahash_alignmask(auth),
			    crypto_ahash_alignmask(auth) + 1);

	ahash_request_set_tfm(ahreq, auth);

	err = crypto_ahash_init(ahreq);
	if (err)
		return ERR_PTR(err);

	ahash_request_set_crypt(ahreq, areq_ctx->hsg, hash, areq_ctx->headlen);
	ahash_request_set_callback(ahreq, aead_request_flags(req) & flags,
				   areq_ctx->update_complete, req);

	err = crypto_ahash_update(ahreq);
	if (err)
		return ERR_PTR(err);

	ahash_request_set_crypt(ahreq, areq_ctx->sg, hash, areq_ctx->cryptlen);
	ahash_request_set_callback(ahreq, aead_request_flags(req) & flags,
				   areq_ctx->update_complete2, req);

	err = crypto_ahash_update(ahreq);
	if (err)
		return ERR_PTR(err);

	ahash_request_set_crypt(ahreq, areq_ctx->tsg, hash,
				areq_ctx->trailen);
	ahash_request_set_callback(ahreq, aead_request_flags(req) & flags,
				   areq_ctx->complete, req);

	err = crypto_ahash_finup(ahreq);
	if (err)
		return ERR_PTR(err);

	return hash;
}

static int crypto_authenc_esn_genicv(struct aead_request *req, u8 *iv,
				     unsigned int flags)
{
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct scatterlist *dst = req->dst;
	struct scatterlist *assoc = req->assoc;
	struct scatterlist *cipher = areq_ctx->cipher;
	struct scatterlist *hsg = areq_ctx->hsg;
	struct scatterlist *tsg = areq_ctx->tsg;
	struct scatterlist *assoc1;
	struct scatterlist *assoc2;
	unsigned int ivsize = crypto_aead_ivsize(authenc_esn);
	unsigned int cryptlen = req->cryptlen;
	struct page *dstp;
	u8 *vdst;
	u8 *hash;

	dstp = sg_page(dst);
	vdst = PageHighMem(dstp) ? NULL : page_address(dstp) + dst->offset;

	if (ivsize) {
		sg_init_table(cipher, 2);
		sg_set_buf(cipher, iv, ivsize);
		scatterwalk_crypto_chain(cipher, dst, vdst == iv + ivsize, 2);
		dst = cipher;
		cryptlen += ivsize;
	}

	if (sg_is_last(assoc))
		return -EINVAL;

	assoc1 = assoc + 1;
	if (sg_is_last(assoc1))
		return -EINVAL;

	assoc2 = assoc + 2;
	if (!sg_is_last(assoc2))
		return -EINVAL;

	sg_init_table(hsg, 2);
	sg_set_page(hsg, sg_page(assoc), assoc->length, assoc->offset);
	sg_set_page(hsg + 1, sg_page(assoc2), assoc2->length, assoc2->offset);

	sg_init_table(tsg, 1);
	sg_set_page(tsg, sg_page(assoc1), assoc1->length, assoc1->offset);

	areq_ctx->cryptlen = cryptlen;
	areq_ctx->headlen = assoc->length + assoc2->length;
	areq_ctx->trailen = assoc1->length;
	areq_ctx->sg = dst;

	areq_ctx->complete = authenc_esn_geniv_ahash_done;
	areq_ctx->update_complete = authenc_esn_geniv_ahash_update_done;
	areq_ctx->update_complete2 = authenc_esn_geniv_ahash_update_done2;

	hash = crypto_authenc_esn_ahash(req, flags);
	if (IS_ERR(hash))
		return PTR_ERR(hash);

	scatterwalk_map_and_copy(hash, dst, cryptlen,
				 crypto_aead_authsize(authenc_esn), 1);
	return 0;
}


static void crypto_authenc_esn_encrypt_done(struct crypto_async_request *req,
					    int err)
{
	struct aead_request *areq = req->data;

	if (!err) {
		struct crypto_aead *authenc_esn = crypto_aead_reqtfm(areq);
		struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
		struct ablkcipher_request *abreq = aead_request_ctx(areq);
		u8 *iv = (u8 *)(abreq + 1) +
			 crypto_ablkcipher_reqsize(ctx->enc);

		err = crypto_authenc_esn_genicv(areq, iv, 0);
	}

	authenc_esn_request_complete(areq, err);
}

static int crypto_authenc_esn_encrypt(struct aead_request *req)
{
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct crypto_ablkcipher *enc = ctx->enc;
	struct scatterlist *dst = req->dst;
	unsigned int cryptlen = req->cryptlen;
	struct ablkcipher_request *abreq = (void *)(areq_ctx->tail
						    + ctx->reqoff);
	u8 *iv = (u8 *)abreq - crypto_ablkcipher_ivsize(enc);
	int err;

	ablkcipher_request_set_tfm(abreq, enc);
	ablkcipher_request_set_callback(abreq, aead_request_flags(req),
					crypto_authenc_esn_encrypt_done, req);
	ablkcipher_request_set_crypt(abreq, req->src, dst, cryptlen, req->iv);

	memcpy(iv, req->iv, crypto_aead_ivsize(authenc_esn));

	err = crypto_ablkcipher_encrypt(abreq);
	if (err)
		return err;

	return crypto_authenc_esn_genicv(req, iv, CRYPTO_TFM_REQ_MAY_SLEEP);
}

static void crypto_authenc_esn_givencrypt_done(struct crypto_async_request *req,
					       int err)
{
	struct aead_request *areq = req->data;

	if (!err) {
		struct skcipher_givcrypt_request *greq = aead_request_ctx(areq);

		err = crypto_authenc_esn_genicv(areq, greq->giv, 0);
	}

	authenc_esn_request_complete(areq, err);
}

static int crypto_authenc_esn_givencrypt(struct aead_givcrypt_request *req)
{
	struct crypto_aead *authenc_esn = aead_givcrypt_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct aead_request *areq = &req->areq;
	struct skcipher_givcrypt_request *greq = aead_request_ctx(areq);
	u8 *iv = req->giv;
	int err;

	skcipher_givcrypt_set_tfm(greq, ctx->enc);
	skcipher_givcrypt_set_callback(greq, aead_request_flags(areq),
				       crypto_authenc_esn_givencrypt_done, areq);
	skcipher_givcrypt_set_crypt(greq, areq->src, areq->dst, areq->cryptlen,
				    areq->iv);
	skcipher_givcrypt_set_giv(greq, iv, req->seq);

	err = crypto_skcipher_givencrypt(greq);
	if (err)
		return err;

	return crypto_authenc_esn_genicv(areq, iv, CRYPTO_TFM_REQ_MAY_SLEEP);
}

static int crypto_authenc_esn_verify(struct aead_request *req)
{
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	u8 *ohash;
	u8 *ihash;
	unsigned int authsize;

	areq_ctx->complete = authenc_esn_verify_ahash_done;
	areq_ctx->update_complete = authenc_esn_verify_ahash_update_done;

	ohash = crypto_authenc_esn_ahash(req, CRYPTO_TFM_REQ_MAY_SLEEP);
	if (IS_ERR(ohash))
		return PTR_ERR(ohash);

	authsize = crypto_aead_authsize(authenc_esn);
	ihash = ohash + authsize;
	scatterwalk_map_and_copy(ihash, areq_ctx->sg, areq_ctx->cryptlen,
				 authsize, 0);
	return memcmp(ihash, ohash, authsize) ? -EBADMSG : 0;
}

static int crypto_authenc_esn_iverify(struct aead_request *req, u8 *iv,
				      unsigned int cryptlen)
{
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct authenc_esn_request_ctx *areq_ctx = aead_request_ctx(req);
	struct scatterlist *src = req->src;
	struct scatterlist *assoc = req->assoc;
	struct scatterlist *cipher = areq_ctx->cipher;
	struct scatterlist *hsg = areq_ctx->hsg;
	struct scatterlist *tsg = areq_ctx->tsg;
	struct scatterlist *assoc1;
	struct scatterlist *assoc2;
	unsigned int ivsize = crypto_aead_ivsize(authenc_esn);
	struct page *srcp;
	u8 *vsrc;

	srcp = sg_page(src);
	vsrc = PageHighMem(srcp) ? NULL : page_address(srcp) + src->offset;

	if (ivsize) {
		sg_init_table(cipher, 2);
		sg_set_buf(cipher, iv, ivsize);
		scatterwalk_crypto_chain(cipher, src, vsrc == iv + ivsize, 2);
		src = cipher;
		cryptlen += ivsize;
	}

	if (sg_is_last(assoc))
		return -EINVAL;

	assoc1 = assoc + 1;
	if (sg_is_last(assoc1))
		return -EINVAL;

	assoc2 = assoc + 2;
	if (!sg_is_last(assoc2))
		return -EINVAL;

	sg_init_table(hsg, 2);
	sg_set_page(hsg, sg_page(assoc), assoc->length, assoc->offset);
	sg_set_page(hsg + 1, sg_page(assoc2), assoc2->length, assoc2->offset);

	sg_init_table(tsg, 1);
	sg_set_page(tsg, sg_page(assoc1), assoc1->length, assoc1->offset);

	areq_ctx->cryptlen = cryptlen;
	areq_ctx->headlen = assoc->length + assoc2->length;
	areq_ctx->trailen = assoc1->length;
	areq_ctx->sg = src;

	areq_ctx->complete = authenc_esn_verify_ahash_done;
	areq_ctx->update_complete = authenc_esn_verify_ahash_update_done;
	areq_ctx->update_complete2 = authenc_esn_verify_ahash_update_done2;

	return crypto_authenc_esn_verify(req);
}

static int crypto_authenc_esn_decrypt(struct aead_request *req)
{
	struct crypto_aead *authenc_esn = crypto_aead_reqtfm(req);
	struct crypto_authenc_esn_ctx *ctx = crypto_aead_ctx(authenc_esn);
	struct ablkcipher_request *abreq = aead_request_ctx(req);
	unsigned int cryptlen = req->cryptlen;
	unsigned int authsize = crypto_aead_authsize(authenc_esn);
	u8 *iv = req->iv;
	int err;

	if (cryptlen < authsize)
		return -EINVAL;
	cryptlen -= authsize;

	err = crypto_authenc_esn_iverify(req, iv, cryptlen);
	if (err)
		return err;

	ablkcipher_request_set_tfm(abreq, ctx->enc);
	ablkcipher_request_set_callback(abreq, aead_request_flags(req),
					req->base.complete, req->base.data);
	ablkcipher_request_set_crypt(abreq, req->src, req->dst, cryptlen, iv);

	return crypto_ablkcipher_decrypt(abreq);
}

static int crypto_authenc_esn_init_tfm(struct crypto_tfm *tfm)
{
	struct crypto_instance *inst = crypto_tfm_alg_instance(tfm);
	struct authenc_esn_instance_ctx *ictx = crypto_instance_ctx(inst);
	struct crypto_authenc_esn_ctx *ctx = crypto_tfm_ctx(tfm);
	struct crypto_ahash *auth;
	struct crypto_ablkcipher *enc;
	int err;

	auth = crypto_spawn_ahash(&ictx->auth);
	if (IS_ERR(auth))
		return PTR_ERR(auth);

	enc = crypto_spawn_skcipher(&ictx->enc);
	err = PTR_ERR(enc);
	if (IS_ERR(enc))
		goto err_free_ahash;

	ctx->auth = auth;
	ctx->enc = enc;

	ctx->reqoff = ALIGN(2 * crypto_ahash_digestsize(auth) +
			    crypto_ahash_alignmask(auth),
			    crypto_ahash_alignmask(auth) + 1) +
		      crypto_ablkcipher_ivsize(enc);

	tfm->crt_aead.reqsize = sizeof(struct authenc_esn_request_ctx) +
				ctx->reqoff +
				max_t(unsigned int,
				crypto_ahash_reqsize(auth) +
				sizeof(struct ahash_request),
				sizeof(struct skcipher_givcrypt_request) +
				crypto_ablkcipher_reqsize(enc));

	return 0;

err_free_ahash:
	crypto_free_ahash(auth);
	return err;
}

static void crypto_authenc_esn_exit_tfm(struct crypto_tfm *tfm)
{
	struct crypto_authenc_esn_ctx *ctx = crypto_tfm_ctx(tfm);

	crypto_free_ahash(ctx->auth);
	crypto_free_ablkcipher(ctx->enc);
}

static struct crypto_instance *crypto_authenc_esn_alloc(struct rtattr **tb)
{
	struct crypto_attr_type *algt;
	struct crypto_instance *inst;
	struct hash_alg_common *auth;
	struct crypto_alg *auth_base;
	struct crypto_alg *enc;
	struct authenc_esn_instance_ctx *ctx;
	const char *enc_name;
	int err;

	algt = crypto_get_attr_type(tb);
	if (IS_ERR(algt))
		return ERR_CAST(algt);

	if ((algt->type ^ CRYPTO_ALG_TYPE_AEAD) & algt->mask)
		return ERR_PTR(-EINVAL);

	auth = ahash_attr_alg(tb[1], CRYPTO_ALG_TYPE_HASH,
			       CRYPTO_ALG_TYPE_AHASH_MASK);
	if (IS_ERR(auth))
		return ERR_CAST(auth);

	auth_base = &auth->base;

	enc_name = crypto_attr_alg_name(tb[2]);
	err = PTR_ERR(enc_name);
	if (IS_ERR(enc_name))
		goto out_put_auth;

	inst = kzalloc(sizeof(*inst) + sizeof(*ctx), GFP_KERNEL);
	err = -ENOMEM;
	if (!inst)
		goto out_put_auth;

	ctx = crypto_instance_ctx(inst);

	err = crypto_init_ahash_spawn(&ctx->auth, auth, inst);
	if (err)
		goto err_free_inst;

	crypto_set_skcipher_spawn(&ctx->enc, inst);
	err = crypto_grab_skcipher(&ctx->enc, enc_name, 0,
				   crypto_requires_sync(algt->type,
							algt->mask));
	if (err)
		goto err_drop_auth;

	enc = crypto_skcipher_spawn_alg(&ctx->enc);

	err = -ENAMETOOLONG;
	if (snprintf(inst->alg.cra_name, CRYPTO_MAX_ALG_NAME,
		     "authencesn(%s,%s)", auth_base->cra_name, enc->cra_name) >=
	    CRYPTO_MAX_ALG_NAME)
		goto err_drop_enc;

	if (snprintf(inst->alg.cra_driver_name, CRYPTO_MAX_ALG_NAME,
		     "authencesn(%s,%s)", auth_base->cra_driver_name,
		     enc->cra_driver_name) >= CRYPTO_MAX_ALG_NAME)
		goto err_drop_enc;

	inst->alg.cra_flags = CRYPTO_ALG_TYPE_AEAD;
	inst->alg.cra_flags |= enc->cra_flags & CRYPTO_ALG_ASYNC;
	inst->alg.cra_priority = enc->cra_priority *
				 10 + auth_base->cra_priority;
	inst->alg.cra_blocksize = enc->cra_blocksize;
	inst->alg.cra_alignmask = auth_base->cra_alignmask | enc->cra_alignmask;
	inst->alg.cra_type = &crypto_aead_type;

	inst->alg.cra_aead.ivsize = enc->cra_ablkcipher.ivsize;
	inst->alg.cra_aead.maxauthsize = auth->digestsize;

	inst->alg.cra_ctxsize = sizeof(struct crypto_authenc_esn_ctx);

	inst->alg.cra_init = crypto_authenc_esn_init_tfm;
	inst->alg.cra_exit = crypto_authenc_esn_exit_tfm;

	inst->alg.cra_aead.setkey = crypto_authenc_esn_setkey;
	inst->alg.cra_aead.encrypt = crypto_authenc_esn_encrypt;
	inst->alg.cra_aead.decrypt = crypto_authenc_esn_decrypt;
	inst->alg.cra_aead.givencrypt = crypto_authenc_esn_givencrypt;

out:
	crypto_mod_put(auth_base);
	return inst;

err_drop_enc:
	crypto_drop_skcipher(&ctx->enc);
err_drop_auth:
	crypto_drop_ahash(&ctx->auth);
err_free_inst:
	kfree(inst);
out_put_auth:
	inst = ERR_PTR(err);
	goto out;
}

static void crypto_authenc_esn_free(struct crypto_instance *inst)
{
	struct authenc_esn_instance_ctx *ctx = crypto_instance_ctx(inst);

	crypto_drop_skcipher(&ctx->enc);
	crypto_drop_ahash(&ctx->auth);
	kfree(inst);
}

static struct crypto_template crypto_authenc_esn_tmpl = {
	.name = "authencesn",
	.alloc = crypto_authenc_esn_alloc,
	.free = crypto_authenc_esn_free,
	.module = THIS_MODULE,
};

static int __init crypto_authenc_esn_module_init(void)
{
	return crypto_register_template(&crypto_authenc_esn_tmpl);
}

static void __exit crypto_authenc_esn_module_exit(void)
{
	crypto_unregister_template(&crypto_authenc_esn_tmpl);
}

module_init(crypto_authenc_esn_module_init);
module_exit(crypto_authenc_esn_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Steffen Klassert <steffen.klassert@secunet.com>");
MODULE_DESCRIPTION("AEAD wrapper for IPsec with extended sequence numbers");
