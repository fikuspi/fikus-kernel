/* 
 * Copyright (C) 2000 - 2007 Jeff Dike (jdike@{addtoit,fikus.intel}.com)
 * Licensed under the GPL
 */

#include <fikus/sched.h>
#include <fikus/spinlock.h>
#include <fikus/slab.h>
#include <fikus/oom.h>
#include <kern_util.h>
#include <os.h>
#include <skas.h>

void (*pm_power_off)(void);

static void kill_off_processes(void)
{
	if (proc_mm)
		/*
		 * FIXME: need to loop over userspace_pids
		 */
		os_kill_ptraced_process(userspace_pid[0], 1);
	else {
		struct task_struct *p;
		int pid;

		read_lock(&tasklist_lock);
		for_each_process(p) {
			struct task_struct *t;

			t = find_lock_task_mm(p);
			if (!t)
				continue;
			pid = t->mm->context.id.u.pid;
			task_unlock(t);
			os_kill_ptraced_process(pid, 1);
		}
		read_unlock(&tasklist_lock);
	}
}

void uml_cleanup(void)
{
	kmalloc_ok = 0;
	do_uml_exitcalls();
	kill_off_processes();
}

void machine_restart(char * __unused)
{
	uml_cleanup();
	reboot_skas();
}

void machine_power_off(void)
{
	uml_cleanup();
	halt_skas();
}

void machine_halt(void)
{
	machine_power_off();
}
