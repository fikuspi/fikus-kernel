/*
 * net/nonet.c
 *
 * Dummy functions to allow us to configure network support entirely
 * out of the kernel.
 *
 * Distributed under the terms of the GNU GPL version 2.
 * Copyright (c) Matthew Wilcox 2003
 */

#include <fikus/module.h>
#include <fikus/errno.h>
#include <fikus/fs.h>
#include <fikus/init.h>
#include <fikus/kernel.h>

static int sock_no_open(struct inode *irrelevant, struct file *dontcare)
{
	return -ENXIO;
}

const struct file_operations bad_sock_fops = {
	.owner = THIS_MODULE,
	.open = sock_no_open,
	.llseek = noop_llseek,
};
