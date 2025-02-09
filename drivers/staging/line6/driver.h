/*
 * Line6 Fikus USB driver - 0.9.1beta
 *
 * Copyright (C) 2004-2010 Markus Grabner (grabner@icg.tugraz.at)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <fikus/spinlock.h>
#include <fikus/usb.h>
#include <sound/core.h>

#include "midi.h"

#define DRIVER_NAME "line6usb"

#define LINE6_TIMEOUT 1
#define LINE6_BUFSIZE_LISTEN 32
#define LINE6_MESSAGE_MAXLEN 256

/*
	Line6 MIDI control commands
*/
#define LINE6_PARAM_CHANGE   0xb0
#define LINE6_PROGRAM_CHANGE 0xc0
#define LINE6_SYSEX_BEGIN    0xf0
#define LINE6_SYSEX_END      0xf7
#define LINE6_RESET          0xff

/*
	MIDI channel for messages initiated by the host
	(and eventually echoed back by the device)
*/
#define LINE6_CHANNEL_HOST   0x00

/*
	MIDI channel for messages initiated by the device
*/
#define LINE6_CHANNEL_DEVICE 0x02

#define LINE6_CHANNEL_UNKNOWN 5	/* don't know yet what this is good for */

#define LINE6_CHANNEL_MASK 0x0f

#define MISSING_CASE	\
	pr_err("line6usb driver bug: missing case in %s:%d\n", \
		__FILE__, __LINE__)

#define CHECK_RETURN(x)		\
do {				\
	err = x;		\
	if (err < 0)		\
		return err;	\
} while (0)

#define CHECK_STARTUP_PROGRESS(x, n)	\
do {					\
	if ((x) >= (n))			\
		return;			\
	x = (n);			\
} while (0)

extern const unsigned char line6_midi_id[3];

static const int SYSEX_DATA_OFS = sizeof(line6_midi_id) + 3;
static const int SYSEX_EXTRA_SIZE = sizeof(line6_midi_id) + 4;

/**
	 Common properties of Line6 devices.
*/
struct line6_properties {
	/**
		 Bit identifying this device in the line6usb driver.
	*/
	int device_bit;

	/**
		 Card id string (maximum 16 characters).
		 This can be used to address the device in ALSA programs as
		 "default:CARD=<id>"
	*/
	const char *id;

	/**
		 Card short name (maximum 32 characters).
	*/
	const char *name;

	/**
		 Bit vector defining this device's capabilities in the
		 line6usb driver.
	*/
	int capabilities;
};

/**
	 Common data shared by all Line6 devices.
	 Corresponds to a pair of USB endpoints.
*/
struct usb_line6 {
	/**
		 USB device.
	*/
	struct usb_device *usbdev;

	/**
		 Product id.
	*/
	int product;

	/**
		 Properties.
	*/
	const struct line6_properties *properties;

	/**
		 Interface number.
	*/
	int interface_number;

	/**
		 Interval (ms).
	*/
	int interval;

	/**
		 Maximum size of USB packet.
	*/
	int max_packet_size;

	/**
		 Device representing the USB interface.
	*/
	struct device *ifcdev;

	/**
		 Line6 sound card data structure.
		 Each device has at least MIDI or PCM.
	*/
	struct snd_card *card;

	/**
		 Line6 PCM device data structure.
	*/
	struct snd_line6_pcm *line6pcm;

	/**
		 Line6 MIDI device data structure.
	*/
	struct snd_line6_midi *line6midi;

	/**
		 USB endpoint for listening to control commands.
	*/
	int ep_control_read;

	/**
		 USB endpoint for writing control commands.
	*/
	int ep_control_write;

	/**
		 URB for listening to PODxt Pro control endpoint.
	*/
	struct urb *urb_listen;

	/**
		 Buffer for listening to PODxt Pro control endpoint.
	*/
	unsigned char *buffer_listen;

	/**
		 Buffer for message to be processed.
	*/
	unsigned char *buffer_message;

	/**
		 Length of message to be processed.
	*/
	int message_length;
};

extern char *line6_alloc_sysex_buffer(struct usb_line6 *line6, int code1,
				      int code2, int size);
extern ssize_t line6_nop_read(struct device *dev,
			      struct device_attribute *attr, char *buf);
extern int line6_read_data(struct usb_line6 *line6, int address, void *data,
			   size_t datalen);
extern int line6_read_serial_number(struct usb_line6 *line6,
				    int *serial_number);
extern int line6_send_program(struct usb_line6 *line6, u8 value);
extern int line6_send_raw_message(struct usb_line6 *line6, const char *buffer,
				  int size);
extern int line6_send_raw_message_async(struct usb_line6 *line6,
					const char *buffer, int size);
extern int line6_send_sysex_message(struct usb_line6 *line6,
				    const char *buffer, int size);
extern ssize_t line6_set_raw(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count);
extern void line6_start_timer(struct timer_list *timer, unsigned int msecs,
			      void (*function) (unsigned long),
			      unsigned long data);
extern int line6_transmit_parameter(struct usb_line6 *line6, int param,
				    u8 value);
extern int line6_version_request_async(struct usb_line6 *line6);
extern int line6_write_data(struct usb_line6 *line6, int address, void *data,
			    size_t datalen);

#endif
