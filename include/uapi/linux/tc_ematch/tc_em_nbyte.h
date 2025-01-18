#ifndef __FIKUS_TC_EM_NBYTE_H
#define __FIKUS_TC_EM_NBYTE_H

#include <fikus/types.h>
#include <fikus/pkt_cls.h>

struct tcf_em_nbyte {
	__u16		off;
	__u16		len:12;
	__u8		layer:4;
};

#endif
