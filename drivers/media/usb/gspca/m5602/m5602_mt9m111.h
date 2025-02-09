/*
 * Driver for the mt9m111 sensor
 *
 * Copyright (C) 2008 Erik Andrén
 * Copyright (C) 2007 Ilyes Gouta. Based on the m5603x Fikus Driver Project.
 * Copyright (C) 2005 m5603x Fikus Driver Project <m5602@x3ng.com.br>
 *
 * Portions of code to USB interface and ALi driver software,
 * Copyright (c) 2006 Willem Duinker
 * v4l2 interface modeled after the V4L2 driver
 * for SN9C10x PC Camera Controllers
 *
 * Some defines taken from the mt9m111 sensor driver
 * Copyright (C) 2008, Robert Jarzmik <robert.jarzmik@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2.
 *
 */

#ifndef M5602_MT9M111_H_
#define M5602_MT9M111_H_

#include "m5602_sensor.h"

/*****************************************************************************/

#define MT9M111_SC_CHIPVER			0x00
#define MT9M111_SC_ROWSTART			0x01
#define MT9M111_SC_COLSTART			0x02
#define MT9M111_SC_WINDOW_HEIGHT		0x03
#define MT9M111_SC_WINDOW_WIDTH			0x04
#define MT9M111_SC_HBLANK_CONTEXT_B		0x05
#define MT9M111_SC_VBLANK_CONTEXT_B		0x06
#define MT9M111_SC_HBLANK_CONTEXT_A		0x07
#define MT9M111_SC_VBLANK_CONTEXT_A		0x08
#define MT9M111_SC_SHUTTER_WIDTH		0x09
#define MT9M111_SC_ROW_SPEED			0x0a
#define MT9M111_SC_EXTRA_DELAY			0x0b
#define MT9M111_SC_SHUTTER_DELAY		0x0c
#define MT9M111_SC_RESET			0x0d
#define MT9M111_SC_R_MODE_CONTEXT_B		0x20
#define MT9M111_SC_R_MODE_CONTEXT_A		0x21
#define MT9M111_SC_FLASH_CONTROL		0x23
#define MT9M111_SC_GREEN_1_GAIN			0x2b
#define MT9M111_SC_BLUE_GAIN			0x2c
#define MT9M111_SC_RED_GAIN			0x2d
#define MT9M111_SC_GREEN_2_GAIN			0x2e
#define MT9M111_SC_GLOBAL_GAIN			0x2f

#define MT9M111_CONTEXT_CONTROL			0xc8
#define MT9M111_PAGE_MAP			0xf0
#define MT9M111_BYTEWISE_ADDRESS		0xf1

#define MT9M111_CP_OPERATING_MODE_CTL		0x06
#define MT9M111_CP_LUMA_OFFSET			0x34
#define MT9M111_CP_LUMA_CLIP			0x35
#define MT9M111_CP_OUTPUT_FORMAT_CTL2_CONTEXT_A 0x3a
#define MT9M111_CP_LENS_CORRECTION_1		0x3b
#define MT9M111_CP_DEFECT_CORR_CONTEXT_A	0x4c
#define MT9M111_CP_DEFECT_CORR_CONTEXT_B	0x4d
#define MT9M111_CP_OUTPUT_FORMAT_CTL2_CONTEXT_B 0x9b
#define MT9M111_CP_GLOBAL_CLK_CONTROL		0xb3

#define MT9M111_CC_AUTO_EXPOSURE_PARAMETER_18   0x65
#define MT9M111_CC_AWB_PARAMETER_7		0x28

#define MT9M111_SENSOR_CORE			0x00
#define MT9M111_COLORPIPE			0x01
#define MT9M111_CAMERA_CONTROL			0x02

#define MT9M111_RESET				(1 << 0)
#define MT9M111_RESTART				(1 << 1)
#define MT9M111_ANALOG_STANDBY			(1 << 2)
#define MT9M111_CHIP_ENABLE			(1 << 3)
#define MT9M111_CHIP_DISABLE			(0 << 3)
#define MT9M111_OUTPUT_DISABLE			(1 << 4)
#define MT9M111_SHOW_BAD_FRAMES			(1 << 0)
#define MT9M111_RESTART_BAD_FRAMES		(1 << 1)
#define MT9M111_SYNCHRONIZE_CHANGES		(1 << 7)

#define MT9M111_RMB_OVER_SIZED			(1 << 0)
#define MT9M111_RMB_MIRROR_ROWS			(1 << 0)
#define MT9M111_RMB_MIRROR_COLS			(1 << 1)
#define MT9M111_RMB_ROW_SKIP_2X			(1 << 2)
#define MT9M111_RMB_COLUMN_SKIP_2X		(1 << 3)
#define MT9M111_RMB_ROW_SKIP_4X			(1 << 4)
#define MT9M111_RMB_COLUMN_SKIP_4X		(1 << 5)

#define MT9M111_COLOR_MATRIX_BYPASS		(1 << 4)
#define MT9M111_SEL_CONTEXT_B			(1 << 3)

#define MT9M111_TRISTATE_PIN_IN_STANDBY		(1 << 1)
#define MT9M111_SOC_SOFT_STANDBY		(1 << 0)

#define MT9M111_2D_DEFECT_CORRECTION_ENABLE	(1 << 0)

#define INITIAL_MAX_GAIN			64
#define MT9M111_DEFAULT_GAIN			283
#define MT9M111_GREEN_GAIN_DEFAULT		0x20
#define MT9M111_BLUE_GAIN_DEFAULT		0x20
#define MT9M111_RED_GAIN_DEFAULT		0x20

/*****************************************************************************/

/* Kernel module parameters */
extern int force_sensor;
extern bool dump_sensor;

int mt9m111_probe(struct sd *sd);
int mt9m111_init(struct sd *sd);
int mt9m111_init_controls(struct sd *sd);
int mt9m111_start(struct sd *sd);
void mt9m111_disconnect(struct sd *sd);

static const struct m5602_sensor mt9m111 = {
	.name = "MT9M111",

	.i2c_slave_id = 0xba,
	.i2c_regW = 2,

	.probe = mt9m111_probe,
	.init = mt9m111_init,
	.init_controls = mt9m111_init_controls,
	.disconnect = mt9m111_disconnect,
	.start = mt9m111_start,
};

static const unsigned char preinit_mt9m111[][4] = {
	{BRIDGE, M5602_XB_MCU_CLK_DIV, 0x02, 0x00},
	{BRIDGE, M5602_XB_MCU_CLK_CTRL, 0xb0, 0x00},
	{BRIDGE, M5602_XB_SEN_CLK_DIV, 0x00, 0x00},
	{BRIDGE, M5602_XB_SEN_CLK_CTRL, 0xb0, 0x00},
	{BRIDGE, M5602_XB_SENSOR_TYPE, 0x0d, 0x00},
	{BRIDGE, M5602_XB_SENSOR_CTRL, 0x00, 0x00},
	{BRIDGE, M5602_XB_ADC_CTRL, 0xc0, 0x00},
	{BRIDGE, M5602_XB_SENSOR_TYPE, 0x09, 0x00},

	{SENSOR, MT9M111_PAGE_MAP, 0x00, 0x00},
	{SENSOR, MT9M111_SC_RESET,
		MT9M111_RESET |
		MT9M111_RESTART |
		MT9M111_ANALOG_STANDBY |
		MT9M111_CHIP_DISABLE,
		MT9M111_SHOW_BAD_FRAMES |
		MT9M111_RESTART_BAD_FRAMES |
		MT9M111_SYNCHRONIZE_CHANGES},

	{BRIDGE, M5602_XB_GPIO_DIR, 0x05, 0x00},
	{BRIDGE, M5602_XB_GPIO_DAT, 0x04, 0x00},
	{BRIDGE, M5602_XB_GPIO_EN_H, 0x3e, 0x00},
	{BRIDGE, M5602_XB_GPIO_DIR_H, 0x3e, 0x00},
	{BRIDGE, M5602_XB_GPIO_DAT_H, 0x02, 0x00},
	{BRIDGE, M5602_XB_GPIO_EN_L, 0xff, 0x00},
	{BRIDGE, M5602_XB_GPIO_DIR_L, 0xff, 0x00},
	{BRIDGE, M5602_XB_GPIO_DAT_L, 0x00, 0x00},

	{BRIDGE, M5602_XB_SEN_CLK_DIV, 0x00, 0x00},
	{BRIDGE, M5602_XB_SEN_CLK_CTRL, 0xb0, 0x00},
	{BRIDGE, M5602_XB_GPIO_DIR, 0x07, 0x00},
	{BRIDGE, M5602_XB_GPIO_DAT, 0x0b, 0x00},
	{BRIDGE, M5602_XB_GPIO_EN_H, 0x06, 0x00},
	{BRIDGE, M5602_XB_GPIO_EN_L, 0x00, 0x00},

	{BRIDGE, M5602_XB_I2C_CLK_DIV, 0x0a, 0x00}
};

static const unsigned char init_mt9m111[][4] = {
	{BRIDGE, M5602_XB_MCU_CLK_DIV, 0x02, 0x00},
	{BRIDGE, M5602_XB_MCU_CLK_CTRL, 0xb0, 0x00},
	{BRIDGE, M5602_XB_SEN_CLK_DIV, 0x00, 0x00},
	{BRIDGE, M5602_XB_SEN_CLK_CTRL, 0xb0, 0x00},
	{BRIDGE, M5602_XB_ADC_CTRL, 0xc0, 0x00},
	{BRIDGE, M5602_XB_SENSOR_TYPE, 0x09, 0x00},

	{BRIDGE, M5602_XB_GPIO_EN_H, 0x06, 0x00},
	{BRIDGE, M5602_XB_GPIO_EN_L, 0x00, 0x00},
	{BRIDGE, M5602_XB_GPIO_DAT, 0x04, 0x00},
	{BRIDGE, M5602_XB_GPIO_DIR_H, 0x3e, 0x00},
	{BRIDGE, M5602_XB_GPIO_DIR_L, 0xff, 0x00},
	{BRIDGE, M5602_XB_GPIO_DAT_H, 0x02, 0x00},
	{BRIDGE, M5602_XB_GPIO_DAT_L, 0x00, 0x00},
	{BRIDGE, M5602_XB_GPIO_DIR, 0x07, 0x00},
	{BRIDGE, M5602_XB_GPIO_DAT, 0x0b, 0x00},
	{BRIDGE, M5602_XB_I2C_CLK_DIV, 0x0a, 0x00},

	{SENSOR, MT9M111_SC_RESET, 0x00, 0x29},
	{SENSOR, MT9M111_PAGE_MAP, 0x00, 0x00},
	{SENSOR, MT9M111_SC_RESET, 0x00, 0x08},
	{SENSOR, MT9M111_PAGE_MAP, 0x00, 0x01},
	{SENSOR, MT9M111_CP_OPERATING_MODE_CTL, 0x00,
			MT9M111_CP_OPERATING_MODE_CTL},
	{SENSOR, MT9M111_CP_LENS_CORRECTION_1, 0x04, 0x2a},
	{SENSOR, MT9M111_CP_DEFECT_CORR_CONTEXT_A, 0x00,
				MT9M111_2D_DEFECT_CORRECTION_ENABLE},
	{SENSOR, MT9M111_CP_DEFECT_CORR_CONTEXT_B, 0x00,
				MT9M111_2D_DEFECT_CORRECTION_ENABLE},
	{SENSOR, MT9M111_CP_LUMA_OFFSET, 0x00, 0x00},
	{SENSOR, MT9M111_CP_LUMA_CLIP, 0xff, 0x00},
	{SENSOR, MT9M111_CP_OUTPUT_FORMAT_CTL2_CONTEXT_A, 0x14, 0x00},
	{SENSOR, MT9M111_CP_OUTPUT_FORMAT_CTL2_CONTEXT_B, 0x14, 0x00},
	{SENSOR, 0xcd, 0x00, 0x0e},
	{SENSOR, 0xd0, 0x00, 0x40},

	{SENSOR, MT9M111_PAGE_MAP, 0x00, 0x02},
	{SENSOR, MT9M111_CC_AUTO_EXPOSURE_PARAMETER_18, 0x00, 0x00},
	{SENSOR, MT9M111_CC_AWB_PARAMETER_7, 0xef, 0x03},

	{SENSOR, MT9M111_PAGE_MAP, 0x00, 0x00},
	{SENSOR, 0x33, 0x03, 0x49},
	{SENSOR, 0x34, 0xc0, 0x19},
	{SENSOR, 0x3f, 0x20, 0x20},
	{SENSOR, 0x40, 0x20, 0x20},
	{SENSOR, 0x5a, 0xc0, 0x0a},
	{SENSOR, 0x70, 0x7b, 0x0a},
	{SENSOR, 0x71, 0xff, 0x00},
	{SENSOR, 0x72, 0x19, 0x0e},
	{SENSOR, 0x73, 0x18, 0x0f},
	{SENSOR, 0x74, 0x57, 0x32},
	{SENSOR, 0x75, 0x56, 0x34},
	{SENSOR, 0x76, 0x73, 0x35},
	{SENSOR, 0x77, 0x30, 0x12},
	{SENSOR, 0x78, 0x79, 0x02},
	{SENSOR, 0x79, 0x75, 0x06},
	{SENSOR, 0x7a, 0x77, 0x0a},
	{SENSOR, 0x7b, 0x78, 0x09},
	{SENSOR, 0x7c, 0x7d, 0x06},
	{SENSOR, 0x7d, 0x31, 0x10},
	{SENSOR, 0x7e, 0x00, 0x7e},
	{SENSOR, 0x80, 0x59, 0x04},
	{SENSOR, 0x81, 0x59, 0x04},
	{SENSOR, 0x82, 0x57, 0x0a},
	{SENSOR, 0x83, 0x58, 0x0b},
	{SENSOR, 0x84, 0x47, 0x0c},
	{SENSOR, 0x85, 0x48, 0x0e},
	{SENSOR, 0x86, 0x5b, 0x02},
	{SENSOR, 0x87, 0x00, 0x5c},
	{SENSOR, MT9M111_CONTEXT_CONTROL, 0x00, MT9M111_SEL_CONTEXT_B},
	{SENSOR, 0x60, 0x00, 0x80},
	{SENSOR, 0x61, 0x00, 0x00},
	{SENSOR, 0x62, 0x00, 0x00},
	{SENSOR, 0x63, 0x00, 0x00},
	{SENSOR, 0x64, 0x00, 0x00},

	{SENSOR, MT9M111_SC_ROWSTART, 0x00, 0x0d}, /* 13 */
	{SENSOR, MT9M111_SC_COLSTART, 0x00, 0x12}, /* 18 */
	{SENSOR, MT9M111_SC_WINDOW_HEIGHT, 0x04, 0x00}, /* 1024 */
	{SENSOR, MT9M111_SC_WINDOW_WIDTH, 0x05, 0x10}, /* 1296 */
	{SENSOR, MT9M111_SC_HBLANK_CONTEXT_B, 0x01, 0x60}, /* 352 */
	{SENSOR, MT9M111_SC_VBLANK_CONTEXT_B, 0x00, 0x11}, /* 17 */
	{SENSOR, MT9M111_SC_HBLANK_CONTEXT_A, 0x01, 0x60}, /* 352 */
	{SENSOR, MT9M111_SC_VBLANK_CONTEXT_A, 0x00, 0x11}, /* 17 */
	{SENSOR, MT9M111_SC_R_MODE_CONTEXT_A, 0x01, 0x0f}, /* 271 */
	{SENSOR, 0x30, 0x04, 0x00},
	/* Set number of blank rows chosen to 400 */
	{SENSOR, MT9M111_SC_SHUTTER_WIDTH, 0x01, 0x90},
};

static const unsigned char start_mt9m111[][4] = {
	{BRIDGE, M5602_XB_SEN_CLK_DIV, 0x06, 0x00},
	{BRIDGE, M5602_XB_SEN_CLK_CTRL, 0xb0, 0x00},
	{BRIDGE, M5602_XB_ADC_CTRL, 0xc0, 0x00},
	{BRIDGE, M5602_XB_SENSOR_TYPE, 0x09, 0x00},
	{BRIDGE, M5602_XB_LINE_OF_FRAME_H, 0x81, 0x00},
	{BRIDGE, M5602_XB_PIX_OF_LINE_H, 0x82, 0x00},
	{BRIDGE, M5602_XB_SIG_INI, 0x01, 0x00},
	{BRIDGE, M5602_XB_VSYNC_PARA, 0x00, 0x00},
	{BRIDGE, M5602_XB_VSYNC_PARA, 0x00, 0x00},
	{BRIDGE, M5602_XB_VSYNC_PARA, 0x00, 0x00},
	{BRIDGE, M5602_XB_VSYNC_PARA, 0x00, 0x00},
};
#endif
