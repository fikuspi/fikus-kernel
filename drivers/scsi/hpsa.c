/*
 *    Disk Array driver for HP Smart Array SAS controllers
 *    Copyright 2000, 2009 Hewlett-Packard Development Company, L.P.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; version 2 of the License.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *    NON INFRINGEMENT.  See the GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *    Questions/Comments/Bugfixes to iss_storagedev@hp.com
 *
 */

#include <fikus/module.h>
#include <fikus/interrupt.h>
#include <fikus/types.h>
#include <fikus/pci.h>
#include <fikus/pci-aspm.h>
#include <fikus/kernel.h>
#include <fikus/slab.h>
#include <fikus/delay.h>
#include <fikus/fs.h>
#include <fikus/timer.h>
#include <fikus/seq_file.h>
#include <fikus/init.h>
#include <fikus/spinlock.h>
#include <fikus/compat.h>
#include <fikus/blktrace_api.h>
#include <fikus/uaccess.h>
#include <fikus/io.h>
#include <fikus/dma-mapping.h>
#include <fikus/completion.h>
#include <fikus/moduleparam.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <fikus/cciss_ioctl.h>
#include <fikus/string.h>
#include <fikus/bitmap.h>
#include <fikus/atomic.h>
#include <fikus/kthread.h>
#include <fikus/jiffies.h>
#include "hpsa_cmd.h"
#include "hpsa.h"

/* HPSA_DRIVER_VERSION must be 3 byte values (0-255) separated by '.' */
#define HPSA_DRIVER_VERSION "3.4.0-1"
#define DRIVER_NAME "HP HPSA Driver (v " HPSA_DRIVER_VERSION ")"
#define HPSA "hpsa"

/* How long to wait (in milliseconds) for board to go into simple mode */
#define MAX_CONFIG_WAIT 30000
#define MAX_IOCTL_CONFIG_WAIT 1000

/*define how many times we will try a command because of bus resets */
#define MAX_CMD_RETRIES 3

/* Embedded module documentation macros - see modules.h */
MODULE_AUTHOR("Hewlett-Packard Company");
MODULE_DESCRIPTION("Driver for HP Smart Array Controller version " \
	HPSA_DRIVER_VERSION);
MODULE_SUPPORTED_DEVICE("HP Smart Array Controllers");
MODULE_VERSION(HPSA_DRIVER_VERSION);
MODULE_LICENSE("GPL");

static int hpsa_allow_any;
module_param(hpsa_allow_any, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(hpsa_allow_any,
		"Allow hpsa driver to access unknown HP Smart Array hardware");
static int hpsa_simple_mode;
module_param(hpsa_simple_mode, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(hpsa_simple_mode,
	"Use 'simple mode' rather than 'performant mode'");

/* define the PCI info for the cards we can control */
static const struct pci_device_id hpsa_pci_device_id[] = {
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSE,     0x103C, 0x3241},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSE,     0x103C, 0x3243},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSE,     0x103C, 0x3245},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSE,     0x103C, 0x3247},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSE,     0x103C, 0x3249},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSE,     0x103C, 0x324A},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSE,     0x103C, 0x324B},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSE,     0x103C, 0x3233},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSF,     0x103C, 0x3350},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSF,     0x103C, 0x3351},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSF,     0x103C, 0x3352},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSF,     0x103C, 0x3353},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSF,     0x103C, 0x334D},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSF,     0x103C, 0x3354},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSF,     0x103C, 0x3355},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSF,     0x103C, 0x3356},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSH,     0x103C, 0x1920},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSH,     0x103C, 0x1921},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSH,     0x103C, 0x1922},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSH,     0x103C, 0x1923},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSH,     0x103C, 0x1924},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSH,     0x103C, 0x1925},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSH,     0x103C, 0x1926},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSH,     0x103C, 0x1928},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSH,     0x103C, 0x1929},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21BD},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21BE},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21BF},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21C0},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21C1},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21C2},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21C3},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21C4},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21C5},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21C7},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21C8},
	{PCI_VENDOR_ID_HP,     PCI_DEVICE_ID_HP_CISSI,     0x103C, 0x21C9},
	{PCI_VENDOR_ID_HP,     PCI_ANY_ID,	PCI_ANY_ID, PCI_ANY_ID,
		PCI_CLASS_STORAGE_RAID << 8, 0xffff << 8, 0},
	{0,}
};

MODULE_DEVICE_TABLE(pci, hpsa_pci_device_id);

/*  board_id = Subsystem Device ID & Vendor ID
 *  product = Marketing Name for the board
 *  access = Address of the struct of function pointers
 */
static struct board_type products[] = {
	{0x3241103C, "Smart Array P212", &SA5_access},
	{0x3243103C, "Smart Array P410", &SA5_access},
	{0x3245103C, "Smart Array P410i", &SA5_access},
	{0x3247103C, "Smart Array P411", &SA5_access},
	{0x3249103C, "Smart Array P812", &SA5_access},
	{0x324A103C, "Smart Array P712m", &SA5_access},
	{0x324B103C, "Smart Array P711m", &SA5_access},
	{0x3350103C, "Smart Array P222", &SA5_access},
	{0x3351103C, "Smart Array P420", &SA5_access},
	{0x3352103C, "Smart Array P421", &SA5_access},
	{0x3353103C, "Smart Array P822", &SA5_access},
	{0x334D103C, "Smart Array P822se", &SA5_access},
	{0x3354103C, "Smart Array P420i", &SA5_access},
	{0x3355103C, "Smart Array P220i", &SA5_access},
	{0x3356103C, "Smart Array P721m", &SA5_access},
	{0x1921103C, "Smart Array P830i", &SA5_access},
	{0x1922103C, "Smart Array P430", &SA5_access},
	{0x1923103C, "Smart Array P431", &SA5_access},
	{0x1924103C, "Smart Array P830", &SA5_access},
	{0x1926103C, "Smart Array P731m", &SA5_access},
	{0x1928103C, "Smart Array P230i", &SA5_access},
	{0x1929103C, "Smart Array P530", &SA5_access},
	{0x21BD103C, "Smart Array", &SA5_access},
	{0x21BE103C, "Smart Array", &SA5_access},
	{0x21BF103C, "Smart Array", &SA5_access},
	{0x21C0103C, "Smart Array", &SA5_access},
	{0x21C1103C, "Smart Array", &SA5_access},
	{0x21C2103C, "Smart Array", &SA5_access},
	{0x21C3103C, "Smart Array", &SA5_access},
	{0x21C4103C, "Smart Array", &SA5_access},
	{0x21C5103C, "Smart Array", &SA5_access},
	{0x21C7103C, "Smart Array", &SA5_access},
	{0x21C8103C, "Smart Array", &SA5_access},
	{0x21C9103C, "Smart Array", &SA5_access},
	{0xFFFF103C, "Unknown Smart Array", &SA5_access},
};

static int number_of_controllers;

static struct list_head hpsa_ctlr_list = LIST_HEAD_INIT(hpsa_ctlr_list);
static spinlock_t lockup_detector_lock;
static struct task_struct *hpsa_lockup_detector;

static irqreturn_t do_hpsa_intr_intx(int irq, void *dev_id);
static irqreturn_t do_hpsa_intr_msi(int irq, void *dev_id);
static int hpsa_ioctl(struct scsi_device *dev, int cmd, void *arg);
static void start_io(struct ctlr_info *h);

#ifdef CONFIG_COMPAT
static int hpsa_compat_ioctl(struct scsi_device *dev, int cmd, void *arg);
#endif

static void cmd_free(struct ctlr_info *h, struct CommandList *c);
static void cmd_special_free(struct ctlr_info *h, struct CommandList *c);
static struct CommandList *cmd_alloc(struct ctlr_info *h);
static struct CommandList *cmd_special_alloc(struct ctlr_info *h);
static int fill_cmd(struct CommandList *c, u8 cmd, struct ctlr_info *h,
	void *buff, size_t size, u8 page_code, unsigned char *scsi3addr,
	int cmd_type);

static int hpsa_scsi_queue_command(struct Scsi_Host *h, struct scsi_cmnd *cmd);
static void hpsa_scan_start(struct Scsi_Host *);
static int hpsa_scan_finished(struct Scsi_Host *sh,
	unsigned long elapsed_time);
static int hpsa_change_queue_depth(struct scsi_device *sdev,
	int qdepth, int reason);

static int hpsa_eh_device_reset_handler(struct scsi_cmnd *scsicmd);
static int hpsa_eh_abort_handler(struct scsi_cmnd *scsicmd);
static int hpsa_slave_alloc(struct scsi_device *sdev);
static void hpsa_slave_destroy(struct scsi_device *sdev);

static void hpsa_update_scsi_devices(struct ctlr_info *h, int hostno);
static int check_for_unit_attention(struct ctlr_info *h,
	struct CommandList *c);
static void check_ioctl_unit_attention(struct ctlr_info *h,
	struct CommandList *c);
/* performant mode helper functions */
static void calc_bucket_map(int *bucket, int num_buckets,
	int nsgs, int *bucket_map);
static void hpsa_put_ctlr_into_performant_mode(struct ctlr_info *h);
static inline u32 next_command(struct ctlr_info *h, u8 q);
static int hpsa_find_cfg_addrs(struct pci_dev *pdev, void __iomem *vaddr,
			       u32 *cfg_base_addr, u64 *cfg_base_addr_index,
			       u64 *cfg_offset);
static int hpsa_pci_find_memory_BAR(struct pci_dev *pdev,
				    unsigned long *memory_bar);
static int hpsa_lookup_board_id(struct pci_dev *pdev, u32 *board_id);
static int hpsa_wait_for_board_state(struct pci_dev *pdev, void __iomem *vaddr,
				     int wait_for_ready);
static inline void finish_cmd(struct CommandList *c);
#define BOARD_NOT_READY 0
#define BOARD_READY 1

static inline struct ctlr_info *sdev_to_hba(struct scsi_device *sdev)
{
	unsigned long *priv = shost_priv(sdev->host);
	return (struct ctlr_info *) *priv;
}

static inline struct ctlr_info *shost_to_hba(struct Scsi_Host *sh)
{
	unsigned long *priv = shost_priv(sh);
	return (struct ctlr_info *) *priv;
}

static int check_for_unit_attention(struct ctlr_info *h,
	struct CommandList *c)
{
	if (c->err_info->SenseInfo[2] != UNIT_ATTENTION)
		return 0;

	switch (c->err_info->SenseInfo[12]) {
	case STATE_CHANGED:
		dev_warn(&h->pdev->dev, HPSA "%d: a state change "
			"detected, command retried\n", h->ctlr);
		break;
	case LUN_FAILED:
		dev_warn(&h->pdev->dev, HPSA "%d: LUN failure "
			"detected, action required\n", h->ctlr);
		break;
	case REPORT_LUNS_CHANGED:
		dev_warn(&h->pdev->dev, HPSA "%d: report LUN data "
			"changed, action required\n", h->ctlr);
	/*
	 * Note: this REPORT_LUNS_CHANGED condition only occurs on the external
	 * target (array) devices.
	 */
		break;
	case POWER_OR_RESET:
		dev_warn(&h->pdev->dev, HPSA "%d: a power on "
			"or device reset detected\n", h->ctlr);
		break;
	case UNIT_ATTENTION_CLEARED:
		dev_warn(&h->pdev->dev, HPSA "%d: unit attention "
		    "cleared by another initiator\n", h->ctlr);
		break;
	default:
		dev_warn(&h->pdev->dev, HPSA "%d: unknown "
			"unit attention detected\n", h->ctlr);
		break;
	}
	return 1;
}

static int check_for_busy(struct ctlr_info *h, struct CommandList *c)
{
	if (c->err_info->CommandStatus != CMD_TARGET_STATUS ||
		(c->err_info->ScsiStatus != SAM_STAT_BUSY &&
		 c->err_info->ScsiStatus != SAM_STAT_TASK_SET_FULL))
		return 0;
	dev_warn(&h->pdev->dev, HPSA "device busy");
	return 1;
}

static ssize_t host_store_rescan(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct ctlr_info *h;
	struct Scsi_Host *shost = class_to_shost(dev);
	h = shost_to_hba(shost);
	hpsa_scan_start(h->scsi_host);
	return count;
}

static ssize_t host_show_firmware_revision(struct device *dev,
	     struct device_attribute *attr, char *buf)
{
	struct ctlr_info *h;
	struct Scsi_Host *shost = class_to_shost(dev);
	unsigned char *fwrev;

	h = shost_to_hba(shost);
	if (!h->hba_inquiry_data)
		return 0;
	fwrev = &h->hba_inquiry_data[32];
	return snprintf(buf, 20, "%c%c%c%c\n",
		fwrev[0], fwrev[1], fwrev[2], fwrev[3]);
}

static ssize_t host_show_commands_outstanding(struct device *dev,
	     struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(dev);
	struct ctlr_info *h = shost_to_hba(shost);

	return snprintf(buf, 20, "%d\n", h->commands_outstanding);
}

static ssize_t host_show_transport_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ctlr_info *h;
	struct Scsi_Host *shost = class_to_shost(dev);

	h = shost_to_hba(shost);
	return snprintf(buf, 20, "%s\n",
		h->transMethod & CFGTBL_Trans_Performant ?
			"performant" : "simple");
}

/* List of controllers which cannot be hard reset on kexec with reset_devices */
static u32 unresettable_controller[] = {
	0x324a103C, /* Smart Array P712m */
	0x324b103C, /* SmartArray P711m */
	0x3223103C, /* Smart Array P800 */
	0x3234103C, /* Smart Array P400 */
	0x3235103C, /* Smart Array P400i */
	0x3211103C, /* Smart Array E200i */
	0x3212103C, /* Smart Array E200 */
	0x3213103C, /* Smart Array E200i */
	0x3214103C, /* Smart Array E200i */
	0x3215103C, /* Smart Array E200i */
	0x3237103C, /* Smart Array E500 */
	0x323D103C, /* Smart Array P700m */
	0x40800E11, /* Smart Array 5i */
	0x409C0E11, /* Smart Array 6400 */
	0x409D0E11, /* Smart Array 6400 EM */
	0x40700E11, /* Smart Array 5300 */
	0x40820E11, /* Smart Array 532 */
	0x40830E11, /* Smart Array 5312 */
	0x409A0E11, /* Smart Array 641 */
	0x409B0E11, /* Smart Array 642 */
	0x40910E11, /* Smart Array 6i */
};

/* List of controllers which cannot even be soft reset */
static u32 soft_unresettable_controller[] = {
	0x40800E11, /* Smart Array 5i */
	0x40700E11, /* Smart Array 5300 */
	0x40820E11, /* Smart Array 532 */
	0x40830E11, /* Smart Array 5312 */
	0x409A0E11, /* Smart Array 641 */
	0x409B0E11, /* Smart Array 642 */
	0x40910E11, /* Smart Array 6i */
	/* Exclude 640x boards.  These are two pci devices in one slot
	 * which share a battery backed cache module.  One controls the
	 * cache, the other accesses the cache through the one that controls
	 * it.  If we reset the one controlling the cache, the other will
	 * likely not be happy.  Just forbid resetting this conjoined mess.
	 * The 640x isn't really supported by hpsa anyway.
	 */
	0x409C0E11, /* Smart Array 6400 */
	0x409D0E11, /* Smart Array 6400 EM */
};

static int ctlr_is_hard_resettable(u32 board_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(unresettable_controller); i++)
		if (unresettable_controller[i] == board_id)
			return 0;
	return 1;
}

static int ctlr_is_soft_resettable(u32 board_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(soft_unresettable_controller); i++)
		if (soft_unresettable_controller[i] == board_id)
			return 0;
	return 1;
}

static int ctlr_is_resettable(u32 board_id)
{
	return ctlr_is_hard_resettable(board_id) ||
		ctlr_is_soft_resettable(board_id);
}

static ssize_t host_show_resettable(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ctlr_info *h;
	struct Scsi_Host *shost = class_to_shost(dev);

	h = shost_to_hba(shost);
	return snprintf(buf, 20, "%d\n", ctlr_is_resettable(h->board_id));
}

static inline int is_logical_dev_addr_mode(unsigned char scsi3addr[])
{
	return (scsi3addr[3] & 0xC0) == 0x40;
}

static const char *raid_label[] = { "0", "4", "1(1+0)", "5", "5+1", "ADG",
	"1(ADM)", "UNKNOWN"
};
#define RAID_UNKNOWN (ARRAY_SIZE(raid_label) - 1)

static ssize_t raid_level_show(struct device *dev,
	     struct device_attribute *attr, char *buf)
{
	ssize_t l = 0;
	unsigned char rlevel;
	struct ctlr_info *h;
	struct scsi_device *sdev;
	struct hpsa_scsi_dev_t *hdev;
	unsigned long flags;

	sdev = to_scsi_device(dev);
	h = sdev_to_hba(sdev);
	spin_lock_irqsave(&h->lock, flags);
	hdev = sdev->hostdata;
	if (!hdev) {
		spin_unlock_irqrestore(&h->lock, flags);
		return -ENODEV;
	}

	/* Is this even a logical drive? */
	if (!is_logical_dev_addr_mode(hdev->scsi3addr)) {
		spin_unlock_irqrestore(&h->lock, flags);
		l = snprintf(buf, PAGE_SIZE, "N/A\n");
		return l;
	}

	rlevel = hdev->raid_level;
	spin_unlock_irqrestore(&h->lock, flags);
	if (rlevel > RAID_UNKNOWN)
		rlevel = RAID_UNKNOWN;
	l = snprintf(buf, PAGE_SIZE, "RAID %s\n", raid_label[rlevel]);
	return l;
}

static ssize_t lunid_show(struct device *dev,
	     struct device_attribute *attr, char *buf)
{
	struct ctlr_info *h;
	struct scsi_device *sdev;
	struct hpsa_scsi_dev_t *hdev;
	unsigned long flags;
	unsigned char lunid[8];

	sdev = to_scsi_device(dev);
	h = sdev_to_hba(sdev);
	spin_lock_irqsave(&h->lock, flags);
	hdev = sdev->hostdata;
	if (!hdev) {
		spin_unlock_irqrestore(&h->lock, flags);
		return -ENODEV;
	}
	memcpy(lunid, hdev->scsi3addr, sizeof(lunid));
	spin_unlock_irqrestore(&h->lock, flags);
	return snprintf(buf, 20, "0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		lunid[0], lunid[1], lunid[2], lunid[3],
		lunid[4], lunid[5], lunid[6], lunid[7]);
}

static ssize_t unique_id_show(struct device *dev,
	     struct device_attribute *attr, char *buf)
{
	struct ctlr_info *h;
	struct scsi_device *sdev;
	struct hpsa_scsi_dev_t *hdev;
	unsigned long flags;
	unsigned char sn[16];

	sdev = to_scsi_device(dev);
	h = sdev_to_hba(sdev);
	spin_lock_irqsave(&h->lock, flags);
	hdev = sdev->hostdata;
	if (!hdev) {
		spin_unlock_irqrestore(&h->lock, flags);
		return -ENODEV;
	}
	memcpy(sn, hdev->device_id, sizeof(sn));
	spin_unlock_irqrestore(&h->lock, flags);
	return snprintf(buf, 16 * 2 + 2,
			"%02X%02X%02X%02X%02X%02X%02X%02X"
			"%02X%02X%02X%02X%02X%02X%02X%02X\n",
			sn[0], sn[1], sn[2], sn[3],
			sn[4], sn[5], sn[6], sn[7],
			sn[8], sn[9], sn[10], sn[11],
			sn[12], sn[13], sn[14], sn[15]);
}

static DEVICE_ATTR(raid_level, S_IRUGO, raid_level_show, NULL);
static DEVICE_ATTR(lunid, S_IRUGO, lunid_show, NULL);
static DEVICE_ATTR(unique_id, S_IRUGO, unique_id_show, NULL);
static DEVICE_ATTR(rescan, S_IWUSR, NULL, host_store_rescan);
static DEVICE_ATTR(firmware_revision, S_IRUGO,
	host_show_firmware_revision, NULL);
static DEVICE_ATTR(commands_outstanding, S_IRUGO,
	host_show_commands_outstanding, NULL);
static DEVICE_ATTR(transport_mode, S_IRUGO,
	host_show_transport_mode, NULL);
static DEVICE_ATTR(resettable, S_IRUGO,
	host_show_resettable, NULL);

static struct device_attribute *hpsa_sdev_attrs[] = {
	&dev_attr_raid_level,
	&dev_attr_lunid,
	&dev_attr_unique_id,
	NULL,
};

static struct device_attribute *hpsa_shost_attrs[] = {
	&dev_attr_rescan,
	&dev_attr_firmware_revision,
	&dev_attr_commands_outstanding,
	&dev_attr_transport_mode,
	&dev_attr_resettable,
	NULL,
};

static struct scsi_host_template hpsa_driver_template = {
	.module			= THIS_MODULE,
	.name			= HPSA,
	.proc_name		= HPSA,
	.queuecommand		= hpsa_scsi_queue_command,
	.scan_start		= hpsa_scan_start,
	.scan_finished		= hpsa_scan_finished,
	.change_queue_depth	= hpsa_change_queue_depth,
	.this_id		= -1,
	.use_clustering		= ENABLE_CLUSTERING,
	.eh_abort_handler	= hpsa_eh_abort_handler,
	.eh_device_reset_handler = hpsa_eh_device_reset_handler,
	.ioctl			= hpsa_ioctl,
	.slave_alloc		= hpsa_slave_alloc,
	.slave_destroy		= hpsa_slave_destroy,
#ifdef CONFIG_COMPAT
	.compat_ioctl		= hpsa_compat_ioctl,
#endif
	.sdev_attrs = hpsa_sdev_attrs,
	.shost_attrs = hpsa_shost_attrs,
	.max_sectors = 8192,
	.no_write_same = 1,
};


/* Enqueuing and dequeuing functions for cmdlists. */
static inline void addQ(struct list_head *list, struct CommandList *c)
{
	list_add_tail(&c->list, list);
}

static inline u32 next_command(struct ctlr_info *h, u8 q)
{
	u32 a;
	struct reply_pool *rq = &h->reply_queue[q];
	unsigned long flags;

	if (unlikely(!(h->transMethod & CFGTBL_Trans_Performant)))
		return h->access.command_completed(h, q);

	if ((rq->head[rq->current_entry] & 1) == rq->wraparound) {
		a = rq->head[rq->current_entry];
		rq->current_entry++;
		spin_lock_irqsave(&h->lock, flags);
		h->commands_outstanding--;
		spin_unlock_irqrestore(&h->lock, flags);
	} else {
		a = FIFO_EMPTY;
	}
	/* Check for wraparound */
	if (rq->current_entry == h->max_commands) {
		rq->current_entry = 0;
		rq->wraparound ^= 1;
	}
	return a;
}

/* set_performant_mode: Modify the tag for cciss performant
 * set bit 0 for pull model, bits 3-1 for block fetch
 * register number
 */
static void set_performant_mode(struct ctlr_info *h, struct CommandList *c)
{
	if (likely(h->transMethod & CFGTBL_Trans_Performant)) {
		c->busaddr |= 1 | (h->blockFetchTable[c->Header.SGList] << 1);
		if (likely(h->msix_vector))
			c->Header.ReplyQueue =
				raw_smp_processor_id() % h->nreply_queues;
	}
}

static int is_firmware_flash_cmd(u8 *cdb)
{
	return cdb[0] == BMIC_WRITE && cdb[6] == BMIC_FLASH_FIRMWARE;
}

/*
 * During firmware flash, the heartbeat register may not update as frequently
 * as it should.  So we dial down lockup detection during firmware flash. and
 * dial it back up when firmware flash completes.
 */
#define HEARTBEAT_SAMPLE_INTERVAL_DURING_FLASH (240 * HZ)
#define HEARTBEAT_SAMPLE_INTERVAL (30 * HZ)
static void dial_down_lockup_detection_during_fw_flash(struct ctlr_info *h,
		struct CommandList *c)
{
	if (!is_firmware_flash_cmd(c->Request.CDB))
		return;
	atomic_inc(&h->firmware_flash_in_progress);
	h->heartbeat_sample_interval = HEARTBEAT_SAMPLE_INTERVAL_DURING_FLASH;
}

static void dial_up_lockup_detection_on_fw_flash_complete(struct ctlr_info *h,
		struct CommandList *c)
{
	if (is_firmware_flash_cmd(c->Request.CDB) &&
		atomic_dec_and_test(&h->firmware_flash_in_progress))
		h->heartbeat_sample_interval = HEARTBEAT_SAMPLE_INTERVAL;
}

static void enqueue_cmd_and_start_io(struct ctlr_info *h,
	struct CommandList *c)
{
	unsigned long flags;

	set_performant_mode(h, c);
	dial_down_lockup_detection_during_fw_flash(h, c);
	spin_lock_irqsave(&h->lock, flags);
	addQ(&h->reqQ, c);
	h->Qdepth++;
	spin_unlock_irqrestore(&h->lock, flags);
	start_io(h);
}

static inline void removeQ(struct CommandList *c)
{
	if (WARN_ON(list_empty(&c->list)))
		return;
	list_del_init(&c->list);
}

static inline int is_hba_lunid(unsigned char scsi3addr[])
{
	return memcmp(scsi3addr, RAID_CTLR_LUNID, 8) == 0;
}

static inline int is_scsi_rev_5(struct ctlr_info *h)
{
	if (!h->hba_inquiry_data)
		return 0;
	if ((h->hba_inquiry_data[2] & 0x07) == 5)
		return 1;
	return 0;
}

static int hpsa_find_target_lun(struct ctlr_info *h,
	unsigned char scsi3addr[], int bus, int *target, int *lun)
{
	/* finds an unused bus, target, lun for a new physical device
	 * assumes h->devlock is held
	 */
	int i, found = 0;
	DECLARE_BITMAP(lun_taken, HPSA_MAX_DEVICES);

	bitmap_zero(lun_taken, HPSA_MAX_DEVICES);

	for (i = 0; i < h->ndevices; i++) {
		if (h->dev[i]->bus == bus && h->dev[i]->target != -1)
			__set_bit(h->dev[i]->target, lun_taken);
	}

	i = find_first_zero_bit(lun_taken, HPSA_MAX_DEVICES);
	if (i < HPSA_MAX_DEVICES) {
		/* *bus = 1; */
		*target = i;
		*lun = 0;
		found = 1;
	}
	return !found;
}

/* Add an entry into h->dev[] array. */
static int hpsa_scsi_add_entry(struct ctlr_info *h, int hostno,
		struct hpsa_scsi_dev_t *device,
		struct hpsa_scsi_dev_t *added[], int *nadded)
{
	/* assumes h->devlock is held */
	int n = h->ndevices;
	int i;
	unsigned char addr1[8], addr2[8];
	struct hpsa_scsi_dev_t *sd;

	if (n >= HPSA_MAX_DEVICES) {
		dev_err(&h->pdev->dev, "too many devices, some will be "
			"inaccessible.\n");
		return -1;
	}

	/* physical devices do not have lun or target assigned until now. */
	if (device->lun != -1)
		/* Logical device, lun is already assigned. */
		goto lun_assigned;

	/* If this device a non-zero lun of a multi-lun device
	 * byte 4 of the 8-byte LUN addr will contain the logical
	 * unit no, zero otherise.
	 */
	if (device->scsi3addr[4] == 0) {
		/* This is not a non-zero lun of a multi-lun device */
		if (hpsa_find_target_lun(h, device->scsi3addr,
			device->bus, &device->target, &device->lun) != 0)
			return -1;
		goto lun_assigned;
	}

	/* This is a non-zero lun of a multi-lun device.
	 * Search through our list and find the device which
	 * has the same 8 byte LUN address, excepting byte 4.
	 * Assign the same bus and target for this new LUN.
	 * Use the logical unit number from the firmware.
	 */
	memcpy(addr1, device->scsi3addr, 8);
	addr1[4] = 0;
	for (i = 0; i < n; i++) {
		sd = h->dev[i];
		memcpy(addr2, sd->scsi3addr, 8);
		addr2[4] = 0;
		/* differ only in byte 4? */
		if (memcmp(addr1, addr2, 8) == 0) {
			device->bus = sd->bus;
			device->target = sd->target;
			device->lun = device->scsi3addr[4];
			break;
		}
	}
	if (device->lun == -1) {
		dev_warn(&h->pdev->dev, "physical device with no LUN=0,"
			" suspect firmware bug or unsupported hardware "
			"configuration.\n");
			return -1;
	}

lun_assigned:

	h->dev[n] = device;
	h->ndevices++;
	added[*nadded] = device;
	(*nadded)++;

	/* initially, (before registering with scsi layer) we don't
	 * know our hostno and we don't want to print anything first
	 * time anyway (the scsi layer's inquiries will show that info)
	 */
	/* if (hostno != -1) */
		dev_info(&h->pdev->dev, "%s device c%db%dt%dl%d added.\n",
			scsi_device_type(device->devtype), hostno,
			device->bus, device->target, device->lun);
	return 0;
}

/* Update an entry in h->dev[] array. */
static void hpsa_scsi_update_entry(struct ctlr_info *h, int hostno,
	int entry, struct hpsa_scsi_dev_t *new_entry)
{
	/* assumes h->devlock is held */
	BUG_ON(entry < 0 || entry >= HPSA_MAX_DEVICES);

	/* Raid level changed. */
	h->dev[entry]->raid_level = new_entry->raid_level;
	dev_info(&h->pdev->dev, "%s device c%db%dt%dl%d updated.\n",
		scsi_device_type(new_entry->devtype), hostno, new_entry->bus,
		new_entry->target, new_entry->lun);
}

/* Replace an entry from h->dev[] array. */
static void hpsa_scsi_replace_entry(struct ctlr_info *h, int hostno,
	int entry, struct hpsa_scsi_dev_t *new_entry,
	struct hpsa_scsi_dev_t *added[], int *nadded,
	struct hpsa_scsi_dev_t *removed[], int *nremoved)
{
	/* assumes h->devlock is held */
	BUG_ON(entry < 0 || entry >= HPSA_MAX_DEVICES);
	removed[*nremoved] = h->dev[entry];
	(*nremoved)++;

	/*
	 * New physical devices won't have target/lun assigned yet
	 * so we need to preserve the values in the slot we are replacing.
	 */
	if (new_entry->target == -1) {
		new_entry->target = h->dev[entry]->target;
		new_entry->lun = h->dev[entry]->lun;
	}

	h->dev[entry] = new_entry;
	added[*nadded] = new_entry;
	(*nadded)++;
	dev_info(&h->pdev->dev, "%s device c%db%dt%dl%d changed.\n",
		scsi_device_type(new_entry->devtype), hostno, new_entry->bus,
			new_entry->target, new_entry->lun);
}

/* Remove an entry from h->dev[] array. */
static void hpsa_scsi_remove_entry(struct ctlr_info *h, int hostno, int entry,
	struct hpsa_scsi_dev_t *removed[], int *nremoved)
{
	/* assumes h->devlock is held */
	int i;
	struct hpsa_scsi_dev_t *sd;

	BUG_ON(entry < 0 || entry >= HPSA_MAX_DEVICES);

	sd = h->dev[entry];
	removed[*nremoved] = h->dev[entry];
	(*nremoved)++;

	for (i = entry; i < h->ndevices-1; i++)
		h->dev[i] = h->dev[i+1];
	h->ndevices--;
	dev_info(&h->pdev->dev, "%s device c%db%dt%dl%d removed.\n",
		scsi_device_type(sd->devtype), hostno, sd->bus, sd->target,
		sd->lun);
}

#define SCSI3ADDR_EQ(a, b) ( \
	(a)[7] == (b)[7] && \
	(a)[6] == (b)[6] && \
	(a)[5] == (b)[5] && \
	(a)[4] == (b)[4] && \
	(a)[3] == (b)[3] && \
	(a)[2] == (b)[2] && \
	(a)[1] == (b)[1] && \
	(a)[0] == (b)[0])

static void fixup_botched_add(struct ctlr_info *h,
	struct hpsa_scsi_dev_t *added)
{
	/* called when scsi_add_device fails in order to re-adjust
	 * h->dev[] to match the mid layer's view.
	 */
	unsigned long flags;
	int i, j;

	spin_lock_irqsave(&h->lock, flags);
	for (i = 0; i < h->ndevices; i++) {
		if (h->dev[i] == added) {
			for (j = i; j < h->ndevices-1; j++)
				h->dev[j] = h->dev[j+1];
			h->ndevices--;
			break;
		}
	}
	spin_unlock_irqrestore(&h->lock, flags);
	kfree(added);
}

static inline int device_is_the_same(struct hpsa_scsi_dev_t *dev1,
	struct hpsa_scsi_dev_t *dev2)
{
	/* we compare everything except lun and target as these
	 * are not yet assigned.  Compare parts likely
	 * to differ first
	 */
	if (memcmp(dev1->scsi3addr, dev2->scsi3addr,
		sizeof(dev1->scsi3addr)) != 0)
		return 0;
	if (memcmp(dev1->device_id, dev2->device_id,
		sizeof(dev1->device_id)) != 0)
		return 0;
	if (memcmp(dev1->model, dev2->model, sizeof(dev1->model)) != 0)
		return 0;
	if (memcmp(dev1->vendor, dev2->vendor, sizeof(dev1->vendor)) != 0)
		return 0;
	if (dev1->devtype != dev2->devtype)
		return 0;
	if (dev1->bus != dev2->bus)
		return 0;
	return 1;
}

static inline int device_updated(struct hpsa_scsi_dev_t *dev1,
	struct hpsa_scsi_dev_t *dev2)
{
	/* Device attributes that can change, but don't mean
	 * that the device is a different device, nor that the OS
	 * needs to be told anything about the change.
	 */
	if (dev1->raid_level != dev2->raid_level)
		return 1;
	return 0;
}

/* Find needle in haystack.  If exact match found, return DEVICE_SAME,
 * and return needle location in *index.  If scsi3addr matches, but not
 * vendor, model, serial num, etc. return DEVICE_CHANGED, and return needle
 * location in *index.
 * In the case of a minor device attribute change, such as RAID level, just
 * return DEVICE_UPDATED, along with the updated device's location in index.
 * If needle not found, return DEVICE_NOT_FOUND.
 */
static int hpsa_scsi_find_entry(struct hpsa_scsi_dev_t *needle,
	struct hpsa_scsi_dev_t *haystack[], int haystack_size,
	int *index)
{
	int i;
#define DEVICE_NOT_FOUND 0
#define DEVICE_CHANGED 1
#define DEVICE_SAME 2
#define DEVICE_UPDATED 3
	for (i = 0; i < haystack_size; i++) {
		if (haystack[i] == NULL) /* previously removed. */
			continue;
		if (SCSI3ADDR_EQ(needle->scsi3addr, haystack[i]->scsi3addr)) {
			*index = i;
			if (device_is_the_same(needle, haystack[i])) {
				if (device_updated(needle, haystack[i]))
					return DEVICE_UPDATED;
				return DEVICE_SAME;
			} else {
				return DEVICE_CHANGED;
			}
		}
	}
	*index = -1;
	return DEVICE_NOT_FOUND;
}

static void adjust_hpsa_scsi_table(struct ctlr_info *h, int hostno,
	struct hpsa_scsi_dev_t *sd[], int nsds)
{
	/* sd contains scsi3 addresses and devtypes, and inquiry
	 * data.  This function takes what's in sd to be the current
	 * reality and updates h->dev[] to reflect that reality.
	 */
	int i, entry, device_change, changes = 0;
	struct hpsa_scsi_dev_t *csd;
	unsigned long flags;
	struct hpsa_scsi_dev_t **added, **removed;
	int nadded, nremoved;
	struct Scsi_Host *sh = NULL;

	added = kzalloc(sizeof(*added) * HPSA_MAX_DEVICES, GFP_KERNEL);
	removed = kzalloc(sizeof(*removed) * HPSA_MAX_DEVICES, GFP_KERNEL);

	if (!added || !removed) {
		dev_warn(&h->pdev->dev, "out of memory in "
			"adjust_hpsa_scsi_table\n");
		goto free_and_out;
	}

	spin_lock_irqsave(&h->devlock, flags);

	/* find any devices in h->dev[] that are not in
	 * sd[] and remove them from h->dev[], and for any
	 * devices which have changed, remove the old device
	 * info and add the new device info.
	 * If minor device attributes change, just update
	 * the existing device structure.
	 */
	i = 0;
	nremoved = 0;
	nadded = 0;
	while (i < h->ndevices) {
		csd = h->dev[i];
		device_change = hpsa_scsi_find_entry(csd, sd, nsds, &entry);
		if (device_change == DEVICE_NOT_FOUND) {
			changes++;
			hpsa_scsi_remove_entry(h, hostno, i,
				removed, &nremoved);
			continue; /* remove ^^^, hence i not incremented */
		} else if (device_change == DEVICE_CHANGED) {
			changes++;
			hpsa_scsi_replace_entry(h, hostno, i, sd[entry],
				added, &nadded, removed, &nremoved);
			/* Set it to NULL to prevent it from being freed
			 * at the bottom of hpsa_update_scsi_devices()
			 */
			sd[entry] = NULL;
		} else if (device_change == DEVICE_UPDATED) {
			hpsa_scsi_update_entry(h, hostno, i, sd[entry]);
		}
		i++;
	}

	/* Now, make sure every device listed in sd[] is also
	 * listed in h->dev[], adding them if they aren't found
	 */

	for (i = 0; i < nsds; i++) {
		if (!sd[i]) /* if already added above. */
			continue;
		device_change = hpsa_scsi_find_entry(sd[i], h->dev,
					h->ndevices, &entry);
		if (device_change == DEVICE_NOT_FOUND) {
			changes++;
			if (hpsa_scsi_add_entry(h, hostno, sd[i],
				added, &nadded) != 0)
				break;
			sd[i] = NULL; /* prevent from being freed later. */
		} else if (device_change == DEVICE_CHANGED) {
			/* should never happen... */
			changes++;
			dev_warn(&h->pdev->dev,
				"device unexpectedly changed.\n");
			/* but if it does happen, we just ignore that device */
		}
	}
	spin_unlock_irqrestore(&h->devlock, flags);

	/* Don't notify scsi mid layer of any changes the first time through
	 * (or if there are no changes) scsi_scan_host will do it later the
	 * first time through.
	 */
	if (hostno == -1 || !changes)
		goto free_and_out;

	sh = h->scsi_host;
	/* Notify scsi mid layer of any removed devices */
	for (i = 0; i < nremoved; i++) {
		struct scsi_device *sdev =
			scsi_device_lookup(sh, removed[i]->bus,
				removed[i]->target, removed[i]->lun);
		if (sdev != NULL) {
			scsi_remove_device(sdev);
			scsi_device_put(sdev);
		} else {
			/* We don't expect to get here.
			 * future cmds to this device will get selection
			 * timeout as if the device was gone.
			 */
			dev_warn(&h->pdev->dev, "didn't find c%db%dt%dl%d "
				" for removal.", hostno, removed[i]->bus,
				removed[i]->target, removed[i]->lun);
		}
		kfree(removed[i]);
		removed[i] = NULL;
	}

	/* Notify scsi mid layer of any added devices */
	for (i = 0; i < nadded; i++) {
		if (scsi_add_device(sh, added[i]->bus,
			added[i]->target, added[i]->lun) == 0)
			continue;
		dev_warn(&h->pdev->dev, "scsi_add_device c%db%dt%dl%d failed, "
			"device not added.\n", hostno, added[i]->bus,
			added[i]->target, added[i]->lun);
		/* now we have to remove it from h->dev,
		 * since it didn't get added to scsi mid layer
		 */
		fixup_botched_add(h, added[i]);
	}

free_and_out:
	kfree(added);
	kfree(removed);
}

/*
 * Lookup bus/target/lun and return corresponding struct hpsa_scsi_dev_t *
 * Assume's h->devlock is held.
 */
static struct hpsa_scsi_dev_t *lookup_hpsa_scsi_dev(struct ctlr_info *h,
	int bus, int target, int lun)
{
	int i;
	struct hpsa_scsi_dev_t *sd;

	for (i = 0; i < h->ndevices; i++) {
		sd = h->dev[i];
		if (sd->bus == bus && sd->target == target && sd->lun == lun)
			return sd;
	}
	return NULL;
}

/* link sdev->hostdata to our per-device structure. */
static int hpsa_slave_alloc(struct scsi_device *sdev)
{
	struct hpsa_scsi_dev_t *sd;
	unsigned long flags;
	struct ctlr_info *h;

	h = sdev_to_hba(sdev);
	spin_lock_irqsave(&h->devlock, flags);
	sd = lookup_hpsa_scsi_dev(h, sdev_channel(sdev),
		sdev_id(sdev), sdev->lun);
	if (sd != NULL)
		sdev->hostdata = sd;
	spin_unlock_irqrestore(&h->devlock, flags);
	return 0;
}

static void hpsa_slave_destroy(struct scsi_device *sdev)
{
	/* nothing to do. */
}

static void hpsa_free_sg_chain_blocks(struct ctlr_info *h)
{
	int i;

	if (!h->cmd_sg_list)
		return;
	for (i = 0; i < h->nr_cmds; i++) {
		kfree(h->cmd_sg_list[i]);
		h->cmd_sg_list[i] = NULL;
	}
	kfree(h->cmd_sg_list);
	h->cmd_sg_list = NULL;
}

static int hpsa_allocate_sg_chain_blocks(struct ctlr_info *h)
{
	int i;

	if (h->chainsize <= 0)
		return 0;

	h->cmd_sg_list = kzalloc(sizeof(*h->cmd_sg_list) * h->nr_cmds,
				GFP_KERNEL);
	if (!h->cmd_sg_list)
		return -ENOMEM;
	for (i = 0; i < h->nr_cmds; i++) {
		h->cmd_sg_list[i] = kmalloc(sizeof(*h->cmd_sg_list[i]) *
						h->chainsize, GFP_KERNEL);
		if (!h->cmd_sg_list[i])
			goto clean;
	}
	return 0;

clean:
	hpsa_free_sg_chain_blocks(h);
	return -ENOMEM;
}

static int hpsa_map_sg_chain_block(struct ctlr_info *h,
	struct CommandList *c)
{
	struct SGDescriptor *chain_sg, *chain_block;
	u64 temp64;

	chain_sg = &c->SG[h->max_cmd_sg_entries - 1];
	chain_block = h->cmd_sg_list[c->cmdindex];
	chain_sg->Ext = HPSA_SG_CHAIN;
	chain_sg->Len = sizeof(*chain_sg) *
		(c->Header.SGTotal - h->max_cmd_sg_entries);
	temp64 = pci_map_single(h->pdev, chain_block, chain_sg->Len,
				PCI_DMA_TODEVICE);
	if (dma_mapping_error(&h->pdev->dev, temp64)) {
		/* prevent subsequent unmapping */
		chain_sg->Addr.lower = 0;
		chain_sg->Addr.upper = 0;
		return -1;
	}
	chain_sg->Addr.lower = (u32) (temp64 & 0x0FFFFFFFFULL);
	chain_sg->Addr.upper = (u32) ((temp64 >> 32) & 0x0FFFFFFFFULL);
	return 0;
}

static void hpsa_unmap_sg_chain_block(struct ctlr_info *h,
	struct CommandList *c)
{
	struct SGDescriptor *chain_sg;
	union u64bit temp64;

	if (c->Header.SGTotal <= h->max_cmd_sg_entries)
		return;

	chain_sg = &c->SG[h->max_cmd_sg_entries - 1];
	temp64.val32.lower = chain_sg->Addr.lower;
	temp64.val32.upper = chain_sg->Addr.upper;
	pci_unmap_single(h->pdev, temp64.val, chain_sg->Len, PCI_DMA_TODEVICE);
}

static void complete_scsi_command(struct CommandList *cp)
{
	struct scsi_cmnd *cmd;
	struct ctlr_info *h;
	struct ErrorInfo *ei;

	unsigned char sense_key;
	unsigned char asc;      /* additional sense code */
	unsigned char ascq;     /* additional sense code qualifier */
	unsigned long sense_data_size;

	ei = cp->err_info;
	cmd = (struct scsi_cmnd *) cp->scsi_cmd;
	h = cp->h;

	scsi_dma_unmap(cmd); /* undo the DMA mappings */
	if (cp->Header.SGTotal > h->max_cmd_sg_entries)
		hpsa_unmap_sg_chain_block(h, cp);

	cmd->result = (DID_OK << 16); 		/* host byte */
	cmd->result |= (COMMAND_COMPLETE << 8);	/* msg byte */
	cmd->result |= ei->ScsiStatus;

	/* copy the sense data whether we need to or not. */
	if (SCSI_SENSE_BUFFERSIZE < sizeof(ei->SenseInfo))
		sense_data_size = SCSI_SENSE_BUFFERSIZE;
	else
		sense_data_size = sizeof(ei->SenseInfo);
	if (ei->SenseLen < sense_data_size)
		sense_data_size = ei->SenseLen;

	memcpy(cmd->sense_buffer, ei->SenseInfo, sense_data_size);
	scsi_set_resid(cmd, ei->ResidualCnt);

	if (ei->CommandStatus == 0) {
		cmd_free(h, cp);
		cmd->scsi_done(cmd);
		return;
	}

	/* an error has occurred */
	switch (ei->CommandStatus) {

	case CMD_TARGET_STATUS:
		if (ei->ScsiStatus) {
			/* Get sense key */
			sense_key = 0xf & ei->SenseInfo[2];
			/* Get additional sense code */
			asc = ei->SenseInfo[12];
			/* Get addition sense code qualifier */
			ascq = ei->SenseInfo[13];
		}

		if (ei->ScsiStatus == SAM_STAT_CHECK_CONDITION) {
			if (check_for_unit_attention(h, cp)) {
				cmd->result = DID_SOFT_ERROR << 16;
				break;
			}
			if (sense_key == ILLEGAL_REQUEST) {
				/*
				 * SCSI REPORT_LUNS is commonly unsupported on
				 * Smart Array.  Suppress noisy complaint.
				 */
				if (cp->Request.CDB[0] == REPORT_LUNS)
					break;

				/* If ASC/ASCQ indicate Logical Unit
				 * Not Supported condition,
				 */
				if ((asc == 0x25) && (ascq == 0x0)) {
					dev_warn(&h->pdev->dev, "cp %p "
						"has check condition\n", cp);
					break;
				}
			}

			if (sense_key == NOT_READY) {
				/* If Sense is Not Ready, Logical Unit
				 * Not ready, Manual Intervention
				 * required
				 */
				if ((asc == 0x04) && (ascq == 0x03)) {
					dev_warn(&h->pdev->dev, "cp %p "
						"has check condition: unit "
						"not ready, manual "
						"intervention required\n", cp);
					break;
				}
			}
			if (sense_key == ABORTED_COMMAND) {
				/* Aborted command is retryable */
				dev_warn(&h->pdev->dev, "cp %p "
					"has check condition: aborted command: "
					"ASC: 0x%x, ASCQ: 0x%x\n",
					cp, asc, ascq);
				cmd->result |= DID_SOFT_ERROR << 16;
				break;
			}
			/* Must be some other type of check condition */
			dev_dbg(&h->pdev->dev, "cp %p has check condition: "
					"unknown type: "
					"Sense: 0x%x, ASC: 0x%x, ASCQ: 0x%x, "
					"Returning result: 0x%x, "
					"cmd=[%02x %02x %02x %02x %02x "
					"%02x %02x %02x %02x %02x %02x "
					"%02x %02x %02x %02x %02x]\n",
					cp, sense_key, asc, ascq,
					cmd->result,
					cmd->cmnd[0], cmd->cmnd[1],
					cmd->cmnd[2], cmd->cmnd[3],
					cmd->cmnd[4], cmd->cmnd[5],
					cmd->cmnd[6], cmd->cmnd[7],
					cmd->cmnd[8], cmd->cmnd[9],
					cmd->cmnd[10], cmd->cmnd[11],
					cmd->cmnd[12], cmd->cmnd[13],
					cmd->cmnd[14], cmd->cmnd[15]);
			break;
		}


		/* Problem was not a check condition
		 * Pass it up to the upper layers...
		 */
		if (ei->ScsiStatus) {
			dev_warn(&h->pdev->dev, "cp %p has status 0x%x "
				"Sense: 0x%x, ASC: 0x%x, ASCQ: 0x%x, "
				"Returning result: 0x%x\n",
				cp, ei->ScsiStatus,
				sense_key, asc, ascq,
				cmd->result);
		} else {  /* scsi status is zero??? How??? */
			dev_warn(&h->pdev->dev, "cp %p SCSI status was 0. "
				"Returning no connection.\n", cp),

			/* Ordinarily, this case should never happen,
			 * but there is a bug in some released firmware
			 * revisions that allows it to happen if, for
			 * example, a 4100 backplane loses power and
			 * the tape drive is in it.  We assume that
			 * it's a fatal error of some kind because we
			 * can't show that it wasn't. We will make it
			 * look like selection timeout since that is
			 * the most common reason for this to occur,
			 * and it's severe enough.
			 */

			cmd->result = DID_NO_CONNECT << 16;
		}
		break;

	case CMD_DATA_UNDERRUN: /* let mid layer handle it. */
		break;
	case CMD_DATA_OVERRUN:
		dev_warn(&h->pdev->dev, "cp %p has"
			" completed with data overrun "
			"reported\n", cp);
		break;
	case CMD_INVALID: {
		/* print_bytes(cp, sizeof(*cp), 1, 0);
		print_cmd(cp); */
		/* We get CMD_INVALID if you address a non-existent device
		 * instead of a selection timeout (no response).  You will
		 * see this if you yank out a drive, then try to access it.
		 * This is kind of a shame because it means that any other
		 * CMD_INVALID (e.g. driver bug) will get interpreted as a
		 * missing target. */
		cmd->result = DID_NO_CONNECT << 16;
	}
		break;
	case CMD_PROTOCOL_ERR:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "cp %p has "
			"protocol error\n", cp);
		break;
	case CMD_HARDWARE_ERR:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "cp %p had  hardware error\n", cp);
		break;
	case CMD_CONNECTION_LOST:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "cp %p had connection lost\n", cp);
		break;
	case CMD_ABORTED:
		cmd->result = DID_ABORT << 16;
		dev_warn(&h->pdev->dev, "cp %p was aborted with status 0x%x\n",
				cp, ei->ScsiStatus);
		break;
	case CMD_ABORT_FAILED:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "cp %p reports abort failed\n", cp);
		break;
	case CMD_UNSOLICITED_ABORT:
		cmd->result = DID_SOFT_ERROR << 16; /* retry the command */
		dev_warn(&h->pdev->dev, "cp %p aborted due to an unsolicited "
			"abort\n", cp);
		break;
	case CMD_TIMEOUT:
		cmd->result = DID_TIME_OUT << 16;
		dev_warn(&h->pdev->dev, "cp %p timedout\n", cp);
		break;
	case CMD_UNABORTABLE:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "Command unabortable\n");
		break;
	default:
		cmd->result = DID_ERROR << 16;
		dev_warn(&h->pdev->dev, "cp %p returned unknown status %x\n",
				cp, ei->CommandStatus);
	}
	cmd_free(h, cp);
	cmd->scsi_done(cmd);
}

static void hpsa_pci_unmap(struct pci_dev *pdev,
	struct CommandList *c, int sg_used, int data_direction)
{
	int i;
	union u64bit addr64;

	for (i = 0; i < sg_used; i++) {
		addr64.val32.lower = c->SG[i].Addr.lower;
		addr64.val32.upper = c->SG[i].Addr.upper;
		pci_unmap_single(pdev, (dma_addr_t) addr64.val, c->SG[i].Len,
			data_direction);
	}
}

static int hpsa_map_one(struct pci_dev *pdev,
		struct CommandList *cp,
		unsigned char *buf,
		size_t buflen,
		int data_direction)
{
	u64 addr64;

	if (buflen == 0 || data_direction == PCI_DMA_NONE) {
		cp->Header.SGList = 0;
		cp->Header.SGTotal = 0;
		return 0;
	}

	addr64 = (u64) pci_map_single(pdev, buf, buflen, data_direction);
	if (dma_mapping_error(&pdev->dev, addr64)) {
		/* Prevent subsequent unmap of something never mapped */
		cp->Header.SGList = 0;
		cp->Header.SGTotal = 0;
		return -1;
	}
	cp->SG[0].Addr.lower =
	  (u32) (addr64 & (u64) 0x00000000FFFFFFFF);
	cp->SG[0].Addr.upper =
	  (u32) ((addr64 >> 32) & (u64) 0x00000000FFFFFFFF);
	cp->SG[0].Len = buflen;
	cp->Header.SGList = (u8) 1;   /* no. SGs contig in this cmd */
	cp->Header.SGTotal = (u16) 1; /* total sgs in this cmd list */
	return 0;
}

static inline void hpsa_scsi_do_simple_cmd_core(struct ctlr_info *h,
	struct CommandList *c)
{
	DECLARE_COMPLETION_ONSTACK(wait);

	c->waiting = &wait;
	enqueue_cmd_and_start_io(h, c);
	wait_for_completion(&wait);
}

static void hpsa_scsi_do_simple_cmd_core_if_no_lockup(struct ctlr_info *h,
	struct CommandList *c)
{
	unsigned long flags;

	/* If controller lockup detected, fake a hardware error. */
	spin_lock_irqsave(&h->lock, flags);
	if (unlikely(h->lockup_detected)) {
		spin_unlock_irqrestore(&h->lock, flags);
		c->err_info->CommandStatus = CMD_HARDWARE_ERR;
	} else {
		spin_unlock_irqrestore(&h->lock, flags);
		hpsa_scsi_do_simple_cmd_core(h, c);
	}
}

#define MAX_DRIVER_CMD_RETRIES 25
static void hpsa_scsi_do_simple_cmd_with_retry(struct ctlr_info *h,
	struct CommandList *c, int data_direction)
{
	int backoff_time = 10, retry_count = 0;

	do {
		memset(c->err_info, 0, sizeof(*c->err_info));
		hpsa_scsi_do_simple_cmd_core(h, c);
		retry_count++;
		if (retry_count > 3) {
			msleep(backoff_time);
			if (backoff_time < 1000)
				backoff_time *= 2;
		}
	} while ((check_for_unit_attention(h, c) ||
			check_for_busy(h, c)) &&
			retry_count <= MAX_DRIVER_CMD_RETRIES);
	hpsa_pci_unmap(h->pdev, c, 1, data_direction);
}

static void hpsa_scsi_interpret_error(struct CommandList *cp)
{
	struct ErrorInfo *ei;
	struct device *d = &cp->h->pdev->dev;

	ei = cp->err_info;
	switch (ei->CommandStatus) {
	case CMD_TARGET_STATUS:
		dev_warn(d, "cmd %p has completed with errors\n", cp);
		dev_warn(d, "cmd %p has SCSI Status = %x\n", cp,
				ei->ScsiStatus);
		if (ei->ScsiStatus == 0)
			dev_warn(d, "SCSI status is abnormally zero.  "
			"(probably indicates selection timeout "
			"reported incorrectly due to a known "
			"firmware bug, circa July, 2001.)\n");
		break;
	case CMD_DATA_UNDERRUN: /* let mid layer handle it. */
			dev_info(d, "UNDERRUN\n");
		break;
	case CMD_DATA_OVERRUN:
		dev_warn(d, "cp %p has completed with data overrun\n", cp);
		break;
	case CMD_INVALID: {
		/* controller unfortunately reports SCSI passthru's
		 * to non-existent targets as invalid commands.
		 */
		dev_warn(d, "cp %p is reported invalid (probably means "
			"target device no longer present)\n", cp);
		/* print_bytes((unsigned char *) cp, sizeof(*cp), 1, 0);
		print_cmd(cp);  */
		}
		break;
	case CMD_PROTOCOL_ERR:
		dev_warn(d, "cp %p has protocol error \n", cp);
		break;
	case CMD_HARDWARE_ERR:
		/* cmd->result = DID_ERROR << 16; */
		dev_warn(d, "cp %p had hardware error\n", cp);
		break;
	case CMD_CONNECTION_LOST:
		dev_warn(d, "cp %p had connection lost\n", cp);
		break;
	case CMD_ABORTED:
		dev_warn(d, "cp %p was aborted\n", cp);
		break;
	case CMD_ABORT_FAILED:
		dev_warn(d, "cp %p reports abort failed\n", cp);
		break;
	case CMD_UNSOLICITED_ABORT:
		dev_warn(d, "cp %p aborted due to an unsolicited abort\n", cp);
		break;
	case CMD_TIMEOUT:
		dev_warn(d, "cp %p timed out\n", cp);
		break;
	case CMD_UNABORTABLE:
		dev_warn(d, "Command unabortable\n");
		break;
	default:
		dev_warn(d, "cp %p returned unknown status %x\n", cp,
				ei->CommandStatus);
	}
}

static int hpsa_scsi_do_inquiry(struct ctlr_info *h, unsigned char *scsi3addr,
			unsigned char page, unsigned char *buf,
			unsigned char bufsize)
{
	int rc = IO_OK;
	struct CommandList *c;
	struct ErrorInfo *ei;

	c = cmd_special_alloc(h);

	if (c == NULL) {			/* trouble... */
		dev_warn(&h->pdev->dev, "cmd_special_alloc returned NULL!\n");
		return -ENOMEM;
	}

	if (fill_cmd(c, HPSA_INQUIRY, h, buf, bufsize,
			page, scsi3addr, TYPE_CMD)) {
		rc = -1;
		goto out;
	}
	hpsa_scsi_do_simple_cmd_with_retry(h, c, PCI_DMA_FROMDEVICE);
	ei = c->err_info;
	if (ei->CommandStatus != 0 && ei->CommandStatus != CMD_DATA_UNDERRUN) {
		hpsa_scsi_interpret_error(c);
		rc = -1;
	}
out:
	cmd_special_free(h, c);
	return rc;
}

static int hpsa_send_reset(struct ctlr_info *h, unsigned char *scsi3addr)
{
	int rc = IO_OK;
	struct CommandList *c;
	struct ErrorInfo *ei;

	c = cmd_special_alloc(h);

	if (c == NULL) {			/* trouble... */
		dev_warn(&h->pdev->dev, "cmd_special_alloc returned NULL!\n");
		return -ENOMEM;
	}

	/* fill_cmd can't fail here, no data buffer to map. */
	(void) fill_cmd(c, HPSA_DEVICE_RESET_MSG, h,
			NULL, 0, 0, scsi3addr, TYPE_MSG);
	hpsa_scsi_do_simple_cmd_core(h, c);
	/* no unmap needed here because no data xfer. */

	ei = c->err_info;
	if (ei->CommandStatus != 0) {
		hpsa_scsi_interpret_error(c);
		rc = -1;
	}
	cmd_special_free(h, c);
	return rc;
}

static void hpsa_get_raid_level(struct ctlr_info *h,
	unsigned char *scsi3addr, unsigned char *raid_level)
{
	int rc;
	unsigned char *buf;

	*raid_level = RAID_UNKNOWN;
	buf = kzalloc(64, GFP_KERNEL);
	if (!buf)
		return;
	rc = hpsa_scsi_do_inquiry(h, scsi3addr, 0xC1, buf, 64);
	if (rc == 0)
		*raid_level = buf[8];
	if (*raid_level > RAID_UNKNOWN)
		*raid_level = RAID_UNKNOWN;
	kfree(buf);
	return;
}

/* Get the device id from inquiry page 0x83 */
static int hpsa_get_device_id(struct ctlr_info *h, unsigned char *scsi3addr,
	unsigned char *device_id, int buflen)
{
	int rc;
	unsigned char *buf;

	if (buflen > 16)
		buflen = 16;
	buf = kzalloc(64, GFP_KERNEL);
	if (!buf)
		return -1;
	rc = hpsa_scsi_do_inquiry(h, scsi3addr, 0x83, buf, 64);
	if (rc == 0)
		memcpy(device_id, &buf[8], buflen);
	kfree(buf);
	return rc != 0;
}

static int hpsa_scsi_do_report_luns(struct ctlr_info *h, int logical,
		struct ReportLUNdata *buf, int bufsize,
		int extended_response)
{
	int rc = IO_OK;
	struct CommandList *c;
	unsigned char scsi3addr[8];
	struct ErrorInfo *ei;

	c = cmd_special_alloc(h);
	if (c == NULL) {			/* trouble... */
		dev_err(&h->pdev->dev, "cmd_special_alloc returned NULL!\n");
		return -1;
	}
	/* address the controller */
	memset(scsi3addr, 0, sizeof(scsi3addr));
	if (fill_cmd(c, logical ? HPSA_REPORT_LOG : HPSA_REPORT_PHYS, h,
		buf, bufsize, 0, scsi3addr, TYPE_CMD)) {
		rc = -1;
		goto out;
	}
	if (extended_response)
		c->Request.CDB[1] = extended_response;
	hpsa_scsi_do_simple_cmd_with_retry(h, c, PCI_DMA_FROMDEVICE);
	ei = c->err_info;
	if (ei->CommandStatus != 0 &&
	    ei->CommandStatus != CMD_DATA_UNDERRUN) {
		hpsa_scsi_interpret_error(c);
		rc = -1;
	}
out:
	cmd_special_free(h, c);
	return rc;
}

static inline int hpsa_scsi_do_report_phys_luns(struct ctlr_info *h,
		struct ReportLUNdata *buf,
		int bufsize, int extended_response)
{
	return hpsa_scsi_do_report_luns(h, 0, buf, bufsize, extended_response);
}

static inline int hpsa_scsi_do_report_log_luns(struct ctlr_info *h,
		struct ReportLUNdata *buf, int bufsize)
{
	return hpsa_scsi_do_report_luns(h, 1, buf, bufsize, 0);
}

static inline void hpsa_set_bus_target_lun(struct hpsa_scsi_dev_t *device,
	int bus, int target, int lun)
{
	device->bus = bus;
	device->target = target;
	device->lun = lun;
}

static int hpsa_update_device_info(struct ctlr_info *h,
	unsigned char scsi3addr[], struct hpsa_scsi_dev_t *this_device,
	unsigned char *is_OBDR_device)
{

#define OBDR_SIG_OFFSET 43
#define OBDR_TAPE_SIG "$DR-10"
#define OBDR_SIG_LEN (sizeof(OBDR_TAPE_SIG) - 1)
#define OBDR_TAPE_INQ_SIZE (OBDR_SIG_OFFSET + OBDR_SIG_LEN)

	unsigned char *inq_buff;
	unsigned char *obdr_sig;

	inq_buff = kzalloc(OBDR_TAPE_INQ_SIZE, GFP_KERNEL);
	if (!inq_buff)
		goto bail_out;

	/* Do an inquiry to the device to see what it is. */
	if (hpsa_scsi_do_inquiry(h, scsi3addr, 0, inq_buff,
		(unsigned char) OBDR_TAPE_INQ_SIZE) != 0) {
		/* Inquiry failed (msg printed already) */
		dev_err(&h->pdev->dev,
			"hpsa_update_device_info: inquiry failed\n");
		goto bail_out;
	}

	this_device->devtype = (inq_buff[0] & 0x1f);
	memcpy(this_device->scsi3addr, scsi3addr, 8);
	memcpy(this_device->vendor, &inq_buff[8],
		sizeof(this_device->vendor));
	memcpy(this_device->model, &inq_buff[16],
		sizeof(this_device->model));
	memset(this_device->device_id, 0,
		sizeof(this_device->device_id));
	hpsa_get_device_id(h, scsi3addr, this_device->device_id,
		sizeof(this_device->device_id));

	if (this_device->devtype == TYPE_DISK &&
		is_logical_dev_addr_mode(scsi3addr))
		hpsa_get_raid_level(h, scsi3addr, &this_device->raid_level);
	else
		this_device->raid_level = RAID_UNKNOWN;

	if (is_OBDR_device) {
		/* See if this is a One-Button-Disaster-Recovery device
		 * by looking for "$DR-10" at offset 43 in inquiry data.
		 */
		obdr_sig = &inq_buff[OBDR_SIG_OFFSET];
		*is_OBDR_device = (this_device->devtype == TYPE_ROM &&
					strncmp(obdr_sig, OBDR_TAPE_SIG,
						OBDR_SIG_LEN) == 0);
	}

	kfree(inq_buff);
	return 0;

bail_out:
	kfree(inq_buff);
	return 1;
}

static unsigned char *ext_target_model[] = {
	"MSA2012",
	"MSA2024",
	"MSA2312",
	"MSA2324",
	"P2000 G3 SAS",
	NULL,
};

static int is_ext_target(struct ctlr_info *h, struct hpsa_scsi_dev_t *device)
{
	int i;

	for (i = 0; ext_target_model[i]; i++)
		if (strncmp(device->model, ext_target_model[i],
			strlen(ext_target_model[i])) == 0)
			return 1;
	return 0;
}

/* Helper function to assign bus, target, lun mapping of devices.
 * Puts non-external target logical volumes on bus 0, external target logical
 * volumes on bus 1, physical devices on bus 2. and the hba on bus 3.
 * Logical drive target and lun are assigned at this time, but
 * physical device lun and target assignment are deferred (assigned
 * in hpsa_find_target_lun, called by hpsa_scsi_add_entry.)
 */
static void figure_bus_target_lun(struct ctlr_info *h,
	u8 *lunaddrbytes, struct hpsa_scsi_dev_t *device)
{
	u32 lunid = le32_to_cpu(*((__le32 *) lunaddrbytes));

	if (!is_logical_dev_addr_mode(lunaddrbytes)) {
		/* physical device, target and lun filled in later */
		if (is_hba_lunid(lunaddrbytes))
			hpsa_set_bus_target_lun(device, 3, 0, lunid & 0x3fff);
		else
			/* defer target, lun assignment for physical devices */
			hpsa_set_bus_target_lun(device, 2, -1, -1);
		return;
	}
	/* It's a logical device */
	if (is_ext_target(h, device)) {
		/* external target way, put logicals on bus 1
		 * and match target/lun numbers box
		 * reports, other smart array, bus 0, target 0, match lunid
		 */
		hpsa_set_bus_target_lun(device,
			1, (lunid >> 16) & 0x3fff, lunid & 0x00ff);
		return;
	}
	hpsa_set_bus_target_lun(device, 0, 0, lunid & 0x3fff);
}

/*
 * If there is no lun 0 on a target, fikus won't find any devices.
 * For the external targets (arrays), we have to manually detect the enclosure
 * which is at lun zero, as CCISS_REPORT_PHYSICAL_LUNS doesn't report
 * it for some reason.  *tmpdevice is the target we're adding,
 * this_device is a pointer into the current element of currentsd[]
 * that we're building up in update_scsi_devices(), below.
 * lunzerobits is a bitmap that tracks which targets already have a
 * lun 0 assigned.
 * Returns 1 if an enclosure was added, 0 if not.
 */
static int add_ext_target_dev(struct ctlr_info *h,
	struct hpsa_scsi_dev_t *tmpdevice,
	struct hpsa_scsi_dev_t *this_device, u8 *lunaddrbytes,
	unsigned long lunzerobits[], int *n_ext_target_devs)
{
	unsigned char scsi3addr[8];

	if (test_bit(tmpdevice->target, lunzerobits))
		return 0; /* There is already a lun 0 on this target. */

	if (!is_logical_dev_addr_mode(lunaddrbytes))
		return 0; /* It's the logical targets that may lack lun 0. */

	if (!is_ext_target(h, tmpdevice))
		return 0; /* Only external target devices have this problem. */

	if (tmpdevice->lun == 0) /* if lun is 0, then we have a lun 0. */
		return 0;

	memset(scsi3addr, 0, 8);
	scsi3addr[3] = tmpdevice->target;
	if (is_hba_lunid(scsi3addr))
		return 0; /* Don't add the RAID controller here. */

	if (is_scsi_rev_5(h))
		return 0; /* p1210m doesn't need to do this. */

	if (*n_ext_target_devs >= MAX_EXT_TARGETS) {
		dev_warn(&h->pdev->dev, "Maximum number of external "
			"target devices exceeded.  Check your hardware "
			"configuration.");
		return 0;
	}

	if (hpsa_update_device_info(h, scsi3addr, this_device, NULL))
		return 0;
	(*n_ext_target_devs)++;
	hpsa_set_bus_target_lun(this_device,
				tmpdevice->bus, tmpdevice->target, 0);
	set_bit(tmpdevice->target, lunzerobits);
	return 1;
}

/*
 * Do CISS_REPORT_PHYS and CISS_REPORT_LOG.  Data is returned in physdev,
 * logdev.  The number of luns in physdev and logdev are returned in
 * *nphysicals and *nlogicals, respectively.
 * Returns 0 on success, -1 otherwise.
 */
static int hpsa_gather_lun_info(struct ctlr_info *h,
	int reportlunsize,
	struct ReportLUNdata *physdev, u32 *nphysicals,
	struct ReportLUNdata *logdev, u32 *nlogicals)
{
	if (hpsa_scsi_do_report_phys_luns(h, physdev, reportlunsize, 0)) {
		dev_err(&h->pdev->dev, "report physical LUNs failed.\n");
		return -1;
	}
	*nphysicals = be32_to_cpu(*((__be32 *)physdev->LUNListLength)) / 8;
	if (*nphysicals > HPSA_MAX_PHYS_LUN) {
		dev_warn(&h->pdev->dev, "maximum physical LUNs (%d) exceeded."
			"  %d LUNs ignored.\n", HPSA_MAX_PHYS_LUN,
			*nphysicals - HPSA_MAX_PHYS_LUN);
		*nphysicals = HPSA_MAX_PHYS_LUN;
	}
	if (hpsa_scsi_do_report_log_luns(h, logdev, reportlunsize)) {
		dev_err(&h->pdev->dev, "report logical LUNs failed.\n");
		return -1;
	}
	*nlogicals = be32_to_cpu(*((__be32 *) logdev->LUNListLength)) / 8;
	/* Reject Logicals in excess of our max capability. */
	if (*nlogicals > HPSA_MAX_LUN) {
		dev_warn(&h->pdev->dev,
			"maximum logical LUNs (%d) exceeded.  "
			"%d LUNs ignored.\n", HPSA_MAX_LUN,
			*nlogicals - HPSA_MAX_LUN);
			*nlogicals = HPSA_MAX_LUN;
	}
	if (*nlogicals + *nphysicals > HPSA_MAX_PHYS_LUN) {
		dev_warn(&h->pdev->dev,
			"maximum logical + physical LUNs (%d) exceeded. "
			"%d LUNs ignored.\n", HPSA_MAX_PHYS_LUN,
			*nphysicals + *nlogicals - HPSA_MAX_PHYS_LUN);
		*nlogicals = HPSA_MAX_PHYS_LUN - *nphysicals;
	}
	return 0;
}

u8 *figure_lunaddrbytes(struct ctlr_info *h, int raid_ctlr_position, int i,
	int nphysicals, int nlogicals, struct ReportLUNdata *physdev_list,
	struct ReportLUNdata *logdev_list)
{
	/* Helper function, figure out where the LUN ID info is coming from
	 * given index i, lists of physical and logical devices, where in
	 * the list the raid controller is supposed to appear (first or last)
	 */

	int logicals_start = nphysicals + (raid_ctlr_position == 0);
	int last_device = nphysicals + nlogicals + (raid_ctlr_position == 0);

	if (i == raid_ctlr_position)
		return RAID_CTLR_LUNID;

	if (i < logicals_start)
		return &physdev_list->LUN[i - (raid_ctlr_position == 0)][0];

	if (i < last_device)
		return &logdev_list->LUN[i - nphysicals -
			(raid_ctlr_position == 0)][0];
	BUG();
	return NULL;
}

static void hpsa_update_scsi_devices(struct ctlr_info *h, int hostno)
{
	/* the idea here is we could get notified
	 * that some devices have changed, so we do a report
	 * physical luns and report logical luns cmd, and adjust
	 * our list of devices accordingly.
	 *
	 * The scsi3addr's of devices won't change so long as the
	 * adapter is not reset.  That means we can rescan and
	 * tell which devices we already know about, vs. new
	 * devices, vs.  disappearing devices.
	 */
	struct ReportLUNdata *physdev_list = NULL;
	struct ReportLUNdata *logdev_list = NULL;
	u32 nphysicals = 0;
	u32 nlogicals = 0;
	u32 ndev_allocated = 0;
	struct hpsa_scsi_dev_t **currentsd, *this_device, *tmpdevice;
	int ncurrent = 0;
	int reportlunsize = sizeof(*physdev_list) + HPSA_MAX_PHYS_LUN * 8;
	int i, n_ext_target_devs, ndevs_to_allocate;
	int raid_ctlr_position;
	DECLARE_BITMAP(lunzerobits, MAX_EXT_TARGETS);

	currentsd = kzalloc(sizeof(*currentsd) * HPSA_MAX_DEVICES, GFP_KERNEL);
	physdev_list = kzalloc(reportlunsize, GFP_KERNEL);
	logdev_list = kzalloc(reportlunsize, GFP_KERNEL);
	tmpdevice = kzalloc(sizeof(*tmpdevice), GFP_KERNEL);

	if (!currentsd || !physdev_list || !logdev_list || !tmpdevice) {
		dev_err(&h->pdev->dev, "out of memory\n");
		goto out;
	}
	memset(lunzerobits, 0, sizeof(lunzerobits));

	if (hpsa_gather_lun_info(h, reportlunsize, physdev_list, &nphysicals,
			logdev_list, &nlogicals))
		goto out;

	/* We might see up to the maximum number of logical and physical disks
	 * plus external target devices, and a device for the local RAID
	 * controller.
	 */
	ndevs_to_allocate = nphysicals + nlogicals + MAX_EXT_TARGETS + 1;

	/* Allocate the per device structures */
	for (i = 0; i < ndevs_to_allocate; i++) {
		if (i >= HPSA_MAX_DEVICES) {
			dev_warn(&h->pdev->dev, "maximum devices (%d) exceeded."
				"  %d devices ignored.\n", HPSA_MAX_DEVICES,
				ndevs_to_allocate - HPSA_MAX_DEVICES);
			break;
		}

		currentsd[i] = kzalloc(sizeof(*currentsd[i]), GFP_KERNEL);
		if (!currentsd[i]) {
			dev_warn(&h->pdev->dev, "out of memory at %s:%d\n",
				__FILE__, __LINE__);
			goto out;
		}
		ndev_allocated++;
	}

	if (unlikely(is_scsi_rev_5(h)))
		raid_ctlr_position = 0;
	else
		raid_ctlr_position = nphysicals + nlogicals;

	/* adjust our table of devices */
	n_ext_target_devs = 0;
	for (i = 0; i < nphysicals + nlogicals + 1; i++) {
		u8 *lunaddrbytes, is_OBDR = 0;

		/* Figure out where the LUN ID info is coming from */
		lunaddrbytes = figure_lunaddrbytes(h, raid_ctlr_position,
			i, nphysicals, nlogicals, physdev_list, logdev_list);
		/* skip masked physical devices. */
		if (lunaddrbytes[3] & 0xC0 &&
			i < nphysicals + (raid_ctlr_position == 0))
			continue;

		/* Get device type, vendor, model, device id */
		if (hpsa_update_device_info(h, lunaddrbytes, tmpdevice,
							&is_OBDR))
			continue; /* skip it if we can't talk to it. */
		figure_bus_target_lun(h, lunaddrbytes, tmpdevice);
		this_device = currentsd[ncurrent];

		/*
		 * For external target devices, we have to insert a LUN 0 which
		 * doesn't show up in CCISS_REPORT_PHYSICAL data, but there
		 * is nonetheless an enclosure device there.  We have to
		 * present that otherwise fikus won't find anything if
		 * there is no lun 0.
		 */
		if (add_ext_target_dev(h, tmpdevice, this_device,
				lunaddrbytes, lunzerobits,
				&n_ext_target_devs)) {
			ncurrent++;
			this_device = currentsd[ncurrent];
		}

		*this_device = *tmpdevice;

		switch (this_device->devtype) {
		case TYPE_ROM:
			/* We don't *really* support actual CD-ROM devices,
			 * just "One Button Disaster Recovery" tape drive
			 * which temporarily pretends to be a CD-ROM drive.
			 * So we check that the device is really an OBDR tape
			 * device by checking for "$DR-10" in bytes 43-48 of
			 * the inquiry data.
			 */
			if (is_OBDR)
				ncurrent++;
			break;
		case TYPE_DISK:
			if (i < nphysicals)
				break;
			ncurrent++;
			break;
		case TYPE_TAPE:
		case TYPE_MEDIUM_CHANGER:
			ncurrent++;
			break;
		case TYPE_RAID:
			/* Only present the Smartarray HBA as a RAID controller.
			 * If it's a RAID controller other than the HBA itself
			 * (an external RAID controller, MSA500 or similar)
			 * don't present it.
			 */
			if (!is_hba_lunid(lunaddrbytes))
				break;
			ncurrent++;
			break;
		default:
			break;
		}
		if (ncurrent >= HPSA_MAX_DEVICES)
			break;
	}
	adjust_hpsa_scsi_table(h, hostno, currentsd, ncurrent);
out:
	kfree(tmpdevice);
	for (i = 0; i < ndev_allocated; i++)
		kfree(currentsd[i]);
	kfree(currentsd);
	kfree(physdev_list);
	kfree(logdev_list);
}

/* hpsa_scatter_gather takes a struct scsi_cmnd, (cmd), and does the pci
 * dma mapping  and fills in the scatter gather entries of the
 * hpsa command, cp.
 */
static int hpsa_scatter_gather(struct ctlr_info *h,
		struct CommandList *cp,
		struct scsi_cmnd *cmd)
{
	unsigned int len;
	struct scatterlist *sg;
	u64 addr64;
	int use_sg, i, sg_index, chained;
	struct SGDescriptor *curr_sg;

	BUG_ON(scsi_sg_count(cmd) > h->maxsgentries);

	use_sg = scsi_dma_map(cmd);
	if (use_sg < 0)
		return use_sg;

	if (!use_sg)
		goto sglist_finished;

	curr_sg = cp->SG;
	chained = 0;
	sg_index = 0;
	scsi_for_each_sg(cmd, sg, use_sg, i) {
		if (i == h->max_cmd_sg_entries - 1 &&
			use_sg > h->max_cmd_sg_entries) {
			chained = 1;
			curr_sg = h->cmd_sg_list[cp->cmdindex];
			sg_index = 0;
		}
		addr64 = (u64) sg_dma_address(sg);
		len  = sg_dma_len(sg);
		curr_sg->Addr.lower = (u32) (addr64 & 0x0FFFFFFFFULL);
		curr_sg->Addr.upper = (u32) ((addr64 >> 32) & 0x0FFFFFFFFULL);
		curr_sg->Len = len;
		curr_sg->Ext = 0;  /* we are not chaining */
		curr_sg++;
	}

	if (use_sg + chained > h->maxSG)
		h->maxSG = use_sg + chained;

	if (chained) {
		cp->Header.SGList = h->max_cmd_sg_entries;
		cp->Header.SGTotal = (u16) (use_sg + 1);
		if (hpsa_map_sg_chain_block(h, cp)) {
			scsi_dma_unmap(cmd);
			return -1;
		}
		return 0;
	}

sglist_finished:

	cp->Header.SGList = (u8) use_sg;   /* no. SGs contig in this cmd */
	cp->Header.SGTotal = (u16) use_sg; /* total sgs in this cmd list */
	return 0;
}


static int hpsa_scsi_queue_command_lck(struct scsi_cmnd *cmd,
	void (*done)(struct scsi_cmnd *))
{
	struct ctlr_info *h;
	struct hpsa_scsi_dev_t *dev;
	unsigned char scsi3addr[8];
	struct CommandList *c;
	unsigned long flags;

	/* Get the ptr to our adapter structure out of cmd->host. */
	h = sdev_to_hba(cmd->device);
	dev = cmd->device->hostdata;
	if (!dev) {
		cmd->result = DID_NO_CONNECT << 16;
		done(cmd);
		return 0;
	}
	memcpy(scsi3addr, dev->scsi3addr, sizeof(scsi3addr));

	spin_lock_irqsave(&h->lock, flags);
	if (unlikely(h->lockup_detected)) {
		spin_unlock_irqrestore(&h->lock, flags);
		cmd->result = DID_ERROR << 16;
		done(cmd);
		return 0;
	}
	spin_unlock_irqrestore(&h->lock, flags);
	c = cmd_alloc(h);
	if (c == NULL) {			/* trouble... */
		dev_err(&h->pdev->dev, "cmd_alloc returned NULL!\n");
		return SCSI_MLQUEUE_HOST_BUSY;
	}

	/* Fill in the command list header */

	cmd->scsi_done = done;    /* save this for use by completion code */

	/* save c in case we have to abort it  */
	cmd->host_scribble = (unsigned char *) c;

	c->cmd_type = CMD_SCSI;
	c->scsi_cmd = cmd;
	c->Header.ReplyQueue = 0;  /* unused in simple mode */
	memcpy(&c->Header.LUN.LunAddrBytes[0], &scsi3addr[0], 8);
	c->Header.Tag.lower = (c->cmdindex << DIRECT_LOOKUP_SHIFT);
	c->Header.Tag.lower |= DIRECT_LOOKUP_BIT;

	/* Fill in the request block... */

	c->Request.Timeout = 0;
	memset(c->Request.CDB, 0, sizeof(c->Request.CDB));
	BUG_ON(cmd->cmd_len > sizeof(c->Request.CDB));
	c->Request.CDBLen = cmd->cmd_len;
	memcpy(c->Request.CDB, cmd->cmnd, cmd->cmd_len);
	c->Request.Type.Type = TYPE_CMD;
	c->Request.Type.Attribute = ATTR_SIMPLE;
	switch (cmd->sc_data_direction) {
	case DMA_TO_DEVICE:
		c->Request.Type.Direction = XFER_WRITE;
		break;
	case DMA_FROM_DEVICE:
		c->Request.Type.Direction = XFER_READ;
		break;
	case DMA_NONE:
		c->Request.Type.Direction = XFER_NONE;
		break;
	case DMA_BIDIRECTIONAL:
		/* This can happen if a buggy application does a scsi passthru
		 * and sets both inlen and outlen to non-zero. ( see
		 * ../scsi/scsi_ioctl.c:scsi_ioctl_send_command() )
		 */

		c->Request.Type.Direction = XFER_RSVD;
		/* This is technically wrong, and hpsa controllers should
		 * reject it with CMD_INVALID, which is the most correct
		 * response, but non-fibre backends appear to let it
		 * slide by, and give the same results as if this field
		 * were set correctly.  Either way is acceptable for
		 * our purposes here.
		 */

		break;

	default:
		dev_err(&h->pdev->dev, "unknown data direction: %d\n",
			cmd->sc_data_direction);
		BUG();
		break;
	}

	if (hpsa_scatter_gather(h, c, cmd) < 0) { /* Fill SG list */
		cmd_free(h, c);
		return SCSI_MLQUEUE_HOST_BUSY;
	}
	enqueue_cmd_and_start_io(h, c);
	/* the cmd'll come back via intr handler in complete_scsi_command()  */
	return 0;
}

static DEF_SCSI_QCMD(hpsa_scsi_queue_command)

static void hpsa_scan_start(struct Scsi_Host *sh)
{
	struct ctlr_info *h = shost_to_hba(sh);
	unsigned long flags;

	/* wait until any scan already in progress is finished. */
	while (1) {
		spin_lock_irqsave(&h->scan_lock, flags);
		if (h->scan_finished)
			break;
		spin_unlock_irqrestore(&h->scan_lock, flags);
		wait_event(h->scan_wait_queue, h->scan_finished);
		/* Note: We don't need to worry about a race between this
		 * thread and driver unload because the midlayer will
		 * have incremented the reference count, so unload won't
		 * happen if we're in here.
		 */
	}
	h->scan_finished = 0; /* mark scan as in progress */
	spin_unlock_irqrestore(&h->scan_lock, flags);

	hpsa_update_scsi_devices(h, h->scsi_host->host_no);

	spin_lock_irqsave(&h->scan_lock, flags);
	h->scan_finished = 1; /* mark scan as finished. */
	wake_up_all(&h->scan_wait_queue);
	spin_unlock_irqrestore(&h->scan_lock, flags);
}

static int hpsa_scan_finished(struct Scsi_Host *sh,
	unsigned long elapsed_time)
{
	struct ctlr_info *h = shost_to_hba(sh);
	unsigned long flags;
	int finished;

	spin_lock_irqsave(&h->scan_lock, flags);
	finished = h->scan_finished;
	spin_unlock_irqrestore(&h->scan_lock, flags);
	return finished;
}

static int hpsa_change_queue_depth(struct scsi_device *sdev,
	int qdepth, int reason)
{
	struct ctlr_info *h = sdev_to_hba(sdev);

	if (reason != SCSI_QDEPTH_DEFAULT)
		return -ENOTSUPP;

	if (qdepth < 1)
		qdepth = 1;
	else
		if (qdepth > h->nr_cmds)
			qdepth = h->nr_cmds;
	scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev), qdepth);
	return sdev->queue_depth;
}

static void hpsa_unregister_scsi(struct ctlr_info *h)
{
	/* we are being forcibly unloaded, and may not refuse. */
	scsi_remove_host(h->scsi_host);
	scsi_host_put(h->scsi_host);
	h->scsi_host = NULL;
}

static int hpsa_register_scsi(struct ctlr_info *h)
{
	struct Scsi_Host *sh;
	int error;

	sh = scsi_host_alloc(&hpsa_driver_template, sizeof(h));
	if (sh == NULL)
		goto fail;

	sh->io_port = 0;
	sh->n_io_port = 0;
	sh->this_id = -1;
	sh->max_channel = 3;
	sh->max_cmd_len = MAX_COMMAND_SIZE;
	sh->max_lun = HPSA_MAX_LUN;
	sh->max_id = HPSA_MAX_LUN;
	sh->can_queue = h->nr_cmds;
	sh->cmd_per_lun = h->nr_cmds;
	sh->sg_tablesize = h->maxsgentries;
	h->scsi_host = sh;
	sh->hostdata[0] = (unsigned long) h;
	sh->irq = h->intr[h->intr_mode];
	sh->unique_id = sh->irq;
	error = scsi_add_host(sh, &h->pdev->dev);
	if (error)
		goto fail_host_put;
	scsi_scan_host(sh);
	return 0;

 fail_host_put:
	dev_err(&h->pdev->dev, "%s: scsi_add_host"
		" failed for controller %d\n", __func__, h->ctlr);
	scsi_host_put(sh);
	return error;
 fail:
	dev_err(&h->pdev->dev, "%s: scsi_host_alloc"
		" failed for controller %d\n", __func__, h->ctlr);
	return -ENOMEM;
}

static int wait_for_device_to_become_ready(struct ctlr_info *h,
	unsigned char lunaddr[])
{
	int rc = 0;
	int count = 0;
	int waittime = 1; /* seconds */
	struct CommandList *c;

	c = cmd_special_alloc(h);
	if (!c) {
		dev_warn(&h->pdev->dev, "out of memory in "
			"wait_for_device_to_become_ready.\n");
		return IO_ERROR;
	}

	/* Send test unit ready until device ready, or give up. */
	while (count < HPSA_TUR_RETRY_LIMIT) {

		/* Wait for a bit.  do this first, because if we send
		 * the TUR right away, the reset will just abort it.
		 */
		msleep(1000 * waittime);
		count++;

		/* Increase wait time with each try, up to a point. */
		if (waittime < HPSA_MAX_WAIT_INTERVAL_SECS)
			waittime = waittime * 2;

		/* Send the Test Unit Ready, fill_cmd can't fail, no mapping */
		(void) fill_cmd(c, TEST_UNIT_READY, h,
				NULL, 0, 0, lunaddr, TYPE_CMD);
		hpsa_scsi_do_simple_cmd_core(h, c);
		/* no unmap needed here because no data xfer. */

		if (c->err_info->CommandStatus == CMD_SUCCESS)
			break;

		if (c->err_info->CommandStatus == CMD_TARGET_STATUS &&
			c->err_info->ScsiStatus == SAM_STAT_CHECK_CONDITION &&
			(c->err_info->SenseInfo[2] == NO_SENSE ||
			c->err_info->SenseInfo[2] == UNIT_ATTENTION))
			break;

		dev_warn(&h->pdev->dev, "waiting %d secs "
			"for device to become ready.\n", waittime);
		rc = 1; /* device not ready. */
	}

	if (rc)
		dev_warn(&h->pdev->dev, "giving up on device.\n");
	else
		dev_warn(&h->pdev->dev, "device is ready.\n");

	cmd_special_free(h, c);
	return rc;
}

/* Need at least one of these error handlers to keep ../scsi/hosts.c from
 * complaining.  Doing a host- or bus-reset can't do anything good here.
 */
static int hpsa_eh_device_reset_handler(struct scsi_cmnd *scsicmd)
{
	int rc;
	struct ctlr_info *h;
	struct hpsa_scsi_dev_t *dev;

	/* find the controller to which the command to be aborted was sent */
	h = sdev_to_hba(scsicmd->device);
	if (h == NULL) /* paranoia */
		return FAILED;
	dev = scsicmd->device->hostdata;
	if (!dev) {
		dev_err(&h->pdev->dev, "hpsa_eh_device_reset_handler: "
			"device lookup failed.\n");
		return FAILED;
	}
	dev_warn(&h->pdev->dev, "resetting device %d:%d:%d:%d\n",
		h->scsi_host->host_no, dev->bus, dev->target, dev->lun);
	/* send a reset to the SCSI LUN which the command was sent to */
	rc = hpsa_send_reset(h, dev->scsi3addr);
	if (rc == 0 && wait_for_device_to_become_ready(h, dev->scsi3addr) == 0)
		return SUCCESS;

	dev_warn(&h->pdev->dev, "resetting device failed.\n");
	return FAILED;
}

static void swizzle_abort_tag(u8 *tag)
{
	u8 original_tag[8];

	memcpy(original_tag, tag, 8);
	tag[0] = original_tag[3];
	tag[1] = original_tag[2];
	tag[2] = original_tag[1];
	tag[3] = original_tag[0];
	tag[4] = original_tag[7];
	tag[5] = original_tag[6];
	tag[6] = original_tag[5];
	tag[7] = original_tag[4];
}

static int hpsa_send_abort(struct ctlr_info *h, unsigned char *scsi3addr,
	struct CommandList *abort, int swizzle)
{
	int rc = IO_OK;
	struct CommandList *c;
	struct ErrorInfo *ei;

	c = cmd_special_alloc(h);
	if (c == NULL) {	/* trouble... */
		dev_warn(&h->pdev->dev, "cmd_special_alloc returned NULL!\n");
		return -ENOMEM;
	}

	/* fill_cmd can't fail here, no buffer to map */
	(void) fill_cmd(c, HPSA_ABORT_MSG, h, abort,
		0, 0, scsi3addr, TYPE_MSG);
	if (swizzle)
		swizzle_abort_tag(&c->Request.CDB[4]);
	hpsa_scsi_do_simple_cmd_core(h, c);
	dev_dbg(&h->pdev->dev, "%s: Tag:0x%08x:%08x: do_simple_cmd_core completed.\n",
		__func__, abort->Header.Tag.upper, abort->Header.Tag.lower);
	/* no unmap needed here because no data xfer. */

	ei = c->err_info;
	switch (ei->CommandStatus) {
	case CMD_SUCCESS:
		break;
	case CMD_UNABORTABLE: /* Very common, don't make noise. */
		rc = -1;
		break;
	default:
		dev_dbg(&h->pdev->dev, "%s: Tag:0x%08x:%08x: interpreting error.\n",
			__func__, abort->Header.Tag.upper,
			abort->Header.Tag.lower);
		hpsa_scsi_interpret_error(c);
		rc = -1;
		break;
	}
	cmd_special_free(h, c);
	dev_dbg(&h->pdev->dev, "%s: Tag:0x%08x:%08x: Finished.\n", __func__,
		abort->Header.Tag.upper, abort->Header.Tag.lower);
	return rc;
}

/*
 * hpsa_find_cmd_in_queue
 *
 * Used to determine whether a command (find) is still present
 * in queue_head.   Optionally excludes the last element of queue_head.
 *
 * This is used to avoid unnecessary aborts.  Commands in h->reqQ have
 * not yet been submitted, and so can be aborted by the driver without
 * sending an abort to the hardware.
 *
 * Returns pointer to command if found in queue, NULL otherwise.
 */
static struct CommandList *hpsa_find_cmd_in_queue(struct ctlr_info *h,
			struct scsi_cmnd *find, struct list_head *queue_head)
{
	unsigned long flags;
	struct CommandList *c = NULL;	/* ptr into cmpQ */

	if (!find)
		return 0;
	spin_lock_irqsave(&h->lock, flags);
	list_for_each_entry(c, queue_head, list) {
		if (c->scsi_cmd == NULL) /* e.g.: passthru ioctl */
			continue;
		if (c->scsi_cmd == find) {
			spin_unlock_irqrestore(&h->lock, flags);
			return c;
		}
	}
	spin_unlock_irqrestore(&h->lock, flags);
	return NULL;
}

static struct CommandList *hpsa_find_cmd_in_queue_by_tag(struct ctlr_info *h,
					u8 *tag, struct list_head *queue_head)
{
	unsigned long flags;
	struct CommandList *c;

	spin_lock_irqsave(&h->lock, flags);
	list_for_each_entry(c, queue_head, list) {
		if (memcmp(&c->Header.Tag, tag, 8) != 0)
			continue;
		spin_unlock_irqrestore(&h->lock, flags);
		return c;
	}
	spin_unlock_irqrestore(&h->lock, flags);
	return NULL;
}

/* Some Smart Arrays need the abort tag swizzled, and some don't.  It's hard to
 * tell which kind we're dealing with, so we send the abort both ways.  There
 * shouldn't be any collisions between swizzled and unswizzled tags due to the
 * way we construct our tags but we check anyway in case the assumptions which
 * make this true someday become false.
 */
static int hpsa_send_abort_both_ways(struct ctlr_info *h,
	unsigned char *scsi3addr, struct CommandList *abort)
{
	u8 swizzled_tag[8];
	struct CommandList *c;
	int rc = 0, rc2 = 0;

	/* we do not expect to find the swizzled tag in our queue, but
	 * check anyway just to be sure the assumptions which make this
	 * the case haven't become wrong.
	 */
	memcpy(swizzled_tag, &abort->Request.CDB[4], 8);
	swizzle_abort_tag(swizzled_tag);
	c = hpsa_find_cmd_in_queue_by_tag(h, swizzled_tag, &h->cmpQ);
	if (c != NULL) {
		dev_warn(&h->pdev->dev, "Unexpectedly found byte-swapped tag in completion queue.\n");
		return hpsa_send_abort(h, scsi3addr, abort, 0);
	}
	rc = hpsa_send_abort(h, scsi3addr, abort, 0);

	/* if the command is still in our queue, we can't conclude that it was
	 * aborted (it might have just completed normally) but in any case
	 * we don't need to try to abort it another way.
	 */
	c = hpsa_find_cmd_in_queue(h, abort->scsi_cmd, &h->cmpQ);
	if (c)
		rc2 = hpsa_send_abort(h, scsi3addr, abort, 1);
	return rc && rc2;
}

/* Send an abort for the specified command.
 *	If the device and controller support it,
 *		send a task abort request.
 */
static int hpsa_eh_abort_handler(struct scsi_cmnd *sc)
{

	int i, rc;
	struct ctlr_info *h;
	struct hpsa_scsi_dev_t *dev;
	struct CommandList *abort; /* pointer to command to be aborted */
	struct CommandList *found;
	struct scsi_cmnd *as;	/* ptr to scsi cmd inside aborted command. */
	char msg[256];		/* For debug messaging. */
	int ml = 0;

	/* Find the controller of the command to be aborted */
	h = sdev_to_hba(sc->device);
	if (WARN(h == NULL,
			"ABORT REQUEST FAILED, Controller lookup failed.\n"))
		return FAILED;

	/* Check that controller supports some kind of task abort */
	if (!(HPSATMF_PHYS_TASK_ABORT & h->TMFSupportFlags) &&
		!(HPSATMF_LOG_TASK_ABORT & h->TMFSupportFlags))
		return FAILED;

	memset(msg, 0, sizeof(msg));
	ml += sprintf(msg+ml, "ABORT REQUEST on C%d:B%d:T%d:L%d ",
		h->scsi_host->host_no, sc->device->channel,
		sc->device->id, sc->device->lun);

	/* Find the device of the command to be aborted */
	dev = sc->device->hostdata;
	if (!dev) {
		dev_err(&h->pdev->dev, "%s FAILED, Device lookup failed.\n",
				msg);
		return FAILED;
	}

	/* Get SCSI command to be aborted */
	abort = (struct CommandList *) sc->host_scribble;
	if (abort == NULL) {
		dev_err(&h->pdev->dev, "%s FAILED, Command to abort is NULL.\n",
				msg);
		return FAILED;
	}

	ml += sprintf(msg+ml, "Tag:0x%08x:%08x ",
		abort->Header.Tag.upper, abort->Header.Tag.lower);
	as  = (struct scsi_cmnd *) abort->scsi_cmd;
	if (as != NULL)
		ml += sprintf(msg+ml, "Command:0x%x SN:0x%lx ",
			as->cmnd[0], as->serial_number);
	dev_dbg(&h->pdev->dev, "%s\n", msg);
	dev_warn(&h->pdev->dev, "Abort request on C%d:B%d:T%d:L%d\n",
		h->scsi_host->host_no, dev->bus, dev->target, dev->lun);

	/* Search reqQ to See if command is queued but not submitted,
	 * if so, complete the command with aborted status and remove
	 * it from the reqQ.
	 */
	found = hpsa_find_cmd_in_queue(h, sc, &h->reqQ);
	if (found) {
		found->err_info->CommandStatus = CMD_ABORTED;
		finish_cmd(found);
		dev_info(&h->pdev->dev, "%s Request SUCCEEDED (driver queue).\n",
				msg);
		return SUCCESS;
	}

	/* not in reqQ, if also not in cmpQ, must have already completed */
	found = hpsa_find_cmd_in_queue(h, sc, &h->cmpQ);
	if (!found)  {
		dev_dbg(&h->pdev->dev, "%s Request SUCCEEDED (not known to driver).\n",
				msg);
		return SUCCESS;
	}

	/*
	 * Command is in flight, or possibly already completed
	 * by the firmware (but not to the scsi mid layer) but we can't
	 * distinguish which.  Send the abort down.
	 */
	rc = hpsa_send_abort_both_ways(h, dev->scsi3addr, abort);
	if (rc != 0) {
		dev_dbg(&h->pdev->dev, "%s Request FAILED.\n", msg);
		dev_warn(&h->pdev->dev, "FAILED abort on device C%d:B%d:T%d:L%d\n",
			h->scsi_host->host_no,
			dev->bus, dev->target, dev->lun);
		return FAILED;
	}
	dev_info(&h->pdev->dev, "%s REQUEST SUCCEEDED.\n", msg);

	/* If the abort(s) above completed and actually aborted the
	 * command, then the command to be aborted should already be
	 * completed.  If not, wait around a bit more to see if they
	 * manage to complete normally.
	 */
#define ABORT_COMPLETE_WAIT_SECS 30
	for (i = 0; i < ABORT_COMPLETE_WAIT_SECS * 10; i++) {
		found = hpsa_find_cmd_in_queue(h, sc, &h->cmpQ);
		if (!found)
			return SUCCESS;
		msleep(100);
	}
	dev_warn(&h->pdev->dev, "%s FAILED. Aborted command has not completed after %d seconds.\n",
		msg, ABORT_COMPLETE_WAIT_SECS);
	return FAILED;
}


/*
 * For operations that cannot sleep, a command block is allocated at init,
 * and managed by cmd_alloc() and cmd_free() using a simple bitmap to track
 * which ones are free or in use.  Lock must be held when calling this.
 * cmd_free() is the complement.
 */
static struct CommandList *cmd_alloc(struct ctlr_info *h)
{
	struct CommandList *c;
	int i;
	union u64bit temp64;
	dma_addr_t cmd_dma_handle, err_dma_handle;
	unsigned long flags;

	spin_lock_irqsave(&h->lock, flags);
	do {
		i = find_first_zero_bit(h->cmd_pool_bits, h->nr_cmds);
		if (i == h->nr_cmds) {
			spin_unlock_irqrestore(&h->lock, flags);
			return NULL;
		}
	} while (test_and_set_bit
		 (i & (BITS_PER_LONG - 1),
		  h->cmd_pool_bits + (i / BITS_PER_LONG)) != 0);
	spin_unlock_irqrestore(&h->lock, flags);

	c = h->cmd_pool + i;
	memset(c, 0, sizeof(*c));
	cmd_dma_handle = h->cmd_pool_dhandle
	    + i * sizeof(*c);
	c->err_info = h->errinfo_pool + i;
	memset(c->err_info, 0, sizeof(*c->err_info));
	err_dma_handle = h->errinfo_pool_dhandle
	    + i * sizeof(*c->err_info);

	c->cmdindex = i;

	INIT_LIST_HEAD(&c->list);
	c->busaddr = (u32) cmd_dma_handle;
	temp64.val = (u64) err_dma_handle;
	c->ErrDesc.Addr.lower = temp64.val32.lower;
	c->ErrDesc.Addr.upper = temp64.val32.upper;
	c->ErrDesc.Len = sizeof(*c->err_info);

	c->h = h;
	return c;
}

/* For operations that can wait for kmalloc to possibly sleep,
 * this routine can be called. Lock need not be held to call
 * cmd_special_alloc. cmd_special_free() is the complement.
 */
static struct CommandList *cmd_special_alloc(struct ctlr_info *h)
{
	struct CommandList *c;
	union u64bit temp64;
	dma_addr_t cmd_dma_handle, err_dma_handle;

	c = pci_alloc_consistent(h->pdev, sizeof(*c), &cmd_dma_handle);
	if (c == NULL)
		return NULL;
	memset(c, 0, sizeof(*c));

	c->cmdindex = -1;

	c->err_info = pci_alloc_consistent(h->pdev, sizeof(*c->err_info),
		    &err_dma_handle);

	if (c->err_info == NULL) {
		pci_free_consistent(h->pdev,
			sizeof(*c), c, cmd_dma_handle);
		return NULL;
	}
	memset(c->err_info, 0, sizeof(*c->err_info));

	INIT_LIST_HEAD(&c->list);
	c->busaddr = (u32) cmd_dma_handle;
	temp64.val = (u64) err_dma_handle;
	c->ErrDesc.Addr.lower = temp64.val32.lower;
	c->ErrDesc.Addr.upper = temp64.val32.upper;
	c->ErrDesc.Len = sizeof(*c->err_info);

	c->h = h;
	return c;
}

static void cmd_free(struct ctlr_info *h, struct CommandList *c)
{
	int i;
	unsigned long flags;

	i = c - h->cmd_pool;
	spin_lock_irqsave(&h->lock, flags);
	clear_bit(i & (BITS_PER_LONG - 1),
		  h->cmd_pool_bits + (i / BITS_PER_LONG));
	spin_unlock_irqrestore(&h->lock, flags);
}

static void cmd_special_free(struct ctlr_info *h, struct CommandList *c)
{
	union u64bit temp64;

	temp64.val32.lower = c->ErrDesc.Addr.lower;
	temp64.val32.upper = c->ErrDesc.Addr.upper;
	pci_free_consistent(h->pdev, sizeof(*c->err_info),
			    c->err_info, (dma_addr_t) temp64.val);
	pci_free_consistent(h->pdev, sizeof(*c),
			    c, (dma_addr_t) (c->busaddr & DIRECT_LOOKUP_MASK));
}

#ifdef CONFIG_COMPAT

static int hpsa_ioctl32_passthru(struct scsi_device *dev, int cmd, void *arg)
{
	IOCTL32_Command_struct __user *arg32 =
	    (IOCTL32_Command_struct __user *) arg;
	IOCTL_Command_struct arg64;
	IOCTL_Command_struct __user *p = compat_alloc_user_space(sizeof(arg64));
	int err;
	u32 cp;

	memset(&arg64, 0, sizeof(arg64));
	err = 0;
	err |= copy_from_user(&arg64.LUN_info, &arg32->LUN_info,
			   sizeof(arg64.LUN_info));
	err |= copy_from_user(&arg64.Request, &arg32->Request,
			   sizeof(arg64.Request));
	err |= copy_from_user(&arg64.error_info, &arg32->error_info,
			   sizeof(arg64.error_info));
	err |= get_user(arg64.buf_size, &arg32->buf_size);
	err |= get_user(cp, &arg32->buf);
	arg64.buf = compat_ptr(cp);
	err |= copy_to_user(p, &arg64, sizeof(arg64));

	if (err)
		return -EFAULT;

	err = hpsa_ioctl(dev, CCISS_PASSTHRU, (void *)p);
	if (err)
		return err;
	err |= copy_in_user(&arg32->error_info, &p->error_info,
			 sizeof(arg32->error_info));
	if (err)
		return -EFAULT;
	return err;
}

static int hpsa_ioctl32_big_passthru(struct scsi_device *dev,
	int cmd, void *arg)
{
	BIG_IOCTL32_Command_struct __user *arg32 =
	    (BIG_IOCTL32_Command_struct __user *) arg;
	BIG_IOCTL_Command_struct arg64;
	BIG_IOCTL_Command_struct __user *p =
	    compat_alloc_user_space(sizeof(arg64));
	int err;
	u32 cp;

	memset(&arg64, 0, sizeof(arg64));
	err = 0;
	err |= copy_from_user(&arg64.LUN_info, &arg32->LUN_info,
			   sizeof(arg64.LUN_info));
	err |= copy_from_user(&arg64.Request, &arg32->Request,
			   sizeof(arg64.Request));
	err |= copy_from_user(&arg64.error_info, &arg32->error_info,
			   sizeof(arg64.error_info));
	err |= get_user(arg64.buf_size, &arg32->buf_size);
	err |= get_user(arg64.malloc_size, &arg32->malloc_size);
	err |= get_user(cp, &arg32->buf);
	arg64.buf = compat_ptr(cp);
	err |= copy_to_user(p, &arg64, sizeof(arg64));

	if (err)
		return -EFAULT;

	err = hpsa_ioctl(dev, CCISS_BIG_PASSTHRU, (void *)p);
	if (err)
		return err;
	err |= copy_in_user(&arg32->error_info, &p->error_info,
			 sizeof(arg32->error_info));
	if (err)
		return -EFAULT;
	return err;
}

static int hpsa_compat_ioctl(struct scsi_device *dev, int cmd, void *arg)
{
	switch (cmd) {
	case CCISS_GETPCIINFO:
	case CCISS_GETINTINFO:
	case CCISS_SETINTINFO:
	case CCISS_GETNODENAME:
	case CCISS_SETNODENAME:
	case CCISS_GETHEARTBEAT:
	case CCISS_GETBUSTYPES:
	case CCISS_GETFIRMVER:
	case CCISS_GETDRIVVER:
	case CCISS_REVALIDVOLS:
	case CCISS_DEREGDISK:
	case CCISS_REGNEWDISK:
	case CCISS_REGNEWD:
	case CCISS_RESCANDISK:
	case CCISS_GETLUNINFO:
		return hpsa_ioctl(dev, cmd, arg);

	case CCISS_PASSTHRU32:
		return hpsa_ioctl32_passthru(dev, cmd, arg);
	case CCISS_BIG_PASSTHRU32:
		return hpsa_ioctl32_big_passthru(dev, cmd, arg);

	default:
		return -ENOIOCTLCMD;
	}
}
#endif

static int hpsa_getpciinfo_ioctl(struct ctlr_info *h, void __user *argp)
{
	struct hpsa_pci_info pciinfo;

	if (!argp)
		return -EINVAL;
	pciinfo.domain = pci_domain_nr(h->pdev->bus);
	pciinfo.bus = h->pdev->bus->number;
	pciinfo.dev_fn = h->pdev->devfn;
	pciinfo.board_id = h->board_id;
	if (copy_to_user(argp, &pciinfo, sizeof(pciinfo)))
		return -EFAULT;
	return 0;
}

static int hpsa_getdrivver_ioctl(struct ctlr_info *h, void __user *argp)
{
	DriverVer_type DriverVer;
	unsigned char vmaj, vmin, vsubmin;
	int rc;

	rc = sscanf(HPSA_DRIVER_VERSION, "%hhu.%hhu.%hhu",
		&vmaj, &vmin, &vsubmin);
	if (rc != 3) {
		dev_info(&h->pdev->dev, "driver version string '%s' "
			"unrecognized.", HPSA_DRIVER_VERSION);
		vmaj = 0;
		vmin = 0;
		vsubmin = 0;
	}
	DriverVer = (vmaj << 16) | (vmin << 8) | vsubmin;
	if (!argp)
		return -EINVAL;
	if (copy_to_user(argp, &DriverVer, sizeof(DriverVer_type)))
		return -EFAULT;
	return 0;
}

static int hpsa_passthru_ioctl(struct ctlr_info *h, void __user *argp)
{
	IOCTL_Command_struct iocommand;
	struct CommandList *c;
	char *buff = NULL;
	union u64bit temp64;
	int rc = 0;

	if (!argp)
		return -EINVAL;
	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;
	if (copy_from_user(&iocommand, argp, sizeof(iocommand)))
		return -EFAULT;
	if ((iocommand.buf_size < 1) &&
	    (iocommand.Request.Type.Direction != XFER_NONE)) {
		return -EINVAL;
	}
	if (iocommand.buf_size > 0) {
		buff = kmalloc(iocommand.buf_size, GFP_KERNEL);
		if (buff == NULL)
			return -EFAULT;
		if (iocommand.Request.Type.Direction == XFER_WRITE) {
			/* Copy the data into the buffer we created */
			if (copy_from_user(buff, iocommand.buf,
				iocommand.buf_size)) {
				rc = -EFAULT;
				goto out_kfree;
			}
		} else {
			memset(buff, 0, iocommand.buf_size);
		}
	}
	c = cmd_special_alloc(h);
	if (c == NULL) {
		rc = -ENOMEM;
		goto out_kfree;
	}
	/* Fill in the command type */
	c->cmd_type = CMD_IOCTL_PEND;
	/* Fill in Command Header */
	c->Header.ReplyQueue = 0; /* unused in simple mode */
	if (iocommand.buf_size > 0) {	/* buffer to fill */
		c->Header.SGList = 1;
		c->Header.SGTotal = 1;
	} else	{ /* no buffers to fill */
		c->Header.SGList = 0;
		c->Header.SGTotal = 0;
	}
	memcpy(&c->Header.LUN, &iocommand.LUN_info, sizeof(c->Header.LUN));
	/* use the kernel address the cmd block for tag */
	c->Header.Tag.lower = c->busaddr;

	/* Fill in Request block */
	memcpy(&c->Request, &iocommand.Request,
		sizeof(c->Request));

	/* Fill in the scatter gather information */
	if (iocommand.buf_size > 0) {
		temp64.val = pci_map_single(h->pdev, buff,
			iocommand.buf_size, PCI_DMA_BIDIRECTIONAL);
		if (dma_mapping_error(&h->pdev->dev, temp64.val)) {
			c->SG[0].Addr.lower = 0;
			c->SG[0].Addr.upper = 0;
			c->SG[0].Len = 0;
			rc = -ENOMEM;
			goto out;
		}
		c->SG[0].Addr.lower = temp64.val32.lower;
		c->SG[0].Addr.upper = temp64.val32.upper;
		c->SG[0].Len = iocommand.buf_size;
		c->SG[0].Ext = 0; /* we are not chaining*/
	}
	hpsa_scsi_do_simple_cmd_core_if_no_lockup(h, c);
	if (iocommand.buf_size > 0)
		hpsa_pci_unmap(h->pdev, c, 1, PCI_DMA_BIDIRECTIONAL);
	check_ioctl_unit_attention(h, c);

	/* Copy the error information out */
	memcpy(&iocommand.error_info, c->err_info,
		sizeof(iocommand.error_info));
	if (copy_to_user(argp, &iocommand, sizeof(iocommand))) {
		rc = -EFAULT;
		goto out;
	}
	if (iocommand.Request.Type.Direction == XFER_READ &&
		iocommand.buf_size > 0) {
		/* Copy the data out of the buffer we created */
		if (copy_to_user(iocommand.buf, buff, iocommand.buf_size)) {
			rc = -EFAULT;
			goto out;
		}
	}
out:
	cmd_special_free(h, c);
out_kfree:
	kfree(buff);
	return rc;
}

static int hpsa_big_passthru_ioctl(struct ctlr_info *h, void __user *argp)
{
	BIG_IOCTL_Command_struct *ioc;
	struct CommandList *c;
	unsigned char **buff = NULL;
	int *buff_size = NULL;
	union u64bit temp64;
	BYTE sg_used = 0;
	int status = 0;
	int i;
	u32 left;
	u32 sz;
	BYTE __user *data_ptr;

	if (!argp)
		return -EINVAL;
	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;
	ioc = (BIG_IOCTL_Command_struct *)
	    kmalloc(sizeof(*ioc), GFP_KERNEL);
	if (!ioc) {
		status = -ENOMEM;
		goto cleanup1;
	}
	if (copy_from_user(ioc, argp, sizeof(*ioc))) {
		status = -EFAULT;
		goto cleanup1;
	}
	if ((ioc->buf_size < 1) &&
	    (ioc->Request.Type.Direction != XFER_NONE)) {
		status = -EINVAL;
		goto cleanup1;
	}
	/* Check kmalloc limits  using all SGs */
	if (ioc->malloc_size > MAX_KMALLOC_SIZE) {
		status = -EINVAL;
		goto cleanup1;
	}
	if (ioc->buf_size > ioc->malloc_size * SG_ENTRIES_IN_CMD) {
		status = -EINVAL;
		goto cleanup1;
	}
	buff = kzalloc(SG_ENTRIES_IN_CMD * sizeof(char *), GFP_KERNEL);
	if (!buff) {
		status = -ENOMEM;
		goto cleanup1;
	}
	buff_size = kmalloc(SG_ENTRIES_IN_CMD * sizeof(int), GFP_KERNEL);
	if (!buff_size) {
		status = -ENOMEM;
		goto cleanup1;
	}
	left = ioc->buf_size;
	data_ptr = ioc->buf;
	while (left) {
		sz = (left > ioc->malloc_size) ? ioc->malloc_size : left;
		buff_size[sg_used] = sz;
		buff[sg_used] = kmalloc(sz, GFP_KERNEL);
		if (buff[sg_used] == NULL) {
			status = -ENOMEM;
			goto cleanup1;
		}
		if (ioc->Request.Type.Direction == XFER_WRITE) {
			if (copy_from_user(buff[sg_used], data_ptr, sz)) {
				status = -ENOMEM;
				goto cleanup1;
			}
		} else
			memset(buff[sg_used], 0, sz);
		left -= sz;
		data_ptr += sz;
		sg_used++;
	}
	c = cmd_special_alloc(h);
	if (c == NULL) {
		status = -ENOMEM;
		goto cleanup1;
	}
	c->cmd_type = CMD_IOCTL_PEND;
	c->Header.ReplyQueue = 0;
	c->Header.SGList = c->Header.SGTotal = sg_used;
	memcpy(&c->Header.LUN, &ioc->LUN_info, sizeof(c->Header.LUN));
	c->Header.Tag.lower = c->busaddr;
	memcpy(&c->Request, &ioc->Request, sizeof(c->Request));
	if (ioc->buf_size > 0) {
		int i;
		for (i = 0; i < sg_used; i++) {
			temp64.val = pci_map_single(h->pdev, buff[i],
				    buff_size[i], PCI_DMA_BIDIRECTIONAL);
			if (dma_mapping_error(&h->pdev->dev, temp64.val)) {
				c->SG[i].Addr.lower = 0;
				c->SG[i].Addr.upper = 0;
				c->SG[i].Len = 0;
				hpsa_pci_unmap(h->pdev, c, i,
					PCI_DMA_BIDIRECTIONAL);
				status = -ENOMEM;
				goto cleanup1;
			}
			c->SG[i].Addr.lower = temp64.val32.lower;
			c->SG[i].Addr.upper = temp64.val32.upper;
			c->SG[i].Len = buff_size[i];
			/* we are not chaining */
			c->SG[i].Ext = 0;
		}
	}
	hpsa_scsi_do_simple_cmd_core_if_no_lockup(h, c);
	if (sg_used)
		hpsa_pci_unmap(h->pdev, c, sg_used, PCI_DMA_BIDIRECTIONAL);
	check_ioctl_unit_attention(h, c);
	/* Copy the error information out */
	memcpy(&ioc->error_info, c->err_info, sizeof(ioc->error_info));
	if (copy_to_user(argp, ioc, sizeof(*ioc))) {
		cmd_special_free(h, c);
		status = -EFAULT;
		goto cleanup1;
	}
	if (ioc->Request.Type.Direction == XFER_READ && ioc->buf_size > 0) {
		/* Copy the data out of the buffer we created */
		BYTE __user *ptr = ioc->buf;
		for (i = 0; i < sg_used; i++) {
			if (copy_to_user(ptr, buff[i], buff_size[i])) {
				cmd_special_free(h, c);
				status = -EFAULT;
				goto cleanup1;
			}
			ptr += buff_size[i];
		}
	}
	cmd_special_free(h, c);
	status = 0;
cleanup1:
	if (buff) {
		for (i = 0; i < sg_used; i++)
			kfree(buff[i]);
		kfree(buff);
	}
	kfree(buff_size);
	kfree(ioc);
	return status;
}

static void check_ioctl_unit_attention(struct ctlr_info *h,
	struct CommandList *c)
{
	if (c->err_info->CommandStatus == CMD_TARGET_STATUS &&
			c->err_info->ScsiStatus != SAM_STAT_CHECK_CONDITION)
		(void) check_for_unit_attention(h, c);
}
/*
 * ioctl
 */
static int hpsa_ioctl(struct scsi_device *dev, int cmd, void *arg)
{
	struct ctlr_info *h;
	void __user *argp = (void __user *)arg;

	h = sdev_to_hba(dev);

	switch (cmd) {
	case CCISS_DEREGDISK:
	case CCISS_REGNEWDISK:
	case CCISS_REGNEWD:
		hpsa_scan_start(h->scsi_host);
		return 0;
	case CCISS_GETPCIINFO:
		return hpsa_getpciinfo_ioctl(h, argp);
	case CCISS_GETDRIVVER:
		return hpsa_getdrivver_ioctl(h, argp);
	case CCISS_PASSTHRU:
		return hpsa_passthru_ioctl(h, argp);
	case CCISS_BIG_PASSTHRU:
		return hpsa_big_passthru_ioctl(h, argp);
	default:
		return -ENOTTY;
	}
}

static int hpsa_send_host_reset(struct ctlr_info *h, unsigned char *scsi3addr,
				u8 reset_type)
{
	struct CommandList *c;

	c = cmd_alloc(h);
	if (!c)
		return -ENOMEM;
	/* fill_cmd can't fail here, no data buffer to map */
	(void) fill_cmd(c, HPSA_DEVICE_RESET_MSG, h, NULL, 0, 0,
		RAID_CTLR_LUNID, TYPE_MSG);
	c->Request.CDB[1] = reset_type; /* fill_cmd defaults to target reset */
	c->waiting = NULL;
	enqueue_cmd_and_start_io(h, c);
	/* Don't wait for completion, the reset won't complete.  Don't free
	 * the command either.  This is the last command we will send before
	 * re-initializing everything, so it doesn't matter and won't leak.
	 */
	return 0;
}

static int fill_cmd(struct CommandList *c, u8 cmd, struct ctlr_info *h,
	void *buff, size_t size, u8 page_code, unsigned char *scsi3addr,
	int cmd_type)
{
	int pci_dir = XFER_NONE;
	struct CommandList *a; /* for commands to be aborted */

	c->cmd_type = CMD_IOCTL_PEND;
	c->Header.ReplyQueue = 0;
	if (buff != NULL && size > 0) {
		c->Header.SGList = 1;
		c->Header.SGTotal = 1;
	} else {
		c->Header.SGList = 0;
		c->Header.SGTotal = 0;
	}
	c->Header.Tag.lower = c->busaddr;
	memcpy(c->Header.LUN.LunAddrBytes, scsi3addr, 8);

	c->Request.Type.Type = cmd_type;
	if (cmd_type == TYPE_CMD) {
		switch (cmd) {
		case HPSA_INQUIRY:
			/* are we trying to read a vital product page */
			if (page_code != 0) {
				c->Request.CDB[1] = 0x01;
				c->Request.CDB[2] = page_code;
			}
			c->Request.CDBLen = 6;
			c->Request.Type.Attribute = ATTR_SIMPLE;
			c->Request.Type.Direction = XFER_READ;
			c->Request.Timeout = 0;
			c->Request.CDB[0] = HPSA_INQUIRY;
			c->Request.CDB[4] = size & 0xFF;
			break;
		case HPSA_REPORT_LOG:
		case HPSA_REPORT_PHYS:
			/* Talking to controller so It's a physical command
			   mode = 00 target = 0.  Nothing to write.
			 */
			c->Request.CDBLen = 12;
			c->Request.Type.Attribute = ATTR_SIMPLE;
			c->Request.Type.Direction = XFER_READ;
			c->Request.Timeout = 0;
			c->Request.CDB[0] = cmd;
			c->Request.CDB[6] = (size >> 24) & 0xFF; /* MSB */
			c->Request.CDB[7] = (size >> 16) & 0xFF;
			c->Request.CDB[8] = (size >> 8) & 0xFF;
			c->Request.CDB[9] = size & 0xFF;
			break;
		case HPSA_CACHE_FLUSH:
			c->Request.CDBLen = 12;
			c->Request.Type.Attribute = ATTR_SIMPLE;
			c->Request.Type.Direction = XFER_WRITE;
			c->Request.Timeout = 0;
			c->Request.CDB[0] = BMIC_WRITE;
			c->Request.CDB[6] = BMIC_CACHE_FLUSH;
			c->Request.CDB[7] = (size >> 8) & 0xFF;
			c->Request.CDB[8] = size & 0xFF;
			break;
		case TEST_UNIT_READY:
			c->Request.CDBLen = 6;
			c->Request.Type.Attribute = ATTR_SIMPLE;
			c->Request.Type.Direction = XFER_NONE;
			c->Request.Timeout = 0;
			break;
		default:
			dev_warn(&h->pdev->dev, "unknown command 0x%c\n", cmd);
			BUG();
			return -1;
		}
	} else if (cmd_type == TYPE_MSG) {
		switch (cmd) {

		case  HPSA_DEVICE_RESET_MSG:
			c->Request.CDBLen = 16;
			c->Request.Type.Type =  1; /* It is a MSG not a CMD */
			c->Request.Type.Attribute = ATTR_SIMPLE;
			c->Request.Type.Direction = XFER_NONE;
			c->Request.Timeout = 0; /* Don't time out */
			memset(&c->Request.CDB[0], 0, sizeof(c->Request.CDB));
			c->Request.CDB[0] =  cmd;
			c->Request.CDB[1] = HPSA_RESET_TYPE_LUN;
			/* If bytes 4-7 are zero, it means reset the */
			/* LunID device */
			c->Request.CDB[4] = 0x00;
			c->Request.CDB[5] = 0x00;
			c->Request.CDB[6] = 0x00;
			c->Request.CDB[7] = 0x00;
			break;
		case  HPSA_ABORT_MSG:
			a = buff;       /* point to command to be aborted */
			dev_dbg(&h->pdev->dev, "Abort Tag:0x%08x:%08x using request Tag:0x%08x:%08x\n",
				a->Header.Tag.upper, a->Header.Tag.lower,
				c->Header.Tag.upper, c->Header.Tag.lower);
			c->Request.CDBLen = 16;
			c->Request.Type.Type = TYPE_MSG;
			c->Request.Type.Attribute = ATTR_SIMPLE;
			c->Request.Type.Direction = XFER_WRITE;
			c->Request.Timeout = 0; /* Don't time out */
			c->Request.CDB[0] = HPSA_TASK_MANAGEMENT;
			c->Request.CDB[1] = HPSA_TMF_ABORT_TASK;
			c->Request.CDB[2] = 0x00; /* reserved */
			c->Request.CDB[3] = 0x00; /* reserved */
			/* Tag to abort goes in CDB[4]-CDB[11] */
			c->Request.CDB[4] = a->Header.Tag.lower & 0xFF;
			c->Request.CDB[5] = (a->Header.Tag.lower >> 8) & 0xFF;
			c->Request.CDB[6] = (a->Header.Tag.lower >> 16) & 0xFF;
			c->Request.CDB[7] = (a->Header.Tag.lower >> 24) & 0xFF;
			c->Request.CDB[8] = a->Header.Tag.upper & 0xFF;
			c->Request.CDB[9] = (a->Header.Tag.upper >> 8) & 0xFF;
			c->Request.CDB[10] = (a->Header.Tag.upper >> 16) & 0xFF;
			c->Request.CDB[11] = (a->Header.Tag.upper >> 24) & 0xFF;
			c->Request.CDB[12] = 0x00; /* reserved */
			c->Request.CDB[13] = 0x00; /* reserved */
			c->Request.CDB[14] = 0x00; /* reserved */
			c->Request.CDB[15] = 0x00; /* reserved */
		break;
		default:
			dev_warn(&h->pdev->dev, "unknown message type %d\n",
				cmd);
			BUG();
		}
	} else {
		dev_warn(&h->pdev->dev, "unknown command type %d\n", cmd_type);
		BUG();
	}

	switch (c->Request.Type.Direction) {
	case XFER_READ:
		pci_dir = PCI_DMA_FROMDEVICE;
		break;
	case XFER_WRITE:
		pci_dir = PCI_DMA_TODEVICE;
		break;
	case XFER_NONE:
		pci_dir = PCI_DMA_NONE;
		break;
	default:
		pci_dir = PCI_DMA_BIDIRECTIONAL;
	}
	if (hpsa_map_one(h->pdev, c, buff, size, pci_dir))
		return -1;
	return 0;
}

/*
 * Map (physical) PCI mem into (virtual) kernel space
 */
static void __iomem *remap_pci_mem(ulong base, ulong size)
{
	ulong page_base = ((ulong) base) & PAGE_MASK;
	ulong page_offs = ((ulong) base) - page_base;
	void __iomem *page_remapped = ioremap_nocache(page_base,
		page_offs + size);

	return page_remapped ? (page_remapped + page_offs) : NULL;
}

/* Takes cmds off the submission queue and sends them to the hardware,
 * then puts them on the queue of cmds waiting for completion.
 */
static void start_io(struct ctlr_info *h)
{
	struct CommandList *c;
	unsigned long flags;

	spin_lock_irqsave(&h->lock, flags);
	while (!list_empty(&h->reqQ)) {
		c = list_entry(h->reqQ.next, struct CommandList, list);
		/* can't do anything if fifo is full */
		if ((h->access.fifo_full(h))) {
			dev_warn(&h->pdev->dev, "fifo full\n");
			break;
		}

		/* Get the first entry from the Request Q */
		removeQ(c);
		h->Qdepth--;

		/* Put job onto the completed Q */
		addQ(&h->cmpQ, c);

		/* Must increment commands_outstanding before unlocking
		 * and submitting to avoid race checking for fifo full
		 * condition.
		 */
		h->commands_outstanding++;
		if (h->commands_outstanding > h->max_outstanding)
			h->max_outstanding = h->commands_outstanding;

		/* Tell the controller execute command */
		spin_unlock_irqrestore(&h->lock, flags);
		h->access.submit_command(h, c);
		spin_lock_irqsave(&h->lock, flags);
	}
	spin_unlock_irqrestore(&h->lock, flags);
}

static inline unsigned long get_next_completion(struct ctlr_info *h, u8 q)
{
	return h->access.command_completed(h, q);
}

static inline bool interrupt_pending(struct ctlr_info *h)
{
	return h->access.intr_pending(h);
}

static inline long interrupt_not_for_us(struct ctlr_info *h)
{
	return (h->access.intr_pending(h) == 0) ||
		(h->interrupts_enabled == 0);
}

static inline int bad_tag(struct ctlr_info *h, u32 tag_index,
	u32 raw_tag)
{
	if (unlikely(tag_index >= h->nr_cmds)) {
		dev_warn(&h->pdev->dev, "bad tag 0x%08x ignored.\n", raw_tag);
		return 1;
	}
	return 0;
}

static inline void finish_cmd(struct CommandList *c)
{
	unsigned long flags;

	spin_lock_irqsave(&c->h->lock, flags);
	removeQ(c);
	spin_unlock_irqrestore(&c->h->lock, flags);
	dial_up_lockup_detection_on_fw_flash_complete(c->h, c);
	if (likely(c->cmd_type == CMD_SCSI))
		complete_scsi_command(c);
	else if (c->cmd_type == CMD_IOCTL_PEND)
		complete(c->waiting);
}

static inline u32 hpsa_tag_contains_index(u32 tag)
{
	return tag & DIRECT_LOOKUP_BIT;
}

static inline u32 hpsa_tag_to_index(u32 tag)
{
	return tag >> DIRECT_LOOKUP_SHIFT;
}


static inline u32 hpsa_tag_discard_error_bits(struct ctlr_info *h, u32 tag)
{
#define HPSA_PERF_ERROR_BITS ((1 << DIRECT_LOOKUP_SHIFT) - 1)
#define HPSA_SIMPLE_ERROR_BITS 0x03
	if (unlikely(!(h->transMethod & CFGTBL_Trans_Performant)))
		return tag & ~HPSA_SIMPLE_ERROR_BITS;
	return tag & ~HPSA_PERF_ERROR_BITS;
}

/* process completion of an indexed ("direct lookup") command */
static inline void process_indexed_cmd(struct ctlr_info *h,
	u32 raw_tag)
{
	u32 tag_index;
	struct CommandList *c;

	tag_index = hpsa_tag_to_index(raw_tag);
	if (!bad_tag(h, tag_index, raw_tag)) {
		c = h->cmd_pool + tag_index;
		finish_cmd(c);
	}
}

/* process completion of a non-indexed command */
static inline void process_nonindexed_cmd(struct ctlr_info *h,
	u32 raw_tag)
{
	u32 tag;
	struct CommandList *c = NULL;
	unsigned long flags;

	tag = hpsa_tag_discard_error_bits(h, raw_tag);
	spin_lock_irqsave(&h->lock, flags);
	list_for_each_entry(c, &h->cmpQ, list) {
		if ((c->busaddr & 0xFFFFFFE0) == (tag & 0xFFFFFFE0)) {
			spin_unlock_irqrestore(&h->lock, flags);
			finish_cmd(c);
			return;
		}
	}
	spin_unlock_irqrestore(&h->lock, flags);
	bad_tag(h, h->nr_cmds + 1, raw_tag);
}

/* Some controllers, like p400, will give us one interrupt
 * after a soft reset, even if we turned interrupts off.
 * Only need to check for this in the hpsa_xxx_discard_completions
 * functions.
 */
static int ignore_bogus_interrupt(struct ctlr_info *h)
{
	if (likely(!reset_devices))
		return 0;

	if (likely(h->interrupts_enabled))
		return 0;

	dev_info(&h->pdev->dev, "Received interrupt while interrupts disabled "
		"(known firmware bug.)  Ignoring.\n");

	return 1;
}

/*
 * Convert &h->q[x] (passed to interrupt handlers) back to h.
 * Relies on (h-q[x] == x) being true for x such that
 * 0 <= x < MAX_REPLY_QUEUES.
 */
static struct ctlr_info *queue_to_hba(u8 *queue)
{
	return container_of((queue - *queue), struct ctlr_info, q[0]);
}

static irqreturn_t hpsa_intx_discard_completions(int irq, void *queue)
{
	struct ctlr_info *h = queue_to_hba(queue);
	u8 q = *(u8 *) queue;
	u32 raw_tag;

	if (ignore_bogus_interrupt(h))
		return IRQ_NONE;

	if (interrupt_not_for_us(h))
		return IRQ_NONE;
	h->last_intr_timestamp = get_jiffies_64();
	while (interrupt_pending(h)) {
		raw_tag = get_next_completion(h, q);
		while (raw_tag != FIFO_EMPTY)
			raw_tag = next_command(h, q);
	}
	return IRQ_HANDLED;
}

static irqreturn_t hpsa_msix_discard_completions(int irq, void *queue)
{
	struct ctlr_info *h = queue_to_hba(queue);
	u32 raw_tag;
	u8 q = *(u8 *) queue;

	if (ignore_bogus_interrupt(h))
		return IRQ_NONE;

	h->last_intr_timestamp = get_jiffies_64();
	raw_tag = get_next_completion(h, q);
	while (raw_tag != FIFO_EMPTY)
		raw_tag = next_command(h, q);
	return IRQ_HANDLED;
}

static irqreturn_t do_hpsa_intr_intx(int irq, void *queue)
{
	struct ctlr_info *h = queue_to_hba((u8 *) queue);
	u32 raw_tag;
	u8 q = *(u8 *) queue;

	if (interrupt_not_for_us(h))
		return IRQ_NONE;
	h->last_intr_timestamp = get_jiffies_64();
	while (interrupt_pending(h)) {
		raw_tag = get_next_completion(h, q);
		while (raw_tag != FIFO_EMPTY) {
			if (likely(hpsa_tag_contains_index(raw_tag)))
				process_indexed_cmd(h, raw_tag);
			else
				process_nonindexed_cmd(h, raw_tag);
			raw_tag = next_command(h, q);
		}
	}
	return IRQ_HANDLED;
}

static irqreturn_t do_hpsa_intr_msi(int irq, void *queue)
{
	struct ctlr_info *h = queue_to_hba(queue);
	u32 raw_tag;
	u8 q = *(u8 *) queue;

	h->last_intr_timestamp = get_jiffies_64();
	raw_tag = get_next_completion(h, q);
	while (raw_tag != FIFO_EMPTY) {
		if (likely(hpsa_tag_contains_index(raw_tag)))
			process_indexed_cmd(h, raw_tag);
		else
			process_nonindexed_cmd(h, raw_tag);
		raw_tag = next_command(h, q);
	}
	return IRQ_HANDLED;
}

/* Send a message CDB to the firmware. Careful, this only works
 * in simple mode, not performant mode due to the tag lookup.
 * We only ever use this immediately after a controller reset.
 */
static int hpsa_message(struct pci_dev *pdev, unsigned char opcode,
			unsigned char type)
{
	struct Command {
		struct CommandListHeader CommandHeader;
		struct RequestBlock Request;
		struct ErrDescriptor ErrorDescriptor;
	};
	struct Command *cmd;
	static const size_t cmd_sz = sizeof(*cmd) +
					sizeof(cmd->ErrorDescriptor);
	dma_addr_t paddr64;
	uint32_t paddr32, tag;
	void __iomem *vaddr;
	int i, err;

	vaddr = pci_ioremap_bar(pdev, 0);
	if (vaddr == NULL)
		return -ENOMEM;

	/* The Inbound Post Queue only accepts 32-bit physical addresses for the
	 * CCISS commands, so they must be allocated from the lower 4GiB of
	 * memory.
	 */
	err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	if (err) {
		iounmap(vaddr);
		return -ENOMEM;
	}

	cmd = pci_alloc_consistent(pdev, cmd_sz, &paddr64);
	if (cmd == NULL) {
		iounmap(vaddr);
		return -ENOMEM;
	}

	/* This must fit, because of the 32-bit consistent DMA mask.  Also,
	 * although there's no guarantee, we assume that the address is at
	 * least 4-byte aligned (most likely, it's page-aligned).
	 */
	paddr32 = paddr64;

	cmd->CommandHeader.ReplyQueue = 0;
	cmd->CommandHeader.SGList = 0;
	cmd->CommandHeader.SGTotal = 0;
	cmd->CommandHeader.Tag.lower = paddr32;
	cmd->CommandHeader.Tag.upper = 0;
	memset(&cmd->CommandHeader.LUN.LunAddrBytes, 0, 8);

	cmd->Request.CDBLen = 16;
	cmd->Request.Type.Type = TYPE_MSG;
	cmd->Request.Type.Attribute = ATTR_HEADOFQUEUE;
	cmd->Request.Type.Direction = XFER_NONE;
	cmd->Request.Timeout = 0; /* Don't time out */
	cmd->Request.CDB[0] = opcode;
	cmd->Request.CDB[1] = type;
	memset(&cmd->Request.CDB[2], 0, 14); /* rest of the CDB is reserved */
	cmd->ErrorDescriptor.Addr.lower = paddr32 + sizeof(*cmd);
	cmd->ErrorDescriptor.Addr.upper = 0;
	cmd->ErrorDescriptor.Len = sizeof(struct ErrorInfo);

	writel(paddr32, vaddr + SA5_REQUEST_PORT_OFFSET);

	for (i = 0; i < HPSA_MSG_SEND_RETRY_LIMIT; i++) {
		tag = readl(vaddr + SA5_REPLY_PORT_OFFSET);
		if ((tag & ~HPSA_SIMPLE_ERROR_BITS) == paddr32)
			break;
		msleep(HPSA_MSG_SEND_RETRY_INTERVAL_MSECS);
	}

	iounmap(vaddr);

	/* we leak the DMA buffer here ... no choice since the controller could
	 *  still complete the command.
	 */
	if (i == HPSA_MSG_SEND_RETRY_LIMIT) {
		dev_err(&pdev->dev, "controller message %02x:%02x timed out\n",
			opcode, type);
		return -ETIMEDOUT;
	}

	pci_free_consistent(pdev, cmd_sz, cmd, paddr64);

	if (tag & HPSA_ERROR_BIT) {
		dev_err(&pdev->dev, "controller message %02x:%02x failed\n",
			opcode, type);
		return -EIO;
	}

	dev_info(&pdev->dev, "controller message %02x:%02x succeeded\n",
		opcode, type);
	return 0;
}

#define hpsa_noop(p) hpsa_message(p, 3, 0)

static int hpsa_controller_hard_reset(struct pci_dev *pdev,
	void * __iomem vaddr, u32 use_doorbell)
{
	u16 pmcsr;
	int pos;

	if (use_doorbell) {
		/* For everything after the P600, the PCI power state method
		 * of resetting the controller doesn't work, so we have this
		 * other way using the doorbell register.
		 */
		dev_info(&pdev->dev, "using doorbell to reset controller\n");
		writel(use_doorbell, vaddr + SA5_DOORBELL);
	} else { /* Try to do it the PCI power state way */

		/* Quoting from the Open CISS Specification: "The Power
		 * Management Control/Status Register (CSR) controls the power
		 * state of the device.  The normal operating state is D0,
		 * CSR=00h.  The software off state is D3, CSR=03h.  To reset
		 * the controller, place the interface device in D3 then to D0,
		 * this causes a secondary PCI reset which will reset the
		 * controller." */

		pos = pci_find_capability(pdev, PCI_CAP_ID_PM);
		if (pos == 0) {
			dev_err(&pdev->dev,
				"hpsa_reset_controller: "
				"PCI PM not supported\n");
			return -ENODEV;
		}
		dev_info(&pdev->dev, "using PCI PM to reset controller\n");
		/* enter the D3hot power management state */
		pci_read_config_word(pdev, pos + PCI_PM_CTRL, &pmcsr);
		pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
		pmcsr |= PCI_D3hot;
		pci_write_config_word(pdev, pos + PCI_PM_CTRL, pmcsr);

		msleep(500);

		/* enter the D0 power management state */
		pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
		pmcsr |= PCI_D0;
		pci_write_config_word(pdev, pos + PCI_PM_CTRL, pmcsr);

		/*
		 * The P600 requires a small delay when changing states.
		 * Otherwise we may think the board did not reset and we bail.
		 * This for kdump only and is particular to the P600.
		 */
		msleep(500);
	}
	return 0;
}

static void init_driver_version(char *driver_version, int len)
{
	memset(driver_version, 0, len);
	strncpy(driver_version, HPSA " " HPSA_DRIVER_VERSION, len - 1);
}

static int write_driver_ver_to_cfgtable(struct CfgTable __iomem *cfgtable)
{
	char *driver_version;
	int i, size = sizeof(cfgtable->driver_version);

	driver_version = kmalloc(size, GFP_KERNEL);
	if (!driver_version)
		return -ENOMEM;

	init_driver_version(driver_version, size);
	for (i = 0; i < size; i++)
		writeb(driver_version[i], &cfgtable->driver_version[i]);
	kfree(driver_version);
	return 0;
}

static void read_driver_ver_from_cfgtable(struct CfgTable __iomem *cfgtable,
					  unsigned char *driver_ver)
{
	int i;

	for (i = 0; i < sizeof(cfgtable->driver_version); i++)
		driver_ver[i] = readb(&cfgtable->driver_version[i]);
}

static int controller_reset_failed(struct CfgTable __iomem *cfgtable)
{

	char *driver_ver, *old_driver_ver;
	int rc, size = sizeof(cfgtable->driver_version);

	old_driver_ver = kmalloc(2 * size, GFP_KERNEL);
	if (!old_driver_ver)
		return -ENOMEM;
	driver_ver = old_driver_ver + size;

	/* After a reset, the 32 bytes of "driver version" in the cfgtable
	 * should have been changed, otherwise we know the reset failed.
	 */
	init_driver_version(old_driver_ver, size);
	read_driver_ver_from_cfgtable(cfgtable, driver_ver);
	rc = !memcmp(driver_ver, old_driver_ver, size);
	kfree(old_driver_ver);
	return rc;
}
/* This does a hard reset of the controller using PCI power management
 * states or the using the doorbell register.
 */
static int hpsa_kdump_hard_reset_controller(struct pci_dev *pdev)
{
	u64 cfg_offset;
	u32 cfg_base_addr;
	u64 cfg_base_addr_index;
	void __iomem *vaddr;
	unsigned long paddr;
	u32 misc_fw_support;
	int rc;
	struct CfgTable __iomem *cfgtable;
	u32 use_doorbell;
	u32 board_id;
	u16 command_register;

	/* For controllers as old as the P600, this is very nearly
	 * the same thing as
	 *
	 * pci_save_state(pci_dev);
	 * pci_set_power_state(pci_dev, PCI_D3hot);
	 * pci_set_power_state(pci_dev, PCI_D0);
	 * pci_restore_state(pci_dev);
	 *
	 * For controllers newer than the P600, the pci power state
	 * method of resetting doesn't work so we have another way
	 * using the doorbell register.
	 */

	rc = hpsa_lookup_board_id(pdev, &board_id);
	if (rc < 0 || !ctlr_is_resettable(board_id)) {
		dev_warn(&pdev->dev, "Not resetting device.\n");
		return -ENODEV;
	}

	/* if controller is soft- but not hard resettable... */
	if (!ctlr_is_hard_resettable(board_id))
		return -ENOTSUPP; /* try soft reset later. */

	/* Save the PCI command register */
	pci_read_config_word(pdev, 4, &command_register);
	/* Turn the board off.  This is so that later pci_restore_state()
	 * won't turn the board on before the rest of config space is ready.
	 */
	pci_disable_device(pdev);
	pci_save_state(pdev);

	/* find the first memory BAR, so we can find the cfg table */
	rc = hpsa_pci_find_memory_BAR(pdev, &paddr);
	if (rc)
		return rc;
	vaddr = remap_pci_mem(paddr, 0x250);
	if (!vaddr)
		return -ENOMEM;

	/* find cfgtable in order to check if reset via doorbell is supported */
	rc = hpsa_find_cfg_addrs(pdev, vaddr, &cfg_base_addr,
					&cfg_base_addr_index, &cfg_offset);
	if (rc)
		goto unmap_vaddr;
	cfgtable = remap_pci_mem(pci_resource_start(pdev,
		       cfg_base_addr_index) + cfg_offset, sizeof(*cfgtable));
	if (!cfgtable) {
		rc = -ENOMEM;
		goto unmap_vaddr;
	}
	rc = write_driver_ver_to_cfgtable(cfgtable);
	if (rc)
		goto unmap_vaddr;

	/* If reset via doorbell register is supported, use that.
	 * There are two such methods.  Favor the newest method.
	 */
	misc_fw_support = readl(&cfgtable->misc_fw_support);
	use_doorbell = misc_fw_support & MISC_FW_DOORBELL_RESET2;
	if (use_doorbell) {
		use_doorbell = DOORBELL_CTLR_RESET2;
	} else {
		use_doorbell = misc_fw_support & MISC_FW_DOORBELL_RESET;
		if (use_doorbell) {
			dev_warn(&pdev->dev, "Soft reset not supported. "
				"Firmware update is required.\n");
			rc = -ENOTSUPP; /* try soft reset */
			goto unmap_cfgtable;
		}
	}

	rc = hpsa_controller_hard_reset(pdev, vaddr, use_doorbell);
	if (rc)
		goto unmap_cfgtable;

	pci_restore_state(pdev);
	rc = pci_enable_device(pdev);
	if (rc) {
		dev_warn(&pdev->dev, "failed to enable device.\n");
		goto unmap_cfgtable;
	}
	pci_write_config_word(pdev, 4, command_register);

	/* Some devices (notably the HP Smart Array 5i Controller)
	   need a little pause here */
	msleep(HPSA_POST_RESET_PAUSE_MSECS);

	/* Wait for board to become not ready, then ready. */
	dev_info(&pdev->dev, "Waiting for board to reset.\n");
	rc = hpsa_wait_for_board_state(pdev, vaddr, BOARD_NOT_READY);
	if (rc) {
		dev_warn(&pdev->dev,
			"failed waiting for board to reset."
			" Will try soft reset.\n");
		rc = -ENOTSUPP; /* Not expected, but try soft reset later */
		goto unmap_cfgtable;
	}
	rc = hpsa_wait_for_board_state(pdev, vaddr, BOARD_READY);
	if (rc) {
		dev_warn(&pdev->dev,
			"failed waiting for board to become ready "
			"after hard reset\n");
		goto unmap_cfgtable;
	}

	rc = controller_reset_failed(vaddr);
	if (rc < 0)
		goto unmap_cfgtable;
	if (rc) {
		dev_warn(&pdev->dev, "Unable to successfully reset "
			"controller. Will try soft reset.\n");
		rc = -ENOTSUPP;
	} else {
		dev_info(&pdev->dev, "board ready after hard reset.\n");
	}

unmap_cfgtable:
	iounmap(cfgtable);

unmap_vaddr:
	iounmap(vaddr);
	return rc;
}

/*
 *  We cannot read the structure directly, for portability we must use
 *   the io functions.
 *   This is for debug only.
 */
static void print_cfg_table(struct device *dev, struct CfgTable *tb)
{
#ifdef HPSA_DEBUG
	int i;
	char temp_name[17];

	dev_info(dev, "Controller Configuration information\n");
	dev_info(dev, "------------------------------------\n");
	for (i = 0; i < 4; i++)
		temp_name[i] = readb(&(tb->Signature[i]));
	temp_name[4] = '\0';
	dev_info(dev, "   Signature = %s\n", temp_name);
	dev_info(dev, "   Spec Number = %d\n", readl(&(tb->SpecValence)));
	dev_info(dev, "   Transport methods supported = 0x%x\n",
	       readl(&(tb->TransportSupport)));
	dev_info(dev, "   Transport methods active = 0x%x\n",
	       readl(&(tb->TransportActive)));
	dev_info(dev, "   Requested transport Method = 0x%x\n",
	       readl(&(tb->HostWrite.TransportRequest)));
	dev_info(dev, "   Coalesce Interrupt Delay = 0x%x\n",
	       readl(&(tb->HostWrite.CoalIntDelay)));
	dev_info(dev, "   Coalesce Interrupt Count = 0x%x\n",
	       readl(&(tb->HostWrite.CoalIntCount)));
	dev_info(dev, "   Max outstanding commands = 0x%d\n",
	       readl(&(tb->CmdsOutMax)));
	dev_info(dev, "   Bus Types = 0x%x\n", readl(&(tb->BusTypes)));
	for (i = 0; i < 16; i++)
		temp_name[i] = readb(&(tb->ServerName[i]));
	temp_name[16] = '\0';
	dev_info(dev, "   Server Name = %s\n", temp_name);
	dev_info(dev, "   Heartbeat Counter = 0x%x\n\n\n",
		readl(&(tb->HeartBeat)));
#endif				/* HPSA_DEBUG */
}

static int find_PCI_BAR_index(struct pci_dev *pdev, unsigned long pci_bar_addr)
{
	int i, offset, mem_type, bar_type;

	if (pci_bar_addr == PCI_BASE_ADDRESS_0)	/* looking for BAR zero? */
		return 0;
	offset = 0;
	for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
		bar_type = pci_resource_flags(pdev, i) & PCI_BASE_ADDRESS_SPACE;
		if (bar_type == PCI_BASE_ADDRESS_SPACE_IO)
			offset += 4;
		else {
			mem_type = pci_resource_flags(pdev, i) &
			    PCI_BASE_ADDRESS_MEM_TYPE_MASK;
			switch (mem_type) {
			case PCI_BASE_ADDRESS_MEM_TYPE_32:
			case PCI_BASE_ADDRESS_MEM_TYPE_1M:
				offset += 4;	/* 32 bit */
				break;
			case PCI_BASE_ADDRESS_MEM_TYPE_64:
				offset += 8;
				break;
			default:	/* reserved in PCI 2.2 */
				dev_warn(&pdev->dev,
				       "base address is invalid\n");
				return -1;
				break;
			}
		}
		if (offset == pci_bar_addr - PCI_BASE_ADDRESS_0)
			return i + 1;
	}
	return -1;
}

/* If MSI/MSI-X is supported by the kernel we will try to enable it on
 * controllers that are capable. If not, we use IO-APIC mode.
 */

static void hpsa_interrupt_mode(struct ctlr_info *h)
{
#ifdef CONFIG_PCI_MSI
	int err, i;
	struct msix_entry hpsa_msix_entries[MAX_REPLY_QUEUES];

	for (i = 0; i < MAX_REPLY_QUEUES; i++) {
		hpsa_msix_entries[i].vector = 0;
		hpsa_msix_entries[i].entry = i;
	}

	/* Some boards advertise MSI but don't really support it */
	if ((h->board_id == 0x40700E11) || (h->board_id == 0x40800E11) ||
	    (h->board_id == 0x40820E11) || (h->board_id == 0x40830E11))
		goto default_int_mode;
	if (pci_find_capability(h->pdev, PCI_CAP_ID_MSIX)) {
		dev_info(&h->pdev->dev, "MSIX\n");
		err = pci_enable_msix(h->pdev, hpsa_msix_entries,
						MAX_REPLY_QUEUES);
		if (!err) {
			for (i = 0; i < MAX_REPLY_QUEUES; i++)
				h->intr[i] = hpsa_msix_entries[i].vector;
			h->msix_vector = 1;
			return;
		}
		if (err > 0) {
			dev_warn(&h->pdev->dev, "only %d MSI-X vectors "
			       "available\n", err);
			goto default_int_mode;
		} else {
			dev_warn(&h->pdev->dev, "MSI-X init failed %d\n",
			       err);
			goto default_int_mode;
		}
	}
	if (pci_find_capability(h->pdev, PCI_CAP_ID_MSI)) {
		dev_info(&h->pdev->dev, "MSI\n");
		if (!pci_enable_msi(h->pdev))
			h->msi_vector = 1;
		else
			dev_warn(&h->pdev->dev, "MSI init failed\n");
	}
default_int_mode:
#endif				/* CONFIG_PCI_MSI */
	/* if we get here we're going to use the default interrupt mode */
	h->intr[h->intr_mode] = h->pdev->irq;
}

static int hpsa_lookup_board_id(struct pci_dev *pdev, u32 *board_id)
{
	int i;
	u32 subsystem_vendor_id, subsystem_device_id;

	subsystem_vendor_id = pdev->subsystem_vendor;
	subsystem_device_id = pdev->subsystem_device;
	*board_id = ((subsystem_device_id << 16) & 0xffff0000) |
		    subsystem_vendor_id;

	for (i = 0; i < ARRAY_SIZE(products); i++)
		if (*board_id == products[i].board_id)
			return i;

	if ((subsystem_vendor_id != PCI_VENDOR_ID_HP &&
		subsystem_vendor_id != PCI_VENDOR_ID_COMPAQ) ||
		!hpsa_allow_any) {
		dev_warn(&pdev->dev, "unrecognized board ID: "
			"0x%08x, ignoring.\n", *board_id);
			return -ENODEV;
	}
	return ARRAY_SIZE(products) - 1; /* generic unknown smart array */
}

static int hpsa_pci_find_memory_BAR(struct pci_dev *pdev,
				    unsigned long *memory_bar)
{
	int i;

	for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
		if (pci_resource_flags(pdev, i) & IORESOURCE_MEM) {
			/* addressing mode bits already removed */
			*memory_bar = pci_resource_start(pdev, i);
			dev_dbg(&pdev->dev, "memory BAR = %lx\n",
				*memory_bar);
			return 0;
		}
	dev_warn(&pdev->dev, "no memory BAR found\n");
	return -ENODEV;
}

static int hpsa_wait_for_board_state(struct pci_dev *pdev, void __iomem *vaddr,
				     int wait_for_ready)
{
	int i, iterations;
	u32 scratchpad;
	if (wait_for_ready)
		iterations = HPSA_BOARD_READY_ITERATIONS;
	else
		iterations = HPSA_BOARD_NOT_READY_ITERATIONS;

	for (i = 0; i < iterations; i++) {
		scratchpad = readl(vaddr + SA5_SCRATCHPAD_OFFSET);
		if (wait_for_ready) {
			if (scratchpad == HPSA_FIRMWARE_READY)
				return 0;
		} else {
			if (scratchpad != HPSA_FIRMWARE_READY)
				return 0;
		}
		msleep(HPSA_BOARD_READY_POLL_INTERVAL_MSECS);
	}
	dev_warn(&pdev->dev, "board not ready, timed out.\n");
	return -ENODEV;
}

static int hpsa_find_cfg_addrs(struct pci_dev *pdev, void __iomem *vaddr,
			       u32 *cfg_base_addr, u64 *cfg_base_addr_index,
			       u64 *cfg_offset)
{
	*cfg_base_addr = readl(vaddr + SA5_CTCFG_OFFSET);
	*cfg_offset = readl(vaddr + SA5_CTMEM_OFFSET);
	*cfg_base_addr &= (u32) 0x0000ffff;
	*cfg_base_addr_index = find_PCI_BAR_index(pdev, *cfg_base_addr);
	if (*cfg_base_addr_index == -1) {
		dev_warn(&pdev->dev, "cannot find cfg_base_addr_index\n");
		return -ENODEV;
	}
	return 0;
}

static int hpsa_find_cfgtables(struct ctlr_info *h)
{
	u64 cfg_offset;
	u32 cfg_base_addr;
	u64 cfg_base_addr_index;
	u32 trans_offset;
	int rc;

	rc = hpsa_find_cfg_addrs(h->pdev, h->vaddr, &cfg_base_addr,
		&cfg_base_addr_index, &cfg_offset);
	if (rc)
		return rc;
	h->cfgtable = remap_pci_mem(pci_resource_start(h->pdev,
		       cfg_base_addr_index) + cfg_offset, sizeof(*h->cfgtable));
	if (!h->cfgtable)
		return -ENOMEM;
	rc = write_driver_ver_to_cfgtable(h->cfgtable);
	if (rc)
		return rc;
	/* Find performant mode table. */
	trans_offset = readl(&h->cfgtable->TransMethodOffset);
	h->transtable = remap_pci_mem(pci_resource_start(h->pdev,
				cfg_base_addr_index)+cfg_offset+trans_offset,
				sizeof(*h->transtable));
	if (!h->transtable)
		return -ENOMEM;
	return 0;
}

static void hpsa_get_max_perf_mode_cmds(struct ctlr_info *h)
{
	h->max_commands = readl(&(h->cfgtable->MaxPerformantModeCommands));

	/* Limit commands in memory limited kdump scenario. */
	if (reset_devices && h->max_commands > 32)
		h->max_commands = 32;

	if (h->max_commands < 16) {
		dev_warn(&h->pdev->dev, "Controller reports "
			"max supported commands of %d, an obvious lie. "
			"Using 16.  Ensure that firmware is up to date.\n",
			h->max_commands);
		h->max_commands = 16;
	}
}

/* Interrogate the hardware for some limits:
 * max commands, max SG elements without chaining, and with chaining,
 * SG chain block size, etc.
 */
static void hpsa_find_board_params(struct ctlr_info *h)
{
	hpsa_get_max_perf_mode_cmds(h);
	h->nr_cmds = h->max_commands - 4; /* Allow room for some ioctls */
	h->maxsgentries = readl(&(h->cfgtable->MaxScatterGatherElements));
	/*
	 * Limit in-command s/g elements to 32 save dma'able memory.
	 * Howvever spec says if 0, use 31
	 */
	h->max_cmd_sg_entries = 31;
	if (h->maxsgentries > 512) {
		h->max_cmd_sg_entries = 32;
		h->chainsize = h->maxsgentries - h->max_cmd_sg_entries + 1;
		h->maxsgentries--; /* save one for chain pointer */
	} else {
		h->maxsgentries = 31; /* default to traditional values */
		h->chainsize = 0;
	}

	/* Find out what task management functions are supported and cache */
	h->TMFSupportFlags = readl(&(h->cfgtable->TMFSupportFlags));
}

static inline bool hpsa_CISS_signature_present(struct ctlr_info *h)
{
	if (!check_signature(h->cfgtable->Signature, "CISS", 4)) {
		dev_warn(&h->pdev->dev, "not a valid CISS config table\n");
		return false;
	}
	return true;
}

/* Need to enable prefetch in the SCSI core for 6400 in x86 */
static inline void hpsa_enable_scsi_prefetch(struct ctlr_info *h)
{
#ifdef CONFIG_X86
	u32 prefetch;

	prefetch = readl(&(h->cfgtable->SCSI_Prefetch));
	prefetch |= 0x100;
	writel(prefetch, &(h->cfgtable->SCSI_Prefetch));
#endif
}

/* Disable DMA prefetch for the P600.  Otherwise an ASIC bug may result
 * in a prefetch beyond physical memory.
 */
static inline void hpsa_p600_dma_prefetch_quirk(struct ctlr_info *h)
{
	u32 dma_prefetch;

	if (h->board_id != 0x3225103C)
		return;
	dma_prefetch = readl(h->vaddr + I2O_DMA1_CFG);
	dma_prefetch |= 0x8000;
	writel(dma_prefetch, h->vaddr + I2O_DMA1_CFG);
}

static void hpsa_wait_for_mode_change_ack(struct ctlr_info *h)
{
	int i;
	u32 doorbell_value;
	unsigned long flags;

	/* under certain very rare conditions, this can take awhile.
	 * (e.g.: hot replace a failed 144GB drive in a RAID 5 set right
	 * as we enter this code.)
	 */
	for (i = 0; i < MAX_CONFIG_WAIT; i++) {
		spin_lock_irqsave(&h->lock, flags);
		doorbell_value = readl(h->vaddr + SA5_DOORBELL);
		spin_unlock_irqrestore(&h->lock, flags);
		if (!(doorbell_value & CFGTBL_ChangeReq))
			break;
		/* delay and try again */
		usleep_range(10000, 20000);
	}
}

static int hpsa_enter_simple_mode(struct ctlr_info *h)
{
	u32 trans_support;

	trans_support = readl(&(h->cfgtable->TransportSupport));
	if (!(trans_support & SIMPLE_MODE))
		return -ENOTSUPP;

	h->max_commands = readl(&(h->cfgtable->CmdsOutMax));
	/* Update the field, and then ring the doorbell */
	writel(CFGTBL_Trans_Simple, &(h->cfgtable->HostWrite.TransportRequest));
	writel(CFGTBL_ChangeReq, h->vaddr + SA5_DOORBELL);
	hpsa_wait_for_mode_change_ack(h);
	print_cfg_table(&h->pdev->dev, h->cfgtable);
	if (!(readl(&(h->cfgtable->TransportActive)) & CFGTBL_Trans_Simple)) {
		dev_warn(&h->pdev->dev,
			"unable to get board into simple mode\n");
		return -ENODEV;
	}
	h->transMethod = CFGTBL_Trans_Simple;
	return 0;
}

static int hpsa_pci_init(struct ctlr_info *h)
{
	int prod_index, err;

	prod_index = hpsa_lookup_board_id(h->pdev, &h->board_id);
	if (prod_index < 0)
		return -ENODEV;
	h->product_name = products[prod_index].product_name;
	h->access = *(products[prod_index].access);

	pci_disable_link_state(h->pdev, PCIE_LINK_STATE_L0S |
			       PCIE_LINK_STATE_L1 | PCIE_LINK_STATE_CLKPM);

	err = pci_enable_device(h->pdev);
	if (err) {
		dev_warn(&h->pdev->dev, "unable to enable PCI device\n");
		return err;
	}

	/* Enable bus mastering (pci_disable_device may disable this) */
	pci_set_master(h->pdev);

	err = pci_request_regions(h->pdev, HPSA);
	if (err) {
		dev_err(&h->pdev->dev,
			"cannot obtain PCI resources, aborting\n");
		return err;
	}
	hpsa_interrupt_mode(h);
	err = hpsa_pci_find_memory_BAR(h->pdev, &h->paddr);
	if (err)
		goto err_out_free_res;
	h->vaddr = remap_pci_mem(h->paddr, 0x250);
	if (!h->vaddr) {
		err = -ENOMEM;
		goto err_out_free_res;
	}
	err = hpsa_wait_for_board_state(h->pdev, h->vaddr, BOARD_READY);
	if (err)
		goto err_out_free_res;
	err = hpsa_find_cfgtables(h);
	if (err)
		goto err_out_free_res;
	hpsa_find_board_params(h);

	if (!hpsa_CISS_signature_present(h)) {
		err = -ENODEV;
		goto err_out_free_res;
	}
	hpsa_enable_scsi_prefetch(h);
	hpsa_p600_dma_prefetch_quirk(h);
	err = hpsa_enter_simple_mode(h);
	if (err)
		goto err_out_free_res;
	return 0;

err_out_free_res:
	if (h->transtable)
		iounmap(h->transtable);
	if (h->cfgtable)
		iounmap(h->cfgtable);
	if (h->vaddr)
		iounmap(h->vaddr);
	pci_disable_device(h->pdev);
	pci_release_regions(h->pdev);
	return err;
}

static void hpsa_hba_inquiry(struct ctlr_info *h)
{
	int rc;

#define HBA_INQUIRY_BYTE_COUNT 64
	h->hba_inquiry_data = kmalloc(HBA_INQUIRY_BYTE_COUNT, GFP_KERNEL);
	if (!h->hba_inquiry_data)
		return;
	rc = hpsa_scsi_do_inquiry(h, RAID_CTLR_LUNID, 0,
		h->hba_inquiry_data, HBA_INQUIRY_BYTE_COUNT);
	if (rc != 0) {
		kfree(h->hba_inquiry_data);
		h->hba_inquiry_data = NULL;
	}
}

static int hpsa_init_reset_devices(struct pci_dev *pdev)
{
	int rc, i;

	if (!reset_devices)
		return 0;

	/* Reset the controller with a PCI power-cycle or via doorbell */
	rc = hpsa_kdump_hard_reset_controller(pdev);

	/* -ENOTSUPP here means we cannot reset the controller
	 * but it's already (and still) up and running in
	 * "performant mode".  Or, it might be 640x, which can't reset
	 * due to concerns about shared bbwc between 6402/6404 pair.
	 */
	if (rc == -ENOTSUPP)
		return rc; /* just try to do the kdump anyhow. */
	if (rc)
		return -ENODEV;

	/* Now try to get the controller to respond to a no-op */
	dev_warn(&pdev->dev, "Waiting for controller to respond to no-op\n");
	for (i = 0; i < HPSA_POST_RESET_NOOP_RETRIES; i++) {
		if (hpsa_noop(pdev) == 0)
			break;
		else
			dev_warn(&pdev->dev, "no-op failed%s\n",
					(i < 11 ? "; re-trying" : ""));
	}
	return 0;
}

static int hpsa_allocate_cmd_pool(struct ctlr_info *h)
{
	h->cmd_pool_bits = kzalloc(
		DIV_ROUND_UP(h->nr_cmds, BITS_PER_LONG) *
		sizeof(unsigned long), GFP_KERNEL);
	h->cmd_pool = pci_alloc_consistent(h->pdev,
		    h->nr_cmds * sizeof(*h->cmd_pool),
		    &(h->cmd_pool_dhandle));
	h->errinfo_pool = pci_alloc_consistent(h->pdev,
		    h->nr_cmds * sizeof(*h->errinfo_pool),
		    &(h->errinfo_pool_dhandle));
	if ((h->cmd_pool_bits == NULL)
	    || (h->cmd_pool == NULL)
	    || (h->errinfo_pool == NULL)) {
		dev_err(&h->pdev->dev, "out of memory in %s", __func__);
		return -ENOMEM;
	}
	return 0;
}

static void hpsa_free_cmd_pool(struct ctlr_info *h)
{
	kfree(h->cmd_pool_bits);
	if (h->cmd_pool)
		pci_free_consistent(h->pdev,
			    h->nr_cmds * sizeof(struct CommandList),
			    h->cmd_pool, h->cmd_pool_dhandle);
	if (h->errinfo_pool)
		pci_free_consistent(h->pdev,
			    h->nr_cmds * sizeof(struct ErrorInfo),
			    h->errinfo_pool,
			    h->errinfo_pool_dhandle);
}

static int hpsa_request_irq(struct ctlr_info *h,
	irqreturn_t (*msixhandler)(int, void *),
	irqreturn_t (*intxhandler)(int, void *))
{
	int rc, i;

	/*
	 * initialize h->q[x] = x so that interrupt handlers know which
	 * queue to process.
	 */
	for (i = 0; i < MAX_REPLY_QUEUES; i++)
		h->q[i] = (u8) i;

	if (h->intr_mode == PERF_MODE_INT && h->msix_vector) {
		/* If performant mode and MSI-X, use multiple reply queues */
		for (i = 0; i < MAX_REPLY_QUEUES; i++)
			rc = request_irq(h->intr[i], msixhandler,
					0, h->devname,
					&h->q[i]);
	} else {
		/* Use single reply pool */
		if (h->msix_vector || h->msi_vector) {
			rc = request_irq(h->intr[h->intr_mode],
				msixhandler, 0, h->devname,
				&h->q[h->intr_mode]);
		} else {
			rc = request_irq(h->intr[h->intr_mode],
				intxhandler, IRQF_SHARED, h->devname,
				&h->q[h->intr_mode]);
		}
	}
	if (rc) {
		dev_err(&h->pdev->dev, "unable to get irq %d for %s\n",
		       h->intr[h->intr_mode], h->devname);
		return -ENODEV;
	}
	return 0;
}

static int hpsa_kdump_soft_reset(struct ctlr_info *h)
{
	if (hpsa_send_host_reset(h, RAID_CTLR_LUNID,
		HPSA_RESET_TYPE_CONTROLLER)) {
		dev_warn(&h->pdev->dev, "Resetting array controller failed.\n");
		return -EIO;
	}

	dev_info(&h->pdev->dev, "Waiting for board to soft reset.\n");
	if (hpsa_wait_for_board_state(h->pdev, h->vaddr, BOARD_NOT_READY)) {
		dev_warn(&h->pdev->dev, "Soft reset had no effect.\n");
		return -1;
	}

	dev_info(&h->pdev->dev, "Board reset, awaiting READY status.\n");
	if (hpsa_wait_for_board_state(h->pdev, h->vaddr, BOARD_READY)) {
		dev_warn(&h->pdev->dev, "Board failed to become ready "
			"after soft reset.\n");
		return -1;
	}

	return 0;
}

static void free_irqs(struct ctlr_info *h)
{
	int i;

	if (!h->msix_vector || h->intr_mode != PERF_MODE_INT) {
		/* Single reply queue, only one irq to free */
		i = h->intr_mode;
		free_irq(h->intr[i], &h->q[i]);
		return;
	}

	for (i = 0; i < MAX_REPLY_QUEUES; i++)
		free_irq(h->intr[i], &h->q[i]);
}

static void hpsa_free_irqs_and_disable_msix(struct ctlr_info *h)
{
	free_irqs(h);
#ifdef CONFIG_PCI_MSI
	if (h->msix_vector) {
		if (h->pdev->msix_enabled)
			pci_disable_msix(h->pdev);
	} else if (h->msi_vector) {
		if (h->pdev->msi_enabled)
			pci_disable_msi(h->pdev);
	}
#endif /* CONFIG_PCI_MSI */
}

static void hpsa_undo_allocations_after_kdump_soft_reset(struct ctlr_info *h)
{
	hpsa_free_irqs_and_disable_msix(h);
	hpsa_free_sg_chain_blocks(h);
	hpsa_free_cmd_pool(h);
	kfree(h->blockFetchTable);
	pci_free_consistent(h->pdev, h->reply_pool_size,
		h->reply_pool, h->reply_pool_dhandle);
	if (h->vaddr)
		iounmap(h->vaddr);
	if (h->transtable)
		iounmap(h->transtable);
	if (h->cfgtable)
		iounmap(h->cfgtable);
	pci_release_regions(h->pdev);
	kfree(h);
}

static void remove_ctlr_from_lockup_detector_list(struct ctlr_info *h)
{
	assert_spin_locked(&lockup_detector_lock);
	if (!hpsa_lockup_detector)
		return;
	if (h->lockup_detected)
		return; /* already stopped the lockup detector */
	list_del(&h->lockup_list);
}

/* Called when controller lockup detected. */
static void fail_all_cmds_on_list(struct ctlr_info *h, struct list_head *list)
{
	struct CommandList *c = NULL;

	assert_spin_locked(&h->lock);
	/* Mark all outstanding commands as failed and complete them. */
	while (!list_empty(list)) {
		c = list_entry(list->next, struct CommandList, list);
		c->err_info->CommandStatus = CMD_HARDWARE_ERR;
		finish_cmd(c);
	}
}

static void controller_lockup_detected(struct ctlr_info *h)
{
	unsigned long flags;

	assert_spin_locked(&lockup_detector_lock);
	remove_ctlr_from_lockup_detector_list(h);
	h->access.set_intr_mask(h, HPSA_INTR_OFF);
	spin_lock_irqsave(&h->lock, flags);
	h->lockup_detected = readl(h->vaddr + SA5_SCRATCHPAD_OFFSET);
	spin_unlock_irqrestore(&h->lock, flags);
	dev_warn(&h->pdev->dev, "Controller lockup detected: 0x%08x\n",
			h->lockup_detected);
	pci_disable_device(h->pdev);
	spin_lock_irqsave(&h->lock, flags);
	fail_all_cmds_on_list(h, &h->cmpQ);
	fail_all_cmds_on_list(h, &h->reqQ);
	spin_unlock_irqrestore(&h->lock, flags);
}

static void detect_controller_lockup(struct ctlr_info *h)
{
	u64 now;
	u32 heartbeat;
	unsigned long flags;

	assert_spin_locked(&lockup_detector_lock);
	now = get_jiffies_64();
	/* If we've received an interrupt recently, we're ok. */
	if (time_after64(h->last_intr_timestamp +
				(h->heartbeat_sample_interval), now))
		return;

	/*
	 * If we've already checked the heartbeat recently, we're ok.
	 * This could happen if someone sends us a signal. We
	 * otherwise don't care about signals in this thread.
	 */
	if (time_after64(h->last_heartbeat_timestamp +
				(h->heartbeat_sample_interval), now))
		return;

	/* If heartbeat has not changed since we last looked, we're not ok. */
	spin_lock_irqsave(&h->lock, flags);
	heartbeat = readl(&h->cfgtable->HeartBeat);
	spin_unlock_irqrestore(&h->lock, flags);
	if (h->last_heartbeat == heartbeat) {
		controller_lockup_detected(h);
		return;
	}

	/* We're ok. */
	h->last_heartbeat = heartbeat;
	h->last_heartbeat_timestamp = now;
}

static int detect_controller_lockup_thread(void *notused)
{
	struct ctlr_info *h;
	unsigned long flags;

	while (1) {
		struct list_head *this, *tmp;

		schedule_timeout_interruptible(HEARTBEAT_SAMPLE_INTERVAL);
		if (kthread_should_stop())
			break;
		spin_lock_irqsave(&lockup_detector_lock, flags);
		list_for_each_safe(this, tmp, &hpsa_ctlr_list) {
			h = list_entry(this, struct ctlr_info, lockup_list);
			detect_controller_lockup(h);
		}
		spin_unlock_irqrestore(&lockup_detector_lock, flags);
	}
	return 0;
}

static void add_ctlr_to_lockup_detector_list(struct ctlr_info *h)
{
	unsigned long flags;

	h->heartbeat_sample_interval = HEARTBEAT_SAMPLE_INTERVAL;
	spin_lock_irqsave(&lockup_detector_lock, flags);
	list_add_tail(&h->lockup_list, &hpsa_ctlr_list);
	spin_unlock_irqrestore(&lockup_detector_lock, flags);
}

static void start_controller_lockup_detector(struct ctlr_info *h)
{
	/* Start the lockup detector thread if not already started */
	if (!hpsa_lockup_detector) {
		spin_lock_init(&lockup_detector_lock);
		hpsa_lockup_detector =
			kthread_run(detect_controller_lockup_thread,
						NULL, HPSA);
	}
	if (!hpsa_lockup_detector) {
		dev_warn(&h->pdev->dev,
			"Could not start lockup detector thread\n");
		return;
	}
	add_ctlr_to_lockup_detector_list(h);
}

static void stop_controller_lockup_detector(struct ctlr_info *h)
{
	unsigned long flags;

	spin_lock_irqsave(&lockup_detector_lock, flags);
	remove_ctlr_from_lockup_detector_list(h);
	/* If the list of ctlr's to monitor is empty, stop the thread */
	if (list_empty(&hpsa_ctlr_list)) {
		spin_unlock_irqrestore(&lockup_detector_lock, flags);
		kthread_stop(hpsa_lockup_detector);
		spin_lock_irqsave(&lockup_detector_lock, flags);
		hpsa_lockup_detector = NULL;
	}
	spin_unlock_irqrestore(&lockup_detector_lock, flags);
}

static int hpsa_init_one(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int dac, rc;
	struct ctlr_info *h;
	int try_soft_reset = 0;
	unsigned long flags;

	if (number_of_controllers == 0)
		printk(KERN_INFO DRIVER_NAME "\n");

	rc = hpsa_init_reset_devices(pdev);
	if (rc) {
		if (rc != -ENOTSUPP)
			return rc;
		/* If the reset fails in a particular way (it has no way to do
		 * a proper hard reset, so returns -ENOTSUPP) we can try to do
		 * a soft reset once we get the controller configured up to the
		 * point that it can accept a command.
		 */
		try_soft_reset = 1;
		rc = 0;
	}

reinit_after_soft_reset:

	/* Command structures must be aligned on a 32-byte boundary because
	 * the 5 lower bits of the address are used by the hardware. and by
	 * the driver.  See comments in hpsa.h for more info.
	 */
#define COMMANDLIST_ALIGNMENT 32
	BUILD_BUG_ON(sizeof(struct CommandList) % COMMANDLIST_ALIGNMENT);
	h = kzalloc(sizeof(*h), GFP_KERNEL);
	if (!h)
		return -ENOMEM;

	h->pdev = pdev;
	h->intr_mode = hpsa_simple_mode ? SIMPLE_MODE_INT : PERF_MODE_INT;
	INIT_LIST_HEAD(&h->cmpQ);
	INIT_LIST_HEAD(&h->reqQ);
	spin_lock_init(&h->lock);
	spin_lock_init(&h->scan_lock);
	rc = hpsa_pci_init(h);
	if (rc != 0)
		goto clean1;

	sprintf(h->devname, HPSA "%d", number_of_controllers);
	h->ctlr = number_of_controllers;
	number_of_controllers++;

	/* configure PCI DMA stuff */
	rc = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
	if (rc == 0) {
		dac = 1;
	} else {
		rc = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (rc == 0) {
			dac = 0;
		} else {
			dev_err(&pdev->dev, "no suitable DMA available\n");
			goto clean1;
		}
	}

	/* make sure the board interrupts are off */
	h->access.set_intr_mask(h, HPSA_INTR_OFF);

	if (hpsa_request_irq(h, do_hpsa_intr_msi, do_hpsa_intr_intx))
		goto clean2;
	dev_info(&pdev->dev, "%s: <0x%x> at IRQ %d%s using DAC\n",
	       h->devname, pdev->device,
	       h->intr[h->intr_mode], dac ? "" : " not");
	if (hpsa_allocate_cmd_pool(h))
		goto clean4;
	if (hpsa_allocate_sg_chain_blocks(h))
		goto clean4;
	init_waitqueue_head(&h->scan_wait_queue);
	h->scan_finished = 1; /* no scan currently in progress */

	pci_set_drvdata(pdev, h);
	h->ndevices = 0;
	h->scsi_host = NULL;
	spin_lock_init(&h->devlock);
	hpsa_put_ctlr_into_performant_mode(h);

	/* At this point, the controller is ready to take commands.
	 * Now, if reset_devices and the hard reset didn't work, try
	 * the soft reset and see if that works.
	 */
	if (try_soft_reset) {

		/* This is kind of gross.  We may or may not get a completion
		 * from the soft reset command, and if we do, then the value
		 * from the fifo may or may not be valid.  So, we wait 10 secs
		 * after the reset throwing away any completions we get during
		 * that time.  Unregister the interrupt handler and register
		 * fake ones to scoop up any residual completions.
		 */
		spin_lock_irqsave(&h->lock, flags);
		h->access.set_intr_mask(h, HPSA_INTR_OFF);
		spin_unlock_irqrestore(&h->lock, flags);
		free_irqs(h);
		rc = hpsa_request_irq(h, hpsa_msix_discard_completions,
					hpsa_intx_discard_completions);
		if (rc) {
			dev_warn(&h->pdev->dev, "Failed to request_irq after "
				"soft reset.\n");
			goto clean4;
		}

		rc = hpsa_kdump_soft_reset(h);
		if (rc)
			/* Neither hard nor soft reset worked, we're hosed. */
			goto clean4;

		dev_info(&h->pdev->dev, "Board READY.\n");
		dev_info(&h->pdev->dev,
			"Waiting for stale completions to drain.\n");
		h->access.set_intr_mask(h, HPSA_INTR_ON);
		msleep(10000);
		h->access.set_intr_mask(h, HPSA_INTR_OFF);

		rc = controller_reset_failed(h->cfgtable);
		if (rc)
			dev_info(&h->pdev->dev,
				"Soft reset appears to have failed.\n");

		/* since the controller's reset, we have to go back and re-init
		 * everything.  Easiest to just forget what we've done and do it
		 * all over again.
		 */
		hpsa_undo_allocations_after_kdump_soft_reset(h);
		try_soft_reset = 0;
		if (rc)
			/* don't go to clean4, we already unallocated */
			return -ENODEV;

		goto reinit_after_soft_reset;
	}

	/* Turn the interrupts on so we can service requests */
	h->access.set_intr_mask(h, HPSA_INTR_ON);

	hpsa_hba_inquiry(h);
	hpsa_register_scsi(h);	/* hook ourselves into SCSI subsystem */
	start_controller_lockup_detector(h);
	return 0;

clean4:
	hpsa_free_sg_chain_blocks(h);
	hpsa_free_cmd_pool(h);
	free_irqs(h);
clean2:
clean1:
	kfree(h);
	return rc;
}

static void hpsa_flush_cache(struct ctlr_info *h)
{
	char *flush_buf;
	struct CommandList *c;

	flush_buf = kzalloc(4, GFP_KERNEL);
	if (!flush_buf)
		return;

	c = cmd_special_alloc(h);
	if (!c) {
		dev_warn(&h->pdev->dev, "cmd_special_alloc returned NULL!\n");
		goto out_of_memory;
	}
	if (fill_cmd(c, HPSA_CACHE_FLUSH, h, flush_buf, 4, 0,
		RAID_CTLR_LUNID, TYPE_CMD)) {
		goto out;
	}
	hpsa_scsi_do_simple_cmd_with_retry(h, c, PCI_DMA_TODEVICE);
	if (c->err_info->CommandStatus != 0)
out:
		dev_warn(&h->pdev->dev,
			"error flushing cache on controller\n");
	cmd_special_free(h, c);
out_of_memory:
	kfree(flush_buf);
}

static void hpsa_shutdown(struct pci_dev *pdev)
{
	struct ctlr_info *h;

	h = pci_get_drvdata(pdev);
	/* Turn board interrupts off  and send the flush cache command
	 * sendcmd will turn off interrupt, and send the flush...
	 * To write all data in the battery backed cache to disks
	 */
	hpsa_flush_cache(h);
	h->access.set_intr_mask(h, HPSA_INTR_OFF);
	hpsa_free_irqs_and_disable_msix(h);
}

static void hpsa_free_device_info(struct ctlr_info *h)
{
	int i;

	for (i = 0; i < h->ndevices; i++)
		kfree(h->dev[i]);
}

static void hpsa_remove_one(struct pci_dev *pdev)
{
	struct ctlr_info *h;

	if (pci_get_drvdata(pdev) == NULL) {
		dev_err(&pdev->dev, "unable to remove device\n");
		return;
	}
	h = pci_get_drvdata(pdev);
	stop_controller_lockup_detector(h);
	hpsa_unregister_scsi(h);	/* unhook from SCSI subsystem */
	hpsa_shutdown(pdev);
	iounmap(h->vaddr);
	iounmap(h->transtable);
	iounmap(h->cfgtable);
	hpsa_free_device_info(h);
	hpsa_free_sg_chain_blocks(h);
	pci_free_consistent(h->pdev,
		h->nr_cmds * sizeof(struct CommandList),
		h->cmd_pool, h->cmd_pool_dhandle);
	pci_free_consistent(h->pdev,
		h->nr_cmds * sizeof(struct ErrorInfo),
		h->errinfo_pool, h->errinfo_pool_dhandle);
	pci_free_consistent(h->pdev, h->reply_pool_size,
		h->reply_pool, h->reply_pool_dhandle);
	kfree(h->cmd_pool_bits);
	kfree(h->blockFetchTable);
	kfree(h->hba_inquiry_data);
	pci_disable_device(pdev);
	pci_release_regions(pdev);
	pci_set_drvdata(pdev, NULL);
	kfree(h);
}

static int hpsa_suspend(__attribute__((unused)) struct pci_dev *pdev,
	__attribute__((unused)) pm_message_t state)
{
	return -ENOSYS;
}

static int hpsa_resume(__attribute__((unused)) struct pci_dev *pdev)
{
	return -ENOSYS;
}

static struct pci_driver hpsa_pci_driver = {
	.name = HPSA,
	.probe = hpsa_init_one,
	.remove = hpsa_remove_one,
	.id_table = hpsa_pci_device_id,	/* id_table */
	.shutdown = hpsa_shutdown,
	.suspend = hpsa_suspend,
	.resume = hpsa_resume,
};

/* Fill in bucket_map[], given nsgs (the max number of
 * scatter gather elements supported) and bucket[],
 * which is an array of 8 integers.  The bucket[] array
 * contains 8 different DMA transfer sizes (in 16
 * byte increments) which the controller uses to fetch
 * commands.  This function fills in bucket_map[], which
 * maps a given number of scatter gather elements to one of
 * the 8 DMA transfer sizes.  The point of it is to allow the
 * controller to only do as much DMA as needed to fetch the
 * command, with the DMA transfer size encoded in the lower
 * bits of the command address.
 */
static void  calc_bucket_map(int bucket[], int num_buckets,
	int nsgs, int *bucket_map)
{
	int i, j, b, size;

	/* even a command with 0 SGs requires 4 blocks */
#define MINIMUM_TRANSFER_BLOCKS 4
#define NUM_BUCKETS 8
	/* Note, bucket_map must have nsgs+1 entries. */
	for (i = 0; i <= nsgs; i++) {
		/* Compute size of a command with i SG entries */
		size = i + MINIMUM_TRANSFER_BLOCKS;
		b = num_buckets; /* Assume the biggest bucket */
		/* Find the bucket that is just big enough */
		for (j = 0; j < 8; j++) {
			if (bucket[j] >= size) {
				b = j;
				break;
			}
		}
		/* for a command with i SG entries, use bucket b. */
		bucket_map[i] = b;
	}
}

static void hpsa_enter_performant_mode(struct ctlr_info *h, u32 use_short_tags)
{
	int i;
	unsigned long register_value;

	/* This is a bit complicated.  There are 8 registers on
	 * the controller which we write to to tell it 8 different
	 * sizes of commands which there may be.  It's a way of
	 * reducing the DMA done to fetch each command.  Encoded into
	 * each command's tag are 3 bits which communicate to the controller
	 * which of the eight sizes that command fits within.  The size of
	 * each command depends on how many scatter gather entries there are.
	 * Each SG entry requires 16 bytes.  The eight registers are programmed
	 * with the number of 16-byte blocks a command of that size requires.
	 * The smallest command possible requires 5 such 16 byte blocks.
	 * the largest command possible requires SG_ENTRIES_IN_CMD + 4 16-byte
	 * blocks.  Note, this only extends to the SG entries contained
	 * within the command block, and does not extend to chained blocks
	 * of SG elements.   bft[] contains the eight values we write to
	 * the registers.  They are not evenly distributed, but have more
	 * sizes for small commands, and fewer sizes for larger commands.
	 */
	int bft[8] = {5, 6, 8, 10, 12, 20, 28, SG_ENTRIES_IN_CMD + 4};
	BUILD_BUG_ON(28 > SG_ENTRIES_IN_CMD + 4);
	/*  5 = 1 s/g entry or 4k
	 *  6 = 2 s/g entry or 8k
	 *  8 = 4 s/g entry or 16k
	 * 10 = 6 s/g entry or 24k
	 */

	/* Controller spec: zero out this buffer. */
	memset(h->reply_pool, 0, h->reply_pool_size);

	bft[7] = SG_ENTRIES_IN_CMD + 4;
	calc_bucket_map(bft, ARRAY_SIZE(bft),
				SG_ENTRIES_IN_CMD, h->blockFetchTable);
	for (i = 0; i < 8; i++)
		writel(bft[i], &h->transtable->BlockFetch[i]);

	/* size of controller ring buffer */
	writel(h->max_commands, &h->transtable->RepQSize);
	writel(h->nreply_queues, &h->transtable->RepQCount);
	writel(0, &h->transtable->RepQCtrAddrLow32);
	writel(0, &h->transtable->RepQCtrAddrHigh32);

	for (i = 0; i < h->nreply_queues; i++) {
		writel(0, &h->transtable->RepQAddr[i].upper);
		writel(h->reply_pool_dhandle +
			(h->max_commands * sizeof(u64) * i),
			&h->transtable->RepQAddr[i].lower);
	}

	writel(CFGTBL_Trans_Performant | use_short_tags |
		CFGTBL_Trans_enable_directed_msix,
		&(h->cfgtable->HostWrite.TransportRequest));
	writel(CFGTBL_ChangeReq, h->vaddr + SA5_DOORBELL);
	hpsa_wait_for_mode_change_ack(h);
	register_value = readl(&(h->cfgtable->TransportActive));
	if (!(register_value & CFGTBL_Trans_Performant)) {
		dev_warn(&h->pdev->dev, "unable to get board into"
					" performant mode\n");
		return;
	}
	/* Change the access methods to the performant access methods */
	h->access = SA5_performant_access;
	h->transMethod = CFGTBL_Trans_Performant;
}

static void hpsa_put_ctlr_into_performant_mode(struct ctlr_info *h)
{
	u32 trans_support;
	int i;

	if (hpsa_simple_mode)
		return;

	trans_support = readl(&(h->cfgtable->TransportSupport));
	if (!(trans_support & PERFORMANT_MODE))
		return;

	h->nreply_queues = h->msix_vector ? MAX_REPLY_QUEUES : 1;
	hpsa_get_max_perf_mode_cmds(h);
	/* Performant mode ring buffer and supporting data structures */
	h->reply_pool_size = h->max_commands * sizeof(u64) * h->nreply_queues;
	h->reply_pool = pci_alloc_consistent(h->pdev, h->reply_pool_size,
				&(h->reply_pool_dhandle));

	for (i = 0; i < h->nreply_queues; i++) {
		h->reply_queue[i].head = &h->reply_pool[h->max_commands * i];
		h->reply_queue[i].size = h->max_commands;
		h->reply_queue[i].wraparound = 1;  /* spec: init to 1 */
		h->reply_queue[i].current_entry = 0;
	}

	/* Need a block fetch table for performant mode */
	h->blockFetchTable = kmalloc(((SG_ENTRIES_IN_CMD + 1) *
				sizeof(u32)), GFP_KERNEL);

	if ((h->reply_pool == NULL)
		|| (h->blockFetchTable == NULL))
		goto clean_up;

	hpsa_enter_performant_mode(h,
		trans_support & CFGTBL_Trans_use_short_tags);

	return;

clean_up:
	if (h->reply_pool)
		pci_free_consistent(h->pdev, h->reply_pool_size,
			h->reply_pool, h->reply_pool_dhandle);
	kfree(h->blockFetchTable);
}

/*
 *  This is it.  Register the PCI driver information for the cards we control
 *  the OS will call our registered routines when it finds one of our cards.
 */
static int __init hpsa_init(void)
{
	return pci_register_driver(&hpsa_pci_driver);
}

static void __exit hpsa_cleanup(void)
{
	pci_unregister_driver(&hpsa_pci_driver);
}

module_init(hpsa_init);
module_exit(hpsa_cleanup);
