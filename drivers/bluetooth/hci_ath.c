/*
 *  Atheros Communication Bluetooth HCIATH3K UART protocol
 *
 *  HCIATH3K (HCI Atheros AR300x Protocol) is a Atheros Communication's
 *  power management protocol extension to H4 to support AR300x Bluetooth Chip.
 *
 *  Copyright (c) 2009-2010 Atheros Communications Inc.
 *
 *  Acknowledgements:
 *  This file is based on hci_h4.c, which was written
 *  by Maxim Krasnyansky and Marcel Holtmann.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <fikus/module.h>
#include <fikus/kernel.h>

#include <fikus/init.h>
#include <fikus/slab.h>
#include <fikus/tty.h>
#include <fikus/errno.h>
#include <fikus/ioctl.h>
#include <fikus/skbuff.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "hci_uart.h"

struct ath_struct {
	struct hci_uart *hu;
	unsigned int cur_sleep;

	struct sk_buff_head txq;
	struct work_struct ctxtsw;
};

static int ath_wakeup_ar3k(struct tty_struct *tty)
{
	struct ktermios ktermios;
	int status = tty->driver->ops->tiocmget(tty);

	if (status & TIOCM_CTS)
		return status;

	/* Disable Automatic RTSCTS */
	ktermios = tty->termios;
	ktermios.c_cflag &= ~CRTSCTS;
	tty_set_termios(tty, &ktermios);

	/* Clear RTS first */
	status = tty->driver->ops->tiocmget(tty);
	tty->driver->ops->tiocmset(tty, 0x00, TIOCM_RTS);
	mdelay(20);

	/* Set RTS, wake up board */
	status = tty->driver->ops->tiocmget(tty);
	tty->driver->ops->tiocmset(tty, TIOCM_RTS, 0x00);
	mdelay(20);

	status = tty->driver->ops->tiocmget(tty);

	/* Disable Automatic RTSCTS */
	ktermios.c_cflag |= CRTSCTS;
	status = tty_set_termios(tty, &ktermios);

	return status;
}

static void ath_hci_uart_work(struct work_struct *work)
{
	int status;
	struct ath_struct *ath;
	struct hci_uart *hu;
	struct tty_struct *tty;

	ath = container_of(work, struct ath_struct, ctxtsw);

	hu = ath->hu;
	tty = hu->tty;

	/* verify and wake up controller */
	if (ath->cur_sleep) {
		status = ath_wakeup_ar3k(tty);
		if (!(status & TIOCM_CTS))
			return;
	}

	/* Ready to send Data */
	clear_bit(HCI_UART_SENDING, &hu->tx_state);
	hci_uart_tx_wakeup(hu);
}

/* Initialize protocol */
static int ath_open(struct hci_uart *hu)
{
	struct ath_struct *ath;

	BT_DBG("hu %p", hu);

	ath = kzalloc(sizeof(*ath), GFP_KERNEL);
	if (!ath)
		return -ENOMEM;

	skb_queue_head_init(&ath->txq);

	hu->priv = ath;
	ath->hu = hu;

	INIT_WORK(&ath->ctxtsw, ath_hci_uart_work);

	return 0;
}

/* Flush protocol data */
static int ath_flush(struct hci_uart *hu)
{
	struct ath_struct *ath = hu->priv;

	BT_DBG("hu %p", hu);

	skb_queue_purge(&ath->txq);

	return 0;
}

/* Close protocol */
static int ath_close(struct hci_uart *hu)
{
	struct ath_struct *ath = hu->priv;

	BT_DBG("hu %p", hu);

	skb_queue_purge(&ath->txq);

	cancel_work_sync(&ath->ctxtsw);

	hu->priv = NULL;
	kfree(ath);

	return 0;
}

#define HCI_OP_ATH_SLEEP 0xFC04

/* Enqueue frame for transmittion */
static int ath_enqueue(struct hci_uart *hu, struct sk_buff *skb)
{
	struct ath_struct *ath = hu->priv;

	if (bt_cb(skb)->pkt_type == HCI_SCODATA_PKT) {
		kfree_skb(skb);
		return 0;
	}

	/*
	 * Update power management enable flag with parameters of
	 * HCI sleep enable vendor specific HCI command.
	 */
	if (bt_cb(skb)->pkt_type == HCI_COMMAND_PKT) {
		struct hci_command_hdr *hdr = (void *)skb->data;

		if (__le16_to_cpu(hdr->opcode) == HCI_OP_ATH_SLEEP)
			ath->cur_sleep = skb->data[HCI_COMMAND_HDR_SIZE];
	}

	BT_DBG("hu %p skb %p", hu, skb);

	/* Prepend skb with frame type */
	memcpy(skb_push(skb, 1), &bt_cb(skb)->pkt_type, 1);

	skb_queue_tail(&ath->txq, skb);
	set_bit(HCI_UART_SENDING, &hu->tx_state);

	schedule_work(&ath->ctxtsw);

	return 0;
}

static struct sk_buff *ath_dequeue(struct hci_uart *hu)
{
	struct ath_struct *ath = hu->priv;

	return skb_dequeue(&ath->txq);
}

/* Recv data */
static int ath_recv(struct hci_uart *hu, void *data, int count)
{
	int ret;

	ret = hci_recv_stream_fragment(hu->hdev, data, count);
	if (ret < 0) {
		BT_ERR("Frame Reassembly Failed");
		return ret;
	}

	return count;
}

static struct hci_uart_proto athp = {
	.id = HCI_UART_ATH3K,
	.open = ath_open,
	.close = ath_close,
	.recv = ath_recv,
	.enqueue = ath_enqueue,
	.dequeue = ath_dequeue,
	.flush = ath_flush,
};

int __init ath_init(void)
{
	int err = hci_uart_register_proto(&athp);

	if (!err)
		BT_INFO("HCIATH3K protocol initialized");
	else
		BT_ERR("HCIATH3K protocol registration failed");

	return err;
}

int __exit ath_deinit(void)
{
	return hci_uart_unregister_proto(&athp);
}
