/*
 * Copyright (C) 2005 Dmitry Torokhov
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef __FIKUS_USB_INPUT_H
#define __FIKUS_USB_INPUT_H

#include <fikus/usb.h>
#include <fikus/input.h>
#include <asm/byteorder.h>

static inline void
usb_to_input_id(const struct usb_device *dev, struct input_id *id)
{
	id->bustype = BUS_USB;
	id->vendor = le16_to_cpu(dev->descriptor.idVendor);
	id->product = le16_to_cpu(dev->descriptor.idProduct);
	id->version = le16_to_cpu(dev->descriptor.bcdDevice);
}

#endif /* __FIKUS_USB_INPUT_H */
