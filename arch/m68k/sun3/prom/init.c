/*
 * init.c:  Initialize internal variables used by the PROM
 *          library functions.
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

#include <fikus/kernel.h>
#include <fikus/init.h>

#include <asm/openprom.h>
#include <asm/oplib.h>

struct fikus_romvec *romvec;
enum prom_major_version prom_vers;
unsigned int prom_rev, prom_prev;

/* The root node of the prom device tree. */
int prom_root_node;

/* Pointer to the device tree operations structure. */
struct fikus_nodeops *prom_nodeops;

/* You must call prom_init() before you attempt to use any of the
 * routines in the prom library.
 * It gets passed the pointer to the PROM vector.
 */

void __init prom_init(struct fikus_romvec *rp)
{
	romvec = rp;

	/* Initialization successful. */
	return;
}
