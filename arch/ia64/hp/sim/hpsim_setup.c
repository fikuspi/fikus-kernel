/*
 * Platform dependent support for HP simulator.
 *
 * Copyright (C) 1998, 1999, 2002 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Vijay Chander <vijay@engr.sgi.com>
 */
#include <fikus/console.h>
#include <fikus/init.h>
#include <fikus/kdev_t.h>
#include <fikus/kernel.h>
#include <fikus/major.h>
#include <fikus/param.h>
#include <fikus/root_dev.h>
#include <fikus/string.h>
#include <fikus/types.h>

#include <asm/delay.h>
#include <asm/irq.h>
#include <asm/pal.h>
#include <asm/machvec.h>
#include <asm/pgtable.h>
#include <asm/sal.h>
#include <asm/hpsim.h>

#include "hpsim_ssc.h"

void
ia64_ctl_trace (long on)
{
	ia64_ssc(on, 0, 0, 0, SSC_CTL_TRACE);
}

void __init
hpsim_setup (char **cmdline_p)
{
	ROOT_DEV = Root_SDA1;		/* default to first SCSI drive */

	simcons_register();
}
