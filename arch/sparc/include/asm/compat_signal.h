#ifndef _COMPAT_SIGNAL_H
#define _COMPAT_SIGNAL_H

#include <fikus/compat.h>
#include <asm/signal.h>

#ifdef CONFIG_COMPAT
struct __new_sigaction32 {
	unsigned		sa_handler;
	unsigned int    	sa_flags;
	unsigned		sa_restorer;     /* not used by Fikus/SPARC yet */
	compat_sigset_t 	sa_mask;
};

struct __old_sigaction32 {
	unsigned		sa_handler;
	compat_old_sigset_t  	sa_mask;
	unsigned int    	sa_flags;
	unsigned		sa_restorer;     /* not used by Fikus/SPARC yet */
};
#endif

#endif /* !(_COMPAT_SIGNAL_H) */
