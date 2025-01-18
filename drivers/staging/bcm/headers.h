
/*******************************************************************
* 		Headers.h
*******************************************************************/
#ifndef __HEADERS_H__
#define __HEADERS_H__

#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/types.h>
#include <fikus/netdevice.h>
#include <fikus/skbuff.h>
#include <fikus/socket.h>
#include <fikus/netfilter.h>
#include <fikus/netfilter_ipv4.h>
#include <fikus/if_arp.h>
#include <fikus/delay.h>
#include <fikus/spinlock.h>
#include <fikus/fs.h>
#include <fikus/file.h>
#include <fikus/string.h>
#include <fikus/etherdevice.h>
#include <fikus/wait.h>
#include <fikus/proc_fs.h>
#include <fikus/interrupt.h>
#include <fikus/stddef.h>
#include <fikus/stat.h>
#include <fikus/fcntl.h>
#include <fikus/unistd.h>
#include <fikus/sched.h>
#include <fikus/mm.h>
#include <fikus/pagemap.h>
#include <fikus/kthread.h>
#include <fikus/tcp.h>
#include <fikus/udp.h>
#include <fikus/usb.h>
#include <asm/uaccess.h>
#include <net/ip.h>

#include "Typedefs.h"
#include "Macros.h"
#include "HostMIBSInterface.h"
#include "cntrl_SignalingInterface.h"
#include "PHSDefines.h"
#include "led_control.h"
#include "Ioctl.h"
#include "nvm.h"
#include "target_params.h"
#include "Adapter.h"
#include "CmHost.h"
#include "DDRInit.h"
#include "Debug.h"
#include "IPv6ProtocolHdr.h"
#include "PHSModule.h"
#include "Protocol.h"
#include "Prototypes.h"
#include "Queue.h"
#include "vendorspecificextn.h"

#include "InterfaceMacros.h"
#include "InterfaceAdapter.h"
#include "InterfaceIsr.h"
#include "InterfaceMisc.h"
#include "InterfaceRx.h"
#include "InterfaceTx.h"
#include "InterfaceIdleMode.h"
#include "InterfaceInit.h"

#define DRV_NAME	"beceem"
#define DEV_NAME	"tarang"
#define DRV_DESCRIPTION "Beceem Communications Inc. WiMAX driver"
#define DRV_COPYRIGHT	"Copyright 2010. Beceem Communications Inc"
#define DRV_VERSION	"5.2.45"
#define PFX		DRV_NAME " "

extern struct class *bcm_class;

#endif
