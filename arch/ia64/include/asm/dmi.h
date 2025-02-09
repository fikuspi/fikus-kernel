#ifndef _ASM_DMI_H
#define _ASM_DMI_H 1

#include <fikus/slab.h>
#include <asm/io.h>

/* Use normal IO mappings for DMI */
#define dmi_ioremap ioremap
#define dmi_iounmap(x,l) iounmap(x)
#define dmi_alloc(l) kzalloc(l, GFP_ATOMIC)

#endif
