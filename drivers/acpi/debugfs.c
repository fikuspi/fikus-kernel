/*
 * debugfs.c - ACPI debugfs interface to userspace.
 */

#include <fikus/export.h>
#include <fikus/init.h>
#include <fikus/debugfs.h>
#include <acpi/acpi_drivers.h>

#define _COMPONENT		ACPI_SYSTEM_COMPONENT
ACPI_MODULE_NAME("debugfs");

struct dentry *acpi_debugfs_dir;
EXPORT_SYMBOL_GPL(acpi_debugfs_dir);

void __init acpi_debugfs_init(void)
{
	acpi_debugfs_dir = debugfs_create_dir("acpi", NULL);
}
