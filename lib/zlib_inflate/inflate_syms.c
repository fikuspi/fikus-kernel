/*
 * fikus/lib/zlib_inflate/inflate_syms.c
 *
 * Exported symbols for the inflate functionality.
 *
 */

#include <fikus/module.h>
#include <fikus/init.h>

#include <fikus/zlib.h>

EXPORT_SYMBOL(zlib_inflate_workspacesize);
EXPORT_SYMBOL(zlib_inflate);
EXPORT_SYMBOL(zlib_inflateInit2);
EXPORT_SYMBOL(zlib_inflateEnd);
EXPORT_SYMBOL(zlib_inflateReset);
EXPORT_SYMBOL(zlib_inflateIncomp); 
EXPORT_SYMBOL(zlib_inflate_blob);
MODULE_LICENSE("GPL");
