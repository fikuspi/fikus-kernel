/*
 * PTP 1588 clock support - private declarations for the core module.
 *
 * Copyright (C) 2010 OMICRON electronics GmbH
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _PTP_PRIVATE_H_
#define _PTP_PRIVATE_H_

#include <fikus/cdev.h>
#include <fikus/device.h>
#include <fikus/mutex.h>
#include <fikus/posix-clock.h>
#include <fikus/ptp_clock.h>
#include <fikus/ptp_clock_kernel.h>
#include <fikus/time.h>

#define PTP_MAX_TIMESTAMPS 128
#define PTP_BUF_TIMESTAMPS 30

struct timestamp_event_queue {
	struct ptp_extts_event buf[PTP_MAX_TIMESTAMPS];
	int head;
	int tail;
	spinlock_t lock;
};

struct ptp_clock {
	struct posix_clock clock;
	struct device *dev;
	struct ptp_clock_info *info;
	dev_t devid;
	int index; /* index into clocks.map */
	struct pps_device *pps_source;
	long dialed_frequency; /* remembers the frequency adjustment */
	struct timestamp_event_queue tsevq; /* simple fifo for time stamps */
	struct mutex tsevq_mux; /* one process at a time reading the fifo */
	wait_queue_head_t tsev_wq;
	int defunct; /* tells readers to go away when clock is being removed */
};

/*
 * The function queue_cnt() is safe for readers to call without
 * holding q->lock. Readers use this function to verify that the queue
 * is nonempty before proceeding with a dequeue operation. The fact
 * that a writer might concurrently increment the tail does not
 * matter, since the queue remains nonempty nonetheless.
 */
static inline int queue_cnt(struct timestamp_event_queue *q)
{
	int cnt = q->tail - q->head;
	return cnt < 0 ? PTP_MAX_TIMESTAMPS + cnt : cnt;
}

/*
 * see ptp_chardev.c
 */

long ptp_ioctl(struct posix_clock *pc,
	       unsigned int cmd, unsigned long arg);

int ptp_open(struct posix_clock *pc, fmode_t fmode);

ssize_t ptp_read(struct posix_clock *pc,
		 uint flags, char __user *buf, size_t cnt);

uint ptp_poll(struct posix_clock *pc,
	      struct file *fp, poll_table *wait);

/*
 * see ptp_sysfs.c
 */

extern const struct attribute_group *ptp_groups[];

int ptp_cleanup_sysfs(struct ptp_clock *ptp);

int ptp_populate_sysfs(struct ptp_clock *ptp);

#endif
