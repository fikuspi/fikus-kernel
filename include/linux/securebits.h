#ifndef _FIKUS_SECUREBITS_H
#define _FIKUS_SECUREBITS_H 1

#include <uapi/fikus/securebits.h>

#define issecure(X)		(issecure_mask(X) & current_cred_xxx(securebits))
#endif /* !_FIKUS_SECUREBITS_H */
