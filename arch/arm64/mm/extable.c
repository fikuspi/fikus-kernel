/*
 * Based on arch/arm/mm/extable.c
 */

#include <fikus/module.h>
#include <fikus/uaccess.h>

int fixup_exception(struct pt_regs *regs)
{
	const struct exception_table_entry *fixup;

	fixup = search_exception_tables(instruction_pointer(regs));
	if (fixup)
		regs->pc = fixup->fixup;

	return fixup != NULL;
}
