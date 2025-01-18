/*
 * Platform dependent support for HP simulator.
 *
 * Copyright (C) 1998, 1999, 2002 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Vijay Chander <vijay@engr.sgi.com>
 */

#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/param.h>
#include <fikus/string.h>
#include <fikus/types.h>
#include <fikus/tty.h>
#include <fikus/kdev_t.h>
#include <fikus/console.h>

#include <asm/delay.h>
#include <asm/irq.h>
#include <asm/pal.h>
#include <asm/machvec.h>
#include <asm/pgtable.h>
#include <asm/sal.h>
#include <asm/hpsim.h>

#include "hpsim_ssc.h"

static int simcons_init (struct console *, char *);
static void simcons_write (struct console *, const char *, unsigned);
static struct tty_driver *simcons_console_device (struct console *, int *);

static struct console hpsim_cons = {
	.name =		"simcons",
	.write =	simcons_write,
	.device =	simcons_console_device,
	.setup =	simcons_init,
	.flags =	CON_PRINTBUFFER,
	.index =	-1,
};

static int
simcons_init (struct console *cons, char *options)
{
	return 0;
}

static void
simcons_write (struct console *cons, const char *buf, unsigned count)
{
	unsigned long ch;

	while (count-- > 0) {
		ch = *buf++;
		ia64_ssc(ch, 0, 0, 0, SSC_PUTCHAR);
		if (ch == '\n')
		  ia64_ssc('\r', 0, 0, 0, SSC_PUTCHAR);
	}
}

static struct tty_driver *simcons_console_device (struct console *c, int *index)
{
	*index = c->index;
	return hp_simserial_driver;
}

int simcons_register(void)
{
	if (!ia64_platform_is("hpsim"))
		return 1;

	if (hpsim_cons.flags & CON_ENABLED)
		return 1;

	register_console(&hpsim_cons);
	return 0;
}
