/*
 *  include/fikus/signalfd.h
 *
 *  Copyright (C) 2007  Davide Libenzi <davidel@xmailserver.org>
 *
 */
#ifndef _FIKUS_SIGNALFD_H
#define _FIKUS_SIGNALFD_H

#include <uapi/fikus/signalfd.h>


#ifdef CONFIG_SIGNALFD

/*
 * Deliver the signal to listening signalfd.
 */
static inline void signalfd_notify(struct task_struct *tsk, int sig)
{
	if (unlikely(waitqueue_active(&tsk->sighand->signalfd_wqh)))
		wake_up(&tsk->sighand->signalfd_wqh);
}

extern void signalfd_cleanup(struct sighand_struct *sighand);

#else /* CONFIG_SIGNALFD */

static inline void signalfd_notify(struct task_struct *tsk, int sig) { }

static inline void signalfd_cleanup(struct sighand_struct *sighand) { }

#endif /* CONFIG_SIGNALFD */

#endif /* _FIKUS_SIGNALFD_H */
