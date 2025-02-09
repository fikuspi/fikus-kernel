/*******************************************************************************

  Intel PRO/1000 Fikus driver
  Copyright(c) 1999 - 2006 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Fikus NICS <fikus.nics@intel.com>
  e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/


/* Fikus PRO/1000 Ethernet Driver main header file */

#ifndef _E1000_H_
#define _E1000_H_

#include <fikus/stddef.h>
#include <fikus/module.h>
#include <fikus/types.h>
#include <asm/byteorder.h>
#include <fikus/init.h>
#include <fikus/mm.h>
#include <fikus/errno.h>
#include <fikus/ioport.h>
#include <fikus/pci.h>
#include <fikus/kernel.h>
#include <fikus/netdevice.h>
#include <fikus/etherdevice.h>
#include <fikus/skbuff.h>
#include <fikus/delay.h>
#include <fikus/timer.h>
#include <fikus/slab.h>
#include <fikus/vmalloc.h>
#include <fikus/interrupt.h>
#include <fikus/string.h>
#include <fikus/pagemap.h>
#include <fikus/dma-mapping.h>
#include <fikus/bitops.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <fikus/capability.h>
#include <fikus/in.h>
#include <fikus/ip.h>
#include <fikus/ipv6.h>
#include <fikus/tcp.h>
#include <fikus/udp.h>
#include <net/pkt_sched.h>
#include <fikus/list.h>
#include <fikus/reboot.h>
#include <net/checksum.h>
#include <fikus/mii.h>
#include <fikus/ethtool.h>
#include <fikus/if_vlan.h>

#define BAR_0		0
#define BAR_1		1
#define BAR_5		5

#define INTEL_E1000_ETHERNET_DEVICE(device_id) {\
	PCI_DEVICE(PCI_VENDOR_ID_INTEL, device_id)}

struct e1000_adapter;

#include "e1000_hw.h"

#define E1000_MAX_INTR			10

/* TX/RX descriptor defines */
#define E1000_DEFAULT_TXD		256
#define E1000_MAX_TXD			256
#define E1000_MIN_TXD			48
#define E1000_MAX_82544_TXD		4096

#define E1000_DEFAULT_RXD		256
#define E1000_MAX_RXD			256
#define E1000_MIN_RXD			48
#define E1000_MAX_82544_RXD		4096

#define E1000_MIN_ITR_USECS		10 /* 100000 irq/sec */
#define E1000_MAX_ITR_USECS		10000 /* 100    irq/sec */

/* this is the size past which hardware will drop packets when setting LPE=0 */
#define MAXIMUM_ETHERNET_VLAN_SIZE	1522

/* Supported Rx Buffer Sizes */
#define E1000_RXBUFFER_128		128    /* Used for packet split */
#define E1000_RXBUFFER_256		256    /* Used for packet split */
#define E1000_RXBUFFER_512		512
#define E1000_RXBUFFER_1024		1024
#define E1000_RXBUFFER_2048		2048
#define E1000_RXBUFFER_4096		4096
#define E1000_RXBUFFER_8192		8192
#define E1000_RXBUFFER_16384		16384

/* SmartSpeed delimiters */
#define E1000_SMARTSPEED_DOWNSHIFT	3
#define E1000_SMARTSPEED_MAX		15

/* Packet Buffer allocations */
#define E1000_PBA_BYTES_SHIFT		0xA
#define E1000_TX_HEAD_ADDR_SHIFT	7
#define E1000_PBA_TX_MASK		0xFFFF0000

/* Flow Control Watermarks */
#define E1000_FC_HIGH_DIFF	0x1638 /* High: 5688 bytes below Rx FIFO size */
#define E1000_FC_LOW_DIFF	0x1640 /* Low:  5696 bytes below Rx FIFO size */

#define E1000_FC_PAUSE_TIME	0xFFFF /* pause for the max or until send xon */

/* How many Tx Descriptors do we need to call netif_wake_queue ? */
#define E1000_TX_QUEUE_WAKE	16
/* How many Rx Buffers do we bundle into one write to the hardware ? */
#define E1000_RX_BUFFER_WRITE	16 /* Must be power of 2 */

#define AUTO_ALL_MODES		0
#define E1000_EEPROM_82544_APM	0x0004
#define E1000_EEPROM_APME	0x0400

#ifndef E1000_MASTER_SLAVE
/* Switch to override PHY master/slave setting */
#define E1000_MASTER_SLAVE	e1000_ms_hw_default
#endif

#define E1000_MNG_VLAN_NONE	(-1)

/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffer
 */
struct e1000_buffer {
	struct sk_buff *skb;
	dma_addr_t dma;
	struct page *page;
	unsigned long time_stamp;
	u16 length;
	u16 next_to_watch;
	unsigned int segs;
	unsigned int bytecount;
	u16 mapped_as_page;
};

struct e1000_tx_ring {
	/* pointer to the descriptor ring memory */
	void *desc;
	/* physical address of the descriptor ring */
	dma_addr_t dma;
	/* length of descriptor ring in bytes */
	unsigned int size;
	/* number of descriptors in the ring */
	unsigned int count;
	/* next descriptor to associate a buffer with */
	unsigned int next_to_use;
	/* next descriptor to check for DD status bit */
	unsigned int next_to_clean;
	/* array of buffer information structs */
	struct e1000_buffer *buffer_info;

	u16 tdh;
	u16 tdt;
	bool last_tx_tso;
};

struct e1000_rx_ring {
	/* pointer to the descriptor ring memory */
	void *desc;
	/* physical address of the descriptor ring */
	dma_addr_t dma;
	/* length of descriptor ring in bytes */
	unsigned int size;
	/* number of descriptors in the ring */
	unsigned int count;
	/* next descriptor to associate a buffer with */
	unsigned int next_to_use;
	/* next descriptor to check for DD status bit */
	unsigned int next_to_clean;
	/* array of buffer information structs */
	struct e1000_buffer *buffer_info;
	struct sk_buff *rx_skb_top;

	/* cpu for rx queue */
	int cpu;

	u16 rdh;
	u16 rdt;
};

#define E1000_DESC_UNUSED(R)						\
	((((R)->next_to_clean > (R)->next_to_use)			\
	  ? 0 : (R)->count) + (R)->next_to_clean - (R)->next_to_use - 1)

#define E1000_RX_DESC_EXT(R, i)						\
	(&(((union e1000_rx_desc_extended *)((R).desc))[i]))
#define E1000_GET_DESC(R, i, type)	(&(((struct type *)((R).desc))[i]))
#define E1000_RX_DESC(R, i)		E1000_GET_DESC(R, i, e1000_rx_desc)
#define E1000_TX_DESC(R, i)		E1000_GET_DESC(R, i, e1000_tx_desc)
#define E1000_CONTEXT_DESC(R, i)	E1000_GET_DESC(R, i, e1000_context_desc)

/* board specific private data structure */

struct e1000_adapter {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
	u16 mng_vlan_id;
	u32 bd_number;
	u32 rx_buffer_len;
	u32 wol;
	u32 smartspeed;
	u32 en_mng_pt;
	u16 link_speed;
	u16 link_duplex;
	spinlock_t stats_lock;
	unsigned int total_tx_bytes;
	unsigned int total_tx_packets;
	unsigned int total_rx_bytes;
	unsigned int total_rx_packets;
	/* Interrupt Throttle Rate */
	u32 itr;
	u32 itr_setting;
	u16 tx_itr;
	u16 rx_itr;

	u8 fc_autoneg;

	/* TX */
	struct e1000_tx_ring *tx_ring;      /* One per active queue */
	unsigned int restart_queue;
	u32 txd_cmd;
	u32 tx_int_delay;
	u32 tx_abs_int_delay;
	u32 gotcl;
	u64 gotcl_old;
	u64 tpt_old;
	u64 colc_old;
	u32 tx_timeout_count;
	u32 tx_fifo_head;
	u32 tx_head_addr;
	u32 tx_fifo_size;
	u8  tx_timeout_factor;
	atomic_t tx_fifo_stall;
	bool pcix_82544;
	bool detect_tx_hung;
	bool dump_buffers;

	/* RX */
	bool (*clean_rx)(struct e1000_adapter *adapter,
			 struct e1000_rx_ring *rx_ring,
			 int *work_done, int work_to_do);
	void (*alloc_rx_buf)(struct e1000_adapter *adapter,
			     struct e1000_rx_ring *rx_ring,
			     int cleaned_count);
	struct e1000_rx_ring *rx_ring;      /* One per active queue */
	struct napi_struct napi;

	int num_tx_queues;
	int num_rx_queues;

	u64 hw_csum_err;
	u64 hw_csum_good;
	u32 alloc_rx_buff_failed;
	u32 rx_int_delay;
	u32 rx_abs_int_delay;
	bool rx_csum;
	u32 gorcl;
	u64 gorcl_old;

	/* OS defined structs */
	struct net_device *netdev;
	struct pci_dev *pdev;

	/* structs defined in e1000_hw.h */
	struct e1000_hw hw;
	struct e1000_hw_stats stats;
	struct e1000_phy_info phy_info;
	struct e1000_phy_stats phy_stats;

	u32 test_icr;
	struct e1000_tx_ring test_tx_ring;
	struct e1000_rx_ring test_rx_ring;

	int msg_enable;

	/* to not mess up cache alignment, always add to the bottom */
	bool tso_force;
	bool smart_power_down;	/* phy smart power down */
	bool quad_port_a;
	unsigned long flags;
	u32 eeprom_wol;

	/* for ioport free */
	int bars;
	int need_ioport;

	bool discarding;

	struct work_struct reset_task;
	struct delayed_work watchdog_task;
	struct delayed_work fifo_stall_task;
	struct delayed_work phy_info_task;

	struct mutex mutex;
};

enum e1000_state_t {
	__E1000_TESTING,
	__E1000_RESETTING,
	__E1000_DOWN
};

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

extern struct net_device *e1000_get_hw_dev(struct e1000_hw *hw);
#define e_dbg(format, arg...) \
	netdev_dbg(e1000_get_hw_dev(hw), format, ## arg)
#define e_err(msglvl, format, arg...) \
	netif_err(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_info(msglvl, format, arg...) \
	netif_info(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_warn(msglvl, format, arg...) \
	netif_warn(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_notice(msglvl, format, arg...) \
	netif_notice(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_dev_info(format, arg...) \
	dev_info(&adapter->pdev->dev, format, ## arg)
#define e_dev_warn(format, arg...) \
	dev_warn(&adapter->pdev->dev, format, ## arg)
#define e_dev_err(format, arg...) \
	dev_err(&adapter->pdev->dev, format, ## arg)

extern char e1000_driver_name[];
extern const char e1000_driver_version[];

extern int e1000_up(struct e1000_adapter *adapter);
extern void e1000_down(struct e1000_adapter *adapter);
extern void e1000_reinit_locked(struct e1000_adapter *adapter);
extern void e1000_reset(struct e1000_adapter *adapter);
extern int e1000_set_spd_dplx(struct e1000_adapter *adapter, u32 spd, u8 dplx);
extern int e1000_setup_all_rx_resources(struct e1000_adapter *adapter);
extern int e1000_setup_all_tx_resources(struct e1000_adapter *adapter);
extern void e1000_free_all_rx_resources(struct e1000_adapter *adapter);
extern void e1000_free_all_tx_resources(struct e1000_adapter *adapter);
extern void e1000_update_stats(struct e1000_adapter *adapter);
extern bool e1000_has_link(struct e1000_adapter *adapter);
extern void e1000_power_up_phy(struct e1000_adapter *);
extern void e1000_set_ethtool_ops(struct net_device *netdev);
extern void e1000_check_options(struct e1000_adapter *adapter);
extern char *e1000_get_hw_dev_name(struct e1000_hw *hw);

#endif /* _E1000_H_ */
