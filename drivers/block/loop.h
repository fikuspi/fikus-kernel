/*
 * loop.h
 *
 * Written by Theodore Ts'o, 3/29/93.
 *
 * Copyright 1993 by Theodore Ts'o.  Redistribution of this file is
 * permitted under the GNU General Public License.
 */
#ifndef _FIKUS_LOOP_H
#define _FIKUS_LOOP_H

#include <fikus/bio.h>
#include <fikus/blkdev.h>
#include <fikus/spinlock.h>
#include <fikus/mutex.h>
#include <uapi/fikus/loop.h>

/* Possible states of device */
enum {
	Lo_unbound,
	Lo_bound,
	Lo_rundown,
};

struct loop_func_table;

struct loop_device {
	int		lo_number;
	int		lo_refcnt;
	loff_t		lo_offset;
	loff_t		lo_sizelimit;
	int		lo_flags;
	int		(*transfer)(struct loop_device *, int cmd,
				    struct page *raw_page, unsigned raw_off,
				    struct page *loop_page, unsigned loop_off,
				    int size, sector_t real_block);
	char		lo_file_name[LO_NAME_SIZE];
	char		lo_crypt_name[LO_NAME_SIZE];
	char		lo_encrypt_key[LO_KEY_SIZE];
	int		lo_encrypt_key_size;
	struct loop_func_table *lo_encryption;
	__u32           lo_init[2];
	kuid_t		lo_key_owner;	/* Who set the key */
	int		(*ioctl)(struct loop_device *, int cmd, 
				 unsigned long arg); 

	struct file *	lo_backing_file;
	struct block_device *lo_device;
	unsigned	lo_blocksize;
	void		*key_data; 

	gfp_t		old_gfp_mask;

	spinlock_t		lo_lock;
	struct bio_list		lo_bio_list;
	unsigned int		lo_bio_count;
	int			lo_state;
	struct mutex		lo_ctl_mutex;
	struct task_struct	*lo_thread;
	wait_queue_head_t	lo_event;
	/* wait queue for incoming requests */
	wait_queue_head_t	lo_req_wait;

	struct request_queue	*lo_queue;
	struct gendisk		*lo_disk;
};

/* Support for loadable transfer modules */
struct loop_func_table {
	int number;	/* filter type */ 
	int (*transfer)(struct loop_device *lo, int cmd,
			struct page *raw_page, unsigned raw_off,
			struct page *loop_page, unsigned loop_off,
			int size, sector_t real_block);
	int (*init)(struct loop_device *, const struct loop_info64 *); 
	/* release is called from loop_unregister_transfer or clr_fd */
	int (*release)(struct loop_device *); 
	int (*ioctl)(struct loop_device *, int cmd, unsigned long arg);
	struct module *owner;
}; 

int loop_register_transfer(struct loop_func_table *funcs);
int loop_unregister_transfer(int number); 

#endif
