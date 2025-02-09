/* Asymmetric public-key algorithm definitions
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

#ifndef _FIKUS_PUBLIC_KEY_H
#define _FIKUS_PUBLIC_KEY_H

#include <fikus/mpi.h>

enum pkey_algo {
	PKEY_ALGO_DSA,
	PKEY_ALGO_RSA,
	PKEY_ALGO__LAST
};

extern const char *const pkey_algo[PKEY_ALGO__LAST];

enum pkey_hash_algo {
	PKEY_HASH_MD4,
	PKEY_HASH_MD5,
	PKEY_HASH_SHA1,
	PKEY_HASH_RIPE_MD_160,
	PKEY_HASH_SHA256,
	PKEY_HASH_SHA384,
	PKEY_HASH_SHA512,
	PKEY_HASH_SHA224,
	PKEY_HASH__LAST
};

extern const char *const pkey_hash_algo[PKEY_HASH__LAST];

enum pkey_id_type {
	PKEY_ID_PGP,		/* OpenPGP generated key ID */
	PKEY_ID_X509,		/* X.509 arbitrary subjectKeyIdentifier */
	PKEY_ID_TYPE__LAST
};

extern const char *const pkey_id_type[PKEY_ID_TYPE__LAST];

/*
 * Cryptographic data for the public-key subtype of the asymmetric key type.
 *
 * Note that this may include private part of the key as well as the public
 * part.
 */
struct public_key {
	const struct public_key_algorithm *algo;
	u8	capabilities;
#define PKEY_CAN_ENCRYPT	0x01
#define PKEY_CAN_DECRYPT	0x02
#define PKEY_CAN_SIGN		0x04
#define PKEY_CAN_VERIFY		0x08
	enum pkey_id_type id_type : 8;
	union {
		MPI	mpi[5];
		struct {
			MPI	p;	/* DSA prime */
			MPI	q;	/* DSA group order */
			MPI	g;	/* DSA group generator */
			MPI	y;	/* DSA public-key value = g^x mod p */
			MPI	x;	/* DSA secret exponent (if present) */
		} dsa;
		struct {
			MPI	n;	/* RSA public modulus */
			MPI	e;	/* RSA public encryption exponent */
			MPI	d;	/* RSA secret encryption exponent (if present) */
			MPI	p;	/* RSA secret prime (if present) */
			MPI	q;	/* RSA secret prime (if present) */
		} rsa;
	};
};

extern void public_key_destroy(void *payload);

/*
 * Public key cryptography signature data
 */
struct public_key_signature {
	u8 *digest;
	u8 digest_size;			/* Number of bytes in digest */
	u8 nr_mpi;			/* Occupancy of mpi[] */
	enum pkey_hash_algo pkey_hash_algo : 8;
	union {
		MPI mpi[2];
		struct {
			MPI s;		/* m^d mod n */
		} rsa;
		struct {
			MPI r;
			MPI s;
		} dsa;
	};
};

struct key;
extern int verify_signature(const struct key *key,
			    const struct public_key_signature *sig);

#endif /* _FIKUS_PUBLIC_KEY_H */
