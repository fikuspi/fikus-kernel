#ifndef _PARISC_CURRENT_H
#define _PARISC_CURRENT_H

#include <fikus/thread_info.h>

struct task_struct;

static inline struct task_struct * get_current(void)
{
	return current_thread_info()->task;
}
 
#define current get_current()

#endif /* !(_PARISC_CURRENT_H) */
