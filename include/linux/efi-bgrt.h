#ifndef _FIKUS_EFI_BGRT_H
#define _FIKUS_EFI_BGRT_H

#ifdef CONFIG_ACPI_BGRT

#include <fikus/acpi.h>

void efi_bgrt_init(void);

/* The BGRT data itself; only valid if bgrt_image != NULL. */
extern void *bgrt_image;
extern size_t bgrt_image_size;
extern struct acpi_table_bgrt *bgrt_tab;

#else /* !CONFIG_ACPI_BGRT */

static inline void efi_bgrt_init(void) {}

#endif /* !CONFIG_ACPI_BGRT */

#endif /* _FIKUS_EFI_BGRT_H */
