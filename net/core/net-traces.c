/*
 * consolidates trace point definitions
 *
 * Copyright (C) 2009 Neil Horman <nhorman@tuxdriver.com>
 */

#include <fikus/netdevice.h>
#include <fikus/etherdevice.h>
#include <fikus/string.h>
#include <fikus/if_arp.h>
#include <fikus/inetdevice.h>
#include <fikus/inet.h>
#include <fikus/interrupt.h>
#include <fikus/export.h>
#include <fikus/netpoll.h>
#include <fikus/sched.h>
#include <fikus/delay.h>
#include <fikus/rcupdate.h>
#include <fikus/types.h>
#include <fikus/workqueue.h>
#include <fikus/netlink.h>
#include <fikus/net_dropmon.h>
#include <fikus/slab.h>

#include <asm/unaligned.h>
#include <asm/bitops.h>

#define CREATE_TRACE_POINTS
#include <trace/events/skb.h>
#include <trace/events/net.h>
#include <trace/events/napi.h>
#include <trace/events/sock.h>
#include <trace/events/udp.h>

EXPORT_TRACEPOINT_SYMBOL_GPL(kfree_skb);

EXPORT_TRACEPOINT_SYMBOL_GPL(napi_poll);
