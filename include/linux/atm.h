/* atm.h - general ATM declarations */
#ifndef _FIKUS_ATM_H
#define _FIKUS_ATM_H

#include <uapi/fikus/atm.h>

#ifdef CONFIG_COMPAT
#include <fikus/compat.h>
struct compat_atmif_sioc {
	int number;
	int length;
	compat_uptr_t arg;
};
#endif
#endif
