/* In-software asymmetric public-key crypto subtype
 *
 * See Documentation/crypto/asymmetric-keys.txt
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#define pr_fmt(fmt) "PKEY: "fmt
#include <fikus/module.h>
#include <fikus/export.h>
#include <fikus/kernel.h>
#include <fikus/slab.h>
#include <fikus/seq_file.h>
#include <keys/asymmetric-subtype.h>
#include "public_key.h"

MODULE_LICENSE("GPL");

const char *const pkey_algo[PKEY_ALGO__LAST] = {
	[PKEY_ALGO_DSA]		= "DSA",
	[PKEY_ALGO_RSA]		= "RSA",
};
EXPORT_SYMBOL_GPL(pkey_algo);

const char *const pkey_hash_algo[PKEY_HASH__LAST] = {
	[PKEY_HASH_MD4]		= "md4",
	[PKEY_HASH_MD5]		= "md5",
	[PKEY_HASH_SHA1]	= "sha1",
	[PKEY_HASH_RIPE_MD_160]	= "rmd160",
	[PKEY_HASH_SHA256]	= "sha256",
	[PKEY_HASH_SHA384]	= "sha384",
	[PKEY_HASH_SHA512]	= "sha512",
	[PKEY_HASH_SHA224]	= "sha224",
};
EXPORT_SYMBOL_GPL(pkey_hash_algo);

const char *const pkey_id_type[PKEY_ID_TYPE__LAST] = {
	[PKEY_ID_PGP]		= "PGP",
	[PKEY_ID_X509]		= "X509",
};
EXPORT_SYMBOL_GPL(pkey_id_type);

/*
 * Provide a part of a description of the key for /proc/keys.
 */
static void public_key_describe(const struct key *asymmetric_key,
				struct seq_file *m)
{
	struct public_key *key = asymmetric_key->payload.data;

	if (key)
		seq_printf(m, "%s.%s",
			   pkey_id_type[key->id_type], key->algo->name);
}

/*
 * Destroy a public key algorithm key.
 */
void public_key_destroy(void *payload)
{
	struct public_key *key = payload;
	int i;

	if (key) {
		for (i = 0; i < ARRAY_SIZE(key->mpi); i++)
			mpi_free(key->mpi[i]);
		kfree(key);
	}
}
EXPORT_SYMBOL_GPL(public_key_destroy);

/*
 * Verify a signature using a public key.
 */
static int public_key_verify_signature(const struct key *key,
				       const struct public_key_signature *sig)
{
	const struct public_key *pk = key->payload.data;

	if (!pk->algo->verify_signature)
		return -ENOTSUPP;

	if (sig->nr_mpi != pk->algo->n_sig_mpi) {
		pr_debug("Signature has %u MPI not %u\n",
			 sig->nr_mpi, pk->algo->n_sig_mpi);
		return -EINVAL;
	}

	return pk->algo->verify_signature(pk, sig);
}

/*
 * Public key algorithm asymmetric key subtype
 */
struct asymmetric_key_subtype public_key_subtype = {
	.owner			= THIS_MODULE,
	.name			= "public_key",
	.describe		= public_key_describe,
	.destroy		= public_key_destroy,
	.verify_signature	= public_key_verify_signature,
};
EXPORT_SYMBOL_GPL(public_key_subtype);
