/*
 * HCI based Driver for Inside Secure microread NFC Chip - i2c layer
 *
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <fikus/module.h>
#include <fikus/i2c.h>
#include <fikus/delay.h>
#include <fikus/slab.h>
#include <fikus/interrupt.h>
#include <fikus/gpio.h>

#include <fikus/nfc.h>
#include <net/nfc/hci.h>
#include <net/nfc/llc.h>

#include "microread.h"

#define MICROREAD_I2C_DRIVER_NAME "microread"

#define MICROREAD_I2C_FRAME_HEADROOM 1
#define MICROREAD_I2C_FRAME_TAILROOM 1

/* framing in HCI mode */
#define MICROREAD_I2C_LLC_LEN		1
#define MICROREAD_I2C_LLC_CRC		1
#define MICROREAD_I2C_LLC_LEN_CRC	(MICROREAD_I2C_LLC_LEN + \
					MICROREAD_I2C_LLC_CRC)
#define MICROREAD_I2C_LLC_MIN_SIZE	(1 + MICROREAD_I2C_LLC_LEN_CRC)
#define MICROREAD_I2C_LLC_MAX_PAYLOAD	29
#define MICROREAD_I2C_LLC_MAX_SIZE	(MICROREAD_I2C_LLC_LEN_CRC + 1 + \
					MICROREAD_I2C_LLC_MAX_PAYLOAD)

struct microread_i2c_phy {
	struct i2c_client *i2c_dev;
	struct nfc_hci_dev *hdev;

	int irq;

	int hard_fault;		/*
				 * < 0 if hardware error occured (e.g. i2c err)
				 * and prevents normal operation.
				 */
};

#define I2C_DUMP_SKB(info, skb)					\
do {								\
	pr_debug("%s:\n", info);				\
	print_hex_dump(KERN_DEBUG, "i2c: ", DUMP_PREFIX_OFFSET,	\
		       16, 1, (skb)->data, (skb)->len, 0);	\
} while (0)

static void microread_i2c_add_len_crc(struct sk_buff *skb)
{
	int i;
	u8 crc = 0;
	int len;

	len = skb->len;
	*skb_push(skb, 1) = len;

	for (i = 0; i < skb->len; i++)
		crc = crc ^ skb->data[i];

	*skb_put(skb, 1) = crc;
}

static void microread_i2c_remove_len_crc(struct sk_buff *skb)
{
	skb_pull(skb, MICROREAD_I2C_FRAME_HEADROOM);
	skb_trim(skb, MICROREAD_I2C_FRAME_TAILROOM);
}

static int check_crc(struct sk_buff *skb)
{
	int i;
	u8 crc = 0;

	for (i = 0; i < skb->len - 1; i++)
		crc = crc ^ skb->data[i];

	if (crc != skb->data[skb->len-1]) {
		pr_err(MICROREAD_I2C_DRIVER_NAME
		       ": CRC error 0x%x != 0x%x\n",
		       crc, skb->data[skb->len-1]);

		pr_info(DRIVER_DESC ": %s : BAD CRC\n", __func__);

		return -EPERM;
	}

	return 0;
}

static int microread_i2c_enable(void *phy_id)
{
	return 0;
}

static void microread_i2c_disable(void *phy_id)
{
	return;
}

static int microread_i2c_write(void *phy_id, struct sk_buff *skb)
{
	int r;
	struct microread_i2c_phy *phy = phy_id;
	struct i2c_client *client = phy->i2c_dev;

	if (phy->hard_fault != 0)
		return phy->hard_fault;

	usleep_range(3000, 6000);

	microread_i2c_add_len_crc(skb);

	I2C_DUMP_SKB("i2c frame written", skb);

	r = i2c_master_send(client, skb->data, skb->len);

	if (r == -EREMOTEIO) {	/* Retry, chip was in standby */
		usleep_range(6000, 10000);
		r = i2c_master_send(client, skb->data, skb->len);
	}

	if (r >= 0) {
		if (r != skb->len)
			r = -EREMOTEIO;
		else
			r = 0;
	}

	microread_i2c_remove_len_crc(skb);

	return r;
}


static int microread_i2c_read(struct microread_i2c_phy *phy,
			      struct sk_buff **skb)
{
	int r;
	u8 len;
	u8 tmp[MICROREAD_I2C_LLC_MAX_SIZE - 1];
	struct i2c_client *client = phy->i2c_dev;

	pr_debug("%s\n", __func__);

	r = i2c_master_recv(client, &len, 1);
	if (r != 1) {
		dev_err(&client->dev, "cannot read len byte\n");
		return -EREMOTEIO;
	}

	if ((len < MICROREAD_I2C_LLC_MIN_SIZE) ||
	    (len > MICROREAD_I2C_LLC_MAX_SIZE)) {
		dev_err(&client->dev, "invalid len byte\n");
		pr_err("invalid len byte\n");
		r = -EBADMSG;
		goto flush;
	}

	*skb = alloc_skb(1 + len, GFP_KERNEL);
	if (*skb == NULL) {
		r = -ENOMEM;
		goto flush;
	}

	*skb_put(*skb, 1) = len;

	r = i2c_master_recv(client, skb_put(*skb, len), len);
	if (r != len) {
		kfree_skb(*skb);
		return -EREMOTEIO;
	}

	I2C_DUMP_SKB("cc frame read", *skb);

	r = check_crc(*skb);
	if (r != 0) {
		kfree_skb(*skb);
		r = -EBADMSG;
		goto flush;
	}

	skb_pull(*skb, 1);
	skb_trim(*skb, (*skb)->len - MICROREAD_I2C_FRAME_TAILROOM);

	usleep_range(3000, 6000);

	return 0;

flush:
	if (i2c_master_recv(client, tmp, sizeof(tmp)) < 0)
		r = -EREMOTEIO;

	usleep_range(3000, 6000);

	return r;
}

static irqreturn_t microread_i2c_irq_thread_fn(int irq, void *phy_id)
{
	struct microread_i2c_phy *phy = phy_id;
	struct i2c_client *client;
	struct sk_buff *skb = NULL;
	int r;

	if (!phy || irq != phy->i2c_dev->irq) {
		WARN_ON_ONCE(1);
		return IRQ_NONE;
	}

	client = phy->i2c_dev;
	dev_dbg(&client->dev, "IRQ\n");

	if (phy->hard_fault != 0)
		return IRQ_HANDLED;

	r = microread_i2c_read(phy, &skb);
	if (r == -EREMOTEIO) {
		phy->hard_fault = r;

		nfc_hci_recv_frame(phy->hdev, NULL);

		return IRQ_HANDLED;
	} else if ((r == -ENOMEM) || (r == -EBADMSG)) {
		return IRQ_HANDLED;
	}

	nfc_hci_recv_frame(phy->hdev, skb);

	return IRQ_HANDLED;
}

static struct nfc_phy_ops i2c_phy_ops = {
	.write = microread_i2c_write,
	.enable = microread_i2c_enable,
	.disable = microread_i2c_disable,
};

static int microread_i2c_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	struct microread_i2c_phy *phy;
	struct microread_nfc_platform_data *pdata =
		dev_get_platdata(&client->dev);
	int r;

	dev_dbg(&client->dev, "client %p", client);

	if (!pdata) {
		dev_err(&client->dev, "client %p: missing platform data",
			client);
		return -EINVAL;
	}

	phy = devm_kzalloc(&client->dev, sizeof(struct microread_i2c_phy),
			   GFP_KERNEL);
	if (!phy) {
		dev_err(&client->dev, "Can't allocate microread phy");
		return -ENOMEM;
	}

	i2c_set_clientdata(client, phy);
	phy->i2c_dev = client;

	r = request_threaded_irq(client->irq, NULL, microread_i2c_irq_thread_fn,
				 IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				 MICROREAD_I2C_DRIVER_NAME, phy);
	if (r) {
		dev_err(&client->dev, "Unable to register IRQ handler");
		return r;
	}

	r = microread_probe(phy, &i2c_phy_ops, LLC_SHDLC_NAME,
			    MICROREAD_I2C_FRAME_HEADROOM,
			    MICROREAD_I2C_FRAME_TAILROOM,
			    MICROREAD_I2C_LLC_MAX_PAYLOAD, &phy->hdev);
	if (r < 0)
		goto err_irq;

	dev_info(&client->dev, "Probed");

	return 0;

err_irq:
	free_irq(client->irq, phy);

	return r;
}

static int microread_i2c_remove(struct i2c_client *client)
{
	struct microread_i2c_phy *phy = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "%s\n", __func__);

	microread_remove(phy->hdev);

	free_irq(client->irq, phy);

	return 0;
}

static struct i2c_device_id microread_i2c_id[] = {
	{ MICROREAD_I2C_DRIVER_NAME, 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, microread_i2c_id);

static struct i2c_driver microread_i2c_driver = {
	.driver = {
		.name = MICROREAD_I2C_DRIVER_NAME,
	},
	.probe		= microread_i2c_probe,
	.remove		= microread_i2c_remove,
	.id_table	= microread_i2c_id,
};

module_i2c_driver(microread_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
