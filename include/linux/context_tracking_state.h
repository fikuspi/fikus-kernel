#ifndef _FIKUS_CONTEXT_TRACKING_STATE_H
#define _FIKUS_CONTEXT_TRACKING_STATE_H

#include <fikus/percpu.h>
#include <fikus/static_key.h>

struct context_tracking {
	/*
	 * When active is false, probes are unset in order
	 * to minimize overhead: TIF flags are cleared
	 * and calls to user_enter/exit are ignored. This
	 * may be further optimized using static keys.
	 */
	bool active;
	enum ctx_state {
		IN_KERNEL = 0,
		IN_USER,
	} state;
};

#ifdef CONFIG_CONTEXT_TRACKING
extern struct static_key context_tracking_enabled;
DECLARE_PER_CPU(struct context_tracking, context_tracking);

static inline bool context_tracking_in_user(void)
{
	return __this_cpu_read(context_tracking.state) == IN_USER;
}

static inline bool context_tracking_active(void)
{
	return __this_cpu_read(context_tracking.active);
}
#else
static inline bool context_tracking_in_user(void) { return false; }
static inline bool context_tracking_active(void) { return false; }
#endif /* CONFIG_CONTEXT_TRACKING */

#endif
