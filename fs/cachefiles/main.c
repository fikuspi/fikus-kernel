/* Network filesystem caching backend to use cache files on a premounted
 * filesystem
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#include <fikus/module.h>
#include <fikus/init.h>
#include <fikus/sched.h>
#include <fikus/completion.h>
#include <fikus/slab.h>
#include <fikus/fs.h>
#include <fikus/file.h>
#include <fikus/namei.h>
#include <fikus/mount.h>
#include <fikus/statfs.h>
#include <fikus/sysctl.h>
#include <fikus/miscdevice.h>
#include "internal.h"

unsigned cachefiles_debug;
module_param_named(debug, cachefiles_debug, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(cachefiles_debug, "CacheFiles debugging mask");

MODULE_DESCRIPTION("Mounted-filesystem based cache");
MODULE_AUTHOR("Red Hat, Inc.");
MODULE_LICENSE("GPL");

struct kmem_cache *cachefiles_object_jar;

static struct miscdevice cachefiles_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "cachefiles",
	.fops	= &cachefiles_daemon_fops,
};

static void cachefiles_object_init_once(void *_object)
{
	struct cachefiles_object *object = _object;

	memset(object, 0, sizeof(*object));
	spin_lock_init(&object->work_lock);
}

/*
 * initialise the fs caching module
 */
static int __init cachefiles_init(void)
{
	int ret;

	ret = misc_register(&cachefiles_dev);
	if (ret < 0)
		goto error_dev;

	/* create an object jar */
	ret = -ENOMEM;
	cachefiles_object_jar =
		kmem_cache_create("cachefiles_object_jar",
				  sizeof(struct cachefiles_object),
				  0,
				  SLAB_HWCACHE_ALIGN,
				  cachefiles_object_init_once);
	if (!cachefiles_object_jar) {
		printk(KERN_NOTICE
		       "CacheFiles: Failed to allocate an object jar\n");
		goto error_object_jar;
	}

	ret = cachefiles_proc_init();
	if (ret < 0)
		goto error_proc;

	printk(KERN_INFO "CacheFiles: Loaded\n");
	return 0;

error_proc:
	kmem_cache_destroy(cachefiles_object_jar);
error_object_jar:
	misc_deregister(&cachefiles_dev);
error_dev:
	kerror("failed to register: %d", ret);
	return ret;
}

fs_initcall(cachefiles_init);

/*
 * clean up on module removal
 */
static void __exit cachefiles_exit(void)
{
	printk(KERN_INFO "CacheFiles: Unloading\n");

	cachefiles_proc_cleanup();
	kmem_cache_destroy(cachefiles_object_jar);
	misc_deregister(&cachefiles_dev);
}

module_exit(cachefiles_exit);
