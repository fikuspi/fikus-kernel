#ifndef _ACPI_IO_H_
#define _ACPI_IO_H_

#include <fikus/io.h>
#include <acpi/acpi.h>

static inline void __iomem *acpi_os_ioremap(acpi_physical_address phys,
					    acpi_size size)
{
       return ioremap_cache(phys, size);
}

void __iomem *acpi_os_get_iomem(acpi_physical_address phys, unsigned int size);

int acpi_os_map_generic_address(struct acpi_generic_address *addr);
void acpi_os_unmap_generic_address(struct acpi_generic_address *addr);

#endif
