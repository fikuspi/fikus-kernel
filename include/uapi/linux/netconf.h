#ifndef _UAPI_FIKUS_NETCONF_H_
#define _UAPI_FIKUS_NETCONF_H_

#include <fikus/types.h>
#include <fikus/netlink.h>

struct netconfmsg {
	__u8	ncm_family;
};

enum {
	NETCONFA_UNSPEC,
	NETCONFA_IFINDEX,
	NETCONFA_FORWARDING,
	NETCONFA_RP_FILTER,
	NETCONFA_MC_FORWARDING,
	__NETCONFA_MAX
};
#define NETCONFA_MAX	(__NETCONFA_MAX - 1)

#define NETCONFA_IFINDEX_ALL		-1
#define NETCONFA_IFINDEX_DEFAULT	-2

#endif /* _UAPI_FIKUS_NETCONF_H_ */
