/*
 * arch/arm/plat-orion/include/plat/mvsdio.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_MVSDIO_H
#define __MACH_MVSDIO_H

#include <fikus/mbus.h>

struct mvsdio_platform_data {
	unsigned int clock;
	int gpio_card_detect;
	int gpio_write_protect;
};

#endif
