/*
 * Copyright (C) 2010-2012 Michael Hennerich, Analog Devices Inc.
 * Copyright (C) 2008-2010 Jonathan Cameron
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * ad799x_ring.c
 */

#include <fikus/interrupt.h>
#include <fikus/slab.h>
#include <fikus/kernel.h>
#include <fikus/list.h>
#include <fikus/i2c.h>
#include <fikus/bitops.h>

#include <fikus/iio/iio.h>
#include <fikus/iio/buffer.h>
#include <fikus/iio/trigger_consumer.h>
#include <fikus/iio/triggered_buffer.h>

#include "ad799x.h"

/**
 * ad799x_trigger_handler() bh of trigger launched polling to ring buffer
 *
 * Currently there is no option in this driver to disable the saving of
 * timestamps within the ring.
 **/

static irqreturn_t ad799x_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct ad799x_state *st = iio_priv(indio_dev);
	s64 time_ns;
	int b_sent;
	u8 cmd;

	switch (st->id) {
	case ad7991:
	case ad7995:
	case ad7999:
		cmd = st->config |
			(*indio_dev->active_scan_mask << AD799X_CHANNEL_SHIFT);
		break;
	case ad7992:
	case ad7993:
	case ad7994:
		cmd = (*indio_dev->active_scan_mask << AD799X_CHANNEL_SHIFT) |
			AD7998_CONV_RES_REG;
		break;
	case ad7997:
	case ad7998:
		cmd = AD7997_8_READ_SEQUENCE | AD7998_CONV_RES_REG;
		break;
	default:
		cmd = 0;
	}

	b_sent = i2c_smbus_read_i2c_block_data(st->client,
			cmd, st->transfer_size, st->rx_buf);
	if (b_sent < 0)
		goto out;

	time_ns = iio_get_time_ns();

	if (indio_dev->scan_timestamp)
		memcpy(st->rx_buf + indio_dev->scan_bytes - sizeof(s64),
			&time_ns, sizeof(time_ns));

	iio_push_to_buffers(indio_dev, st->rx_buf);
out:
	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

int ad799x_register_ring_funcs_and_init(struct iio_dev *indio_dev)
{
	return iio_triggered_buffer_setup(indio_dev, NULL,
		&ad799x_trigger_handler, NULL);
}

void ad799x_ring_cleanup(struct iio_dev *indio_dev)
{
	iio_triggered_buffer_cleanup(indio_dev);
}
