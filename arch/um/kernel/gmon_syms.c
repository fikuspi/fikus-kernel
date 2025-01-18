/*
 * Copyright (C) 2001 - 2007 Jeff Dike (jdike@{addtoit,fikus.intel}.com)
 * Licensed under the GPL
 */

#include <fikus/module.h>

extern void __bb_init_func(void *)  __attribute__((weak));
EXPORT_SYMBOL(__bb_init_func);
