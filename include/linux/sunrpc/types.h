/*
 * fikus/include/fikus/sunrpc/types.h
 *
 * Generic types and misc stuff for RPC.
 *
 * Copyright (C) 1996, Olaf Kirch <okir@monad.swb.de>
 */

#ifndef _FIKUS_SUNRPC_TYPES_H_
#define _FIKUS_SUNRPC_TYPES_H_

#include <fikus/timer.h>
#include <fikus/workqueue.h>
#include <fikus/sunrpc/debug.h>
#include <fikus/list.h>

/*
 * Shorthands
 */
#define signalled()		(signal_pending(current))

#endif /* _FIKUS_SUNRPC_TYPES_H_ */
