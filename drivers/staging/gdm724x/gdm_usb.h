/*
 * Copyright (c) 2012 GCT Semiconductor, Inc. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _GDM_USB_H_
#define _GDM_USB_H_

#include <fikus/types.h>
#include <fikus/usb.h>
#include <fikus/list.h>
#include <fikus/time.h>

#include "gdm_endian.h"
#include "hci_packet.h"

#define PM_NORMAL 0
#define PM_SUSPEND 1
#define AUTO_SUSPEND_TIMER 5000 /* ms */

#define RX_BUF_SIZE		(1024*32)
#define TX_BUF_SIZE		(1024*32)
#define SDU_BUF_SIZE	2048
#define MAX_SDU_SIZE	(1024*30)
#define MAX_PACKET_IN_MULTI_SDU	256

#define VID_GCT			0x1076
#define PID_GDM7240		0x8000
#define PID_GDM7243		0x9000

#define NETWORK_INTERFACE 1
#define USB_SC_SCSI 0x06
#define USB_PR_BULK 0x50

#define MAX_NUM_SDU_BUF	64

struct usb_tx {
	struct list_head list;
	struct urb *urb;
	u8 *buf;
	u32 len;
	void (*callback)(void *cb_data);
	void *cb_data;
	struct tx_cxt *tx;
	u8 is_sdu;
};

struct usb_tx_sdu {
	struct list_head list;
	u8 *buf;
	u32 len;
	void (*callback)(void *cb_data);
	void *cb_data;
};

struct usb_rx {
	struct list_head to_host_list;
	struct list_head free_list;
	struct list_head rx_submit_list;
	struct rx_cxt	*rx;
	struct urb *urb;
	u8 *buf;
	int (*callback)(void *cb_data, void *data, int len, int context);
	void *cb_data;
	void *index;
};

struct tx_cxt {
	struct list_head sdu_list;
	struct list_head hci_list;
	struct list_head free_list;
	u32 avail_count;
	spinlock_t lock;
};

struct rx_cxt {
	struct list_head to_host_list;
	struct list_head rx_submit_list;
	struct list_head free_list;
	u32	avail_count;
	spinlock_t to_host_lock;
	spinlock_t rx_lock;
	spinlock_t submit_lock;
};

struct lte_udev {
	struct usb_device *usbdev;
	struct gdm_endian gdm_ed;
	struct tx_cxt tx;
	struct rx_cxt rx;
	struct delayed_work work_tx;
	struct delayed_work work_rx;
	u8 send_complete;
	u8 tx_stop;
	struct usb_interface *intf;
	int (*rx_cb)(void *cb_data, void *data, int len, int context);
	int usb_state;
	u8 request_mac_addr;
};

#endif /* _GDM_USB_H_ */
