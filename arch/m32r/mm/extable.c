/*
 * fikus/arch/m32r/mm/extable.c
 */

#include <fikus/module.h>
#include <asm/uaccess.h>

int fixup_exception(struct pt_regs *regs)
{
	const struct exception_table_entry *fixup;

	fixup = search_exception_tables(regs->bpc);
	if (fixup) {
		regs->bpc = fixup->fixup;
		return 1;
	}

	return 0;
}
