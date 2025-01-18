#ifndef _FIKUS_CRC_CCITT_H
#define _FIKUS_CRC_CCITT_H

#include <fikus/types.h>

extern u16 const crc_ccitt_table[256];

extern u16 crc_ccitt(u16 crc, const u8 *buffer, size_t len);

static inline u16 crc_ccitt_byte(u16 crc, const u8 c)
{
	return (crc >> 8) ^ crc_ccitt_table[(crc ^ c) & 0xff];
}

#endif /* _FIKUS_CRC_CCITT_H */
