/*
 * mtip32xx.h - Header file for the P320 SSD Block Driver
 *   Copyright (C) 2011 Micron Technology, Inc.
 *
 * Portions of this code were derived from works subjected to the
 * following copyright:
 *    Copyright (C) 2009 Integrated Device Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MTIP32XX_H__
#define __MTIP32XX_H__

#include <fikus/spinlock.h>
#include <fikus/rwsem.h>
#include <fikus/ata.h>
#include <fikus/interrupt.h>
#include <fikus/genhd.h>

/* Offset of Subsystem Device ID in pci confoguration space */
#define PCI_SUBSYSTEM_DEVICEID	0x2E

/* offset of Device Control register in PCIe extended capabilites space */
#define PCIE_CONFIG_EXT_DEVICE_CONTROL_OFFSET	0x48

/* check for erase mode support during secure erase */
#define MTIP_SEC_ERASE_MODE     0x2

/* # of times to retry timed out/failed IOs */
#define MTIP_MAX_RETRIES	2

/* Various timeout values in ms */
#define MTIP_NCQ_COMMAND_TIMEOUT_MS       5000
#define MTIP_IOCTL_COMMAND_TIMEOUT_MS     5000
#define MTIP_INTERNAL_COMMAND_TIMEOUT_MS  5000

/* check for timeouts every 500ms */
#define MTIP_TIMEOUT_CHECK_PERIOD	500

/* ftl rebuild */
#define MTIP_FTL_REBUILD_OFFSET		142
#define MTIP_FTL_REBUILD_MAGIC		0xED51
#define MTIP_FTL_REBUILD_TIMEOUT_MS	2400000

/* unaligned IO handling */
#define MTIP_MAX_UNALIGNED_SLOTS	8

/* Macro to extract the tag bit number from a tag value. */
#define MTIP_TAG_BIT(tag)	(tag & 0x1F)

/*
 * Macro to extract the tag index from a tag value. The index
 * is used to access the correct s_active/Command Issue register based
 * on the tag value.
 */
#define MTIP_TAG_INDEX(tag)	(tag >> 5)

/*
 * Maximum number of scatter gather entries
 * a single command may have.
 */
#define MTIP_MAX_SG		128

/*
 * Maximum number of slot groups (Command Issue & s_active registers)
 * NOTE: This is the driver maximum; check dd->slot_groups for actual value.
 */
#define MTIP_MAX_SLOT_GROUPS	8

/* Internal command tag. */
#define MTIP_TAG_INTERNAL	0

/* Micron Vendor ID & P320x SSD Device ID */
#define PCI_VENDOR_ID_MICRON    0x1344
#define P320H_DEVICE_ID		0x5150
#define P320M_DEVICE_ID		0x5151
#define P320S_DEVICE_ID		0x5152
#define P325M_DEVICE_ID		0x5153
#define P420H_DEVICE_ID		0x5160
#define P420M_DEVICE_ID		0x5161
#define P425M_DEVICE_ID		0x5163

/* Driver name and version strings */
#define MTIP_DRV_NAME		"mtip32xx"
#define MTIP_DRV_VERSION	"1.2.6os3"

/* Maximum number of minor device numbers per device. */
#define MTIP_MAX_MINORS		16

/* Maximum number of supported command slots. */
#define MTIP_MAX_COMMAND_SLOTS	(MTIP_MAX_SLOT_GROUPS * 32)

/*
 * Per-tag bitfield size in longs.
 * Fikus bit manipulation functions
 * (i.e. test_and_set_bit, find_next_zero_bit)
 * manipulate memory in longs, so we try to make the math work.
 * take the slot groups and find the number of longs, rounding up.
 * Careful! i386 and x86_64 use different size longs!
 */
#define U32_PER_LONG	(sizeof(long) / sizeof(u32))
#define SLOTBITS_IN_LONGS ((MTIP_MAX_SLOT_GROUPS + \
					(U32_PER_LONG-1))/U32_PER_LONG)

/* BAR number used to access the HBA registers. */
#define MTIP_ABAR		5

#ifdef DEBUG
 #define dbg_printk(format, arg...)	\
	printk(pr_fmt(format), ##arg);
#else
 #define dbg_printk(format, arg...)
#endif

#define MTIP_DFS_MAX_BUF_SIZE 1024

#define __force_bit2int (unsigned int __force)

enum {
	/* below are bit numbers in 'flags' defined in mtip_port */
	MTIP_PF_IC_ACTIVE_BIT       = 0, /* pio/ioctl */
	MTIP_PF_EH_ACTIVE_BIT       = 1, /* error handling */
	MTIP_PF_SE_ACTIVE_BIT       = 2, /* secure erase */
	MTIP_PF_DM_ACTIVE_BIT       = 3, /* download microcde */
	MTIP_PF_PAUSE_IO      =	((1 << MTIP_PF_IC_ACTIVE_BIT) |
				(1 << MTIP_PF_EH_ACTIVE_BIT) |
				(1 << MTIP_PF_SE_ACTIVE_BIT) |
				(1 << MTIP_PF_DM_ACTIVE_BIT)),

	MTIP_PF_SVC_THD_ACTIVE_BIT  = 4,
	MTIP_PF_ISSUE_CMDS_BIT      = 5,
	MTIP_PF_REBUILD_BIT         = 6,
	MTIP_PF_SVC_THD_STOP_BIT    = 8,

	/* below are bit numbers in 'dd_flag' defined in driver_data */
	MTIP_DDF_SEC_LOCK_BIT	    = 0,
	MTIP_DDF_REMOVE_PENDING_BIT = 1,
	MTIP_DDF_OVER_TEMP_BIT      = 2,
	MTIP_DDF_WRITE_PROTECT_BIT  = 3,
	MTIP_DDF_STOP_IO      = ((1 << MTIP_DDF_REMOVE_PENDING_BIT) |
				(1 << MTIP_DDF_SEC_LOCK_BIT) |
				(1 << MTIP_DDF_OVER_TEMP_BIT) |
				(1 << MTIP_DDF_WRITE_PROTECT_BIT)),

	MTIP_DDF_CLEANUP_BIT        = 5,
	MTIP_DDF_RESUME_BIT         = 6,
	MTIP_DDF_INIT_DONE_BIT      = 7,
	MTIP_DDF_REBUILD_FAILED_BIT = 8,
};

struct smart_attr {
	u8 attr_id;
	u16 flags;
	u8 cur;
	u8 worst;
	u32 data;
	u8 res[3];
} __packed;

struct mtip_work {
	struct work_struct work;
	void *port;
	int cpu_binding;
	u32 completed;
} ____cacheline_aligned_in_smp;

#define DEFINE_HANDLER(group)                                  \
	void mtip_workq_sdbf##group(struct work_struct *work)       \
	{                                                      \
		struct mtip_work *w = (struct mtip_work *) work;         \
		mtip_workq_sdbfx(w->port, group, w->completed);     \
	}

#define MTIP_TRIM_TIMEOUT_MS		240000
#define MTIP_MAX_TRIM_ENTRIES		8
#define MTIP_MAX_TRIM_ENTRY_LEN		0xfff8

struct mtip_trim_entry {
	u32 lba;   /* starting lba of region */
	u16 rsvd;  /* unused */
	u16 range; /* # of 512b blocks to trim */
} __packed;

struct mtip_trim {
	/* Array of regions to trim */
	struct mtip_trim_entry entry[MTIP_MAX_TRIM_ENTRIES];
} __packed;

/* Register Frame Information Structure (FIS), host to device. */
struct host_to_dev_fis {
	/*
	 * FIS type.
	 * - 27h Register FIS, host to device.
	 * - 34h Register FIS, device to host.
	 * - 39h DMA Activate FIS, device to host.
	 * - 41h DMA Setup FIS, bi-directional.
	 * - 46h Data FIS, bi-directional.
	 * - 58h BIST Activate FIS, bi-directional.
	 * - 5Fh PIO Setup FIS, device to host.
	 * - A1h Set Device Bits FIS, device to host.
	 */
	unsigned char type;
	unsigned char opts;
	unsigned char command;
	unsigned char features;

	union {
		unsigned char lba_low;
		unsigned char sector;
	};
	union {
		unsigned char lba_mid;
		unsigned char cyl_low;
	};
	union {
		unsigned char lba_hi;
		unsigned char cyl_hi;
	};
	union {
		unsigned char device;
		unsigned char head;
	};

	union {
		unsigned char lba_low_ex;
		unsigned char sector_ex;
	};
	union {
		unsigned char lba_mid_ex;
		unsigned char cyl_low_ex;
	};
	union {
		unsigned char lba_hi_ex;
		unsigned char cyl_hi_ex;
	};
	unsigned char features_ex;

	unsigned char sect_count;
	unsigned char sect_cnt_ex;
	unsigned char res2;
	unsigned char control;

	unsigned int res3;
};

/* Command header structure. */
struct mtip_cmd_hdr {
	/*
	 * Command options.
	 * - Bits 31:16 Number of PRD entries.
	 * - Bits 15:8 Unused in this implementation.
	 * - Bit 7 Prefetch bit, informs the drive to prefetch PRD entries.
	 * - Bit 6 Write bit, should be set when writing data to the device.
	 * - Bit 5 Unused in this implementation.
	 * - Bits 4:0 Length of the command FIS in DWords (DWord = 4 bytes).
	 */
	unsigned int opts;
	/* This field is unsed when using NCQ. */
	union {
		unsigned int byte_count;
		unsigned int status;
	};
	/*
	 * Lower 32 bits of the command table address associated with this
	 * header. The command table addresses must be 128 byte aligned.
	 */
	unsigned int ctba;
	/*
	 * If 64 bit addressing is used this field is the upper 32 bits
	 * of the command table address associated with this command.
	 */
	unsigned int ctbau;
	/* Reserved and unused. */
	unsigned int res[4];
};

/* Command scatter gather structure (PRD). */
struct mtip_cmd_sg {
	/*
	 * Low 32 bits of the data buffer address. For P320 this
	 * address must be 8 byte aligned signified by bits 2:0 being
	 * set to 0.
	 */
	unsigned int dba;
	/*
	 * When 64 bit addressing is used this field is the upper
	 * 32 bits of the data buffer address.
	 */
	unsigned int dba_upper;
	/* Unused. */
	unsigned int reserved;
	/*
	 * Bit 31: interrupt when this data block has been transferred.
	 * Bits 30..22: reserved
	 * Bits 21..0: byte count (minus 1).  For P320 the byte count must be
	 * 8 byte aligned signified by bits 2:0 being set to 1.
	 */
	unsigned int info;
};
struct mtip_port;

/* Structure used to describe a command. */
struct mtip_cmd {

	struct mtip_cmd_hdr *command_header; /* ptr to command header entry */

	dma_addr_t command_header_dma; /* corresponding physical address */

	void *command; /* ptr to command table entry */

	dma_addr_t command_dma; /* corresponding physical address */

	void *comp_data; /* data passed to completion function comp_func() */
	/*
	 * Completion function called by the ISR upon completion of
	 * a command.
	 */
	void (*comp_func)(struct mtip_port *port,
				int tag,
				void *data,
				int status);
	/* Additional callback function that may be called by comp_func() */
	void (*async_callback)(void *data, int status);

	void *async_data; /* Addl. data passed to async_callback() */

	int scatter_ents; /* Number of scatter list entries used */

	int unaligned; /* command is unaligned on 4k boundary */

	struct scatterlist sg[MTIP_MAX_SG]; /* Scatter list entries */

	int retries; /* The number of retries left for this command. */

	int direction; /* Data transfer direction */

	unsigned long comp_time; /* command completion time, in jiffies */

	atomic_t active; /* declares if this command sent to the drive. */
};

/* Structure used to describe a port. */
struct mtip_port {
	/* Pointer back to the driver data for this port. */
	struct driver_data *dd;
	/*
	 * Used to determine if the data pointed to by the
	 * identify field is valid.
	 */
	unsigned long identify_valid;
	/* Base address of the memory mapped IO for the port. */
	void __iomem *mmio;
	/* Array of pointers to the memory mapped s_active registers. */
	void __iomem *s_active[MTIP_MAX_SLOT_GROUPS];
	/* Array of pointers to the memory mapped completed registers. */
	void __iomem *completed[MTIP_MAX_SLOT_GROUPS];
	/* Array of pointers to the memory mapped Command Issue registers. */
	void __iomem *cmd_issue[MTIP_MAX_SLOT_GROUPS];
	/*
	 * Pointer to the beginning of the command header memory as used
	 * by the driver.
	 */
	void *command_list;
	/*
	 * Pointer to the beginning of the command header memory as used
	 * by the DMA.
	 */
	dma_addr_t command_list_dma;
	/*
	 * Pointer to the beginning of the RX FIS memory as used
	 * by the driver.
	 */
	void *rxfis;
	/*
	 * Pointer to the beginning of the RX FIS memory as used
	 * by the DMA.
	 */
	dma_addr_t rxfis_dma;
	/*
	 * Pointer to the beginning of the command table memory as used
	 * by the driver.
	 */
	void *command_table;
	/*
	 * Pointer to the beginning of the command table memory as used
	 * by the DMA.
	 */
	dma_addr_t command_tbl_dma;
	/*
	 * Pointer to the beginning of the identify data memory as used
	 * by the driver.
	 */
	u16 *identify;
	/*
	 * Pointer to the beginning of the identify data memory as used
	 * by the DMA.
	 */
	dma_addr_t identify_dma;
	/*
	 * Pointer to the beginning of a sector buffer that is used
	 * by the driver when issuing internal commands.
	 */
	u16 *sector_buffer;
	/*
	 * Pointer to the beginning of a sector buffer that is used
	 * by the DMA when the driver issues internal commands.
	 */
	dma_addr_t sector_buffer_dma;
	/*
	 * Bit significant, used to determine if a command slot has
	 * been allocated. i.e. the slot is in use.  Bits are cleared
	 * when the command slot and all associated data structures
	 * are no longer needed.
	 */
	u16 *log_buf;
	dma_addr_t log_buf_dma;

	u8 *smart_buf;
	dma_addr_t smart_buf_dma;

	unsigned long allocated[SLOTBITS_IN_LONGS];
	/*
	 * used to queue commands when an internal command is in progress
	 * or error handling is active
	 */
	unsigned long cmds_to_issue[SLOTBITS_IN_LONGS];
	/*
	 * Array of command slots. Structure includes pointers to the
	 * command header and command table, and completion function and data
	 * pointers.
	 */
	struct mtip_cmd commands[MTIP_MAX_COMMAND_SLOTS];
	/* Used by mtip_service_thread to wait for an event */
	wait_queue_head_t svc_wait;
	/*
	 * indicates the state of the port. Also, helps the service thread
	 * to determine its action on wake up.
	 */
	unsigned long flags;
	/*
	 * Timer used to complete commands that have been active for too long.
	 */
	struct timer_list cmd_timer;
	unsigned long ic_pause_timer;
	/*
	 * Semaphore used to block threads if there are no
	 * command slots available.
	 */
	struct semaphore cmd_slot;

	/* Semaphore to control queue depth of unaligned IOs */
	struct semaphore cmd_slot_unal;

	/* Spinlock for working around command-issue bug. */
	spinlock_t cmd_issue_lock[MTIP_MAX_SLOT_GROUPS];
};

/*
 * Driver private data structure.
 *
 * One structure is allocated per probed device.
 */
struct driver_data {
	void __iomem *mmio; /* Base address of the HBA registers. */

	int major; /* Major device number. */

	int instance; /* Instance number. First device probed is 0, ... */

	struct gendisk *disk; /* Pointer to our gendisk structure. */

	struct pci_dev *pdev; /* Pointer to the PCI device structure. */

	struct request_queue *queue; /* Our request queue. */

	struct mtip_port *port; /* Pointer to the port data structure. */

	unsigned product_type; /* magic value declaring the product type */

	unsigned slot_groups; /* number of slot groups the product supports */

	unsigned long index; /* Index to determine the disk name */

	unsigned long dd_flag; /* NOTE: use atomic bit operations on this */

	struct task_struct *mtip_svc_handler; /* task_struct of svc thd */

	struct dentry *dfs_node;

	bool trim_supp; /* flag indicating trim support */

	int numa_node; /* NUMA support */

	char workq_name[32];

	struct workqueue_struct *isr_workq;

	struct mtip_work work[MTIP_MAX_SLOT_GROUPS];

	atomic_t irq_workers_active;

	int isr_binding;

	int unal_qdepth; /* qdepth of unaligned IO queue */

	struct list_head online_list; /* linkage for online list */

	struct list_head remove_list; /* linkage for removing list */
};

#endif
