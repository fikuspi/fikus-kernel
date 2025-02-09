/*
 * arch/sh/boards/se/7619/setup.c
 *
 * Copyright (C) 2006 Yoshinori Sato
 *
 * Hitachi SH7619 SolutionEngine Support.
 */

#include <fikus/init.h>
#include <fikus/platform_device.h>
#include <asm/io.h>
#include <asm/machvec.h>

static int se7619_mode_pins(void)
{
	return MODE_PIN2 | MODE_PIN0;
}

/*
 * The Machine Vector
 */

static struct sh_machine_vector mv_se __initmv = {
	.mv_name		= "SolutionEngine",
	.mv_mode_pins		= se7619_mode_pins,
};
