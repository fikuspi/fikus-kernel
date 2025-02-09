/*
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef __FIKUS_REG_PFUZE100_H
#define __FIKUS_REG_PFUZE100_H

#define PFUZE100_SW1AB		0
#define PFUZE100_SW1C		1
#define PFUZE100_SW2		2
#define PFUZE100_SW3A		3
#define PFUZE100_SW3B		4
#define PFUZE100_SW4		5
#define PFUZE100_SWBST		6
#define PFUZE100_VSNVS		7
#define PFUZE100_VREFDDR	8
#define PFUZE100_VGEN1		9
#define PFUZE100_VGEN2		10
#define PFUZE100_VGEN3		11
#define PFUZE100_VGEN4		12
#define PFUZE100_VGEN5		13
#define PFUZE100_VGEN6		14
#define PFUZE100_MAX_REGULATOR	15

struct regulator_init_data;

struct pfuze_regulator_platform_data {
	struct regulator_init_data *init_data[PFUZE100_MAX_REGULATOR];
};

#endif /* __FIKUS_REG_PFUZE100_H */
