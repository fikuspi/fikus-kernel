/*
    comedi/drivers/amplc_pci263.c
    Driver for Amplicon PCI263 relay board.

    Copyright (C) 2002 MEV Ltd. <http://www.mev.co.uk/>

    COMEDI - Fikus Control and Measurement Device Interface
    Copyright (C) 2000 David A. Schleef <ds@schleef.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
/*
Driver: amplc_pci263
Description: Amplicon PCI263
Author: Ian Abbott <abbotti@mev.co.uk>
Devices: [Amplicon] PCI263 (amplc_pci263)
Updated: Fri, 12 Apr 2013 15:19:36 +0100
Status: works

Configuration options: not applicable, uses PCI auto config

The board appears as one subdevice, with 16 digital outputs, each
connected to a reed-relay. Relay contacts are closed when output is 1.
The state of the outputs can be read.
*/

#include <fikus/module.h>
#include <fikus/pci.h>

#include "../comedidev.h"

#define PCI263_DRIVER_NAME	"amplc_pci263"

/* PCI263 PCI configuration register information */
#define PCI_DEVICE_ID_AMPLICON_PCI263 0x000c

static int pci263_do_insn_bits(struct comedi_device *dev,
			       struct comedi_subdevice *s,
			       struct comedi_insn *insn, unsigned int *data)
{
	/* The insn data is a mask in data[0] and the new data
	 * in data[1], each channel cooresponding to a bit. */
	if (data[0]) {
		s->state &= ~data[0];
		s->state |= data[0] & data[1];
		/* Write out the new digital output lines */
		outb(s->state & 0xFF, dev->iobase);
		outb(s->state >> 8, dev->iobase + 1);
	}

	data[1] = s->state;

	return insn->n;
}

static int pci263_auto_attach(struct comedi_device *dev,
			      unsigned long context_unused)
{
	struct pci_dev *pci_dev = comedi_to_pci_dev(dev);
	struct comedi_subdevice *s;
	int ret;

	ret = comedi_pci_enable(dev);
	if (ret)
		return ret;

	dev->iobase = pci_resource_start(pci_dev, 2);
	ret = comedi_alloc_subdevices(dev, 1);
	if (ret)
		return ret;

	s = &dev->subdevices[0];
	/* digital output subdevice */
	s->type = COMEDI_SUBD_DO;
	s->subdev_flags = SDF_READABLE | SDF_WRITABLE;
	s->n_chan = 16;
	s->maxdata = 1;
	s->range_table = &range_digital;
	s->insn_bits = pci263_do_insn_bits;
	/* read initial relay state */
	s->state = inb(dev->iobase) | (inb(dev->iobase + 1) << 8);

	dev_info(dev->class_dev, "%s (pci %s) attached\n", dev->board_name,
		 pci_name(pci_dev));
	return 0;
}

static struct comedi_driver amplc_pci263_driver = {
	.driver_name	= PCI263_DRIVER_NAME,
	.module		= THIS_MODULE,
	.auto_attach	= pci263_auto_attach,
	.detach		= comedi_pci_disable,
};

static DEFINE_PCI_DEVICE_TABLE(pci263_pci_table) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_AMPLICON, PCI_DEVICE_ID_AMPLICON_PCI263) },
	{0}
};
MODULE_DEVICE_TABLE(pci, pci263_pci_table);

static int amplc_pci263_pci_probe(struct pci_dev *dev,
				  const struct pci_device_id *id)
{
	return comedi_pci_auto_config(dev, &amplc_pci263_driver,
				      id->driver_data);
}

static struct pci_driver amplc_pci263_pci_driver = {
	.name		= PCI263_DRIVER_NAME,
	.id_table	= pci263_pci_table,
	.probe		= &amplc_pci263_pci_probe,
	.remove		= comedi_pci_auto_unconfig,
};
module_comedi_pci_driver(amplc_pci263_driver, amplc_pci263_pci_driver);

MODULE_AUTHOR("Comedi http://www.comedi.org");
MODULE_DESCRIPTION("Comedi driver for Amplicon PCI263 relay board");
MODULE_LICENSE("GPL");
