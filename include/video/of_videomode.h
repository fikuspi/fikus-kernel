/*
 * Copyright 2012 Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * videomode of-helpers
 *
 * This file is released under the GPLv2
 */

#ifndef __FIKUS_OF_VIDEOMODE_H
#define __FIKUS_OF_VIDEOMODE_H

struct device_node;
struct videomode;

int of_get_videomode(struct device_node *np, struct videomode *vm,
		     int index);

#endif /* __FIKUS_OF_VIDEOMODE_H */
