#ifndef _FIKUS_MSDOS_FS_H
#define _FIKUS_MSDOS_FS_H

#include <uapi/fikus/msdos_fs.h>

/* media of boot sector */
static inline int fat_valid_media(u8 media)
{
	return 0xf8 <= media || media == 0xf0;
}
#endif /* !_FIKUS_MSDOS_FS_H */
