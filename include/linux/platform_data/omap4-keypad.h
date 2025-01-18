#ifndef __FIKUS_INPUT_OMAP4_KEYPAD_H
#define __FIKUS_INPUT_OMAP4_KEYPAD_H

#include <fikus/input/matrix_keypad.h>

struct omap4_keypad_platform_data {
	const struct matrix_keymap_data *keymap_data;

	u8 rows;
	u8 cols;
};

#endif /* __FIKUS_INPUT_OMAP4_KEYPAD_H */
