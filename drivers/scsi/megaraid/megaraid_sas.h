/*
 *  Fikus MegaRAID driver for SAS based RAID controllers
 *
 *  Copyright (c) 2003-2012  LSI Corporation.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  FILE: megaraid_sas.h
 *
 *  Authors: LSI Corporation
 *
 *  Send feedback to: <megaraidfikus@lsi.com>
 *
 *  Mail to: LSI Corporation, 1621 Barber Lane, Milpitas, CA 95035
 *     ATTN: Fikusraid
 */

#ifndef LSI_MEGARAID_SAS_H
#define LSI_MEGARAID_SAS_H

/*
 * MegaRAID SAS Driver meta data
 */
#define MEGASAS_VERSION				"06.700.06.00-rc1"
#define MEGASAS_RELDATE				"Aug. 31, 2013"
#define MEGASAS_EXT_VERSION			"Sat. Aug. 31 17:00:00 PDT 2013"

/*
 * Device IDs
 */
#define	PCI_DEVICE_ID_LSI_SAS1078R		0x0060
#define	PCI_DEVICE_ID_LSI_SAS1078DE		0x007C
#define	PCI_DEVICE_ID_LSI_VERDE_ZCR		0x0413
#define	PCI_DEVICE_ID_LSI_SAS1078GEN2		0x0078
#define	PCI_DEVICE_ID_LSI_SAS0079GEN2		0x0079
#define	PCI_DEVICE_ID_LSI_SAS0073SKINNY		0x0073
#define	PCI_DEVICE_ID_LSI_SAS0071SKINNY		0x0071
#define	PCI_DEVICE_ID_LSI_FUSION		0x005b
#define PCI_DEVICE_ID_LSI_INVADER		0x005d
#define PCI_DEVICE_ID_LSI_FURY			0x005f

/*
 * Intel HBA SSDIDs
 */
#define MEGARAID_INTEL_RS3DC080_SSDID		0x9360
#define MEGARAID_INTEL_RS3DC040_SSDID		0x9362
#define MEGARAID_INTEL_RS3SC008_SSDID		0x9380
#define MEGARAID_INTEL_RS3MC044_SSDID		0x9381
#define MEGARAID_INTEL_RS3WC080_SSDID		0x9341
#define MEGARAID_INTEL_RS3WC040_SSDID		0x9343

/*
 * Intel HBA branding
 */
#define MEGARAID_INTEL_RS3DC080_BRANDING	\
	"Intel(R) RAID Controller RS3DC080"
#define MEGARAID_INTEL_RS3DC040_BRANDING	\
	"Intel(R) RAID Controller RS3DC040"
#define MEGARAID_INTEL_RS3SC008_BRANDING	\
	"Intel(R) RAID Controller RS3SC008"
#define MEGARAID_INTEL_RS3MC044_BRANDING	\
	"Intel(R) RAID Controller RS3MC044"
#define MEGARAID_INTEL_RS3WC080_BRANDING	\
	"Intel(R) RAID Controller RS3WC080"
#define MEGARAID_INTEL_RS3WC040_BRANDING	\
	"Intel(R) RAID Controller RS3WC040"

/*
 * =====================================
 * MegaRAID SAS MFI firmware definitions
 * =====================================
 */

/*
 * MFI stands for  MegaRAID SAS FW Interface. This is just a moniker for 
 * protocol between the software and firmware. Commands are issued using
 * "message frames"
 */

/*
 * FW posts its state in upper 4 bits of outbound_msg_0 register
 */
#define MFI_STATE_MASK				0xF0000000
#define MFI_STATE_UNDEFINED			0x00000000
#define MFI_STATE_BB_INIT			0x10000000
#define MFI_STATE_FW_INIT			0x40000000
#define MFI_STATE_WAIT_HANDSHAKE		0x60000000
#define MFI_STATE_FW_INIT_2			0x70000000
#define MFI_STATE_DEVICE_SCAN			0x80000000
#define MFI_STATE_BOOT_MESSAGE_PENDING		0x90000000
#define MFI_STATE_FLUSH_CACHE			0xA0000000
#define MFI_STATE_READY				0xB0000000
#define MFI_STATE_OPERATIONAL			0xC0000000
#define MFI_STATE_FAULT				0xF0000000
#define MFI_RESET_REQUIRED			0x00000001
#define MFI_RESET_ADAPTER			0x00000002
#define MEGAMFI_FRAME_SIZE			64

/*
 * During FW init, clear pending cmds & reset state using inbound_msg_0
 *
 * ABORT	: Abort all pending cmds
 * READY	: Move from OPERATIONAL to READY state; discard queue info
 * MFIMODE	: Discard (possible) low MFA posted in 64-bit mode (??)
 * CLR_HANDSHAKE: FW is waiting for HANDSHAKE from BIOS or Driver
 * HOTPLUG	: Resume from Hotplug
 * MFI_STOP_ADP	: Send signal to FW to stop processing
 */
#define WRITE_SEQUENCE_OFFSET		(0x0000000FC) /* I20 */
#define HOST_DIAGNOSTIC_OFFSET		(0x000000F8)  /* I20 */
#define DIAG_WRITE_ENABLE			(0x00000080)
#define DIAG_RESET_ADAPTER			(0x00000004)

#define MFI_ADP_RESET				0x00000040
#define MFI_INIT_ABORT				0x00000001
#define MFI_INIT_READY				0x00000002
#define MFI_INIT_MFIMODE			0x00000004
#define MFI_INIT_CLEAR_HANDSHAKE		0x00000008
#define MFI_INIT_HOTPLUG			0x00000010
#define MFI_STOP_ADP				0x00000020
#define MFI_RESET_FLAGS				MFI_INIT_READY| \
						MFI_INIT_MFIMODE| \
						MFI_INIT_ABORT

/*
 * MFI frame flags
 */
#define MFI_FRAME_POST_IN_REPLY_QUEUE		0x0000
#define MFI_FRAME_DONT_POST_IN_REPLY_QUEUE	0x0001
#define MFI_FRAME_SGL32				0x0000
#define MFI_FRAME_SGL64				0x0002
#define MFI_FRAME_SENSE32			0x0000
#define MFI_FRAME_SENSE64			0x0004
#define MFI_FRAME_DIR_NONE			0x0000
#define MFI_FRAME_DIR_WRITE			0x0008
#define MFI_FRAME_DIR_READ			0x0010
#define MFI_FRAME_DIR_BOTH			0x0018
#define MFI_FRAME_IEEE                          0x0020

/*
 * Definition for cmd_status
 */
#define MFI_CMD_STATUS_POLL_MODE		0xFF

/*
 * MFI command opcodes
 */
#define MFI_CMD_INIT				0x00
#define MFI_CMD_LD_READ				0x01
#define MFI_CMD_LD_WRITE			0x02
#define MFI_CMD_LD_SCSI_IO			0x03
#define MFI_CMD_PD_SCSI_IO			0x04
#define MFI_CMD_DCMD				0x05
#define MFI_CMD_ABORT				0x06
#define MFI_CMD_SMP				0x07
#define MFI_CMD_STP				0x08
#define MFI_CMD_INVALID				0xff

#define MR_DCMD_CTRL_GET_INFO			0x01010000
#define MR_DCMD_LD_GET_LIST			0x03010000
#define MR_DCMD_LD_LIST_QUERY			0x03010100

#define MR_DCMD_CTRL_CACHE_FLUSH		0x01101000
#define MR_FLUSH_CTRL_CACHE			0x01
#define MR_FLUSH_DISK_CACHE			0x02

#define MR_DCMD_CTRL_SHUTDOWN			0x01050000
#define MR_DCMD_HIBERNATE_SHUTDOWN		0x01060000
#define MR_ENABLE_DRIVE_SPINDOWN		0x01

#define MR_DCMD_CTRL_EVENT_GET_INFO		0x01040100
#define MR_DCMD_CTRL_EVENT_GET			0x01040300
#define MR_DCMD_CTRL_EVENT_WAIT			0x01040500
#define MR_DCMD_LD_GET_PROPERTIES		0x03030000

#define MR_DCMD_CLUSTER				0x08000000
#define MR_DCMD_CLUSTER_RESET_ALL		0x08010100
#define MR_DCMD_CLUSTER_RESET_LD		0x08010200
#define MR_DCMD_PD_LIST_QUERY                   0x02010100

/*
 * Global functions
 */
extern u8 MR_ValidateMapInfo(struct megasas_instance *instance);


/*
 * MFI command completion codes
 */
enum MFI_STAT {
	MFI_STAT_OK = 0x00,
	MFI_STAT_INVALID_CMD = 0x01,
	MFI_STAT_INVALID_DCMD = 0x02,
	MFI_STAT_INVALID_PARAMETER = 0x03,
	MFI_STAT_INVALID_SEQUENCE_NUMBER = 0x04,
	MFI_STAT_ABORT_NOT_POSSIBLE = 0x05,
	MFI_STAT_APP_HOST_CODE_NOT_FOUND = 0x06,
	MFI_STAT_APP_IN_USE = 0x07,
	MFI_STAT_APP_NOT_INITIALIZED = 0x08,
	MFI_STAT_ARRAY_INDEX_INVALID = 0x09,
	MFI_STAT_ARRAY_ROW_NOT_EMPTY = 0x0a,
	MFI_STAT_CONFIG_RESOURCE_CONFLICT = 0x0b,
	MFI_STAT_DEVICE_NOT_FOUND = 0x0c,
	MFI_STAT_DRIVE_TOO_SMALL = 0x0d,
	MFI_STAT_FLASH_ALLOC_FAIL = 0x0e,
	MFI_STAT_FLASH_BUSY = 0x0f,
	MFI_STAT_FLASH_ERROR = 0x10,
	MFI_STAT_FLASH_IMAGE_BAD = 0x11,
	MFI_STAT_FLASH_IMAGE_INCOMPLETE = 0x12,
	MFI_STAT_FLASH_NOT_OPEN = 0x13,
	MFI_STAT_FLASH_NOT_STARTED = 0x14,
	MFI_STAT_FLUSH_FAILED = 0x15,
	MFI_STAT_HOST_CODE_NOT_FOUNT = 0x16,
	MFI_STAT_LD_CC_IN_PROGRESS = 0x17,
	MFI_STAT_LD_INIT_IN_PROGRESS = 0x18,
	MFI_STAT_LD_LBA_OUT_OF_RANGE = 0x19,
	MFI_STAT_LD_MAX_CONFIGURED = 0x1a,
	MFI_STAT_LD_NOT_OPTIMAL = 0x1b,
	MFI_STAT_LD_RBLD_IN_PROGRESS = 0x1c,
	MFI_STAT_LD_RECON_IN_PROGRESS = 0x1d,
	MFI_STAT_LD_WRONG_RAID_LEVEL = 0x1e,
	MFI_STAT_MAX_SPARES_EXCEEDED = 0x1f,
	MFI_STAT_MEMORY_NOT_AVAILABLE = 0x20,
	MFI_STAT_MFC_HW_ERROR = 0x21,
	MFI_STAT_NO_HW_PRESENT = 0x22,
	MFI_STAT_NOT_FOUND = 0x23,
	MFI_STAT_NOT_IN_ENCL = 0x24,
	MFI_STAT_PD_CLEAR_IN_PROGRESS = 0x25,
	MFI_STAT_PD_TYPE_WRONG = 0x26,
	MFI_STAT_PR_DISABLED = 0x27,
	MFI_STAT_ROW_INDEX_INVALID = 0x28,
	MFI_STAT_SAS_CONFIG_INVALID_ACTION = 0x29,
	MFI_STAT_SAS_CONFIG_INVALID_DATA = 0x2a,
	MFI_STAT_SAS_CONFIG_INVALID_PAGE = 0x2b,
	MFI_STAT_SAS_CONFIG_INVALID_TYPE = 0x2c,
	MFI_STAT_SCSI_DONE_WITH_ERROR = 0x2d,
	MFI_STAT_SCSI_IO_FAILED = 0x2e,
	MFI_STAT_SCSI_RESERVATION_CONFLICT = 0x2f,
	MFI_STAT_SHUTDOWN_FAILED = 0x30,
	MFI_STAT_TIME_NOT_SET = 0x31,
	MFI_STAT_WRONG_STATE = 0x32,
	MFI_STAT_LD_OFFLINE = 0x33,
	MFI_STAT_PEER_NOTIFICATION_REJECTED = 0x34,
	MFI_STAT_PEER_NOTIFICATION_FAILED = 0x35,
	MFI_STAT_RESERVATION_IN_PROGRESS = 0x36,
	MFI_STAT_I2C_ERRORS_DETECTED = 0x37,
	MFI_STAT_PCI_ERRORS_DETECTED = 0x38,
	MFI_STAT_CONFIG_SEQ_MISMATCH = 0x67,

	MFI_STAT_INVALID_STATUS = 0xFF
};

/*
 * Number of mailbox bytes in DCMD message frame
 */
#define MFI_MBOX_SIZE				12

enum MR_EVT_CLASS {

	MR_EVT_CLASS_DEBUG = -2,
	MR_EVT_CLASS_PROGRESS = -1,
	MR_EVT_CLASS_INFO = 0,
	MR_EVT_CLASS_WARNING = 1,
	MR_EVT_CLASS_CRITICAL = 2,
	MR_EVT_CLASS_FATAL = 3,
	MR_EVT_CLASS_DEAD = 4,

};

enum MR_EVT_LOCALE {

	MR_EVT_LOCALE_LD = 0x0001,
	MR_EVT_LOCALE_PD = 0x0002,
	MR_EVT_LOCALE_ENCL = 0x0004,
	MR_EVT_LOCALE_BBU = 0x0008,
	MR_EVT_LOCALE_SAS = 0x0010,
	MR_EVT_LOCALE_CTRL = 0x0020,
	MR_EVT_LOCALE_CONFIG = 0x0040,
	MR_EVT_LOCALE_CLUSTER = 0x0080,
	MR_EVT_LOCALE_ALL = 0xffff,

};

enum MR_EVT_ARGS {

	MR_EVT_ARGS_NONE,
	MR_EVT_ARGS_CDB_SENSE,
	MR_EVT_ARGS_LD,
	MR_EVT_ARGS_LD_COUNT,
	MR_EVT_ARGS_LD_LBA,
	MR_EVT_ARGS_LD_OWNER,
	MR_EVT_ARGS_LD_LBA_PD_LBA,
	MR_EVT_ARGS_LD_PROG,
	MR_EVT_ARGS_LD_STATE,
	MR_EVT_ARGS_LD_STRIP,
	MR_EVT_ARGS_PD,
	MR_EVT_ARGS_PD_ERR,
	MR_EVT_ARGS_PD_LBA,
	MR_EVT_ARGS_PD_LBA_LD,
	MR_EVT_ARGS_PD_PROG,
	MR_EVT_ARGS_PD_STATE,
	MR_EVT_ARGS_PCI,
	MR_EVT_ARGS_RATE,
	MR_EVT_ARGS_STR,
	MR_EVT_ARGS_TIME,
	MR_EVT_ARGS_ECC,
	MR_EVT_ARGS_LD_PROP,
	MR_EVT_ARGS_PD_SPARE,
	MR_EVT_ARGS_PD_INDEX,
	MR_EVT_ARGS_DIAG_PASS,
	MR_EVT_ARGS_DIAG_FAIL,
	MR_EVT_ARGS_PD_LBA_LBA,
	MR_EVT_ARGS_PORT_PHY,
	MR_EVT_ARGS_PD_MISSING,
	MR_EVT_ARGS_PD_ADDRESS,
	MR_EVT_ARGS_BITMAP,
	MR_EVT_ARGS_CONNECTOR,
	MR_EVT_ARGS_PD_PD,
	MR_EVT_ARGS_PD_FRU,
	MR_EVT_ARGS_PD_PATHINFO,
	MR_EVT_ARGS_PD_POWER_STATE,
	MR_EVT_ARGS_GENERIC,
};

/*
 * define constants for device list query options
 */
enum MR_PD_QUERY_TYPE {
	MR_PD_QUERY_TYPE_ALL                = 0,
	MR_PD_QUERY_TYPE_STATE              = 1,
	MR_PD_QUERY_TYPE_POWER_STATE        = 2,
	MR_PD_QUERY_TYPE_MEDIA_TYPE         = 3,
	MR_PD_QUERY_TYPE_SPEED              = 4,
	MR_PD_QUERY_TYPE_EXPOSED_TO_HOST    = 5,
};

enum MR_LD_QUERY_TYPE {
	MR_LD_QUERY_TYPE_ALL	         = 0,
	MR_LD_QUERY_TYPE_EXPOSED_TO_HOST = 1,
	MR_LD_QUERY_TYPE_USED_TGT_IDS    = 2,
	MR_LD_QUERY_TYPE_CLUSTER_ACCESS  = 3,
	MR_LD_QUERY_TYPE_CLUSTER_LOCALE  = 4,
};


#define MR_EVT_CFG_CLEARED                              0x0004
#define MR_EVT_LD_STATE_CHANGE                          0x0051
#define MR_EVT_PD_INSERTED                              0x005b
#define MR_EVT_PD_REMOVED                               0x0070
#define MR_EVT_LD_CREATED                               0x008a
#define MR_EVT_LD_DELETED                               0x008b
#define MR_EVT_FOREIGN_CFG_IMPORTED                     0x00db
#define MR_EVT_LD_OFFLINE                               0x00fc
#define MR_EVT_CTRL_HOST_BUS_SCAN_REQUESTED             0x0152
#define MAX_LOGICAL_DRIVES				64

enum MR_PD_STATE {
	MR_PD_STATE_UNCONFIGURED_GOOD   = 0x00,
	MR_PD_STATE_UNCONFIGURED_BAD    = 0x01,
	MR_PD_STATE_HOT_SPARE           = 0x02,
	MR_PD_STATE_OFFLINE             = 0x10,
	MR_PD_STATE_FAILED              = 0x11,
	MR_PD_STATE_REBUILD             = 0x14,
	MR_PD_STATE_ONLINE              = 0x18,
	MR_PD_STATE_COPYBACK            = 0x20,
	MR_PD_STATE_SYSTEM              = 0x40,
 };


 /*
 * defines the physical drive address structure
 */
struct MR_PD_ADDRESS {
	u16     deviceId;
	u16     enclDeviceId;

	union {
		struct {
			u8  enclIndex;
			u8  slotNumber;
		} mrPdAddress;
		struct {
			u8  enclPosition;
			u8  enclConnectorIndex;
		} mrEnclAddress;
	};
	u8      scsiDevType;
	union {
		u8      connectedPortBitmap;
		u8      connectedPortNumbers;
	};
	u64     sasAddr[2];
} __packed;

/*
 * defines the physical drive list structure
 */
struct MR_PD_LIST {
	u32             size;
	u32             count;
	struct MR_PD_ADDRESS   addr[1];
} __packed;

struct megasas_pd_list {
	u16             tid;
	u8             driveType;
	u8             driveState;
} __packed;

 /*
 * defines the logical drive reference structure
 */
union  MR_LD_REF {
	struct {
		u8      targetId;
		u8      reserved;
		u16     seqNum;
	};
	u32     ref;
} __packed;

/*
 * defines the logical drive list structure
 */
struct MR_LD_LIST {
	u32     ldCount;
	u32     reserved;
	struct {
		union MR_LD_REF   ref;
		u8          state;
		u8          reserved[3];
		u64         size;
	} ldList[MAX_LOGICAL_DRIVES];
} __packed;

struct MR_LD_TARGETID_LIST {
	u32	size;
	u32	count;
	u8	pad[3];
	u8	targetId[MAX_LOGICAL_DRIVES];
};


/*
 * SAS controller properties
 */
struct megasas_ctrl_prop {

	u16 seq_num;
	u16 pred_fail_poll_interval;
	u16 intr_throttle_count;
	u16 intr_throttle_timeouts;
	u8 rebuild_rate;
	u8 patrol_read_rate;
	u8 bgi_rate;
	u8 cc_rate;
	u8 recon_rate;
	u8 cache_flush_interval;
	u8 spinup_drv_count;
	u8 spinup_delay;
	u8 cluster_enable;
	u8 coercion_mode;
	u8 alarm_enable;
	u8 disable_auto_rebuild;
	u8 disable_battery_warn;
	u8 ecc_bucket_size;
	u16 ecc_bucket_leak_rate;
	u8 restore_hotspare_on_insertion;
	u8 expose_encl_devices;
	u8 maintainPdFailHistory;
	u8 disallowHostRequestReordering;
	u8 abortCCOnError;
	u8 loadBalanceMode;
	u8 disableAutoDetectBackplane;

	u8 snapVDSpace;

	/*
	* Add properties that can be controlled by
	* a bit in the following structure.
	*/
	struct {
#if   defined(__BIG_ENDIAN_BITFIELD)
		u32     reserved:18;
		u32     enableJBOD:1;
		u32     disableSpinDownHS:1;
		u32     allowBootWithPinnedCache:1;
		u32     disableOnlineCtrlReset:1;
		u32     enableSecretKeyControl:1;
		u32     autoEnhancedImport:1;
		u32     enableSpinDownUnconfigured:1;
		u32     SSDPatrolReadEnabled:1;
		u32     SSDSMARTerEnabled:1;
		u32     disableNCQ:1;
		u32     useFdeOnly:1;
		u32     prCorrectUnconfiguredAreas:1;
		u32     SMARTerEnabled:1;
		u32     copyBackDisabled:1;
#else
		u32     copyBackDisabled:1;
		u32     SMARTerEnabled:1;
		u32     prCorrectUnconfiguredAreas:1;
		u32     useFdeOnly:1;
		u32     disableNCQ:1;
		u32     SSDSMARTerEnabled:1;
		u32     SSDPatrolReadEnabled:1;
		u32     enableSpinDownUnconfigured:1;
		u32     autoEnhancedImport:1;
		u32     enableSecretKeyControl:1;
		u32     disableOnlineCtrlReset:1;
		u32     allowBootWithPinnedCache:1;
		u32     disableSpinDownHS:1;
		u32     enableJBOD:1;
		u32     reserved:18;
#endif
	} OnOffProperties;
	u8 autoSnapVDSpace;
	u8 viewSpace;
	u16 spinDownTime;
	u8  reserved[24];
} __packed;

/*
 * SAS controller information
 */
struct megasas_ctrl_info {

	/*
	 * PCI device information
	 */
	struct {

		u16 vendor_id;
		u16 device_id;
		u16 sub_vendor_id;
		u16 sub_device_id;
		u8 reserved[24];

	} __attribute__ ((packed)) pci;

	/*
	 * Host interface information
	 */
	struct {

		u8 PCIX:1;
		u8 PCIE:1;
		u8 iSCSI:1;
		u8 SAS_3G:1;
		u8 reserved_0:4;
		u8 reserved_1[6];
		u8 port_count;
		u64 port_addr[8];

	} __attribute__ ((packed)) host_interface;

	/*
	 * Device (backend) interface information
	 */
	struct {

		u8 SPI:1;
		u8 SAS_3G:1;
		u8 SATA_1_5G:1;
		u8 SATA_3G:1;
		u8 reserved_0:4;
		u8 reserved_1[6];
		u8 port_count;
		u64 port_addr[8];

	} __attribute__ ((packed)) device_interface;

	/*
	 * List of components residing in flash. All str are null terminated
	 */
	u32 image_check_word;
	u32 image_component_count;

	struct {

		char name[8];
		char version[32];
		char build_date[16];
		char built_time[16];

	} __attribute__ ((packed)) image_component[8];

	/*
	 * List of flash components that have been flashed on the card, but
	 * are not in use, pending reset of the adapter. This list will be
	 * empty if a flash operation has not occurred. All stings are null
	 * terminated
	 */
	u32 pending_image_component_count;

	struct {

		char name[8];
		char version[32];
		char build_date[16];
		char build_time[16];

	} __attribute__ ((packed)) pending_image_component[8];

	u8 max_arms;
	u8 max_spans;
	u8 max_arrays;
	u8 max_lds;

	char product_name[80];
	char serial_no[32];

	/*
	 * Other physical/controller/operation information. Indicates the
	 * presence of the hardware
	 */
	struct {

		u32 bbu:1;
		u32 alarm:1;
		u32 nvram:1;
		u32 uart:1;
		u32 reserved:28;

	} __attribute__ ((packed)) hw_present;

	u32 current_fw_time;

	/*
	 * Maximum data transfer sizes
	 */
	u16 max_concurrent_cmds;
	u16 max_sge_count;
	u32 max_request_size;

	/*
	 * Logical and physical device counts
	 */
	u16 ld_present_count;
	u16 ld_degraded_count;
	u16 ld_offline_count;

	u16 pd_present_count;
	u16 pd_disk_present_count;
	u16 pd_disk_pred_failure_count;
	u16 pd_disk_failed_count;

	/*
	 * Memory size information
	 */
	u16 nvram_size;
	u16 memory_size;
	u16 flash_size;

	/*
	 * Error counters
	 */
	u16 mem_correctable_error_count;
	u16 mem_uncorrectable_error_count;

	/*
	 * Cluster information
	 */
	u8 cluster_permitted;
	u8 cluster_active;

	/*
	 * Additional max data transfer sizes
	 */
	u16 max_strips_per_io;

	/*
	 * Controller capabilities structures
	 */
	struct {

		u32 raid_level_0:1;
		u32 raid_level_1:1;
		u32 raid_level_5:1;
		u32 raid_level_1E:1;
		u32 raid_level_6:1;
		u32 reserved:27;

	} __attribute__ ((packed)) raid_levels;

	struct {

		u32 rbld_rate:1;
		u32 cc_rate:1;
		u32 bgi_rate:1;
		u32 recon_rate:1;
		u32 patrol_rate:1;
		u32 alarm_control:1;
		u32 cluster_supported:1;
		u32 bbu:1;
		u32 spanning_allowed:1;
		u32 dedicated_hotspares:1;
		u32 revertible_hotspares:1;
		u32 foreign_config_import:1;
		u32 self_diagnostic:1;
		u32 mixed_redundancy_arr:1;
		u32 global_hot_spares:1;
		u32 reserved:17;

	} __attribute__ ((packed)) adapter_operations;

	struct {

		u32 read_policy:1;
		u32 write_policy:1;
		u32 io_policy:1;
		u32 access_policy:1;
		u32 disk_cache_policy:1;
		u32 reserved:27;

	} __attribute__ ((packed)) ld_operations;

	struct {

		u8 min;
		u8 max;
		u8 reserved[2];

	} __attribute__ ((packed)) stripe_sz_ops;

	struct {

		u32 force_online:1;
		u32 force_offline:1;
		u32 force_rebuild:1;
		u32 reserved:29;

	} __attribute__ ((packed)) pd_operations;

	struct {

		u32 ctrl_supports_sas:1;
		u32 ctrl_supports_sata:1;
		u32 allow_mix_in_encl:1;
		u32 allow_mix_in_ld:1;
		u32 allow_sata_in_cluster:1;
		u32 reserved:27;

	} __attribute__ ((packed)) pd_mix_support;

	/*
	 * Define ECC single-bit-error bucket information
	 */
	u8 ecc_bucket_count;
	u8 reserved_2[11];

	/*
	 * Include the controller properties (changeable items)
	 */
	struct megasas_ctrl_prop properties;

	/*
	 * Define FW pkg version (set in envt v'bles on OEM basis)
	 */
	char package_version[0x60];


	/*
	* If adapterOperations.supportMoreThan8Phys is set,
	* and deviceInterface.portCount is greater than 8,
	* SAS Addrs for first 8 ports shall be populated in
	* deviceInterface.portAddr, and the rest shall be
	* populated in deviceInterfacePortAddr2.
	*/
	u64         deviceInterfacePortAddr2[8]; /*6a0h */
	u8          reserved3[128];              /*6e0h */

	struct {                                /*760h */
		u16 minPdRaidLevel_0:4;
		u16 maxPdRaidLevel_0:12;

		u16 minPdRaidLevel_1:4;
		u16 maxPdRaidLevel_1:12;

		u16 minPdRaidLevel_5:4;
		u16 maxPdRaidLevel_5:12;

		u16 minPdRaidLevel_1E:4;
		u16 maxPdRaidLevel_1E:12;

		u16 minPdRaidLevel_6:4;
		u16 maxPdRaidLevel_6:12;

		u16 minPdRaidLevel_10:4;
		u16 maxPdRaidLevel_10:12;

		u16 minPdRaidLevel_50:4;
		u16 maxPdRaidLevel_50:12;

		u16 minPdRaidLevel_60:4;
		u16 maxPdRaidLevel_60:12;

		u16 minPdRaidLevel_1E_RLQ0:4;
		u16 maxPdRaidLevel_1E_RLQ0:12;

		u16 minPdRaidLevel_1E0_RLQ0:4;
		u16 maxPdRaidLevel_1E0_RLQ0:12;

		u16 reserved[6];
	} pdsForRaidLevels;

	u16 maxPds;                             /*780h */
	u16 maxDedHSPs;                         /*782h */
	u16 maxGlobalHSPs;                      /*784h */
	u16 ddfSize;                            /*786h */
	u8  maxLdsPerArray;                     /*788h */
	u8  partitionsInDDF;                    /*789h */
	u8  lockKeyBinding;                     /*78ah */
	u8  maxPITsPerLd;                       /*78bh */
	u8  maxViewsPerLd;                      /*78ch */
	u8  maxTargetId;                        /*78dh */
	u16 maxBvlVdSize;                       /*78eh */

	u16 maxConfigurableSSCSize;             /*790h */
	u16 currentSSCsize;                     /*792h */

	char    expanderFwVersion[12];          /*794h */

	u16 PFKTrialTimeRemaining;              /*7A0h */

	u16 cacheMemorySize;                    /*7A2h */

	struct {                                /*7A4h */
#if   defined(__BIG_ENDIAN_BITFIELD)
		u32     reserved:11;
		u32     supportUnevenSpans:1;
		u32     dedicatedHotSparesLimited:1;
		u32     headlessMode:1;
		u32     supportEmulatedDrives:1;
		u32     supportResetNow:1;
		u32     realTimeScheduler:1;
		u32     supportSSDPatrolRead:1;
		u32     supportPerfTuning:1;
		u32     disableOnlinePFKChange:1;
		u32     supportJBOD:1;
		u32     supportBootTimePFKChange:1;
		u32     supportSetLinkSpeed:1;
		u32     supportEmergencySpares:1;
		u32     supportSuspendResumeBGops:1;
		u32     blockSSDWriteCacheChange:1;
		u32     supportShieldState:1;
		u32     supportLdBBMInfo:1;
		u32     supportLdPIType3:1;
		u32     supportLdPIType2:1;
		u32     supportLdPIType1:1;
		u32     supportPIcontroller:1;
#else
		u32     supportPIcontroller:1;
		u32     supportLdPIType1:1;
		u32     supportLdPIType2:1;
		u32     supportLdPIType3:1;
		u32     supportLdBBMInfo:1;
		u32     supportShieldState:1;
		u32     blockSSDWriteCacheChange:1;
		u32     supportSuspendResumeBGops:1;
		u32     supportEmergencySpares:1;
		u32     supportSetLinkSpeed:1;
		u32     supportBootTimePFKChange:1;
		u32     supportJBOD:1;
		u32     disableOnlinePFKChange:1;
		u32     supportPerfTuning:1;
		u32     supportSSDPatrolRead:1;
		u32     realTimeScheduler:1;

		u32     supportResetNow:1;
		u32     supportEmulatedDrives:1;
		u32     headlessMode:1;
		u32     dedicatedHotSparesLimited:1;


		u32     supportUnevenSpans:1;
		u32     reserved:11;
#endif
	} adapterOperations2;

	u8  driverVersion[32];                  /*7A8h */
	u8  maxDAPdCountSpinup60;               /*7C8h */
	u8  temperatureROC;                     /*7C9h */
	u8  temperatureCtrl;                    /*7CAh */
	u8  reserved4;                          /*7CBh */
	u16 maxConfigurablePds;                 /*7CCh */


	u8  reserved5[2];                       /*0x7CDh */

	/*
	* HA cluster information
	*/
	struct {
		u32     peerIsPresent:1;
		u32     peerIsIncompatible:1;
		u32     hwIncompatible:1;
		u32     fwVersionMismatch:1;
		u32     ctrlPropIncompatible:1;
		u32     premiumFeatureMismatch:1;
		u32     reserved:26;
	} cluster;

	char clusterId[16];                     /*7D4h */

	u8          pad[0x800-0x7E4];           /*7E4 */
} __packed;

/*
 * ===============================
 * MegaRAID SAS driver definitions
 * ===============================
 */
#define MEGASAS_MAX_PD_CHANNELS			2
#define MEGASAS_MAX_LD_CHANNELS			1
#define MEGASAS_MAX_CHANNELS			(MEGASAS_MAX_PD_CHANNELS + \
						MEGASAS_MAX_LD_CHANNELS)
#define MEGASAS_MAX_DEV_PER_CHANNEL		128
#define MEGASAS_DEFAULT_INIT_ID			-1
#define MEGASAS_MAX_LUN				8
#define MEGASAS_MAX_LD				64
#define MEGASAS_DEFAULT_CMD_PER_LUN		256
#define MEGASAS_MAX_PD                          (MEGASAS_MAX_PD_CHANNELS * \
						MEGASAS_MAX_DEV_PER_CHANNEL)
#define MEGASAS_MAX_LD_IDS			(MEGASAS_MAX_LD_CHANNELS * \
						MEGASAS_MAX_DEV_PER_CHANNEL)

#define MEGASAS_MAX_SECTORS                    (2*1024)
#define MEGASAS_MAX_SECTORS_IEEE		(2*128)
#define MEGASAS_DBG_LVL				1

#define MEGASAS_FW_BUSY				1

/* Frame Type */
#define IO_FRAME				0
#define PTHRU_FRAME				1

/*
 * When SCSI mid-layer calls driver's reset routine, driver waits for
 * MEGASAS_RESET_WAIT_TIME seconds for all outstanding IO to complete. Note
 * that the driver cannot _actually_ abort or reset pending commands. While
 * it is waiting for the commands to complete, it prints a diagnostic message
 * every MEGASAS_RESET_NOTICE_INTERVAL seconds
 */
#define MEGASAS_RESET_WAIT_TIME			180
#define MEGASAS_INTERNAL_CMD_WAIT_TIME		180
#define	MEGASAS_RESET_NOTICE_INTERVAL		5
#define MEGASAS_IOCTL_CMD			0
#define MEGASAS_DEFAULT_CMD_TIMEOUT		90
#define MEGASAS_THROTTLE_QUEUE_DEPTH		16

/*
 * FW reports the maximum of number of commands that it can accept (maximum
 * commands that can be outstanding) at any time. The driver must report a
 * lower number to the mid layer because it can issue a few internal commands
 * itself (E.g, AEN, abort cmd, IOCTLs etc). The number of commands it needs
 * is shown below
 */
#define MEGASAS_INT_CMDS			32
#define MEGASAS_SKINNY_INT_CMDS			5

#define MEGASAS_MAX_MSIX_QUEUES			128
/*
 * FW can accept both 32 and 64 bit SGLs. We want to allocate 32/64 bit
 * SGLs based on the size of dma_addr_t
 */
#define IS_DMA64				(sizeof(dma_addr_t) == 8)

#define MFI_XSCALE_OMR0_CHANGE_INTERRUPT		0x00000001

#define MFI_INTR_FLAG_REPLY_MESSAGE			0x00000001
#define MFI_INTR_FLAG_FIRMWARE_STATE_CHANGE		0x00000002
#define MFI_G2_OUTBOUND_DOORBELL_CHANGE_INTERRUPT	0x00000004

#define MFI_OB_INTR_STATUS_MASK			0x00000002
#define MFI_POLL_TIMEOUT_SECS			60

#define MFI_REPLY_1078_MESSAGE_INTERRUPT	0x80000000
#define MFI_REPLY_GEN2_MESSAGE_INTERRUPT	0x00000001
#define MFI_GEN2_ENABLE_INTERRUPT_MASK		(0x00000001 | 0x00000004)
#define MFI_REPLY_SKINNY_MESSAGE_INTERRUPT	0x40000000
#define MFI_SKINNY_ENABLE_INTERRUPT_MASK	(0x00000001)

#define MFI_1068_PCSR_OFFSET			0x84
#define MFI_1068_FW_HANDSHAKE_OFFSET		0x64
#define MFI_1068_FW_READY			0xDDDD0000

#define MR_MAX_REPLY_QUEUES_OFFSET              0X0000001F
#define MR_MAX_REPLY_QUEUES_EXT_OFFSET          0X003FC000
#define MR_MAX_REPLY_QUEUES_EXT_OFFSET_SHIFT    14
#define MR_MAX_MSIX_REG_ARRAY                   16
/*
* register set for both 1068 and 1078 controllers
* structure extended for 1078 registers
*/
 
struct megasas_register_set {
	u32	doorbell;                       /*0000h*/
	u32	fusion_seq_offset;		/*0004h*/
	u32	fusion_host_diag;		/*0008h*/
	u32	reserved_01;			/*000Ch*/

	u32 	inbound_msg_0;			/*0010h*/
	u32 	inbound_msg_1;			/*0014h*/
	u32 	outbound_msg_0;			/*0018h*/
	u32 	outbound_msg_1;			/*001Ch*/

	u32 	inbound_doorbell;		/*0020h*/
	u32 	inbound_intr_status;		/*0024h*/
	u32 	inbound_intr_mask;		/*0028h*/

	u32 	outbound_doorbell;		/*002Ch*/
	u32 	outbound_intr_status;		/*0030h*/
	u32 	outbound_intr_mask;		/*0034h*/

	u32 	reserved_1[2];			/*0038h*/

	u32 	inbound_queue_port;		/*0040h*/
	u32 	outbound_queue_port;		/*0044h*/

	u32	reserved_2[9];			/*0048h*/
	u32	reply_post_host_index;		/*006Ch*/
	u32	reserved_2_2[12];		/*0070h*/

	u32 	outbound_doorbell_clear;	/*00A0h*/

	u32 	reserved_3[3];			/*00A4h*/

	u32 	outbound_scratch_pad ;		/*00B0h*/
	u32	outbound_scratch_pad_2;         /*00B4h*/

	u32	reserved_4[2];			/*00B8h*/

	u32 	inbound_low_queue_port ;	/*00C0h*/

	u32 	inbound_high_queue_port ;	/*00C4h*/

	u32 	reserved_5;			/*00C8h*/
	u32	res_6[11];			/*CCh*/
	u32	host_diag;
	u32	seq_offset;
	u32 	index_registers[807];		/*00CCh*/
} __attribute__ ((packed));

struct megasas_sge32 {

	u32 phys_addr;
	u32 length;

} __attribute__ ((packed));

struct megasas_sge64 {

	u64 phys_addr;
	u32 length;

} __attribute__ ((packed));

struct megasas_sge_skinny {
	u64 phys_addr;
	u32 length;
	u32 flag;
} __packed;

union megasas_sgl {

	struct megasas_sge32 sge32[1];
	struct megasas_sge64 sge64[1];
	struct megasas_sge_skinny sge_skinny[1];

} __attribute__ ((packed));

struct megasas_header {

	u8 cmd;			/*00h */
	u8 sense_len;		/*01h */
	u8 cmd_status;		/*02h */
	u8 scsi_status;		/*03h */

	u8 target_id;		/*04h */
	u8 lun;			/*05h */
	u8 cdb_len;		/*06h */
	u8 sge_count;		/*07h */

	u32 context;		/*08h */
	u32 pad_0;		/*0Ch */

	u16 flags;		/*10h */
	u16 timeout;		/*12h */
	u32 data_xferlen;	/*14h */

} __attribute__ ((packed));

union megasas_sgl_frame {

	struct megasas_sge32 sge32[8];
	struct megasas_sge64 sge64[5];

} __attribute__ ((packed));

typedef union _MFI_CAPABILITIES {
	struct {
#if   defined(__BIG_ENDIAN_BITFIELD)
		u32     reserved:30;
		u32     support_additional_msix:1;
		u32     support_fp_remote_lun:1;
#else
		u32     support_fp_remote_lun:1;
		u32     support_additional_msix:1;
		u32     reserved:30;
#endif
	} mfi_capabilities;
	u32     reg;
} MFI_CAPABILITIES;

struct megasas_init_frame {

	u8 cmd;			/*00h */
	u8 reserved_0;		/*01h */
	u8 cmd_status;		/*02h */

	u8 reserved_1;		/*03h */
	MFI_CAPABILITIES driver_operations; /*04h*/

	u32 context;		/*08h */
	u32 pad_0;		/*0Ch */

	u16 flags;		/*10h */
	u16 reserved_3;		/*12h */
	u32 data_xfer_len;	/*14h */

	u32 queue_info_new_phys_addr_lo;	/*18h */
	u32 queue_info_new_phys_addr_hi;	/*1Ch */
	u32 queue_info_old_phys_addr_lo;	/*20h */
	u32 queue_info_old_phys_addr_hi;	/*24h */

	u32 reserved_4[6];	/*28h */

} __attribute__ ((packed));

struct megasas_init_queue_info {

	u32 init_flags;		/*00h */
	u32 reply_queue_entries;	/*04h */

	u32 reply_queue_start_phys_addr_lo;	/*08h */
	u32 reply_queue_start_phys_addr_hi;	/*0Ch */
	u32 producer_index_phys_addr_lo;	/*10h */
	u32 producer_index_phys_addr_hi;	/*14h */
	u32 consumer_index_phys_addr_lo;	/*18h */
	u32 consumer_index_phys_addr_hi;	/*1Ch */

} __attribute__ ((packed));

struct megasas_io_frame {

	u8 cmd;			/*00h */
	u8 sense_len;		/*01h */
	u8 cmd_status;		/*02h */
	u8 scsi_status;		/*03h */

	u8 target_id;		/*04h */
	u8 access_byte;		/*05h */
	u8 reserved_0;		/*06h */
	u8 sge_count;		/*07h */

	u32 context;		/*08h */
	u32 pad_0;		/*0Ch */

	u16 flags;		/*10h */
	u16 timeout;		/*12h */
	u32 lba_count;		/*14h */

	u32 sense_buf_phys_addr_lo;	/*18h */
	u32 sense_buf_phys_addr_hi;	/*1Ch */

	u32 start_lba_lo;	/*20h */
	u32 start_lba_hi;	/*24h */

	union megasas_sgl sgl;	/*28h */

} __attribute__ ((packed));

struct megasas_pthru_frame {

	u8 cmd;			/*00h */
	u8 sense_len;		/*01h */
	u8 cmd_status;		/*02h */
	u8 scsi_status;		/*03h */

	u8 target_id;		/*04h */
	u8 lun;			/*05h */
	u8 cdb_len;		/*06h */
	u8 sge_count;		/*07h */

	u32 context;		/*08h */
	u32 pad_0;		/*0Ch */

	u16 flags;		/*10h */
	u16 timeout;		/*12h */
	u32 data_xfer_len;	/*14h */

	u32 sense_buf_phys_addr_lo;	/*18h */
	u32 sense_buf_phys_addr_hi;	/*1Ch */

	u8 cdb[16];		/*20h */
	union megasas_sgl sgl;	/*30h */

} __attribute__ ((packed));

struct megasas_dcmd_frame {

	u8 cmd;			/*00h */
	u8 reserved_0;		/*01h */
	u8 cmd_status;		/*02h */
	u8 reserved_1[4];	/*03h */
	u8 sge_count;		/*07h */

	u32 context;		/*08h */
	u32 pad_0;		/*0Ch */

	u16 flags;		/*10h */
	u16 timeout;		/*12h */

	u32 data_xfer_len;	/*14h */
	u32 opcode;		/*18h */

	union {			/*1Ch */
		u8 b[12];
		u16 s[6];
		u32 w[3];
	} mbox;

	union megasas_sgl sgl;	/*28h */

} __attribute__ ((packed));

struct megasas_abort_frame {

	u8 cmd;			/*00h */
	u8 reserved_0;		/*01h */
	u8 cmd_status;		/*02h */

	u8 reserved_1;		/*03h */
	u32 reserved_2;		/*04h */

	u32 context;		/*08h */
	u32 pad_0;		/*0Ch */

	u16 flags;		/*10h */
	u16 reserved_3;		/*12h */
	u32 reserved_4;		/*14h */

	u32 abort_context;	/*18h */
	u32 pad_1;		/*1Ch */

	u32 abort_mfi_phys_addr_lo;	/*20h */
	u32 abort_mfi_phys_addr_hi;	/*24h */

	u32 reserved_5[6];	/*28h */

} __attribute__ ((packed));

struct megasas_smp_frame {

	u8 cmd;			/*00h */
	u8 reserved_1;		/*01h */
	u8 cmd_status;		/*02h */
	u8 connection_status;	/*03h */

	u8 reserved_2[3];	/*04h */
	u8 sge_count;		/*07h */

	u32 context;		/*08h */
	u32 pad_0;		/*0Ch */

	u16 flags;		/*10h */
	u16 timeout;		/*12h */

	u32 data_xfer_len;	/*14h */
	u64 sas_addr;		/*18h */

	union {
		struct megasas_sge32 sge32[2];	/* [0]: resp [1]: req */
		struct megasas_sge64 sge64[2];	/* [0]: resp [1]: req */
	} sgl;

} __attribute__ ((packed));

struct megasas_stp_frame {

	u8 cmd;			/*00h */
	u8 reserved_1;		/*01h */
	u8 cmd_status;		/*02h */
	u8 reserved_2;		/*03h */

	u8 target_id;		/*04h */
	u8 reserved_3[2];	/*05h */
	u8 sge_count;		/*07h */

	u32 context;		/*08h */
	u32 pad_0;		/*0Ch */

	u16 flags;		/*10h */
	u16 timeout;		/*12h */

	u32 data_xfer_len;	/*14h */

	u16 fis[10];		/*18h */
	u32 stp_flags;

	union {
		struct megasas_sge32 sge32[2];	/* [0]: resp [1]: data */
		struct megasas_sge64 sge64[2];	/* [0]: resp [1]: data */
	} sgl;

} __attribute__ ((packed));

union megasas_frame {

	struct megasas_header hdr;
	struct megasas_init_frame init;
	struct megasas_io_frame io;
	struct megasas_pthru_frame pthru;
	struct megasas_dcmd_frame dcmd;
	struct megasas_abort_frame abort;
	struct megasas_smp_frame smp;
	struct megasas_stp_frame stp;

	u8 raw_bytes[64];
};

struct megasas_cmd;

union megasas_evt_class_locale {

	struct {
		u16 locale;
		u8 reserved;
		s8 class;
	} __attribute__ ((packed)) members;

	u32 word;

} __attribute__ ((packed));

struct megasas_evt_log_info {
	u32 newest_seq_num;
	u32 oldest_seq_num;
	u32 clear_seq_num;
	u32 shutdown_seq_num;
	u32 boot_seq_num;

} __attribute__ ((packed));

struct megasas_progress {

	u16 progress;
	u16 elapsed_seconds;

} __attribute__ ((packed));

struct megasas_evtarg_ld {

	u16 target_id;
	u8 ld_index;
	u8 reserved;

} __attribute__ ((packed));

struct megasas_evtarg_pd {
	u16 device_id;
	u8 encl_index;
	u8 slot_number;

} __attribute__ ((packed));

struct megasas_evt_detail {

	u32 seq_num;
	u32 time_stamp;
	u32 code;
	union megasas_evt_class_locale cl;
	u8 arg_type;
	u8 reserved1[15];

	union {
		struct {
			struct megasas_evtarg_pd pd;
			u8 cdb_length;
			u8 sense_length;
			u8 reserved[2];
			u8 cdb[16];
			u8 sense[64];
		} __attribute__ ((packed)) cdbSense;

		struct megasas_evtarg_ld ld;

		struct {
			struct megasas_evtarg_ld ld;
			u64 count;
		} __attribute__ ((packed)) ld_count;

		struct {
			u64 lba;
			struct megasas_evtarg_ld ld;
		} __attribute__ ((packed)) ld_lba;

		struct {
			struct megasas_evtarg_ld ld;
			u32 prevOwner;
			u32 newOwner;
		} __attribute__ ((packed)) ld_owner;

		struct {
			u64 ld_lba;
			u64 pd_lba;
			struct megasas_evtarg_ld ld;
			struct megasas_evtarg_pd pd;
		} __attribute__ ((packed)) ld_lba_pd_lba;

		struct {
			struct megasas_evtarg_ld ld;
			struct megasas_progress prog;
		} __attribute__ ((packed)) ld_prog;

		struct {
			struct megasas_evtarg_ld ld;
			u32 prev_state;
			u32 new_state;
		} __attribute__ ((packed)) ld_state;

		struct {
			u64 strip;
			struct megasas_evtarg_ld ld;
		} __attribute__ ((packed)) ld_strip;

		struct megasas_evtarg_pd pd;

		struct {
			struct megasas_evtarg_pd pd;
			u32 err;
		} __attribute__ ((packed)) pd_err;

		struct {
			u64 lba;
			struct megasas_evtarg_pd pd;
		} __attribute__ ((packed)) pd_lba;

		struct {
			u64 lba;
			struct megasas_evtarg_pd pd;
			struct megasas_evtarg_ld ld;
		} __attribute__ ((packed)) pd_lba_ld;

		struct {
			struct megasas_evtarg_pd pd;
			struct megasas_progress prog;
		} __attribute__ ((packed)) pd_prog;

		struct {
			struct megasas_evtarg_pd pd;
			u32 prevState;
			u32 newState;
		} __attribute__ ((packed)) pd_state;

		struct {
			u16 vendorId;
			u16 deviceId;
			u16 subVendorId;
			u16 subDeviceId;
		} __attribute__ ((packed)) pci;

		u32 rate;
		char str[96];

		struct {
			u32 rtc;
			u32 elapsedSeconds;
		} __attribute__ ((packed)) time;

		struct {
			u32 ecar;
			u32 elog;
			char str[64];
		} __attribute__ ((packed)) ecc;

		u8 b[96];
		u16 s[48];
		u32 w[24];
		u64 d[12];
	} args;

	char description[128];

} __attribute__ ((packed));

struct megasas_aen_event {
	struct delayed_work hotplug_work;
	struct megasas_instance *instance;
};

struct megasas_irq_context {
	struct megasas_instance *instance;
	u32 MSIxIndex;
};

struct megasas_instance {

	u32 *producer;
	dma_addr_t producer_h;
	u32 *consumer;
	dma_addr_t consumer_h;

	u32 *reply_queue;
	dma_addr_t reply_queue_h;

	unsigned long base_addr;
	struct megasas_register_set __iomem *reg_set;
	u32 *reply_post_host_index_addr[MR_MAX_MSIX_REG_ARRAY];
	struct megasas_pd_list          pd_list[MEGASAS_MAX_PD];
	u8     ld_ids[MEGASAS_MAX_LD_IDS];
	s8 init_id;

	u16 max_num_sge;
	u16 max_fw_cmds;
	/* For Fusion its num IOCTL cmds, for others MFI based its
	   max_fw_cmds */
	u16 max_mfi_cmds;
	u32 max_sectors_per_req;
	struct megasas_aen_event *ev;

	struct megasas_cmd **cmd_list;
	struct list_head cmd_pool;
	/* used to sync fire the cmd to fw */
	spinlock_t cmd_pool_lock;
	/* used to sync fire the cmd to fw */
	spinlock_t hba_lock;
	/* used to synch producer, consumer ptrs in dpc */
	spinlock_t completion_lock;
	struct dma_pool *frame_dma_pool;
	struct dma_pool *sense_dma_pool;

	struct megasas_evt_detail *evt_detail;
	dma_addr_t evt_detail_h;
	struct megasas_cmd *aen_cmd;
	struct mutex aen_mutex;
	struct semaphore ioctl_sem;

	struct Scsi_Host *host;

	wait_queue_head_t int_cmd_wait_q;
	wait_queue_head_t abort_cmd_wait_q;

	struct pci_dev *pdev;
	u32 unique_id;
	u32 fw_support_ieee;

	atomic_t fw_outstanding;
	atomic_t fw_reset_no_pci_access;

	struct megasas_instance_template *instancet;
	struct tasklet_struct isr_tasklet;
	struct work_struct work_init;

	u8 flag;
	u8 unload;
	u8 flag_ieee;
	u8 issuepend_done;
	u8 disableOnlineCtrlReset;
	u8 UnevenSpanSupport;
	u8 adprecovery;
	unsigned long last_time;
	u32 mfiStatus;
	u32 last_seq_num;

	struct list_head internal_reset_pending_q;

	/* Ptr to hba specific information */
	void *ctrl_context;
	unsigned int msix_vectors;
	struct msix_entry msixentry[MEGASAS_MAX_MSIX_QUEUES];
	struct megasas_irq_context irq_context[MEGASAS_MAX_MSIX_QUEUES];
	u64 map_id;
	struct megasas_cmd *map_update_cmd;
	unsigned long bar;
	long reset_flags;
	struct mutex reset_mutex;
	int throttlequeuedepth;
	u8 mask_interrupts;
	u8 is_imr;
};

enum {
	MEGASAS_HBA_OPERATIONAL			= 0,
	MEGASAS_ADPRESET_SM_INFAULT		= 1,
	MEGASAS_ADPRESET_SM_FW_RESET_SUCCESS	= 2,
	MEGASAS_ADPRESET_SM_OPERATIONAL		= 3,
	MEGASAS_HW_CRITICAL_ERROR		= 4,
	MEGASAS_ADPRESET_INPROG_SIGN		= 0xDEADDEAD,
};

struct megasas_instance_template {
	void (*fire_cmd)(struct megasas_instance *, dma_addr_t, \
		u32, struct megasas_register_set __iomem *);

	void (*enable_intr)(struct megasas_instance *);
	void (*disable_intr)(struct megasas_instance *);

	int (*clear_intr)(struct megasas_register_set __iomem *);

	u32 (*read_fw_status_reg)(struct megasas_register_set __iomem *);
	int (*adp_reset)(struct megasas_instance *, \
		struct megasas_register_set __iomem *);
	int (*check_reset)(struct megasas_instance *, \
		struct megasas_register_set __iomem *);
	irqreturn_t (*service_isr)(int irq, void *devp);
	void (*tasklet)(unsigned long);
	u32 (*init_adapter)(struct megasas_instance *);
	u32 (*build_and_issue_cmd) (struct megasas_instance *,
				    struct scsi_cmnd *);
	void (*issue_dcmd) (struct megasas_instance *instance,
			    struct megasas_cmd *cmd);
};

#define MEGASAS_IS_LOGICAL(scp)						\
	(scp->device->channel < MEGASAS_MAX_PD_CHANNELS) ? 0 : 1

#define MEGASAS_DEV_INDEX(inst, scp)					\
	((scp->device->channel % 2) * MEGASAS_MAX_DEV_PER_CHANNEL) + 	\
	scp->device->id

struct megasas_cmd {

	union megasas_frame *frame;
	dma_addr_t frame_phys_addr;
	u8 *sense;
	dma_addr_t sense_phys_addr;

	u32 index;
	u8 sync_cmd;
	u8 cmd_status;
	u8 abort_aen;
	u8 retry_for_fw_reset;


	struct list_head list;
	struct scsi_cmnd *scmd;
	struct megasas_instance *instance;
	union {
		struct {
			u16 smid;
			u16 resvd;
		} context;
		u32 frame_count;
	};
};

#define MAX_MGMT_ADAPTERS		1024
#define MAX_IOCTL_SGE			16

struct megasas_iocpacket {

	u16 host_no;
	u16 __pad1;
	u32 sgl_off;
	u32 sge_count;
	u32 sense_off;
	u32 sense_len;
	union {
		u8 raw[128];
		struct megasas_header hdr;
	} frame;

	struct iovec sgl[MAX_IOCTL_SGE];

} __attribute__ ((packed));

struct megasas_aen {
	u16 host_no;
	u16 __pad1;
	u32 seq_num;
	u32 class_locale_word;
} __attribute__ ((packed));

#ifdef CONFIG_COMPAT
struct compat_megasas_iocpacket {
	u16 host_no;
	u16 __pad1;
	u32 sgl_off;
	u32 sge_count;
	u32 sense_off;
	u32 sense_len;
	union {
		u8 raw[128];
		struct megasas_header hdr;
	} frame;
	struct compat_iovec sgl[MAX_IOCTL_SGE];
} __attribute__ ((packed));

#define MEGASAS_IOC_FIRMWARE32	_IOWR('M', 1, struct compat_megasas_iocpacket)
#endif

#define MEGASAS_IOC_FIRMWARE	_IOWR('M', 1, struct megasas_iocpacket)
#define MEGASAS_IOC_GET_AEN	_IOW('M', 3, struct megasas_aen)

struct megasas_mgmt_info {

	u16 count;
	struct megasas_instance *instance[MAX_MGMT_ADAPTERS];
	int max_index;
};

u8
MR_BuildRaidContext(struct megasas_instance *instance,
		    struct IO_REQUEST_INFO *io_info,
		    struct RAID_CONTEXT *pRAID_Context,
		    struct MR_FW_RAID_MAP_ALL *map, u8 **raidLUN);
u16 MR_TargetIdToLdGet(u32 ldTgtId, struct MR_FW_RAID_MAP_ALL *map);
struct MR_LD_RAID *MR_LdRaidGet(u32 ld, struct MR_FW_RAID_MAP_ALL *map);
u16 MR_ArPdGet(u32 ar, u32 arm, struct MR_FW_RAID_MAP_ALL *map);
u16 MR_LdSpanArrayGet(u32 ld, u32 span, struct MR_FW_RAID_MAP_ALL *map);
u16 MR_PdDevHandleGet(u32 pd, struct MR_FW_RAID_MAP_ALL *map);
u16 MR_GetLDTgtId(u32 ld, struct MR_FW_RAID_MAP_ALL *map);

#endif				/*LSI_MEGARAID_SAS_H */
