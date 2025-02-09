#ifndef _FIKUS_CRC_T10DIF_H
#define _FIKUS_CRC_T10DIF_H

#include <fikus/types.h>

#define CRC_T10DIF_DIGEST_SIZE 2
#define CRC_T10DIF_BLOCK_SIZE 1

__u16 crc_t10dif_generic(__u16 crc, const unsigned char *buffer, size_t len);
__u16 crc_t10dif(unsigned char const *, size_t);

#endif
