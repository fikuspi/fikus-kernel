/*
    module/drivers.c
    functions for manipulating drivers

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

#include <fikus/device.h>
#include <fikus/module.h>
#include <fikus/errno.h>
#include <fikus/kconfig.h>
#include <fikus/kernel.h>
#include <fikus/sched.h>
#include <fikus/fcntl.h>
#include <fikus/ioport.h>
#include <fikus/mm.h>
#include <fikus/slab.h>
#include <fikus/highmem.h>	/* for SuSE brokenness */
#include <fikus/vmalloc.h>
#include <fikus/cdev.h>
#include <fikus/dma-mapping.h>
#include <fikus/io.h>
#include <fikus/interrupt.h>
#include <fikus/firmware.h>

#include "comedidev.h"
#include "comedi_internal.h"

struct comedi_driver *comedi_drivers;
DEFINE_MUTEX(comedi_drivers_list_lock);

int comedi_set_hw_dev(struct comedi_device *dev, struct device *hw_dev)
{
	if (hw_dev == dev->hw_dev)
		return 0;
	if (dev->hw_dev != NULL)
		return -EEXIST;
	dev->hw_dev = get_device(hw_dev);
	return 0;
}
EXPORT_SYMBOL_GPL(comedi_set_hw_dev);

static void comedi_clear_hw_dev(struct comedi_device *dev)
{
	put_device(dev->hw_dev);
	dev->hw_dev = NULL;
}

/**
 * comedi_alloc_devpriv() - Allocate memory for the device private data.
 * @dev: comedi_device struct
 * @size: size of the memory to allocate
 */
void *comedi_alloc_devpriv(struct comedi_device *dev, size_t size)
{
	dev->private = kzalloc(size, GFP_KERNEL);
	return dev->private;
}
EXPORT_SYMBOL_GPL(comedi_alloc_devpriv);

int comedi_alloc_subdevices(struct comedi_device *dev, int num_subdevices)
{
	struct comedi_subdevice *s;
	int i;

	if (num_subdevices < 1)
		return -EINVAL;

	s = kcalloc(num_subdevices, sizeof(*s), GFP_KERNEL);
	if (!s)
		return -ENOMEM;
	dev->subdevices = s;
	dev->n_subdevices = num_subdevices;

	for (i = 0; i < num_subdevices; ++i) {
		s = &dev->subdevices[i];
		s->device = dev;
		s->index = i;
		s->async_dma_dir = DMA_NONE;
		spin_lock_init(&s->spin_lock);
		s->minor = -1;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(comedi_alloc_subdevices);

static void cleanup_device(struct comedi_device *dev)
{
	int i;
	struct comedi_subdevice *s;

	if (dev->subdevices) {
		for (i = 0; i < dev->n_subdevices; i++) {
			s = &dev->subdevices[i];
			if (s->runflags & SRF_FREE_SPRIV)
				kfree(s->private);
			comedi_free_subdevice_minor(s);
			if (s->async) {
				comedi_buf_alloc(dev, s, 0);
				kfree(s->async);
			}
		}
		kfree(dev->subdevices);
		dev->subdevices = NULL;
		dev->n_subdevices = 0;
	}
	kfree(dev->private);
	dev->private = NULL;
	dev->driver = NULL;
	dev->board_name = NULL;
	dev->board_ptr = NULL;
	dev->iobase = 0;
	dev->iolen = 0;
	dev->ioenabled = false;
	dev->irq = 0;
	dev->read_subdev = NULL;
	dev->write_subdev = NULL;
	dev->open = NULL;
	dev->close = NULL;
	comedi_clear_hw_dev(dev);
}

void comedi_device_detach(struct comedi_device *dev)
{
	dev->attached = false;
	if (dev->driver)
		dev->driver->detach(dev);
	cleanup_device(dev);
}

static int poll_invalid(struct comedi_device *dev, struct comedi_subdevice *s)
{
	return -EINVAL;
}

int insn_inval(struct comedi_device *dev, struct comedi_subdevice *s,
	       struct comedi_insn *insn, unsigned int *data)
{
	return -EINVAL;
}

/**
 * comedi_dio_insn_config() - boilerplate (*insn_config) for DIO subdevices.
 * @dev: comedi_device struct
 * @s: comedi_subdevice struct
 * @insn: comedi_insn struct
 * @data: parameters for the @insn
 * @mask: io_bits mask for grouped channels
 */
int comedi_dio_insn_config(struct comedi_device *dev,
			   struct comedi_subdevice *s,
			   struct comedi_insn *insn,
			   unsigned int *data,
			   unsigned int mask)
{
	unsigned int chan_mask = 1 << CR_CHAN(insn->chanspec);

	if (!mask)
		mask = chan_mask;

	switch (data[0]) {
	case INSN_CONFIG_DIO_INPUT:
		s->io_bits &= ~mask;
		break;

	case INSN_CONFIG_DIO_OUTPUT:
		s->io_bits |= mask;
		break;

	case INSN_CONFIG_DIO_QUERY:
		data[1] = (s->io_bits & mask) ? COMEDI_OUTPUT : COMEDI_INPUT;
		return insn->n;

	default:
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(comedi_dio_insn_config);

static int insn_rw_emulate_bits(struct comedi_device *dev,
				struct comedi_subdevice *s,
				struct comedi_insn *insn, unsigned int *data)
{
	struct comedi_insn new_insn;
	int ret;
	static const unsigned channels_per_bitfield = 32;

	unsigned chan = CR_CHAN(insn->chanspec);
	const unsigned base_bitfield_channel =
	    (chan < channels_per_bitfield) ? 0 : chan;
	unsigned int new_data[2];
	memset(new_data, 0, sizeof(new_data));
	memset(&new_insn, 0, sizeof(new_insn));
	new_insn.insn = INSN_BITS;
	new_insn.chanspec = base_bitfield_channel;
	new_insn.n = 2;
	new_insn.subdev = insn->subdev;

	if (insn->insn == INSN_WRITE) {
		if (!(s->subdev_flags & SDF_WRITABLE))
			return -EINVAL;
		new_data[0] = 1 << (chan - base_bitfield_channel); /* mask */
		new_data[1] = data[0] ? (1 << (chan - base_bitfield_channel))
			      : 0; /* bits */
	}

	ret = s->insn_bits(dev, s, &new_insn, new_data);
	if (ret < 0)
		return ret;

	if (insn->insn == INSN_READ)
		data[0] = (new_data[1] >> (chan - base_bitfield_channel)) & 1;

	return 1;
}

static int __comedi_device_postconfig_async(struct comedi_device *dev,
					    struct comedi_subdevice *s)
{
	struct comedi_async *async;
	unsigned int buf_size;
	int ret;

	if ((s->subdev_flags & (SDF_CMD_READ | SDF_CMD_WRITE)) == 0) {
		dev_warn(dev->class_dev,
			 "async subdevices must support SDF_CMD_READ or SDF_CMD_WRITE\n");
		return -EINVAL;
	}
	if (!s->do_cmdtest) {
		dev_warn(dev->class_dev,
			 "async subdevices must have a do_cmdtest() function\n");
		return -EINVAL;
	}

	async = kzalloc(sizeof(*async), GFP_KERNEL);
	if (!async)
		return -ENOMEM;

	init_waitqueue_head(&async->wait_head);
	async->subdevice = s;
	s->async = async;

	async->max_bufsize = comedi_default_buf_maxsize_kb * 1024;
	buf_size = comedi_default_buf_size_kb * 1024;
	if (buf_size > async->max_bufsize)
		buf_size = async->max_bufsize;

	if (comedi_buf_alloc(dev, s, buf_size) < 0) {
		dev_warn(dev->class_dev, "Buffer allocation failed\n");
		return -ENOMEM;
	}
	if (s->buf_change) {
		ret = s->buf_change(dev, s, buf_size);
		if (ret < 0)
			return ret;
	}

	comedi_alloc_subdevice_minor(s);

	return 0;
}

static int __comedi_device_postconfig(struct comedi_device *dev)
{
	struct comedi_subdevice *s;
	int ret;
	int i;

	for (i = 0; i < dev->n_subdevices; i++) {
		s = &dev->subdevices[i];

		if (s->type == COMEDI_SUBD_UNUSED)
			continue;

		if (s->len_chanlist == 0)
			s->len_chanlist = 1;

		if (s->do_cmd) {
			ret = __comedi_device_postconfig_async(dev, s);
			if (ret)
				return ret;
		}

		if (!s->range_table && !s->range_table_list)
			s->range_table = &range_unknown;

		if (!s->insn_read && s->insn_bits)
			s->insn_read = insn_rw_emulate_bits;
		if (!s->insn_write && s->insn_bits)
			s->insn_write = insn_rw_emulate_bits;

		if (!s->insn_read)
			s->insn_read = insn_inval;
		if (!s->insn_write)
			s->insn_write = insn_inval;
		if (!s->insn_bits)
			s->insn_bits = insn_inval;
		if (!s->insn_config)
			s->insn_config = insn_inval;

		if (!s->poll)
			s->poll = poll_invalid;
	}

	return 0;
}

/* do a little post-config cleanup */
static int comedi_device_postconfig(struct comedi_device *dev)
{
	int ret;

	ret = __comedi_device_postconfig(dev);
	if (ret < 0)
		return ret;
	smp_wmb();
	dev->attached = true;
	return 0;
}

/*
 * Generic recognize function for drivers that register their supported
 * board names.
 *
 * 'driv->board_name' points to a 'const char *' member within the
 * zeroth element of an array of some private board information
 * structure, say 'struct foo_board' containing a member 'const char
 * *board_name' that is initialized to point to a board name string that
 * is one of the candidates matched against this function's 'name'
 * parameter.
 *
 * 'driv->offset' is the size of the private board information
 * structure, say 'sizeof(struct foo_board)', and 'driv->num_names' is
 * the length of the array of private board information structures.
 *
 * If one of the board names in the array of private board information
 * structures matches the name supplied to this function, the function
 * returns a pointer to the pointer to the board name, otherwise it
 * returns NULL.  The return value ends up in the 'board_ptr' member of
 * a 'struct comedi_device' that the low-level comedi driver's
 * 'attach()' hook can convert to a point to a particular element of its
 * array of private board information structures by subtracting the
 * offset of the member that points to the board name.  (No subtraction
 * is required if the board name pointer is the first member of the
 * private board information structure, which is generally the case.)
 */
static void *comedi_recognize(struct comedi_driver *driv, const char *name)
{
	char **name_ptr = (char **)driv->board_name;
	int i;

	for (i = 0; i < driv->num_names; i++) {
		if (strcmp(*name_ptr, name) == 0)
			return name_ptr;
		name_ptr = (void *)name_ptr + driv->offset;
	}

	return NULL;
}

static void comedi_report_boards(struct comedi_driver *driv)
{
	unsigned int i;
	const char *const *name_ptr;

	pr_info("comedi: valid board names for %s driver are:\n",
		driv->driver_name);

	name_ptr = driv->board_name;
	for (i = 0; i < driv->num_names; i++) {
		pr_info(" %s\n", *name_ptr);
		name_ptr = (const char **)((char *)name_ptr + driv->offset);
	}

	if (driv->num_names == 0)
		pr_info(" %s\n", driv->driver_name);
}

/**
 * comedi_load_firmware() - Request and load firmware for a device.
 * @dev: comedi_device struct
 * @hw_device: device struct for the comedi_device
 * @name: the name of the firmware image
 * @cb: callback to the upload the firmware image
 * @context: private context from the driver
 */
int comedi_load_firmware(struct comedi_device *dev,
			 struct device *device,
			 const char *name,
			 int (*cb)(struct comedi_device *dev,
				   const u8 *data, size_t size,
				   unsigned long context),
			 unsigned long context)
{
	const struct firmware *fw;
	int ret;

	if (!cb)
		return -EINVAL;

	ret = reject_firmware(&fw, name, device);
	if (ret == 0) {
		ret = cb(dev, fw->data, fw->size, context);
		release_firmware(fw);
	}

	return ret < 0 ? ret : 0;
}
EXPORT_SYMBOL_GPL(comedi_load_firmware);

/**
 * __comedi_request_region() - Request an I/O reqion for a legacy driver.
 * @dev: comedi_device struct
 * @start: base address of the I/O reqion
 * @len: length of the I/O region
 */
int __comedi_request_region(struct comedi_device *dev,
			    unsigned long start, unsigned long len)
{
	if (!start) {
		dev_warn(dev->class_dev,
			 "%s: a I/O base address must be specified\n",
			 dev->board_name);
		return -EINVAL;
	}

	if (!request_region(start, len, dev->board_name)) {
		dev_warn(dev->class_dev, "%s: I/O port conflict (%#lx,%lu)\n",
			 dev->board_name, start, len);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(__comedi_request_region);

/**
 * comedi_request_region() - Request an I/O reqion for a legacy driver.
 * @dev: comedi_device struct
 * @start: base address of the I/O reqion
 * @len: length of the I/O region
 */
int comedi_request_region(struct comedi_device *dev,
			  unsigned long start, unsigned long len)
{
	int ret;

	ret = __comedi_request_region(dev, start, len);
	if (ret == 0) {
		dev->iobase = start;
		dev->iolen = len;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(comedi_request_region);

/**
 * comedi_legacy_detach() - A generic (*detach) function for legacy drivers.
 * @dev: comedi_device struct
 */
void comedi_legacy_detach(struct comedi_device *dev)
{
	if (dev->irq) {
		free_irq(dev->irq, dev);
		dev->irq = 0;
	}
	if (dev->iobase && dev->iolen) {
		release_region(dev->iobase, dev->iolen);
		dev->iobase = 0;
		dev->iolen = 0;
	}
}
EXPORT_SYMBOL_GPL(comedi_legacy_detach);

int comedi_device_attach(struct comedi_device *dev, struct comedi_devconfig *it)
{
	struct comedi_driver *driv;
	int ret;

	if (dev->attached)
		return -EBUSY;

	mutex_lock(&comedi_drivers_list_lock);
	for (driv = comedi_drivers; driv; driv = driv->next) {
		if (!try_module_get(driv->module))
			continue;
		if (driv->num_names) {
			dev->board_ptr = comedi_recognize(driv, it->board_name);
			if (dev->board_ptr)
				break;
		} else if (strcmp(driv->driver_name, it->board_name) == 0)
			break;
		module_put(driv->module);
	}
	if (driv == NULL) {
		/*  recognize has failed if we get here */
		/*  report valid board names before returning error */
		for (driv = comedi_drivers; driv; driv = driv->next) {
			if (!try_module_get(driv->module))
				continue;
			comedi_report_boards(driv);
			module_put(driv->module);
		}
		ret = -EIO;
		goto out;
	}
	if (driv->attach == NULL) {
		/* driver does not support manual configuration */
		dev_warn(dev->class_dev,
			 "driver '%s' does not support attach using comedi_config\n",
			 driv->driver_name);
		module_put(driv->module);
		ret = -ENOSYS;
		goto out;
	}
	/* initialize dev->driver here so
	 * comedi_error() can be called from attach */
	dev->driver = driv;
	dev->board_name = dev->board_ptr ? *(const char **)dev->board_ptr
					 : dev->driver->driver_name;
	ret = driv->attach(dev, it);
	if (ret >= 0)
		ret = comedi_device_postconfig(dev);
	if (ret < 0) {
		comedi_device_detach(dev);
		module_put(driv->module);
	}
	/* On success, the driver module count has been incremented. */
out:
	mutex_unlock(&comedi_drivers_list_lock);
	return ret;
}

int comedi_auto_config(struct device *hardware_device,
		       struct comedi_driver *driver, unsigned long context)
{
	struct comedi_device *dev;
	int ret;

	if (!hardware_device) {
		pr_warn("BUG! comedi_auto_config called with NULL hardware_device\n");
		return -EINVAL;
	}
	if (!driver) {
		dev_warn(hardware_device,
			 "BUG! comedi_auto_config called with NULL comedi driver\n");
		return -EINVAL;
	}

	if (!driver->auto_attach) {
		dev_warn(hardware_device,
			 "BUG! comedi driver '%s' has no auto_attach handler\n",
			 driver->driver_name);
		return -EINVAL;
	}

	dev = comedi_alloc_board_minor(hardware_device);
	if (IS_ERR(dev))
		return PTR_ERR(dev);
	/* Note: comedi_alloc_board_minor() locked dev->mutex. */

	dev->driver = driver;
	dev->board_name = dev->driver->driver_name;
	ret = driver->auto_attach(dev, context);
	if (ret >= 0)
		ret = comedi_device_postconfig(dev);
	if (ret < 0)
		comedi_device_detach(dev);
	mutex_unlock(&dev->mutex);

	if (ret < 0)
		comedi_release_hardware_device(hardware_device);
	return ret;
}
EXPORT_SYMBOL_GPL(comedi_auto_config);

void comedi_auto_unconfig(struct device *hardware_device)
{
	if (hardware_device == NULL)
		return;
	comedi_release_hardware_device(hardware_device);
}
EXPORT_SYMBOL_GPL(comedi_auto_unconfig);

int comedi_driver_register(struct comedi_driver *driver)
{
	mutex_lock(&comedi_drivers_list_lock);
	driver->next = comedi_drivers;
	comedi_drivers = driver;
	mutex_unlock(&comedi_drivers_list_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(comedi_driver_register);

void comedi_driver_unregister(struct comedi_driver *driver)
{
	struct comedi_driver *prev;
	int i;

	/* unlink the driver */
	mutex_lock(&comedi_drivers_list_lock);
	if (comedi_drivers == driver) {
		comedi_drivers = driver->next;
	} else {
		for (prev = comedi_drivers; prev->next; prev = prev->next) {
			if (prev->next == driver) {
				prev->next = driver->next;
				break;
			}
		}
	}
	mutex_unlock(&comedi_drivers_list_lock);

	/* check for devices using this driver */
	for (i = 0; i < COMEDI_NUM_BOARD_MINORS; i++) {
		struct comedi_device *dev = comedi_dev_from_minor(i);

		if (!dev)
			continue;

		mutex_lock(&dev->mutex);
		if (dev->attached && dev->driver == driver) {
			if (dev->use_count)
				dev_warn(dev->class_dev,
					 "BUG! detaching device with use_count=%d\n",
					 dev->use_count);
			comedi_device_detach(dev);
		}
		mutex_unlock(&dev->mutex);
	}
}
EXPORT_SYMBOL_GPL(comedi_driver_unregister);
