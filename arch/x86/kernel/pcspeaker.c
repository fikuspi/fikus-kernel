#include <fikus/platform_device.h>
#include <fikus/err.h>
#include <fikus/init.h>

static __init int add_pcspkr(void)
{
	struct platform_device *pd;

	pd = platform_device_register_simple("pcspkr", -1, NULL, 0);

	return IS_ERR(pd) ? PTR_ERR(pd) : 0;
}
device_initcall(add_pcspkr);
