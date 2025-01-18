/*
 * decompress.c
 *
 * Detect the decompression method based on magic number
 */

#include <fikus/decompress/generic.h>

#include <fikus/decompress/bunzip2.h>
#include <fikus/decompress/unlzma.h>
#include <fikus/decompress/unxz.h>
#include <fikus/decompress/inflate.h>
#include <fikus/decompress/unlzo.h>
#include <fikus/decompress/unlz4.h>

#include <fikus/types.h>
#include <fikus/string.h>
#include <fikus/init.h>

#ifndef CONFIG_DECOMPRESS_GZIP
# define gunzip NULL
#endif
#ifndef CONFIG_DECOMPRESS_BZIP2
# define bunzip2 NULL
#endif
#ifndef CONFIG_DECOMPRESS_LZMA
# define unlzma NULL
#endif
#ifndef CONFIG_DECOMPRESS_XZ
# define unxz NULL
#endif
#ifndef CONFIG_DECOMPRESS_LZO
# define unlzo NULL
#endif
#ifndef CONFIG_DECOMPRESS_LZ4
# define unlz4 NULL
#endif

struct compress_format {
	unsigned char magic[2];
	const char *name;
	decompress_fn decompressor;
};

static const struct compress_format compressed_formats[] __initconst = {
	{ {037, 0213}, "gzip", gunzip },
	{ {037, 0236}, "gzip", gunzip },
	{ {0x42, 0x5a}, "bzip2", bunzip2 },
	{ {0x5d, 0x00}, "lzma", unlzma },
	{ {0xfd, 0x37}, "xz", unxz },
	{ {0x89, 0x4c}, "lzo", unlzo },
	{ {0x02, 0x21}, "lz4", unlz4 },
	{ {0, 0}, NULL, NULL }
};

decompress_fn __init decompress_method(const unsigned char *inbuf, int len,
				const char **name)
{
	const struct compress_format *cf;

	if (len < 2)
		return NULL;	/* Need at least this much... */

	for (cf = compressed_formats; cf->name; cf++) {
		if (!memcmp(inbuf, cf->magic, 2))
			break;

	}
	if (name)
		*name = cf->name;
	return cf->decompressor;
}
