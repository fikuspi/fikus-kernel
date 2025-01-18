/*
 * net/sched/cls_rsvp.c	Special RSVP packet classifier for IPv4.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 */

#include <fikus/module.h>
#include <fikus/types.h>
#include <fikus/kernel.h>
#include <fikus/string.h>
#include <fikus/errno.h>
#include <fikus/skbuff.h>
#include <net/ip.h>
#include <net/netlink.h>
#include <net/act_api.h>
#include <net/pkt_cls.h>

#define RSVP_DST_LEN	1
#define RSVP_ID		"rsvp"
#define RSVP_OPS	cls_rsvp_ops

#include "cls_rsvp.h"
MODULE_LICENSE("GPL");
