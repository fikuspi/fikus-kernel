/*
 * INET		An implementation of the TCP/IP protocol suite for the FIKUS
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the Interfaces handler.
 *
 * Version:	@(#)dev.h	1.0.10	08/12/93
 *
 * Authors:	Ross Biro
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *		Corey Minyard <wf-rch!minyard@relay.EU.net>
 *		Donald J. Becker, <becker@cesdis.gsfc.nasa.gov>
 *		Alan Cox, <alan@lxorguk.ukuu.org.uk>
 *		Bjorn Ekwall. <bj0rn@blox.se>
 *              Pekka Riikonen <priikone@poseidon.pspt.fi>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *		Moved to /usr/include/fikus for NET3
 */
#ifndef _UAPI_FIKUS_NETDEVICE_H
#define _UAPI_FIKUS_NETDEVICE_H

#include <fikus/if.h>
#include <fikus/if_ether.h>
#include <fikus/if_packet.h>
#include <fikus/if_link.h>


#define MAX_ADDR_LEN	32		/* Largest hardware address length */

/* Initial net device group. All devices belong to group 0 by default. */
#define INIT_NETDEV_GROUP	0



/* Media selection options. */
enum {
        IF_PORT_UNKNOWN = 0,
        IF_PORT_10BASE2,
        IF_PORT_10BASET,
        IF_PORT_AUI,
        IF_PORT_100BASET,
        IF_PORT_100BASETX,
        IF_PORT_100BASEFX
};


#endif /* _UAPI_FIKUS_NETDEVICE_H */
