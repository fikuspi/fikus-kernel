/* atmppp.h - RFC2364 PPPoATM */

/* Written 2000 by Mitchell Blank Jr */

#ifndef _FIKUS_ATMPPP_H
#define _FIKUS_ATMPPP_H

#include <fikus/atm.h>

#define PPPOATM_ENCAPS_AUTODETECT	(0)
#define PPPOATM_ENCAPS_VC		(1)
#define PPPOATM_ENCAPS_LLC		(2)

/*
 * This is for the ATM_SETBACKEND call - these are like socket families:
 * the first element of the structure is the backend number and the rest
 * is per-backend specific
 */
struct atm_backend_ppp {
	atm_backend_t	backend_num;	/* ATM_BACKEND_PPP */
	int		encaps;		/* PPPOATM_ENCAPS_* */
};

#endif	/* _FIKUS_ATMPPP_H */
