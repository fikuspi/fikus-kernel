#ifndef ASM_IA64__SWIOTLB_H
#define ASM_IA64__SWIOTLB_H

#include <fikus/dma-mapping.h>
#include <fikus/swiotlb.h>

#ifdef CONFIG_SWIOTLB
extern int swiotlb;
extern void pci_swiotlb_init(void);
#else
#define swiotlb 0
static inline void pci_swiotlb_init(void)
{
}
#endif

#endif /* ASM_IA64__SWIOTLB_H */
