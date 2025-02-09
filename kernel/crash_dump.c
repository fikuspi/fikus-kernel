#include <fikus/kernel.h>
#include <fikus/crash_dump.h>
#include <fikus/init.h>
#include <fikus/errno.h>
#include <fikus/export.h>

/*
 * If we have booted due to a crash, max_pfn will be a very low value. We need
 * to know the amount of memory that the previous kernel used.
 */
unsigned long saved_max_pfn;

/*
 * stores the physical address of elf header of crash image
 *
 * Note: elfcorehdr_addr is not just limited to vmcore. It is also used by
 * is_kdump_kernel() to determine if we are booting after a panic. Hence put
 * it under CONFIG_CRASH_DUMP and not CONFIG_PROC_VMCORE.
 */
unsigned long long elfcorehdr_addr = ELFCORE_ADDR_MAX;

/*
 * stores the size of elf header of crash image
 */
unsigned long long elfcorehdr_size;

/*
 * elfcorehdr= specifies the location of elf core header stored by the crashed
 * kernel. This option will be passed by kexec loader to the capture kernel.
 *
 * Syntax: elfcorehdr=[size[KMG]@]offset[KMG]
 */
static int __init setup_elfcorehdr(char *arg)
{
	char *end;
	if (!arg)
		return -EINVAL;
	elfcorehdr_addr = memparse(arg, &end);
	if (*end == '@') {
		elfcorehdr_size = elfcorehdr_addr;
		elfcorehdr_addr = memparse(end + 1, &end);
	}
	return end > arg ? 0 : -EINVAL;
}
early_param("elfcorehdr", setup_elfcorehdr);
