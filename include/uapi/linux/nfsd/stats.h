/*
 * fikus/include/fikus/nfsd/stats.h
 *
 * Statistics for NFS server.
 *
 * Copyright (C) 1995, 1996 Olaf Kirch <okir@monad.swb.de>
 */

#ifndef _UAPIFIKUS_NFSD_STATS_H
#define _UAPIFIKUS_NFSD_STATS_H

#include <fikus/nfs4.h>

/* thread usage wraps very million seconds (approx one fortnight) */
#define	NFSD_USAGE_WRAP	(HZ*1000000)

#endif /* _UAPIFIKUS_NFSD_STATS_H */
