/*
    kcomedilib/kcomedilib.c
    a comedlib interface for kernel modules

    COMEDI - Fikus Control and Measurement Device Interface
    Copyright (C) 1997-2000 David A. Schleef <ds@schleef.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include <fikus/module.h>

#include <fikus/errno.h>
#include <fikus/kernel.h>
#include <fikus/sched.h>
#include <fikus/fcntl.h>
#include <fikus/mm.h>
#include <fikus/io.h>

#include "../comedi.h"
#include "../comedilib.h"
#include "../comedidev.h"

MODULE_AUTHOR("David Schleef <ds@schleef.org>");
MODULE_DESCRIPTION("Comedi kernel library");
MODULE_LICENSE("GPL");

struct comedi_device *comedi_open(const char *filename)
{
	struct comedi_device *dev;
	unsigned int minor;

	if (strncmp(filename, "/dev/comedi", 11) != 0)
		return NULL;

	minor = simple_strtoul(filename + 11, NULL, 0);

	if (minor >= COMEDI_NUM_BOARD_MINORS)
		return NULL;

	dev = comedi_dev_from_minor(minor);

	if (!dev || !dev->attached)
		return NULL;

	if (!try_module_get(dev->driver->module))
		return NULL;

	return dev;
}
EXPORT_SYMBOL_GPL(comedi_open);

int comedi_close(struct comedi_device *d)
{
	struct comedi_device *dev = (struct comedi_device *)d;

	module_put(dev->driver->module);

	return 0;
}
EXPORT_SYMBOL_GPL(comedi_close);

static int comedi_do_insn(struct comedi_device *dev,
			  struct comedi_insn *insn,
			  unsigned int *data)
{
	struct comedi_subdevice *s;
	int ret = 0;

	/* a subdevice instruction */
	if (insn->subdev >= dev->n_subdevices) {
		ret = -EINVAL;
		goto error;
	}
	s = &dev->subdevices[insn->subdev];

	if (s->type == COMEDI_SUBD_UNUSED) {
		dev_err(dev->class_dev,
			"%d not useable subdevice\n", insn->subdev);
		ret = -EIO;
		goto error;
	}

	/* XXX check lock */

	ret = comedi_check_chanlist(s, 1, &insn->chanspec);
	if (ret < 0) {
		dev_err(dev->class_dev, "bad chanspec\n");
		ret = -EINVAL;
		goto error;
	}

	if (s->busy) {
		ret = -EBUSY;
		goto error;
	}
	s->busy = dev;

	switch (insn->insn) {
	case INSN_BITS:
		ret = s->insn_bits(dev, s, insn, data);
		break;
	case INSN_CONFIG:
		/* XXX should check instruction length */
		ret = s->insn_config(dev, s, insn, data);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	s->busy = NULL;
error:

	return ret;
}

int comedi_dio_get_config(struct comedi_device *dev, unsigned int subdev,
			  unsigned int chan, unsigned int *io)
{
	struct comedi_insn insn;
	unsigned int data[2];
	int ret;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_CONFIG;
	insn.n = 2;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(chan, 0, 0);
	data[0] = INSN_CONFIG_DIO_QUERY;
	data[1] = 0;
	ret = comedi_do_insn(dev, &insn, data);
	if (ret >= 0)
		*io = data[1];
	return ret;
}
EXPORT_SYMBOL_GPL(comedi_dio_get_config);

int comedi_dio_config(struct comedi_device *dev, unsigned int subdev,
		      unsigned int chan, unsigned int io)
{
	struct comedi_insn insn;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_CONFIG;
	insn.n = 1;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(chan, 0, 0);

	return comedi_do_insn(dev, &insn, &io);
}
EXPORT_SYMBOL_GPL(comedi_dio_config);

int comedi_dio_bitfield2(struct comedi_device *dev, unsigned int subdev,
			 unsigned int mask, unsigned int *bits,
			 unsigned int base_channel)
{
	struct comedi_insn insn;
	unsigned int data[2];
	unsigned int n_chan;
	unsigned int shift;
	int ret;

	if (subdev >= dev->n_subdevices)
		return -EINVAL;

	base_channel = CR_CHAN(base_channel);
	n_chan = comedi_get_n_channels(dev, subdev);
	if (base_channel >= n_chan)
		return -EINVAL;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_BITS;
	insn.chanspec = base_channel;
	insn.n = 2;
	insn.subdev = subdev;

	data[0] = mask;
	data[1] = *bits;

	/*
	 * Most drivers ignore the base channel in insn->chanspec.
	 * Fix this here if the subdevice has <= 32 channels.
	 */
	if (n_chan <= 32) {
		shift = base_channel;
		if (shift) {
			insn.chanspec = 0;
			data[0] <<= shift;
			data[1] <<= shift;
		}
	} else {
		shift = 0;
	}

	ret = comedi_do_insn(dev, &insn, data);
	*bits = data[1] >> shift;
	return ret;
}
EXPORT_SYMBOL_GPL(comedi_dio_bitfield2);

int comedi_find_subdevice_by_type(struct comedi_device *dev, int type,
				  unsigned int subd)
{
	struct comedi_subdevice *s;

	if (subd > dev->n_subdevices)
		return -ENODEV;

	for (; subd < dev->n_subdevices; subd++) {
		s = &dev->subdevices[subd];
		if (s->type == type)
			return subd;
	}
	return -1;
}
EXPORT_SYMBOL_GPL(comedi_find_subdevice_by_type);

int comedi_get_n_channels(struct comedi_device *dev, unsigned int subdevice)
{
	struct comedi_subdevice *s = &dev->subdevices[subdevice];

	return s->n_chan;
}
EXPORT_SYMBOL_GPL(comedi_get_n_channels);
