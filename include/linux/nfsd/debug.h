/*
 * fikus/include/fikus/nfsd/debug.h
 *
 * Debugging-related stuff for nfsd
 *
 * Copyright (C) 1995 Olaf Kirch <okir@monad.swb.de>
 */
#ifndef FIKUS_NFSD_DEBUG_H
#define FIKUS_NFSD_DEBUG_H

#include <uapi/fikus/nfsd/debug.h>

# undef ifdebug
# ifdef NFSD_DEBUG
#  define ifdebug(flag)		if (nfsd_debug & NFSDDBG_##flag)
# else
#  define ifdebug(flag)		if (0)
# endif
#endif /* FIKUS_NFSD_DEBUG_H */
