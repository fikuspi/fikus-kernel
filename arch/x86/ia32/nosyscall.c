#include <fikus/kernel.h>
#include <fikus/errno.h>

long compat_ni_syscall(void)
{
	return -ENOSYS;
}
