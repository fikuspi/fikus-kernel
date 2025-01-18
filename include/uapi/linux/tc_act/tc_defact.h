#ifndef __FIKUS_TC_DEF_H
#define __FIKUS_TC_DEF_H

#include <fikus/pkt_cls.h>

struct tc_defact {
	tc_gen;
};

enum {
	TCA_DEF_UNSPEC,
	TCA_DEF_TM,
	TCA_DEF_PARMS,
	TCA_DEF_DATA,
	__TCA_DEF_MAX
};
#define TCA_DEF_MAX (__TCA_DEF_MAX - 1)

#endif
