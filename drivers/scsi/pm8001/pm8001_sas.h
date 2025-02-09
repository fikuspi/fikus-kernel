/*
 * PMC-Sierra PM8001/8081/8088/8089 SAS/SATA based host adapters driver
 *
 * Copyright (c) 2008-2009 USI Co., Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 */

#ifndef _PM8001_SAS_H_
#define _PM8001_SAS_H_

#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/spinlock.h>
#include <fikus/delay.h>
#include <fikus/types.h>
#include <fikus/ctype.h>
#include <fikus/dma-mapping.h>
#include <fikus/pci.h>
#include <fikus/interrupt.h>
#include <fikus/workqueue.h>
#include <scsi/libsas.h>
#include <scsi/scsi_tcq.h>
#include <scsi/sas_ata.h>
#include <fikus/atomic.h>
#include "pm8001_defs.h"

#define DRV_NAME		"pm80xx"
#define DRV_VERSION		"0.1.37"
#define PM8001_FAIL_LOGGING	0x01 /* Error message logging */
#define PM8001_INIT_LOGGING	0x02 /* driver init logging */
#define PM8001_DISC_LOGGING	0x04 /* discovery layer logging */
#define PM8001_IO_LOGGING	0x08 /* I/O path logging */
#define PM8001_EH_LOGGING	0x10 /* libsas EH function logging*/
#define PM8001_IOCTL_LOGGING	0x20 /* IOCTL message logging */
#define PM8001_MSG_LOGGING	0x40 /* misc message logging */
#define pm8001_printk(format, arg...)	printk(KERN_INFO "pm80xx %s %d:" \
			format, __func__, __LINE__, ## arg)
#define PM8001_CHECK_LOGGING(HBA, LEVEL, CMD)	\
do {						\
	if (unlikely(HBA->logging_level & LEVEL))	\
		do {					\
			CMD;				\
		} while (0);				\
} while (0);

#define PM8001_EH_DBG(HBA, CMD)			\
	PM8001_CHECK_LOGGING(HBA, PM8001_EH_LOGGING, CMD)

#define PM8001_INIT_DBG(HBA, CMD)		\
	PM8001_CHECK_LOGGING(HBA, PM8001_INIT_LOGGING, CMD)

#define PM8001_DISC_DBG(HBA, CMD)		\
	PM8001_CHECK_LOGGING(HBA, PM8001_DISC_LOGGING, CMD)

#define PM8001_IO_DBG(HBA, CMD)		\
	PM8001_CHECK_LOGGING(HBA, PM8001_IO_LOGGING, CMD)

#define PM8001_FAIL_DBG(HBA, CMD)		\
	PM8001_CHECK_LOGGING(HBA, PM8001_FAIL_LOGGING, CMD)

#define PM8001_IOCTL_DBG(HBA, CMD)		\
	PM8001_CHECK_LOGGING(HBA, PM8001_IOCTL_LOGGING, CMD)

#define PM8001_MSG_DBG(HBA, CMD)		\
	PM8001_CHECK_LOGGING(HBA, PM8001_MSG_LOGGING, CMD)


#define PM8001_USE_TASKLET
#define PM8001_USE_MSIX
#define PM8001_READ_VPD


#define DEV_IS_EXPANDER(type)	((type == SAS_EDGE_EXPANDER_DEVICE) || (type == SAS_FANOUT_EXPANDER_DEVICE))

#define PM8001_NAME_LENGTH		32/* generic length of strings */
extern struct list_head hba_list;
extern const struct pm8001_dispatch pm8001_8001_dispatch;
extern const struct pm8001_dispatch pm8001_80xx_dispatch;

struct pm8001_hba_info;
struct pm8001_ccb_info;
struct pm8001_device;
/* define task management IU */
struct pm8001_tmf_task {
	u8	tmf;
	u32	tag_of_task_to_be_managed;
};
struct pm8001_ioctl_payload {
	u32	signature;
	u16	major_function;
	u16	minor_function;
	u16	length;
	u16	status;
	u16	offset;
	u16	id;
	u8	*func_specific;
};

struct pm8001_dispatch {
	char *name;
	int (*chip_init)(struct pm8001_hba_info *pm8001_ha);
	int (*chip_soft_rst)(struct pm8001_hba_info *pm8001_ha);
	void (*chip_rst)(struct pm8001_hba_info *pm8001_ha);
	int (*chip_ioremap)(struct pm8001_hba_info *pm8001_ha);
	void (*chip_iounmap)(struct pm8001_hba_info *pm8001_ha);
	irqreturn_t (*isr)(struct pm8001_hba_info *pm8001_ha, u8 vec);
	u32 (*is_our_interupt)(struct pm8001_hba_info *pm8001_ha);
	int (*isr_process_oq)(struct pm8001_hba_info *pm8001_ha, u8 vec);
	void (*interrupt_enable)(struct pm8001_hba_info *pm8001_ha, u8 vec);
	void (*interrupt_disable)(struct pm8001_hba_info *pm8001_ha, u8 vec);
	void (*make_prd)(struct scatterlist *scatter, int nr, void *prd);
	int (*smp_req)(struct pm8001_hba_info *pm8001_ha,
		struct pm8001_ccb_info *ccb);
	int (*ssp_io_req)(struct pm8001_hba_info *pm8001_ha,
		struct pm8001_ccb_info *ccb);
	int (*sata_req)(struct pm8001_hba_info *pm8001_ha,
		struct pm8001_ccb_info *ccb);
	int (*phy_start_req)(struct pm8001_hba_info *pm8001_ha,	u8 phy_id);
	int (*phy_stop_req)(struct pm8001_hba_info *pm8001_ha, u8 phy_id);
	int (*reg_dev_req)(struct pm8001_hba_info *pm8001_ha,
		struct pm8001_device *pm8001_dev, u32 flag);
	int (*dereg_dev_req)(struct pm8001_hba_info *pm8001_ha, u32 device_id);
	int (*phy_ctl_req)(struct pm8001_hba_info *pm8001_ha,
		u32 phy_id, u32 phy_op);
	int (*task_abort)(struct pm8001_hba_info *pm8001_ha,
		struct pm8001_device *pm8001_dev, u8 flag, u32 task_tag,
		u32 cmd_tag);
	int (*ssp_tm_req)(struct pm8001_hba_info *pm8001_ha,
		struct pm8001_ccb_info *ccb, struct pm8001_tmf_task *tmf);
	int (*get_nvmd_req)(struct pm8001_hba_info *pm8001_ha, void *payload);
	int (*set_nvmd_req)(struct pm8001_hba_info *pm8001_ha, void *payload);
	int (*fw_flash_update_req)(struct pm8001_hba_info *pm8001_ha,
		void *payload);
	int (*set_dev_state_req)(struct pm8001_hba_info *pm8001_ha,
		struct pm8001_device *pm8001_dev, u32 state);
	int (*sas_diag_start_end_req)(struct pm8001_hba_info *pm8001_ha,
		u32 state);
	int (*sas_diag_execute_req)(struct pm8001_hba_info *pm8001_ha,
		u32 state);
	int (*sas_re_init_req)(struct pm8001_hba_info *pm8001_ha);
};

struct pm8001_chip_info {
	u32     encrypt;
	u32	n_phy;
	const struct pm8001_dispatch	*dispatch;
};
#define PM8001_CHIP_DISP	(pm8001_ha->chip->dispatch)

struct pm8001_port {
	struct asd_sas_port	sas_port;
	u8			port_attached;
	u8			wide_port_phymap;
	u8			port_state;
	struct list_head	list;
};

struct pm8001_phy {
	struct pm8001_hba_info	*pm8001_ha;
	struct pm8001_port	*port;
	struct asd_sas_phy	sas_phy;
	struct sas_identify	identify;
	struct scsi_device	*sdev;
	u64			dev_sas_addr;
	u32			phy_type;
	struct completion	*enable_completion;
	u32			frame_rcvd_size;
	u8			frame_rcvd[32];
	u8			phy_attached;
	u8			phy_state;
	enum sas_linkrate	minimum_linkrate;
	enum sas_linkrate	maximum_linkrate;
};

struct pm8001_device {
	enum sas_device_type	dev_type;
	struct domain_device	*sas_device;
	u32			attached_phy;
	u32			id;
	struct completion	*dcompletion;
	struct completion	*setds_completion;
	u32			device_id;
	u32			running_req;
};

struct pm8001_prd_imt {
	__le32			len;
	__le32			e;
};

struct pm8001_prd {
	__le64			addr;		/* 64-bit buffer address */
	struct pm8001_prd_imt	im_len;		/* 64-bit length */
} __attribute__ ((packed));
/*
 * CCB(Command Control Block)
 */
struct pm8001_ccb_info {
	struct list_head	entry;
	struct sas_task		*task;
	u32			n_elem;
	u32			ccb_tag;
	dma_addr_t		ccb_dma_handle;
	struct pm8001_device	*device;
	struct pm8001_prd	buf_prd[PM8001_MAX_DMA_SG];
	struct fw_control_ex	*fw_control_context;
	u8			open_retry;
};

struct mpi_mem {
	void			*virt_ptr;
	dma_addr_t		phys_addr;
	u32			phys_addr_hi;
	u32			phys_addr_lo;
	u32			total_len;
	u32			num_elements;
	u32			element_size;
	u32			alignment;
};

struct mpi_mem_req {
	/* The number of element in the  mpiMemory array */
	u32			count;
	/* The array of structures that define memroy regions*/
	struct mpi_mem		region[USI_MAX_MEMCNT];
};

struct encrypt {
	u32	cipher_mode;
	u32	sec_mode;
	u32	status;
	u32	flag;
};

struct sas_phy_attribute_table {
	u32	phystart1_16[16];
	u32	outbound_hw_event_pid1_16[16];
};

union main_cfg_table {
	struct {
	u32			signature;
	u32			interface_rev;
	u32			firmware_rev;
	u32			max_out_io;
	u32			max_sgl;
	u32			ctrl_cap_flag;
	u32			gst_offset;
	u32			inbound_queue_offset;
	u32			outbound_queue_offset;
	u32			inbound_q_nppd_hppd;
	u32			outbound_hw_event_pid0_3;
	u32			outbound_hw_event_pid4_7;
	u32			outbound_ncq_event_pid0_3;
	u32			outbound_ncq_event_pid4_7;
	u32			outbound_tgt_ITNexus_event_pid0_3;
	u32			outbound_tgt_ITNexus_event_pid4_7;
	u32			outbound_tgt_ssp_event_pid0_3;
	u32			outbound_tgt_ssp_event_pid4_7;
	u32			outbound_tgt_smp_event_pid0_3;
	u32			outbound_tgt_smp_event_pid4_7;
	u32			upper_event_log_addr;
	u32			lower_event_log_addr;
	u32			event_log_size;
	u32			event_log_option;
	u32			upper_iop_event_log_addr;
	u32			lower_iop_event_log_addr;
	u32			iop_event_log_size;
	u32			iop_event_log_option;
	u32			fatal_err_interrupt;
	u32			fatal_err_dump_offset0;
	u32			fatal_err_dump_length0;
	u32			fatal_err_dump_offset1;
	u32			fatal_err_dump_length1;
	u32			hda_mode_flag;
	u32			anolog_setup_table_offset;
	u32			rsvd[4];
	} pm8001_tbl;

	struct {
	u32			signature;
	u32			interface_rev;
	u32			firmware_rev;
	u32			max_out_io;
	u32			max_sgl;
	u32			ctrl_cap_flag;
	u32			gst_offset;
	u32			inbound_queue_offset;
	u32			outbound_queue_offset;
	u32			inbound_q_nppd_hppd;
	u32			rsvd[8];
	u32			crc_core_dump;
	u32			rsvd1;
	u32			upper_event_log_addr;
	u32			lower_event_log_addr;
	u32			event_log_size;
	u32			event_log_severity;
	u32			upper_pcs_event_log_addr;
	u32			lower_pcs_event_log_addr;
	u32			pcs_event_log_size;
	u32			pcs_event_log_severity;
	u32			fatal_err_interrupt;
	u32			fatal_err_dump_offset0;
	u32			fatal_err_dump_length0;
	u32			fatal_err_dump_offset1;
	u32			fatal_err_dump_length1;
	u32			gpio_led_mapping;
	u32			analog_setup_table_offset;
	u32			int_vec_table_offset;
	u32			phy_attr_table_offset;
	u32			port_recovery_timer;
	u32			interrupt_reassertion_delay;
	} pm80xx_tbl;
};

union general_status_table {
	struct {
	u32			gst_len_mpistate;
	u32			iq_freeze_state0;
	u32			iq_freeze_state1;
	u32			msgu_tcnt;
	u32			iop_tcnt;
	u32			rsvd;
	u32			phy_state[8];
	u32			gpio_input_val;
	u32			rsvd1[2];
	u32			recover_err_info[8];
	} pm8001_tbl;
	struct {
	u32			gst_len_mpistate;
	u32			iq_freeze_state0;
	u32			iq_freeze_state1;
	u32			msgu_tcnt;
	u32			iop_tcnt;
	u32			rsvd[9];
	u32			gpio_input_val;
	u32			rsvd1[2];
	u32			recover_err_info[8];
	} pm80xx_tbl;
};
struct inbound_queue_table {
	u32			element_pri_size_cnt;
	u32			upper_base_addr;
	u32			lower_base_addr;
	u32			ci_upper_base_addr;
	u32			ci_lower_base_addr;
	u32			pi_pci_bar;
	u32			pi_offset;
	u32			total_length;
	void			*base_virt;
	void			*ci_virt;
	u32			reserved;
	__le32			consumer_index;
	u32			producer_idx;
};
struct outbound_queue_table {
	u32			element_size_cnt;
	u32			upper_base_addr;
	u32			lower_base_addr;
	void			*base_virt;
	u32			pi_upper_base_addr;
	u32			pi_lower_base_addr;
	u32			ci_pci_bar;
	u32			ci_offset;
	u32			total_length;
	void			*pi_virt;
	u32			interrup_vec_cnt_delay;
	u32			dinterrup_to_pci_offset;
	__le32			producer_index;
	u32			consumer_idx;
};
struct pm8001_hba_memspace {
	void __iomem  		*memvirtaddr;
	u64			membase;
	u32			memsize;
};
struct pm8001_hba_info {
	char			name[PM8001_NAME_LENGTH];
	struct list_head	list;
	unsigned long		flags;
	spinlock_t		lock;/* host-wide lock */
	struct pci_dev		*pdev;/* our device */
	struct device		*dev;
	struct pm8001_hba_memspace io_mem[6];
	struct mpi_mem_req	memoryMap;
	struct encrypt		encrypt_info; /* support encryption */
	void __iomem	*msg_unit_tbl_addr;/*Message Unit Table Addr*/
	void __iomem	*main_cfg_tbl_addr;/*Main Config Table Addr*/
	void __iomem	*general_stat_tbl_addr;/*General Status Table Addr*/
	void __iomem	*inbnd_q_tbl_addr;/*Inbound Queue Config Table Addr*/
	void __iomem	*outbnd_q_tbl_addr;/*Outbound Queue Config Table Addr*/
	void __iomem	*pspa_q_tbl_addr;
			/*MPI SAS PHY attributes Queue Config Table Addr*/
	void __iomem	*ivt_tbl_addr; /*MPI IVT Table Addr */
	union main_cfg_table	main_cfg_tbl;
	union general_status_table	gs_tbl;
	struct inbound_queue_table	inbnd_q_tbl[PM8001_MAX_SPCV_INB_NUM];
	struct outbound_queue_table	outbnd_q_tbl[PM8001_MAX_SPCV_OUTB_NUM];
	struct sas_phy_attribute_table	phy_attr_table;
					/* MPI SAS PHY attributes */
	u8			sas_addr[SAS_ADDR_SIZE];
	struct sas_ha_struct	*sas;/* SCSI/SAS glue */
	struct Scsi_Host	*shost;
	u32			chip_id;
	const struct pm8001_chip_info	*chip;
	struct completion	*nvmd_completion;
	int			tags_num;
	unsigned long		*tags;
	struct pm8001_phy	phy[PM8001_MAX_PHYS];
	struct pm8001_port	port[PM8001_MAX_PHYS];
	u32			id;
	u32			irq;
	u32			iomb_size; /* SPC and SPCV IOMB size */
	struct pm8001_device	*devices;
	struct pm8001_ccb_info	*ccb_info;
#ifdef PM8001_USE_MSIX
	struct msix_entry	msix_entries[PM8001_MAX_MSIX_VEC];
					/*for msi-x interrupt*/
	int			number_of_intr;/*will be used in remove()*/
#endif
#ifdef PM8001_USE_TASKLET
	struct tasklet_struct	tasklet;
#endif
	u32			logging_level;
	u32			fw_status;
	u32			smp_exp_mode;
	u32			int_vector;
	const struct firmware 	*fw_image;
	u8			outq[PM8001_MAX_MSIX_VEC];
};

struct pm8001_work {
	struct work_struct work;
	struct pm8001_hba_info *pm8001_ha;
	void *data;
	int handler;
};

struct pm8001_fw_image_header {
	u8 vender_id[8];
	u8 product_id;
	u8 hardware_rev;
	u8 dest_partition;
	u8 reserved;
	u8 fw_rev[4];
	__be32  image_length;
	__be32 image_crc;
	__be32 startup_entry;
} __attribute__((packed, aligned(4)));


/**
 * FW Flash Update status values
 */
#define FLASH_UPDATE_COMPLETE_PENDING_REBOOT	0x00
#define FLASH_UPDATE_IN_PROGRESS		0x01
#define FLASH_UPDATE_HDR_ERR			0x02
#define FLASH_UPDATE_OFFSET_ERR			0x03
#define FLASH_UPDATE_CRC_ERR			0x04
#define FLASH_UPDATE_LENGTH_ERR			0x05
#define FLASH_UPDATE_HW_ERR			0x06
#define FLASH_UPDATE_DNLD_NOT_SUPPORTED		0x10
#define FLASH_UPDATE_DISABLED			0x11

#define	NCQ_READ_LOG_FLAG			0x80000000
#define	NCQ_ABORT_ALL_FLAG			0x40000000
#define	NCQ_2ND_RLE_FLAG			0x20000000
/**
 * brief param structure for firmware flash update.
 */
struct fw_flash_updata_info {
	u32			cur_image_offset;
	u32			cur_image_len;
	u32			total_image_len;
	struct pm8001_prd	sgl;
};

struct fw_control_info {
	u32			retcode;/*ret code (status)*/
	u32			phase;/*ret code phase*/
	u32			phaseCmplt;/*percent complete for the current
	update phase */
	u32			version;/*Hex encoded firmware version number*/
	u32			offset;/*Used for downloading firmware	*/
	u32			len; /*len of buffer*/
	u32			size;/* Used in OS VPD and Trace get size
	operations.*/
	u32			reserved;/* padding required for 64 bit
	alignment */
	u8			buffer[1];/* Start of buffer */
};
struct fw_control_ex {
	struct fw_control_info *fw_control;
	void			*buffer;/* keep buffer pointer to be
	freed when the response comes*/
	void			*virtAddr;/* keep virtual address of the data */
	void			*usrAddr;/* keep virtual address of the
	user data */
	dma_addr_t		phys_addr;
	u32			len; /* len of buffer  */
	void			*payload; /* pointer to IOCTL Payload */
	u8			inProgress;/*if 1 - the IOCTL request is in
	progress */
	void			*param1;
	void			*param2;
	void			*param3;
};

/* pm8001 workqueue */
extern struct workqueue_struct *pm8001_wq;

/******************** function prototype *********************/
int pm8001_tag_alloc(struct pm8001_hba_info *pm8001_ha, u32 *tag_out);
void pm8001_tag_init(struct pm8001_hba_info *pm8001_ha);
u32 pm8001_get_ncq_tag(struct sas_task *task, u32 *tag);
void pm8001_ccb_free(struct pm8001_hba_info *pm8001_ha, u32 ccb_idx);
void pm8001_ccb_task_free(struct pm8001_hba_info *pm8001_ha,
	struct sas_task *task, struct pm8001_ccb_info *ccb, u32 ccb_idx);
int pm8001_phy_control(struct asd_sas_phy *sas_phy, enum phy_func func,
	void *funcdata);
void pm8001_scan_start(struct Scsi_Host *shost);
int pm8001_scan_finished(struct Scsi_Host *shost, unsigned long time);
int pm8001_queue_command(struct sas_task *task, const int num,
	gfp_t gfp_flags);
int pm8001_abort_task(struct sas_task *task);
int pm8001_abort_task_set(struct domain_device *dev, u8 *lun);
int pm8001_clear_aca(struct domain_device *dev, u8 *lun);
int pm8001_clear_task_set(struct domain_device *dev, u8 *lun);
int pm8001_dev_found(struct domain_device *dev);
void pm8001_dev_gone(struct domain_device *dev);
int pm8001_lu_reset(struct domain_device *dev, u8 *lun);
int pm8001_I_T_nexus_reset(struct domain_device *dev);
int pm8001_I_T_nexus_event_handler(struct domain_device *dev);
int pm8001_query_task(struct sas_task *task);
void pm8001_open_reject_retry(
	struct pm8001_hba_info *pm8001_ha,
	struct sas_task *task_to_close,
	struct pm8001_device *device_to_close);
int pm8001_mem_alloc(struct pci_dev *pdev, void **virt_addr,
	dma_addr_t *pphys_addr, u32 *pphys_addr_hi, u32 *pphys_addr_lo,
	u32 mem_size, u32 align);

void pm8001_chip_iounmap(struct pm8001_hba_info *pm8001_ha);
int pm8001_mpi_build_cmd(struct pm8001_hba_info *pm8001_ha,
			struct inbound_queue_table *circularQ,
			u32 opCode, void *payload, u32 responseQueue);
int pm8001_mpi_msg_free_get(struct inbound_queue_table *circularQ,
				u16 messageSize, void **messagePtr);
u32 pm8001_mpi_msg_free_set(struct pm8001_hba_info *pm8001_ha, void *pMsg,
			struct outbound_queue_table *circularQ, u8 bc);
u32 pm8001_mpi_msg_consume(struct pm8001_hba_info *pm8001_ha,
			struct outbound_queue_table *circularQ,
			void **messagePtr1, u8 *pBC);
int pm8001_chip_set_dev_state_req(struct pm8001_hba_info *pm8001_ha,
			struct pm8001_device *pm8001_dev, u32 state);
int pm8001_chip_fw_flash_update_req(struct pm8001_hba_info *pm8001_ha,
					void *payload);
int pm8001_chip_fw_flash_update_build(struct pm8001_hba_info *pm8001_ha,
					void *fw_flash_updata_info, u32 tag);
int pm8001_chip_set_nvmd_req(struct pm8001_hba_info *pm8001_ha, void *payload);
int pm8001_chip_get_nvmd_req(struct pm8001_hba_info *pm8001_ha, void *payload);
int pm8001_chip_ssp_tm_req(struct pm8001_hba_info *pm8001_ha,
				struct pm8001_ccb_info *ccb,
				struct pm8001_tmf_task *tmf);
int pm8001_chip_abort_task(struct pm8001_hba_info *pm8001_ha,
				struct pm8001_device *pm8001_dev,
				u8 flag, u32 task_tag, u32 cmd_tag);
int pm8001_chip_dereg_dev_req(struct pm8001_hba_info *pm8001_ha, u32 device_id);
void pm8001_chip_make_sg(struct scatterlist *scatter, int nr, void *prd);
void pm8001_work_fn(struct work_struct *work);
int pm8001_handle_event(struct pm8001_hba_info *pm8001_ha,
					void *data, int handler);
void pm8001_mpi_set_dev_state_resp(struct pm8001_hba_info *pm8001_ha,
							void *piomb);
void pm8001_mpi_set_nvmd_resp(struct pm8001_hba_info *pm8001_ha,
							void *piomb);
void pm8001_mpi_get_nvmd_resp(struct pm8001_hba_info *pm8001_ha,
							void *piomb);
int pm8001_mpi_local_phy_ctl(struct pm8001_hba_info *pm8001_ha,
							void *piomb);
void pm8001_get_lrate_mode(struct pm8001_phy *phy, u8 link_rate);
void pm8001_get_attached_sas_addr(struct pm8001_phy *phy, u8 *sas_addr);
void pm8001_bytes_dmaed(struct pm8001_hba_info *pm8001_ha, int i);
int pm8001_mpi_reg_resp(struct pm8001_hba_info *pm8001_ha, void *piomb);
int pm8001_mpi_dereg_resp(struct pm8001_hba_info *pm8001_ha, void *piomb);
int pm8001_mpi_fw_flash_update_resp(struct pm8001_hba_info *pm8001_ha,
							void *piomb);
int pm8001_mpi_general_event(struct pm8001_hba_info *pm8001_ha , void *piomb);
int pm8001_mpi_task_abort_resp(struct pm8001_hba_info *pm8001_ha, void *piomb);
struct sas_task *pm8001_alloc_task(void);
void pm8001_task_done(struct sas_task *task);
void pm8001_free_task(struct sas_task *task);
void pm8001_tag_free(struct pm8001_hba_info *pm8001_ha, u32 tag);
struct pm8001_device *pm8001_find_dev(struct pm8001_hba_info *pm8001_ha,
					u32 device_id);
int pm80xx_set_thermal_config(struct pm8001_hba_info *pm8001_ha);

int pm8001_bar4_shift(struct pm8001_hba_info *pm8001_ha, u32 shiftValue);

/* ctl shared API */
extern struct device_attribute *pm8001_host_attrs[];

#endif

