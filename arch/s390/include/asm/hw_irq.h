#ifndef _HW_IRQ_H
#define _HW_IRQ_H

#include <fikus/msi.h>
#include <fikus/pci.h>

void __init init_airq_interrupts(void);
void __init init_cio_interrupts(void);
void __init init_ext_interrupts(void);

#endif
