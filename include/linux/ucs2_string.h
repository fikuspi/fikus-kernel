#ifndef _FIKUS_UCS2_STRING_H_
#define _FIKUS_UCS2_STRING_H_

#include <fikus/types.h>	/* for size_t */
#include <fikus/stddef.h>	/* for NULL */

typedef u16 ucs2_char_t;

unsigned long ucs2_strnlen(const ucs2_char_t *s, size_t maxlength);
unsigned long ucs2_strlen(const ucs2_char_t *s);
unsigned long ucs2_strsize(const ucs2_char_t *data, unsigned long maxlength);
int ucs2_strncmp(const ucs2_char_t *a, const ucs2_char_t *b, size_t len);

#endif /* _FIKUS_UCS2_STRING_H_ */
