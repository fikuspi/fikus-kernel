/*
 * Exported ksyms of ARCH_MX1
 *
 * Copyright (C) 2008, Darius Augulis <augulis.darius@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <fikus/platform_device.h>
#include <fikus/module.h>

#include <fikus/platform_data/camera-mx1.h>

/* IMX camera FIQ handler */
EXPORT_SYMBOL(mx1_camera_sof_fiq_start);
EXPORT_SYMBOL(mx1_camera_sof_fiq_end);
