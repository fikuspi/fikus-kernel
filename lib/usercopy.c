#include <fikus/export.h>
#include <fikus/bug.h>
#include <fikus/uaccess.h>

void copy_from_user_overflow(void)
{
	WARN(1, "Buffer overflow detected!\n");
}
EXPORT_SYMBOL(copy_from_user_overflow);
