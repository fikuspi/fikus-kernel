#define _FIKUS_STRING_H_

#include <fikus/compiler.h>	/* for inline */
#include <fikus/types.h>	/* for size_t */
#include <fikus/stddef.h>	/* for NULL */
#include <fikus/linkage.h>
#include <asm/string.h>

extern unsigned long free_mem_ptr;
extern unsigned long free_mem_end_ptr;
extern void error(char *);

#define STATIC static
#define STATIC_RW_DATA	/* non-static please */

/* Diagnostic functions */
#ifdef DEBUG
#  define Assert(cond,msg) {if(!(cond)) error(msg);}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif

/* Not needed, but used in some headers pulled in by decompressors */
extern char * strstr(const char * s1, const char *s2);

#ifdef CONFIG_KERNEL_GZIP
#include "../../../../lib/decompress_inflate.c"
#endif

#ifdef CONFIG_KERNEL_LZO
#include "../../../../lib/decompress_unlzo.c"
#endif

#ifdef CONFIG_KERNEL_LZMA
#include "../../../../lib/decompress_unlzma.c"
#endif

#ifdef CONFIG_KERNEL_XZ
#define memmove memmove
#define memcpy memcpy
#include "../../../../lib/decompress_unxz.c"
#endif

#ifdef CONFIG_KERNEL_LZ4
#include "../../../../lib/decompress_unlz4.c"
#endif

int do_decompress(u8 *input, int len, u8 *output, void (*error)(char *x))
{
	return decompress(input, len, NULL, NULL, output, NULL, error);
}
