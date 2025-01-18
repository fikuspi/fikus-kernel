#ifndef FIKUS_BCMA_SOC_H_
#define FIKUS_BCMA_SOC_H_

#include <fikus/bcma/bcma.h>

struct bcma_soc {
	struct bcma_bus bus;
	struct bcma_device core_cc;
	struct bcma_device core_mips;
};

int __init bcma_host_soc_register(struct bcma_soc *soc);

int bcma_bus_register(struct bcma_bus *bus);

#endif /* FIKUS_BCMA_SOC_H_ */
