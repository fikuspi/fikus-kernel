/*=====================================================
 * CopyRight (C) 2007 Qualcomm Inc. All Rights Reserved.
 *
 *
 * This file is part of Express Card USB Driver
 *
 * $Id:
 *====================================================
 */
#include <fikus/init.h>
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/usb.h>
#include <fikus/netdevice.h>
#include <fikus/etherdevice.h>
#include <fikus/firmware.h>
#include "ft1000_usb.h"

#include <fikus/kthread.h>

MODULE_DESCRIPTION("FT1000 EXPRESS CARD DRIVER");
MODULE_LICENSE("Dual MPL/GPL");
MODULE_SUPPORTED_DEVICE("QFT FT1000 Express Cards");

void *pFileStart;
size_t FileLength;

#define VENDOR_ID 0x1291	/* Qualcomm vendor id */
#define PRODUCT_ID 0x11		/* fake product id */

/* table of devices that work with this driver */
static struct usb_device_id id_table[] = {
	{USB_DEVICE(VENDOR_ID, PRODUCT_ID)},
	{},
};

MODULE_DEVICE_TABLE(usb, id_table);

static bool gPollingfailed = FALSE;
static int ft1000_poll_thread(void *arg)
{
	int ret;

	while (!kthread_should_stop()) {
		msleep(10);
		if (!gPollingfailed) {
			ret = ft1000_poll(arg);
			if (ret != STATUS_SUCCESS) {
				DEBUG("ft1000_poll_thread: polling failed\n");
				gPollingfailed = TRUE;
			}
		}
	}
	return STATUS_SUCCESS;
}

static int ft1000_probe(struct usb_interface *interface,
			const struct usb_device_id *id)
{
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_device *dev;
	unsigned numaltsetting;
	int i, ret = 0, size;

	struct ft1000_usb *ft1000dev;
	struct ft1000_info *pft1000info = NULL;
	const struct firmware *dsp_fw;

	ft1000dev = kzalloc(sizeof(struct ft1000_usb), GFP_KERNEL);
	if (!ft1000dev)
		return -ENOMEM;

	dev = interface_to_usbdev(interface);
	DEBUG("ft1000_probe: usb device descriptor info:\n");
	DEBUG("ft1000_probe: number of configuration is %d\n",
	      dev->descriptor.bNumConfigurations);

	ft1000dev->dev = dev;
	ft1000dev->status = 0;
	ft1000dev->net = NULL;
	ft1000dev->tx_urb = usb_alloc_urb(0, GFP_KERNEL);
	ft1000dev->rx_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!ft1000dev->tx_urb || !ft1000dev->rx_urb) {
		ret = -ENOMEM;
		goto err_fw;
	}

	DEBUG("ft1000_probe is called\n");
	numaltsetting = interface->num_altsetting;
	DEBUG("ft1000_probe: number of alt settings is :%d\n", numaltsetting);
	iface_desc = interface->cur_altsetting;
	DEBUG("ft1000_probe: number of endpoints is %d\n",
	      iface_desc->desc.bNumEndpoints);
	DEBUG("ft1000_probe: descriptor type is %d\n",
	      iface_desc->desc.bDescriptorType);
	DEBUG("ft1000_probe: interface number is %d\n",
	      iface_desc->desc.bInterfaceNumber);
	DEBUG("ft1000_probe: alternatesetting is %d\n",
	      iface_desc->desc.bAlternateSetting);
	DEBUG("ft1000_probe: interface class is %d\n",
	      iface_desc->desc.bInterfaceClass);
	DEBUG("ft1000_probe: control endpoint info:\n");
	DEBUG("ft1000_probe: descriptor0 type -- %d\n",
	      iface_desc->endpoint[0].desc.bmAttributes);
	DEBUG("ft1000_probe: descriptor1 type -- %d\n",
	      iface_desc->endpoint[1].desc.bmAttributes);
	DEBUG("ft1000_probe: descriptor2 type -- %d\n",
	      iface_desc->endpoint[2].desc.bmAttributes);

	for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
		endpoint =
		    (struct usb_endpoint_descriptor *)&iface_desc->
		    endpoint[i].desc;
		DEBUG("endpoint %d\n", i);
		DEBUG("bEndpointAddress=%x, bmAttributes=%x\n",
		      endpoint->bEndpointAddress, endpoint->bmAttributes);
		if ((endpoint->bEndpointAddress & USB_DIR_IN)
		    && ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
			USB_ENDPOINT_XFER_BULK)) {
			ft1000dev->bulk_in_endpointAddr =
			    endpoint->bEndpointAddress;
			DEBUG("ft1000_probe: in: %d\n",
			      endpoint->bEndpointAddress);
		}

		if (!(endpoint->bEndpointAddress & USB_DIR_IN)
		    && ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
			USB_ENDPOINT_XFER_BULK)) {
			ft1000dev->bulk_out_endpointAddr =
			    endpoint->bEndpointAddress;
			DEBUG("ft1000_probe: out: %d\n",
			      endpoint->bEndpointAddress);
		}
	}

	DEBUG("bulk_in=%d, bulk_out=%d\n", ft1000dev->bulk_in_endpointAddr,
	      ft1000dev->bulk_out_endpointAddr);

	ret = reject_firmware(&dsp_fw, "/*(DEBLOBBED)*/", &dev->dev);
	if (ret < 0) {
		pr_err("Error reject_firmware().\n");
		goto err_fw;
	}

	size = max_t(uint, dsp_fw->size, 4096);
	pFileStart = kmalloc(size, GFP_KERNEL);

	if (!pFileStart) {
		release_firmware(dsp_fw);
		ret = -ENOMEM;
		goto err_fw;
	}

	memcpy(pFileStart, dsp_fw->data, dsp_fw->size);
	FileLength = dsp_fw->size;
	release_firmware(dsp_fw);

	DEBUG("ft1000_probe: start downloading dsp image...\n");

	ret = init_ft1000_netdev(ft1000dev);
	if (ret)
		goto err_load;

	pft1000info = netdev_priv(ft1000dev->net);

	DEBUG("In probe: pft1000info=%p\n", pft1000info);
	ret = dsp_reload(ft1000dev);
	if (ret) {
		pr_err("Problem with DSP image loading\n");
		goto err_load;
	}

	gPollingfailed = FALSE;
	ft1000dev->pPollThread =
	    kthread_run(ft1000_poll_thread, ft1000dev, "ft1000_poll");

	if (IS_ERR(ft1000dev->pPollThread)) {
		ret = PTR_ERR(ft1000dev->pPollThread);
		goto err_load;
	}

	msleep(500);

	while (!pft1000info->CardReady) {
		if (gPollingfailed) {
			ret = -EIO;
			goto err_thread;
		}
		msleep(100);
		DEBUG("ft1000_probe::Waiting for Card Ready\n");
	}

	DEBUG("ft1000_probe::Card Ready!!!! Registering network device\n");

	ret = reg_ft1000_netdev(ft1000dev, interface);
	if (ret)
		goto err_thread;

	ret = ft1000_init_proc(ft1000dev->net);
	if (ret)
		goto err_proc;

	ft1000dev->NetDevRegDone = 1;

	return 0;

err_proc:
	unregister_netdev(ft1000dev->net);
	free_netdev(ft1000dev->net);
err_thread:
	kthread_stop(ft1000dev->pPollThread);
err_load:
	kfree(pFileStart);
err_fw:
	usb_free_urb(ft1000dev->rx_urb);
	usb_free_urb(ft1000dev->tx_urb);
	kfree(ft1000dev);
	return ret;
}

static void ft1000_disconnect(struct usb_interface *interface)
{
	struct ft1000_info *pft1000info;
	struct ft1000_usb *ft1000dev;

	DEBUG("ft1000_disconnect is called\n");

	pft1000info = (struct ft1000_info *) usb_get_intfdata(interface);
	DEBUG("In disconnect pft1000info=%p\n", pft1000info);

	if (pft1000info) {
		ft1000dev = pft1000info->priv;
		ft1000_cleanup_proc(pft1000info);
		if (ft1000dev->pPollThread)
			kthread_stop(ft1000dev->pPollThread);

		DEBUG("ft1000_disconnect: threads are terminated\n");

		if (ft1000dev->net) {
			DEBUG("ft1000_disconnect: destroy char driver\n");
			ft1000_destroy_dev(ft1000dev->net);
			unregister_netdev(ft1000dev->net);
			DEBUG
			    ("ft1000_disconnect: network device unregistered\n");
			free_netdev(ft1000dev->net);

		}

		usb_free_urb(ft1000dev->rx_urb);
		usb_free_urb(ft1000dev->tx_urb);

		DEBUG("ft1000_disconnect: urb freed\n");

		kfree(ft1000dev);
	}
	kfree(pFileStart);

	return;
}

static struct usb_driver ft1000_usb_driver = {
	.name = "ft1000usb",
	.probe = ft1000_probe,
	.disconnect = ft1000_disconnect,
	.id_table = id_table,
};

module_usb_driver(ft1000_usb_driver);
