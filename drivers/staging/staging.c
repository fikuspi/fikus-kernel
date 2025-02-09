#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/module.h>

static int __init staging_init(void)
{
	return 0;
}

static void __exit staging_exit(void)
{
}

module_init(staging_init);
module_exit(staging_exit);

MODULE_AUTHOR("Greg Kroah-Hartman");
MODULE_DESCRIPTION("Staging Core");
MODULE_LICENSE("GPL");
