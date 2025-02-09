#ifndef _UAPI__FIKUS_BRIDGE_NETFILTER_H
#define _UAPI__FIKUS_BRIDGE_NETFILTER_H

/* bridge-specific defines for netfilter. 
 */

#include <fikus/netfilter.h>
#include <fikus/if_ether.h>
#include <fikus/if_vlan.h>
#include <fikus/if_pppox.h>

/* Bridge Hooks */
/* After promisc drops, checksum checks. */
#define NF_BR_PRE_ROUTING	0
/* If the packet is destined for this box. */
#define NF_BR_LOCAL_IN		1
/* If the packet is destined for another interface. */
#define NF_BR_FORWARD		2
/* Packets coming from a local process. */
#define NF_BR_LOCAL_OUT		3
/* Packets about to hit the wire. */
#define NF_BR_POST_ROUTING	4
/* Not really a hook, but used for the ebtables broute table */
#define NF_BR_BROUTING		5
#define NF_BR_NUMHOOKS		6

#endif /* _UAPI__FIKUS_BRIDGE_NETFILTER_H */
