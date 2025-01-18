/*
 * lib/bust_spinlocks.c
 *
 * Provides a minimal bust_spinlocks for architectures which don't have one of their own.
 *
 * bust_spinlocks() clears any spinlocks which would prevent oops, die(), BUG()
 * and panic() information from reaching the user.
 */

#include <fikus/kernel.h>
#include <fikus/printk.h>
#include <fikus/spinlock.h>
#include <fikus/tty.h>
#include <fikus/wait.h>
#include <fikus/vt_kern.h>
#include <fikus/console.h>


void __attribute__((weak)) bust_spinlocks(int yes)
{
	if (yes) {
		++oops_in_progress;
	} else {
#ifdef CONFIG_VT
		unblank_screen();
#endif
		console_unblank();
		if (--oops_in_progress == 0)
			wake_up_klogd();
	}
}
