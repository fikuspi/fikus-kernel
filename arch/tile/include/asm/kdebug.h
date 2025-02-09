/*
 * Copyright 2012 Tilera Corporation. All Rights Reserved.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#ifndef _ASM_TILE_KDEBUG_H
#define _ASM_TILE_KDEBUG_H

#include <fikus/notifier.h>

enum die_val {
	DIE_OOPS = 1,
	DIE_BREAK,
	DIE_SSTEPBP,
	DIE_PAGE_FAULT,
	DIE_COMPILED_BPT
};

#endif /* _ASM_TILE_KDEBUG_H */
