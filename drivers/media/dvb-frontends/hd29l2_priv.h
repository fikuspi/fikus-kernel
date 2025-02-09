/*
 * HDIC HD29L2 DMB-TH demodulator driver
 *
 * Copyright (C) 2011 Metropolia University of Applied Sciences, Electria R&D
 *
 * Author: Antti Palosaari <crope@iki.fi>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef HD29L2_PRIV
#define HD29L2_PRIV

#include <fikus/dvb/version.h>
#include "dvb_frontend.h"
#include "dvb_math.h"
#include "hd29l2.h"

#define HD29L2_XTAL 30400000 /* Hz */


#define HD29L2_QAM4NR 0x00
#define HD29L2_QAM4   0x01
#define HD29L2_QAM16  0x02
#define HD29L2_QAM32  0x03
#define HD29L2_QAM64  0x04

#define HD29L2_CODE_RATE_04 0x00
#define HD29L2_CODE_RATE_06 0x08
#define HD29L2_CODE_RATE_08 0x10

#define HD29L2_PN945 0x00
#define HD29L2_PN595 0x01
#define HD29L2_PN420 0x02

#define HD29L2_CARRIER_SINGLE 0x00
#define HD29L2_CARRIER_MULTI  0x01

#define HD29L2_INTERLEAVER_720 0x00
#define HD29L2_INTERLEAVER_420 0x01

struct reg_val {
	u8 reg;
	u8 val;
};

struct reg_mod_vals {
	u8 reg;
	u8 val[5];
};

struct hd29l2_priv {
	struct i2c_adapter *i2c;
	struct dvb_frontend fe;
	struct hd29l2_config cfg;
	u8 tuner_i2c_addr_programmed:1;

	fe_status_t fe_status;
};

static const struct reg_mod_vals reg_mod_vals_tab[] = {
	/* REG, QAM4NR, QAM4,QAM16,QAM32,QAM64 */
	{ 0x01, { 0x10, 0x10, 0x10, 0x10, 0x10 } },
	{ 0x02, { 0x07, 0x07, 0x07, 0x07, 0x07 } },
	{ 0x03, { 0x10, 0x10, 0x10, 0x10, 0x10 } },
	{ 0x04, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x05, { 0x61, 0x60, 0x60, 0x61, 0x60 } },
	{ 0x06, { 0xff, 0xff, 0xff, 0xff, 0xff } },
	{ 0x07, { 0xff, 0xff, 0xff, 0xff, 0xff } },
	{ 0x08, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x09, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x0a, { 0x15, 0x15, 0x03, 0x03, 0x03 } },
	{ 0x0d, { 0x78, 0x78, 0x88, 0x78, 0x78 } },
	{ 0x0e, { 0xa0, 0x90, 0xa0, 0xa0, 0xa0 } },
	{ 0x0f, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x10, { 0xa0, 0xa0, 0x58, 0x38, 0x38 } },
	{ 0x11, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x12, { 0x5a, 0x5a, 0x5a, 0x5a, 0x5a } },
	{ 0x13, { 0xa2, 0xa2, 0xa2, 0xa2, 0xa2 } },
	{ 0x17, { 0x40, 0x40, 0x40, 0x40, 0x40 } },
	{ 0x18, { 0x21, 0x21, 0x42, 0x52, 0x42 } },
	{ 0x19, { 0x21, 0x21, 0x62, 0x72, 0x62 } },
	{ 0x1a, { 0x32, 0x43, 0xa9, 0xb9, 0xa9 } },
	{ 0x1b, { 0x32, 0x43, 0xb9, 0xd8, 0xb9 } },
	{ 0x1c, { 0x02, 0x02, 0x03, 0x02, 0x03 } },
	{ 0x1d, { 0x0c, 0x0c, 0x01, 0x02, 0x02 } },
	{ 0x1e, { 0x02, 0x02, 0x02, 0x01, 0x02 } },
	{ 0x1f, { 0x02, 0x02, 0x01, 0x02, 0x04 } },
	{ 0x20, { 0x01, 0x02, 0x01, 0x01, 0x01 } },
	{ 0x21, { 0x08, 0x08, 0x0a, 0x0a, 0x0a } },
	{ 0x22, { 0x06, 0x06, 0x04, 0x05, 0x05 } },
	{ 0x23, { 0x06, 0x06, 0x05, 0x03, 0x05 } },
	{ 0x24, { 0x08, 0x08, 0x05, 0x07, 0x07 } },
	{ 0x25, { 0x16, 0x10, 0x10, 0x0a, 0x10 } },
	{ 0x26, { 0x14, 0x14, 0x04, 0x04, 0x04 } },
	{ 0x27, { 0x58, 0x58, 0x58, 0x5c, 0x58 } },
	{ 0x28, { 0x0a, 0x0a, 0x0a, 0x0a, 0x0a } },
	{ 0x29, { 0x0a, 0x0a, 0x0a, 0x0a, 0x0a } },
	{ 0x2a, { 0x08, 0x0a, 0x08, 0x08, 0x08 } },
	{ 0x2b, { 0x08, 0x08, 0x08, 0x08, 0x08 } },
	{ 0x2c, { 0x06, 0x06, 0x06, 0x06, 0x06 } },
	{ 0x2d, { 0x05, 0x06, 0x06, 0x06, 0x06 } },
	{ 0x2e, { 0x21, 0x21, 0x21, 0x21, 0x21 } },
	{ 0x2f, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x30, { 0x14, 0x14, 0x14, 0x14, 0x14 } },
	{ 0x33, { 0xb7, 0xb7, 0xb7, 0xb7, 0xb7 } },
	{ 0x34, { 0x81, 0x81, 0x81, 0x81, 0x81 } },
	{ 0x35, { 0x80, 0x80, 0x80, 0x80, 0x80 } },
	{ 0x37, { 0x70, 0x70, 0x70, 0x70, 0x70 } },
	{ 0x38, { 0x04, 0x04, 0x02, 0x02, 0x02 } },
	{ 0x39, { 0x07, 0x07, 0x05, 0x05, 0x05 } },
	{ 0x3a, { 0x06, 0x06, 0x06, 0x06, 0x06 } },
	{ 0x3b, { 0x03, 0x03, 0x03, 0x03, 0x03 } },
	{ 0x3c, { 0x07, 0x06, 0x04, 0x04, 0x04 } },
	{ 0x3d, { 0xf0, 0xf0, 0xf0, 0xf0, 0x80 } },
	{ 0x3e, { 0x60, 0x60, 0x60, 0x60, 0xff } },
	{ 0x3f, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x40, { 0x5b, 0x5b, 0x5b, 0x57, 0x50 } },
	{ 0x41, { 0x30, 0x30, 0x30, 0x30, 0x18 } },
	{ 0x42, { 0x20, 0x20, 0x20, 0x00, 0x30 } },
	{ 0x43, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x44, { 0x3f, 0x3f, 0x3f, 0x3f, 0x3f } },
	{ 0x45, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x46, { 0x0a, 0x0a, 0x0a, 0x0a, 0x0a } },
	{ 0x47, { 0x00, 0x00, 0x95, 0x00, 0x95 } },
	{ 0x48, { 0xc0, 0xc0, 0xc0, 0xc0, 0xc0 } },
	{ 0x49, { 0xc0, 0xc0, 0xc0, 0xc0, 0xc0 } },
	{ 0x4a, { 0x40, 0x40, 0x33, 0x11, 0x11 } },
	{ 0x4b, { 0x40, 0x40, 0x00, 0x00, 0x00 } },
	{ 0x4c, { 0x40, 0x40, 0x99, 0x11, 0x11 } },
	{ 0x4d, { 0x40, 0x40, 0x00, 0x00, 0x00 } },
	{ 0x4e, { 0x40, 0x40, 0x66, 0x77, 0x77 } },
	{ 0x4f, { 0x40, 0x40, 0x00, 0x00, 0x00 } },
	{ 0x50, { 0x40, 0x40, 0x88, 0x33, 0x11 } },
	{ 0x51, { 0x40, 0x40, 0x00, 0x00, 0x00 } },
	{ 0x52, { 0x40, 0x40, 0x88, 0x02, 0x02 } },
	{ 0x53, { 0x40, 0x40, 0x00, 0x02, 0x02 } },
	{ 0x54, { 0x00, 0x00, 0x88, 0x33, 0x33 } },
	{ 0x55, { 0x40, 0x40, 0x00, 0x00, 0x00 } },
	{ 0x56, { 0x00, 0x00, 0x00, 0x0b, 0x00 } },
	{ 0x57, { 0x40, 0x40, 0x0a, 0x0b, 0x0a } },
	{ 0x58, { 0xaa, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x59, { 0x7a, 0x40, 0x02, 0x02, 0x02 } },
	{ 0x5a, { 0x18, 0x18, 0x01, 0x01, 0x01 } },
	{ 0x5b, { 0x18, 0x18, 0x01, 0x01, 0x01 } },
	{ 0x5c, { 0x18, 0x18, 0x01, 0x01, 0x01 } },
	{ 0x5d, { 0x18, 0x18, 0x01, 0x01, 0x01 } },
	{ 0x5e, { 0xc0, 0xc0, 0xc0, 0xff, 0xc0 } },
	{ 0x5f, { 0xc0, 0xc0, 0xc0, 0xff, 0xc0 } },
	{ 0x60, { 0x40, 0x40, 0x00, 0x30, 0x30 } },
	{ 0x61, { 0x40, 0x40, 0x10, 0x30, 0x30 } },
	{ 0x62, { 0x40, 0x40, 0x00, 0x30, 0x30 } },
	{ 0x63, { 0x40, 0x40, 0x05, 0x30, 0x30 } },
	{ 0x64, { 0x40, 0x40, 0x06, 0x00, 0x30 } },
	{ 0x65, { 0x40, 0x40, 0x06, 0x08, 0x30 } },
	{ 0x66, { 0x40, 0x40, 0x00, 0x00, 0x20 } },
	{ 0x67, { 0x40, 0x40, 0x01, 0x04, 0x20 } },
	{ 0x68, { 0x00, 0x00, 0x30, 0x00, 0x20 } },
	{ 0x69, { 0xa0, 0xa0, 0x00, 0x08, 0x20 } },
	{ 0x6a, { 0x00, 0x00, 0x30, 0x00, 0x25 } },
	{ 0x6b, { 0xa0, 0xa0, 0x00, 0x06, 0x25 } },
	{ 0x6c, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x6d, { 0xa0, 0x60, 0x0c, 0x03, 0x0c } },
	{ 0x6e, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x6f, { 0xa0, 0x60, 0x04, 0x01, 0x04 } },
	{ 0x70, { 0x58, 0x58, 0xaa, 0xaa, 0xaa } },
	{ 0x71, { 0x58, 0x58, 0xaa, 0xaa, 0xaa } },
	{ 0x72, { 0x58, 0x58, 0xff, 0xff, 0xff } },
	{ 0x73, { 0x58, 0x58, 0xff, 0xff, 0xff } },
	{ 0x74, { 0x06, 0x06, 0x09, 0x05, 0x05 } },
	{ 0x75, { 0x06, 0x06, 0x0a, 0x10, 0x10 } },
	{ 0x76, { 0x10, 0x10, 0x06, 0x0a, 0x0a } },
	{ 0x77, { 0x12, 0x18, 0x28, 0x10, 0x28 } },
	{ 0x78, { 0xf8, 0xf8, 0xf8, 0xf8, 0xf8 } },
	{ 0x79, { 0x15, 0x15, 0x03, 0x03, 0x03 } },
	{ 0x7a, { 0x02, 0x02, 0x01, 0x04, 0x03 } },
	{ 0x7b, { 0x01, 0x02, 0x03, 0x03, 0x03 } },
	{ 0x7c, { 0x28, 0x28, 0x28, 0x28, 0x28 } },
	{ 0x7f, { 0x25, 0x92, 0x5f, 0x17, 0x2d } },
	{ 0x80, { 0x64, 0x64, 0x64, 0x74, 0x64 } },
	{ 0x83, { 0x06, 0x03, 0x04, 0x04, 0x04 } },
	{ 0x84, { 0xff, 0xff, 0xff, 0xff, 0xff } },
	{ 0x85, { 0x05, 0x05, 0x05, 0x05, 0x05 } },
	{ 0x86, { 0x00, 0x00, 0x11, 0x11, 0x11 } },
	{ 0x87, { 0x03, 0x03, 0x03, 0x03, 0x03 } },
	{ 0x88, { 0x09, 0x09, 0x09, 0x09, 0x09 } },
	{ 0x89, { 0x20, 0x20, 0x30, 0x20, 0x20 } },
	{ 0x8a, { 0x03, 0x03, 0x02, 0x03, 0x02 } },
	{ 0x8b, { 0x00, 0x07, 0x09, 0x00, 0x09 } },
	{ 0x8c, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x8d, { 0x4f, 0x4f, 0x4f, 0x3f, 0x4f } },
	{ 0x8e, { 0xf0, 0xf0, 0x60, 0xf0, 0xa0 } },
	{ 0x8f, { 0xe8, 0xe8, 0xe8, 0xe8, 0xe8 } },
	{ 0x90, { 0x10, 0x10, 0x10, 0x10, 0x10 } },
	{ 0x91, { 0x40, 0x40, 0x70, 0x70, 0x10 } },
	{ 0x92, { 0x00, 0x00, 0x00, 0x00, 0x04 } },
	{ 0x93, { 0x60, 0x60, 0x60, 0x60, 0x60 } },
	{ 0x94, { 0x00, 0x00, 0x00, 0x00, 0x03 } },
	{ 0x95, { 0x09, 0x09, 0x47, 0x47, 0x47 } },
	{ 0x96, { 0x80, 0xa0, 0xa0, 0x40, 0xa0 } },
	{ 0x97, { 0x60, 0x60, 0x60, 0x60, 0x60 } },
	{ 0x98, { 0x50, 0x50, 0x50, 0x30, 0x50 } },
	{ 0x99, { 0x10, 0x10, 0x10, 0x10, 0x10 } },
	{ 0x9a, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0x9b, { 0x40, 0x40, 0x40, 0x30, 0x40 } },
	{ 0x9c, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xa0, { 0xf0, 0xf0, 0xf0, 0xf0, 0xf0 } },
	{ 0xa1, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xa2, { 0x30, 0x30, 0x00, 0x30, 0x00 } },
	{ 0xa3, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xa4, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xa5, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xa6, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xa7, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xa8, { 0x77, 0x77, 0x77, 0x77, 0x77 } },
	{ 0xa9, { 0x02, 0x02, 0x02, 0x02, 0x02 } },
	{ 0xaa, { 0x40, 0x40, 0x40, 0x40, 0x40 } },
	{ 0xac, { 0x1f, 0x1f, 0x1f, 0x1f, 0x1f } },
	{ 0xad, { 0x14, 0x14, 0x14, 0x14, 0x14 } },
	{ 0xae, { 0x78, 0x78, 0x78, 0x78, 0x78 } },
	{ 0xaf, { 0x06, 0x06, 0x06, 0x06, 0x07 } },
	{ 0xb0, { 0x1b, 0x1b, 0x1b, 0x19, 0x1b } },
	{ 0xb1, { 0x18, 0x17, 0x17, 0x18, 0x17 } },
	{ 0xb2, { 0x35, 0x82, 0x82, 0x38, 0x82 } },
	{ 0xb3, { 0xb6, 0xce, 0xc7, 0x5c, 0xb0 } },
	{ 0xb4, { 0x3f, 0x3e, 0x3e, 0x3f, 0x3e } },
	{ 0xb5, { 0x70, 0x58, 0x50, 0x68, 0x50 } },
	{ 0xb6, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xb7, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xb8, { 0x03, 0x03, 0x01, 0x01, 0x01 } },
	{ 0xb9, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xba, { 0x06, 0x06, 0x0a, 0x05, 0x0a } },
	{ 0xbb, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xbc, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xbd, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xbe, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xbf, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xc0, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xc1, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xc2, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xc3, { 0x00, 0x00, 0x88, 0x66, 0x88 } },
	{ 0xc4, { 0x10, 0x10, 0x00, 0x00, 0x00 } },
	{ 0xc5, { 0x00, 0x00, 0x44, 0x60, 0x44 } },
	{ 0xc6, { 0x10, 0x0a, 0x00, 0x00, 0x00 } },
	{ 0xc7, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xc8, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xc9, { 0x90, 0x04, 0x00, 0x00, 0x00 } },
	{ 0xca, { 0x90, 0x08, 0x01, 0x01, 0x01 } },
	{ 0xcb, { 0xa0, 0x04, 0x00, 0x44, 0x00 } },
	{ 0xcc, { 0xa0, 0x10, 0x03, 0x00, 0x03 } },
	{ 0xcd, { 0x06, 0x06, 0x06, 0x05, 0x06 } },
	{ 0xce, { 0x05, 0x05, 0x01, 0x01, 0x01 } },
	{ 0xcf, { 0x40, 0x20, 0x18, 0x18, 0x18 } },
	{ 0xd0, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xd1, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xd2, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xd3, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xd4, { 0x05, 0x05, 0x05, 0x05, 0x05 } },
	{ 0xd5, { 0x05, 0x05, 0x05, 0x03, 0x05 } },
	{ 0xd6, { 0xac, 0x22, 0xca, 0x8f, 0xca } },
	{ 0xd7, { 0x20, 0x20, 0x20, 0x20, 0x20 } },
	{ 0xd8, { 0x01, 0x01, 0x01, 0x01, 0x01 } },
	{ 0xd9, { 0x00, 0x00, 0x0f, 0x00, 0x0f } },
	{ 0xda, { 0x00, 0xff, 0xff, 0x0e, 0xff } },
	{ 0xdb, { 0x0a, 0x0a, 0x0a, 0x0a, 0x0a } },
	{ 0xdc, { 0x0a, 0x0a, 0x0a, 0x0a, 0x0a } },
	{ 0xdd, { 0x05, 0x05, 0x05, 0x05, 0x05 } },
	{ 0xde, { 0x0a, 0x0a, 0x0a, 0x0a, 0x0a } },
	{ 0xdf, { 0x42, 0x42, 0x44, 0x44, 0x04 } },
	{ 0xe0, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xe1, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xe2, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xe3, { 0x00, 0x00, 0x26, 0x06, 0x26 } },
	{ 0xe4, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xe5, { 0x01, 0x0a, 0x01, 0x01, 0x01 } },
	{ 0xe6, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xe7, { 0x08, 0x08, 0x08, 0x08, 0x08 } },
	{ 0xe8, { 0x63, 0x63, 0x63, 0x63, 0x63 } },
	{ 0xe9, { 0x59, 0x59, 0x59, 0x59, 0x59 } },
	{ 0xea, { 0x80, 0x80, 0x20, 0x80, 0x80 } },
	{ 0xeb, { 0x37, 0x37, 0x78, 0x37, 0x77 } },
	{ 0xec, { 0x1f, 0x1f, 0x25, 0x25, 0x25 } },
	{ 0xed, { 0x0a, 0x0a, 0x0a, 0x0a, 0x0a } },
	{ 0xee, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ 0xef, { 0x70, 0x70, 0x58, 0x38, 0x58 } },
	{ 0xf0, { 0x00, 0x00, 0x00, 0x00, 0x00 } },
};

#endif /* HD29L2_PRIV */
