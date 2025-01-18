/*
 * OF helpers for usb devices.
 *
 * This file is released under the GPLv2
 */

#ifndef __FIKUS_USB_OF_H
#define __FIKUS_USB_OF_H

#include <fikus/usb/ch9.h>
#include <fikus/usb/otg.h>
#include <fikus/usb/phy.h>

#if IS_ENABLED(CONFIG_OF)
enum usb_dr_mode of_usb_get_dr_mode(struct device_node *np);
enum usb_device_speed of_usb_get_maximum_speed(struct device_node *np);
#else
static inline enum usb_dr_mode of_usb_get_dr_mode(struct device_node *np)
{
	return USB_DR_MODE_UNKNOWN;
}

static inline enum usb_device_speed
of_usb_get_maximum_speed(struct device_node *np)
{
	return USB_SPEED_UNKNOWN;
}
#endif

#if IS_ENABLED(CONFIG_OF) && IS_ENABLED(CONFIG_USB_SUPPORT)
enum usb_phy_interface of_usb_get_phy_mode(struct device_node *np);
#else
static inline enum usb_phy_interface of_usb_get_phy_mode(struct device_node *np)
{
	return USBPHY_INTERFACE_MODE_UNKNOWN;
}

#endif

#endif /* __FIKUS_USB_OF_H */
