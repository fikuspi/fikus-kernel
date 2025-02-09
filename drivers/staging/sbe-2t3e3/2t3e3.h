/*
 * SBE 2T3E3 synchronous serial card driver for Fikus
 *
 * Copyright (C) 2009-2010 Krzysztof Halasa <khc@pm.waw.pl>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This code is based on a driver written by SBE Inc.
 */

#ifndef T3E3_H
#define T3E3_H

#include <fikus/hdlc.h>
#include <fikus/interrupt.h>
#include <fikus/netdevice.h>
#include <fikus/pci.h>
#include <fikus/io.h>
#include "ctrl.h"

/**************************************************************
 *  21143
 **************************************************************/

/* CSR */
#define SBE_2T3E3_21143_REG_BUS_MODE					0
#define SBE_2T3E3_21143_REG_TRANSMIT_POLL_DEMAND			1
#define SBE_2T3E3_21143_REG_RECEIVE_POLL_DEMAND				2
#define SBE_2T3E3_21143_REG_RECEIVE_LIST_BASE_ADDRESS			3
#define SBE_2T3E3_21143_REG_TRANSMIT_LIST_BASE_ADDRESS			4
#define SBE_2T3E3_21143_REG_STATUS					5
#define SBE_2T3E3_21143_REG_OPERATION_MODE				6
#define SBE_2T3E3_21143_REG_INTERRUPT_ENABLE				7
#define SBE_2T3E3_21143_REG_MISSED_FRAMES_AND_OVERFLOW_COUNTER		8
#define SBE_2T3E3_21143_REG_BOOT_ROM_SERIAL_ROM_AND_MII_MANAGEMENT	9
#define SBE_2T3E3_21143_REG_BOOT_ROM_PROGRAMMING_ADDRESS		10
#define SBE_2T3E3_21143_REG_GENERAL_PURPOSE_TIMER_AND_INTERRUPT_MITIGATION_CONTROL 11
#define SBE_2T3E3_21143_REG_SIA_STATUS					12
#define SBE_2T3E3_21143_REG_SIA_CONNECTIVITY				13
#define SBE_2T3E3_21143_REG_SIA_TRANSMIT_AND_RECEIVE			14
#define SBE_2T3E3_21143_REG_SIA_AND_GENERAL_PURPOSE_PORT		15
#define SBE_2T3E3_21143_REG_MAX						16

/* CSR0 - BUS_MODE */
#define SBE_2T3E3_21143_VAL_WRITE_AND_INVALIDATE_ENABLE		0x01000000
#define SBE_2T3E3_21143_VAL_READ_LINE_ENABLE			0x00800000
#define SBE_2T3E3_21143_VAL_READ_MULTIPLE_ENABLE		0x00200000
#define SBE_2T3E3_21143_VAL_TRANSMIT_AUTOMATIC_POLLING_200us	0x00020000
#define SBE_2T3E3_21143_VAL_TRANSMIT_AUTOMATIC_POLLING_DISABLED	0x00000000
#define SBE_2T3E3_21143_VAL_CACHE_ALIGNMENT_32			0x0000c000
#define SBE_2T3E3_21143_VAL_CACHE_ALIGNMENT_16			0x00008000
#define SBE_2T3E3_21143_VAL_CACHE_ALIGNMENT_8			0x00004000
#define SBE_2T3E3_21143_VAL_BUS_ARBITRATION_RR			0x00000002
#define SBE_2T3E3_21143_VAL_SOFTWARE_RESET			0x00000001

/* CSR5 - STATUS */
#define SBE_2T3E3_21143_VAL_GENERAL_PURPOSE_PORT_INTERRUPT	0x04000000
#define SBE_2T3E3_21143_VAL_ERROR_BITS				0x03800000
#define SBE_2T3E3_21143_VAL_PARITY_ERROR			0x00000000
#define SBE_2T3E3_21143_VAL_MASTER_ABORT			0x00800000
#define SBE_2T3E3_21143_VAL_TARGET_ABORT			0x01000000
#define SBE_2T3E3_21143_VAL_TRANSMISSION_PROCESS_STATE		0x00700000
#define SBE_2T3E3_21143_VAL_TX_STOPPED				0x00000000
#define SBE_2T3E3_21143_VAL_TX_SUSPENDED			0x00600000
#define SBE_2T3E3_21143_VAL_RECEIVE_PROCESS_STATE		0x000e0000
#define SBE_2T3E3_21143_VAL_RX_STOPPED				0x00000000
#define SBE_2T3E3_21143_VAL_RX_SUSPENDED			0x000a0000
#define SBE_2T3E3_21143_VAL_NORMAL_INTERRUPT_SUMMARY		0x00010000
#define SBE_2T3E3_21143_VAL_ABNORMAL_INTERRUPT_SUMMARY		0x00008000
#define SBE_2T3E3_21143_VAL_EARLY_RECEIVE_INTERRUPT		0x00004000
#define SBE_2T3E3_21143_VAL_FATAL_BUS_ERROR			0x00002000
#define SBE_2T3E3_21143_VAL_GENERAL_PURPOSE_TIMER_EXPIRED	0x00000800
#define SBE_2T3E3_21143_VAL_EARLY_TRANSMIT_INTERRUPT		0x00000400
#define SBE_2T3E3_21143_VAL_RECEIVE_WATCHDOG_TIMEOUT		0x00000200
#define SBE_2T3E3_21143_VAL_RECEIVE_PROCESS_STOPPED		0x00000100
#define SBE_2T3E3_21143_VAL_RECEIVE_BUFFER_UNAVAILABLE		0x00000080
#define SBE_2T3E3_21143_VAL_RECEIVE_INTERRUPT			0x00000040
#define SBE_2T3E3_21143_VAL_TRANSMIT_UNDERFLOW			0x00000020
#define SBE_2T3E3_21143_VAL_TRANSMIT_JABBER_TIMEOUT		0x00000008
#define SBE_2T3E3_21143_VAL_TRANSMIT_BUFFER_UNAVAILABLE		0x00000004
#define SBE_2T3E3_21143_VAL_TRANSMIT_PROCESS_STOPPED		0x00000002
#define SBE_2T3E3_21143_VAL_TRANSMIT_INTERRUPT			0x00000001

/* CSR6 - OPERATION_MODE */
#define SBE_2T3E3_21143_VAL_SPECIAL_CAPTURE_EFFECT_ENABLE	0x80000000
#define SBE_2T3E3_21143_VAL_RECEIVE_ALL				0x40000000
#define SBE_2T3E3_21143_VAL_MUST_BE_ONE				0x02000000
#define SBE_2T3E3_21143_VAL_SCRAMBLER_MODE			0x01000000
#define SBE_2T3E3_21143_VAL_PCS_FUNCTION			0x00800000
#define SBE_2T3E3_21143_VAL_TRANSMIT_THRESHOLD_MODE_10Mbs	0x00400000
#define SBE_2T3E3_21143_VAL_TRANSMIT_THRESHOLD_MODE_100Mbs	0x00000000
#define SBE_2T3E3_21143_VAL_STORE_AND_FORWARD			0x00200000
#define SBE_2T3E3_21143_VAL_HEARTBEAT_DISABLE			0x00080000
#define SBE_2T3E3_21143_VAL_PORT_SELECT				0x00040000
#define SBE_2T3E3_21143_VAL_CAPTURE_EFFECT_ENABLE		0x00020000
#define SBE_2T3E3_21143_VAL_THRESHOLD_CONTROL_BITS		0x0000c000
#define SBE_2T3E3_21143_VAL_THRESHOLD_CONTROL_BITS_1		0x00000000
#define SBE_2T3E3_21143_VAL_THRESHOLD_CONTROL_BITS_2		0x00004000
#define SBE_2T3E3_21143_VAL_THRESHOLD_CONTROL_BITS_3		0x00008000
#define SBE_2T3E3_21143_VAL_THRESHOLD_CONTROL_BITS_4		0x0000c000
#define SBE_2T3E3_21143_VAL_TRANSMISSION_START			0x00002000
#define SBE_2T3E3_21143_VAL_OPERATING_MODE			0x00000c00
#define SBE_2T3E3_21143_VAL_LOOPBACK_OFF			0x00000000
#define SBE_2T3E3_21143_VAL_LOOPBACK_EXTERNAL			0x00000800
#define SBE_2T3E3_21143_VAL_LOOPBACK_INTERNAL			0x00000400
#define SBE_2T3E3_21143_VAL_FULL_DUPLEX_MODE			0x00000200
#define SBE_2T3E3_21143_VAL_PASS_ALL_MULTICAST			0x00000080
#define SBE_2T3E3_21143_VAL_PROMISCUOUS_MODE			0x00000040
#define SBE_2T3E3_21143_VAL_PASS_BAD_FRAMES			0x00000008
#define SBE_2T3E3_21143_VAL_RECEIVE_START			0x00000002

/* CSR7 - INTERRUPT_ENABLE */
#define SBE_2T3E3_21143_VAL_LINK_CHANGED_ENABLE			0x08000000
#define SBE_2T3E3_21143_VAL_GENERAL_PURPOSE_PORT_ENABLE		0x04000000
#define SBE_2T3E3_21143_VAL_NORMAL_INTERRUPT_SUMMARY_ENABLE	0x00010000
#define SBE_2T3E3_21143_VAL_ABNORMAL_INTERRUPT_SUMMARY_ENABLE	0x00008000
#define SBE_2T3E3_21143_VAL_EARLY_RECEIVE_INTERRUPT_ENABLE	0x00004000
#define SBE_2T3E3_21143_VAL_FATAL_BUS_ERROR_ENABLE		0x00002000
#define SBE_2T3E3_21143_VAL_LINK_FAIL_ENABLE			0x00001000
#define SBE_2T3E3_21143_VAL_GENERAL_PURPOSE_TIMER_ENABLE	0x00000800
#define SBE_2T3E3_21143_VAL_EARLY_TRANSMIT_INTERRUPT_ENABLE	0x00000400
#define SBE_2T3E3_21143_VAL_RECEIVE_WATCHDOG_TIMEOUT_ENABLE	0x00000200
#define SBE_2T3E3_21143_VAL_RECEIVE_STOPPED_ENABLE		0x00000100
#define SBE_2T3E3_21143_VAL_RECEIVE_BUFFER_UNAVAILABLE_ENABLE	0x00000080
#define SBE_2T3E3_21143_VAL_RECEIVE_INTERRUPT_ENABLE		0x00000040
#define SBE_2T3E3_21143_VAL_TRANSMIT_UNDERFLOW_INTERRUPT_ENABLE	0x00000020
#define SBE_2T3E3_21143_VAL_TRANSMIT_JABBER_TIMEOUT_ENABLE	0x00000008
#define SBE_2T3E3_21143_VAL_TRANSMIT_BUFFER_UNAVAILABLE_ENABLE	0x00000004
#define SBE_2T3E3_21143_VAL_TRANSMIT_STOPPED_ENABLE		0x00000002
#define SBE_2T3E3_21143_VAL_TRANSMIT_INTERRUPT_ENABLE		0x00000001

/* CSR8 - MISSED_FRAMES_AND_OVERFLOW_COUNTER */
#define SBE_2T3E3_21143_VAL_OVERFLOW_COUNTER_OVERFLOW		0x10000000
#define SBE_2T3E3_21143_VAL_OVERFLOW_COUNTER			0x0ffe0000
#define SBE_2T3E3_21143_VAL_MISSED_FRAME_OVERFLOW		0x00010000
#define SBE_2T3E3_21143_VAL_MISSED_FRAMES_COUNTER		0x0000ffff

/* CSR9 - BOOT_ROM_SERIAL_ROM_AND_MII_MANAGEMENT */
#define SBE_2T3E3_21143_VAL_MII_MANAGEMENT_DATA_IN		0x00080000
#define SBE_2T3E3_21143_VAL_MII_MANAGEMENT_READ_MODE		0x00040000
#define SBE_2T3E3_21143_VAL_MII_MANAGEMENT_DATA_OUT		0x00020000
#define SBE_2T3E3_21143_VAL_MII_MANAGEMENT_CLOCK		0x00010000
#define SBE_2T3E3_21143_VAL_READ_OPERATION			0x00004000
#define SBE_2T3E3_21143_VAL_WRITE_OPERATION			0x00002000
#define SBE_2T3E3_21143_VAL_BOOT_ROM_SELECT			0x00001000
#define SBE_2T3E3_21143_VAL_SERIAL_ROM_SELECT			0x00000800
#define SBE_2T3E3_21143_VAL_BOOT_ROM_DATA			0x000000ff
#define SBE_2T3E3_21143_VAL_SERIAL_ROM_DATA_OUT			0x00000008
#define SBE_2T3E3_21143_VAL_SERIAL_ROM_DATA_IN			0x00000004
#define SBE_2T3E3_21143_VAL_SERIAL_ROM_CLOCK			0x00000002
#define SBE_2T3E3_21143_VAL_SERIAL_ROM_CHIP_SELECT		0x00000001

/* CSR11 - GENERAL_PURPOSE_TIMER_AND_INTERRUPT_MITIGATION_CONTROL */
#define SBE_2T3E3_21143_VAL_CYCLE_SIZE				0x80000000
#define SBE_2T3E3_21143_VAL_TRANSMIT_TIMER			0x78000000
#define SBE_2T3E3_21143_VAL_NUMBER_OF_TRANSMIT_PACKETS		0x07000000
#define SBE_2T3E3_21143_VAL_RECEIVE_TIMER			0x00f00000
#define SBE_2T3E3_21143_VAL_NUMBER_OF_RECEIVE_PACKETS		0x000e0000
#define SBE_2T3E3_21143_VAL_CONTINUOUS_MODE			0x00010000
#define SBE_2T3E3_21143_VAL_TIMER_VALUE				0x0000ffff

/* CSR12 - SIA_STATUS */
#define SBE_2T3E3_21143_VAL_10BASE_T_RECEIVE_PORT_ACTIVITY	0x00000200
#define SBE_2T3E3_21143_VAL_AUI_RECEIVE_PORT_ACTIVITY		0x00000100
#define SBE_2T3E3_21143_VAL_10Mbs_LINK_STATUS			0x00000004
#define SBE_2T3E3_21143_VAL_100Mbs_LINK_STATUS			0x00000002
#define SBE_2T3E3_21143_VAL_MII_RECEIVE_PORT_ACTIVITY		0x00000001

/* CSR13 - SIA_CONNECTIVITY */
#define SBE_2T3E3_21143_VAL_10BASE_T_OR_AUI			0x00000008
#define SBE_2T3E3_21143_VAL_SIA_RESET				0x00000001

/* CSR14 - SIA_TRANSMIT_AND_RECEIVE */
#define SBE_2T3E3_21143_VAL_100BASE_TX_FULL_DUPLEX		0x00020000
#define SBE_2T3E3_21143_VAL_COLLISION_DETECT_ENABLE		0x00000400
#define SBE_2T3E3_21143_VAL_COLLISION_SQUELCH_ENABLE		0x00000200
#define SBE_2T3E3_21143_VAL_RECEIVE_SQUELCH_ENABLE		0x00000100
#define SBE_2T3E3_21143_VAL_LINK_PULSE_SEND_ENABLE		0x00000004
#define SBE_2T3E3_21143_VAL_ENCODER_ENABLE			0x00000001

/* CSR15 - SIA_AND_GENERAL_PURPOSE_PORT */
#define SBE_2T3E3_21143_VAL_RECEIVE_WATCHDOG_DISABLE		0x00000010
#define SBE_2T3E3_21143_VAL_AUI_BNC_MODE			0x00000008
#define SBE_2T3E3_21143_VAL_HOST_UNJAB				0x00000002
#define SBE_2T3E3_21143_VAL_JABBER_DISABLE			0x00000001

/**************************************************************
 *  CPLD
 **************************************************************/

/* reg_map indexes */
#define SBE_2T3E3_CPLD_REG_PCRA				0
#define SBE_2T3E3_CPLD_REG_PCRB				1
#define SBE_2T3E3_CPLD_REG_PLCR				2
#define SBE_2T3E3_CPLD_REG_PLTR				3
#define SBE_2T3E3_CPLD_REG_PPFR				4
#define SBE_2T3E3_CPLD_REG_BOARD_ID			5
#define SBE_2T3E3_CPLD_REG_FPGA_VERSION			6
#define SBE_2T3E3_CPLD_REG_FRAMER_BASE_ADDRESS		7
#define SBE_2T3E3_CPLD_REG_SERIAL_CHIP_SELECT		8
#define SBE_2T3E3_CPLD_REG_STATIC_RESET			9
#define SBE_2T3E3_CPLD_REG_PULSE_RESET			10
#define SBE_2T3E3_CPLD_REG_FPGA_RECONFIGURATION		11
#define SBE_2T3E3_CPLD_REG_LEDR				12
#define SBE_2T3E3_CPLD_REG_PICSR			13
#define SBE_2T3E3_CPLD_REG_PIER				14
#define SBE_2T3E3_CPLD_REG_PCRC				15
#define SBE_2T3E3_CPLD_REG_PBWF				16
#define SBE_2T3E3_CPLD_REG_PBWL				17

#define SBE_2T3E3_CPLD_REG_MAX				18

/**********/

/* val_map indexes */
#define SBE_2T3E3_CPLD_VAL_LIU_SELECT			0
#define SBE_2T3E3_CPLD_VAL_DAC_SELECT			1
#define SBE_2T3E3_CPLD_VAL_LOOP_TIMING_SOURCE		2
#define SBE_2T3E3_CPLD_VAL_LIU_FRAMER_RESET		3

/* PCRA */
#define SBE_2T3E3_CPLD_VAL_CRC32				0x40
#define SBE_2T3E3_CPLD_VAL_TRANSPARENT_MODE			0x20
#define SBE_2T3E3_CPLD_VAL_REAR_PANEL				0x10
#define SBE_2T3E3_CPLD_VAL_RAW_MODE				0x08
#define SBE_2T3E3_CPLD_VAL_ALT					0x04
#define SBE_2T3E3_CPLD_VAL_LOOP_TIMING				0x02
#define SBE_2T3E3_CPLD_VAL_LOCAL_CLOCK_E3			0x01

/* PCRB */
#define SBE_2T3E3_CPLD_VAL_PAD_COUNT				0x30
#define SBE_2T3E3_CPLD_VAL_PAD_COUNT_1				0x00
#define SBE_2T3E3_CPLD_VAL_PAD_COUNT_2				0x10
#define SBE_2T3E3_CPLD_VAL_PAD_COUNT_3				0x20
#define SBE_2T3E3_CPLD_VAL_PAD_COUNT_4				0x30
#define SBE_2T3E3_CPLD_VAL_SCRAMBLER_TYPE			0x02
#define SBE_2T3E3_CPLD_VAL_SCRAMBLER_ENABLE			0x01

/* PCRC */
#define SBE_2T3E3_CPLD_VAL_FRACTIONAL_MODE_NONE			0x00
#define SBE_2T3E3_CPLD_VAL_FRACTIONAL_MODE_0			0x01
#define SBE_2T3E3_CPLD_VAL_FRACTIONAL_MODE_1			0x11
#define SBE_2T3E3_CPLD_VAL_FRACTIONAL_MODE_2			0x21

/* PLTR */
#define SBE_2T3E3_CPLD_VAL_LCV_COUNTER				0xff

/* SCSR */
#define SBE_2T3E3_CPLD_VAL_EEPROM_SELECT			0x10

/* PICSR */
#define SBE_2T3E3_CPLD_VAL_LOSS_OF_SIGNAL_THRESHOLD_LEVEL_1	0x80
#define SBE_2T3E3_CPLD_VAL_RECEIVE_LOSS_OF_SIGNAL_CHANGE	0x40
#define SBE_2T3E3_CPLD_VAL_INTERRUPT_FROM_ETHERNET_ASSERTED	0x20
#define SBE_2T3E3_CPLD_VAL_INTERRUPT_FROM_FRAMER_ASSERTED	0x10
#define SBE_2T3E3_CPLD_VAL_LCV_LIMIT_EXCEEDED			0x08
#define SBE_2T3E3_CPLD_VAL_DMO_SIGNAL_DETECTED			0x04
#define SBE_2T3E3_CPLD_VAL_RECEIVE_LOSS_OF_LOCK_DETECTED	0x02
#define SBE_2T3E3_CPLD_VAL_RECEIVE_LOSS_OF_SIGNAL_DETECTED	0x01

/* PIER */
#define SBE_2T3E3_CPLD_VAL_RECEIVE_LOS_CHANGE_ENABLE		0x40
#define SBE_2T3E3_CPLD_VAL_INTERRUPT_FROM_ETHERNET_ENABLE	0x20
#define SBE_2T3E3_CPLD_VAL_INTERRUPT_FROM_FRAMER_ENABLE		0x10
#define SBE_2T3E3_CPLD_VAL_LCV_INTERRUPT_ENABLE			0x08
#define SBE_2T3E3_CPLD_VAL_DMO_ENABLE				0x04
#define SBE_2T3E3_CPLD_VAL_RECEIVE_LOSS_OF_LOCK_ENABLE		0x02
#define SBE_2T3E3_CPLD_VAL_RECEIVE_LOSS_OF_SIGNAL_ENABLE	0x01

/**************************************************************
 *  Framer
 **************************************************************/

/* reg_map indexes */
/* common */
#define SBE_2T3E3_FRAMER_REG_OPERATING_MODE				0
#define SBE_2T3E3_FRAMER_REG_IO_CONTROL					1
#define SBE_2T3E3_FRAMER_REG_BLOCK_INTERRUPT_ENABLE			2
#define SBE_2T3E3_FRAMER_REG_BLOCK_INTERRUPT_STATUS			3
#define SBE_2T3E3_FRAMER_REG_PMON_LCV_EVENT_COUNT_MSB			28
#define SBE_2T3E3_FRAMER_REG_PMON_LCV_EVENT_COUNT_LSB			29
#define SBE_2T3E3_FRAMER_REG_PMON_FRAMING_BIT_ERROR_EVENT_COUNT_MSB	30
#define SBE_2T3E3_FRAMER_REG_PMON_FRAMING_BIT_ERROR_EVENT_COUNT_LSB	31
#define SBE_2T3E3_FRAMER_REG_PMON_PARITY_ERROR_EVENT_COUNT_MSB		32
#define SBE_2T3E3_FRAMER_REG_PMON_PARITY_ERROR_EVENT_COUNT_LSB		33
#define SBE_2T3E3_FRAMER_REG_PMON_FEBE_EVENT_COUNT_MSB			34
#define SBE_2T3E3_FRAMER_REG_PMON_FEBE_EVENT_COUNT_LSB			35
#define SBE_2T3E3_FRAMER_REG_PMON_CP_BIT_ERROR_EVENT_COUNT_MSB		36
#define SBE_2T3E3_FRAMER_REG_PMON_CP_BIT_ERROR_EVENT_COUNT_LSB		37
#define SBE_2T3E3_FRAMER_REG_PMON_HOLDING_REGISTER			38
#define SBE_2T3E3_FRAMER_REG_ONE_SECOND_ERROR_STATUS			39
#define SBE_2T3E3_FRAMER_REG_LCV_ONE_SECOND_ACCUMULATOR_MSB		40
#define SBE_2T3E3_FRAMER_REG_LCV_ONE_SECOND_ACCUMULATOR_LSB		41
#define SBE_2T3E3_FRAMER_REG_FRAME_PARITY_ERROR_ONE_SECOND_ACCUMULATOR_MSB  42
#define SBE_2T3E3_FRAMER_REG_FRAME_PARITY_ERROR_ONE_SECOND_ACCUMULATOR_LSB  43
#define SBE_2T3E3_FRAMER_REG_FRAME_CP_BIT_ERROR_ONE_SECOND_ACCUMULATOR_MSB  44
#define SBE_2T3E3_FRAMER_REG_FRAME_CP_BIT_ERROR_ONE_SECOND_ACCUMULATOR_LSB  45
#define SBE_2T3E3_FRAMER_REG_LINE_INTERFACE_DRIVE			46
#define SBE_2T3E3_FRAMER_REG_LINE_INTERFACE_SCAN			47

/* T3 */
#define SBE_2T3E3_FRAMER_REG_T3_RX_CONFIGURATION_STATUS			4
#define SBE_2T3E3_FRAMER_REG_T3_RX_STATUS				5
#define SBE_2T3E3_FRAMER_REG_T3_RX_INTERRUPT_ENABLE			6
#define SBE_2T3E3_FRAMER_REG_T3_RX_INTERRUPT_STATUS			7
#define SBE_2T3E3_FRAMER_REG_T3_RX_SYNC_DETECT_ENABLE			8
#define SBE_2T3E3_FRAMER_REG_T3_RX_FEAC					10
#define SBE_2T3E3_FRAMER_REG_T3_RX_FEAC_INTERRUPT_ENABLE_STATUS		11
#define SBE_2T3E3_FRAMER_REG_T3_RX_LAPD_CONTROL				12
#define SBE_2T3E3_FRAMER_REG_T3_RX_LAPD_STATUS				13
#define SBE_2T3E3_FRAMER_REG_T3_TX_CONFIGURATION			16
#define SBE_2T3E3_FRAMER_REG_T3_TX_FEAC_CONFIGURATION_STATUS		17
#define SBE_2T3E3_FRAMER_REG_T3_TX_FEAC					18
#define SBE_2T3E3_FRAMER_REG_T3_TX_LAPD_CONFIGURATION			19
#define SBE_2T3E3_FRAMER_REG_T3_TX_LAPD_STATUS				20
#define SBE_2T3E3_FRAMER_REG_T3_TX_MBIT_MASK				21
#define SBE_2T3E3_FRAMER_REG_T3_TX_FBIT_MASK				22
#define SBE_2T3E3_FRAMER_REG_T3_TX_FBIT_MASK_2				23
#define SBE_2T3E3_FRAMER_REG_T3_TX_FBIT_MASK_3				24

/* E3 */
#define SBE_2T3E3_FRAMER_REG_E3_RX_CONFIGURATION_STATUS_1		4
#define SBE_2T3E3_FRAMER_REG_E3_RX_CONFIGURATION_STATUS_2		5
#define SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_ENABLE_1			6
#define SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_ENABLE_2			7
#define SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_STATUS_1			8
#define SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_STATUS_2			9
#define SBE_2T3E3_FRAMER_REG_E3_RX_LAPD_CONTROL				12
#define SBE_2T3E3_FRAMER_REG_E3_RX_LAPD_STATUS				13
#define SBE_2T3E3_FRAMER_REG_E3_RX_NR_BYTE				14
#define SBE_2T3E3_FRAMER_REG_E3_RX_SERVICE_BITS				14
#define SBE_2T3E3_FRAMER_REG_E3_RX_GC_BYTE				15
#define SBE_2T3E3_FRAMER_REG_E3_TX_CONFIGURATION			16
#define SBE_2T3E3_FRAMER_REG_E3_TX_LAPD_CONFIGURATION			19
#define SBE_2T3E3_FRAMER_REG_E3_TX_LAPD_STATUS				19
#define SBE_2T3E3_FRAMER_REG_E3_TX_GC_BYTE				21
#define SBE_2T3E3_FRAMER_REG_E3_TX_SERVICE_BITS				21
#define SBE_2T3E3_FRAMER_REG_E3_TX_MA_BYTE				22
#define SBE_2T3E3_FRAMER_REG_E3_TX_NR_BYTE				23
#define SBE_2T3E3_FRAMER_REG_E3_TX_FA1_ERROR_MASK			25
#define SBE_2T3E3_FRAMER_REG_E3_TX_FAS_ERROR_MASK_UPPER			25
#define SBE_2T3E3_FRAMER_REG_E3_TX_FA2_ERROR_MASK			26
#define SBE_2T3E3_FRAMER_REG_E3_TX_FAS_ERROR_MASK_LOWER			26
#define SBE_2T3E3_FRAMER_REG_E3_TX_BIP8_MASK				27
#define SBE_2T3E3_FRAMER_REG_E3_TX_BIP4_MASK				27

#define SBE_2T3E3_FRAMER_REG_MAX					48

/**********/

/* OPERATING_MODE */
#define SBE_2T3E3_FRAMER_VAL_LOCAL_LOOPBACK_MODE		0x80
#define SBE_2T3E3_FRAMER_VAL_T3_E3_SELECT			0x40
#define SBE_2T3E3_FRAMER_VAL_INTERNAL_LOS_ENABLE		0x20
#define SBE_2T3E3_FRAMER_VAL_RESET				0x10
#define SBE_2T3E3_FRAMER_VAL_INTERRUPT_ENABLE_RESET		0x08
#define SBE_2T3E3_FRAMER_VAL_FRAME_FORMAT_SELECT		0x04
#define SBE_2T3E3_FRAMER_VAL_TIMING_ASYNCH_TXINCLK		0x03
#define SBE_2T3E3_FRAMER_VAL_E3_G751				0x00
#define SBE_2T3E3_FRAMER_VAL_E3_G832				0x04
#define SBE_2T3E3_FRAMER_VAL_T3_CBIT				0x40
#define SBE_2T3E3_FRAMER_VAL_T3_M13				0x44
#define SBE_2T3E3_FRAMER_VAL_LOOPBACK_ON			0x80
#define SBE_2T3E3_FRAMER_VAL_LOOPBACK_OFF			0x00

/* IO_CONTROL */
#define SBE_2T3E3_FRAMER_VAL_DISABLE_TX_LOSS_OF_CLOCK		0x80
#define SBE_2T3E3_FRAMER_VAL_LOSS_OF_CLOCK_STATUS		0x40
#define SBE_2T3E3_FRAMER_VAL_DISABLE_RX_LOSS_OF_CLOCK		0x20
#define SBE_2T3E3_FRAMER_VAL_AMI_LINE_CODE			0x10
#define SBE_2T3E3_FRAMER_VAL_UNIPOLAR				0x08
#define SBE_2T3E3_FRAMER_VAL_TX_LINE_CLOCK_INVERT		0x04
#define SBE_2T3E3_FRAMER_VAL_RX_LINE_CLOCK_INVERT		0x02
#define SBE_2T3E3_FRAMER_VAL_REFRAME				0x01

/* BLOCK_INTERRUPT_ENABLE */
#define SBE_2T3E3_FRAMER_VAL_RX_INTERRUPT_ENABLE		0x80
#define SBE_2T3E3_FRAMER_VAL_TX_INTERRUPT_ENABLE		0x02
#define SBE_2T3E3_FRAMER_VAL_ONE_SECOND_INTERRUPT_ENABLE	0x01

/* BLOCK_INTERRUPT_STATUS */
#define SBE_2T3E3_FRAMER_VAL_RX_INTERRUPT_STATUS		0x80
#define SBE_2T3E3_FRAMER_VAL_TX_INTERRUPT_STATUS		0x02
#define SBE_2T3E3_FRAMER_VAL_ONE_SECOND_INTERRUPT_STATUS	0x01

/**********/

/* T3_RX_CONFIGURATION_STATUS */
#define SBE_2T3E3_FRAMER_VAL_T3_RX_AIS				0x80
#define SBE_2T3E3_FRAMER_VAL_T3_RX_LOS				0x40
#define SBE_2T3E3_FRAMER_VAL_T3_RX_IDLE				0x20
#define SBE_2T3E3_FRAMER_VAL_T3_RX_OOF				0x10
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FRAMING_ON_PARITY		0x04
#define SBE_2T3E3_FRAMER_VAL_T3_RX_F_SYNC_ALGO			0x02
#define SBE_2T3E3_FRAMER_VAL_T3_RX_M_SYNC_ALGO			0x01

/* T3_RX_STATUS */
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FERF				0x10
#define SBE_2T3E3_FRAMER_VAL_T3_RX_AIC				0x04
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FEBE				0x07

/* T3_RX_INTERRUPT_ENABLE */
#define SBE_2T3E3_FRAMER_VAL_T3_RX_CP_BIT_ERROR_INTERRUPT_ENABLE 0x80
#define SBE_2T3E3_FRAMER_VAL_T3_RX_LOS_INTERRUPT_ENABLE		0x40
#define SBE_2T3E3_FRAMER_VAL_T3_RX_AIS_INTERRUPT_ENABLE		0x20
#define SBE_2T3E3_FRAMER_VAL_T3_RX_IDLE_INTERRUPT_ENABLE	0x10
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FERF_INTERRUPT_ENABLE	0x08
#define SBE_2T3E3_FRAMER_VAL_T3_RX_AIC_INTERRUPT_ENABLE		0x04
#define SBE_2T3E3_FRAMER_VAL_T3_RX_OOF_INTERRUPT_ENABLE		0x02
#define SBE_2T3E3_FRAMER_VAL_T3_RX_P_BIT_INTERRUPT_ENABLE	0x01

/* T3_RX_INTERRUPT_STATUS */
#define SBE_2T3E3_FRAMER_VAL_T3_RX_CP_BIT_ERROR_INTERRUPT_STATUS 0x80
#define SBE_2T3E3_FRAMER_VAL_T3_RX_LOS_INTERRUPT_STATUS		0x40
#define SBE_2T3E3_FRAMER_VAL_T3_RX_AIS_INTERRUPT_STATUS		0x20
#define SBE_2T3E3_FRAMER_VAL_T3_RX_IDLE_INTERRUPT_STATUS	0x10
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FERF_INTERRUPT_STATUS	0x08
#define SBE_2T3E3_FRAMER_VAL_T3_RX_AIC_INTERRUPT_STATUS		0x04
#define SBE_2T3E3_FRAMER_VAL_T3_RX_OOF_INTERRUPT_STATUS		0x02
#define SBE_2T3E3_FRAMER_VAL_T3_RX_P_BIT_INTERRUPT_STATUS	0x01

/* T3_RX_FEAC_INTERRUPT_ENABLE_STATUS */
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FEAC_VALID			0x10
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FEAC_REMOVE_INTERRUPT_ENABLE	0x08
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FEAC_REMOVE_INTERRUPT_STATUS	0x04
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FEAC_VALID_INTERRUPT_ENABLE	0x02
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FEAC_VALID_INTERRUPT_STATUS	0x01

/* T3_RX_LAPD_CONTROL */
#define SBE_2T3E3_FRAMER_VAL_T3_RX_LAPD_ENABLE			0x04
#define SBE_2T3E3_FRAMER_VAL_T3_RX_LAPD_INTERRUPT_ENABLE	0x02
#define SBE_2T3E3_FRAMER_VAL_T3_RX_LAPD_INTERRUPT_STATUS	0x01

/* T3_RX_LAPD_STATUS */
#define SBE_2T3E3_FRAMER_VAL_T3_RX_ABORT			0x40
#define SBE_2T3E3_FRAMER_VAL_T3_RX_LAPD_TYPE			0x30
#define SBE_2T3E3_FRAMER_VAL_T3_RX_CR_TYPE			0x08
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FCS_ERROR			0x04
#define SBE_2T3E3_FRAMER_VAL_T3_RX_END_OF_MESSAGE		0x02
#define SBE_2T3E3_FRAMER_VAL_T3_RX_FLAG_PRESENT			0x01

/* T3_TX_CONFIGURATION */
#define SBE_2T3E3_FRAMER_VAL_T3_TX_YELLOW_ALARM			0x80
#define SBE_2T3E3_FRAMER_VAL_T3_TX_X_BIT			0x40
#define SBE_2T3E3_FRAMER_VAL_T3_TX_IDLE				0x20
#define SBE_2T3E3_FRAMER_VAL_T3_TX_AIS				0x10
#define SBE_2T3E3_FRAMER_VAL_T3_TX_LOS				0x08
#define SBE_2T3E3_FRAMER_VAL_T3_TX_FERF_ON_LOS			0x04
#define SBE_2T3E3_FRAMER_VAL_T3_TX_FERF_ON_OOF			0x02
#define SBE_2T3E3_FRAMER_VAL_T3_TX_FERF_ON_AIS			0x01

/* T3_TX_FEAC_CONFIGURATION_STATUS */
#define SBE_2T3E3_FRAMER_VAL_T3_TX_FEAC_INTERRUPT_ENABLE	0x10
#define SBE_2T3E3_FRAMER_VAL_T3_TX_FEAC_INTERRUPT_STATUS	0x08
#define SBE_2T3E3_FRAMER_VAL_T3_TX_FEAC_ENABLE			0x04
#define SBE_2T3E3_FRAMER_VAL_T3_TX_FEAC_GO			0x02
#define SBE_2T3E3_FRAMER_VAL_T3_TX_FEAC_BUSY			0x01

/* T3_TX_LAPD_STATUS */
#define SBE_2T3E3_FRAMER_VAL_T3_TX_DL_START			0x08
#define SBE_2T3E3_FRAMER_VAL_T3_TX_DL_BUSY			0x04
#define SBE_2T3E3_FRAMER_VAL_T3_TX_LAPD_INTERRUPT_ENABLE	0x02
#define SBE_2T3E3_FRAMER_VAL_T3_TX_LAPD_INTERRUPT_STATUS	0x01

/**********/

/* E3_RX_CONFIGURATION_STATUS_1 */
#define SBE_2T3E3_FRAMER_VAL_E3_RX_PAYLOAD_TYPE			0xe0
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FERF_ALGO			0x10
#define SBE_2T3E3_FRAMER_VAL_E3_RX_T_MARK_ALGO			0x08
#define SBE_2T3E3_FRAMER_VAL_E3_RX_PAYLOAD_EXPECTED		0x07
#define SBE_2T3E3_FRAMER_VAL_E3_RX_BIP4				0x01

/* E3_RX_CONFIGURATION_STATUS_2 */
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LOF_ALGO			0x80
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LOF				0x40
#define SBE_2T3E3_FRAMER_VAL_E3_RX_OOF				0x20
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LOS				0x10
#define SBE_2T3E3_FRAMER_VAL_E3_RX_AIS				0x08
#define SBE_2T3E3_FRAMER_VAL_E3_RX_PAYLOAD_UNSTABLE		0x04
#define SBE_2T3E3_FRAMER_VAL_E3_RX_T_MARK			0x02
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FERF				0x01

/* E3_RX_INTERRUPT_ENABLE_1 */
#define SBE_2T3E3_FRAMER_VAL_E3_RX_COFA_INTERRUPT_ENABLE	0x10
#define SBE_2T3E3_FRAMER_VAL_E3_RX_OOF_INTERRUPT_ENABLE		0x08
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LOF_INTERRUPT_ENABLE		0x04
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LOS_INTERRUPT_ENABLE		0x02
#define SBE_2T3E3_FRAMER_VAL_E3_RX_AIS_INTERRUPT_ENABLE		0x01

/* E3_RX_INTERRUPT_ENABLE_2 */
#define SBE_2T3E3_FRAMER_VAL_E3_RX_TTB_CHANGE_INTERRUPT_ENABLE	0x40
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FEBE_INTERRUPT_ENABLE	0x10
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FERF_INTERRUPT_ENABLE	0x08
#define SBE_2T3E3_FRAMER_VAL_E3_RX_BIP8_ERROR_INTERRUPT_ENABLE	0x04
#define SBE_2T3E3_FRAMER_VAL_E3_RX_BIP4_ERROR_INTERRUPT_ENABLE	0x04
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FRAMING_BYTE_ERROR_INTERRUPT_ENABLE 0x02
#define SBE_2T3E3_FRAMER_VAL_E3_RX_PAYLOAD_MISMATCH_INTERRUPT_ENABLE 0x01

/* E3_RX_INTERRUPT_STATUS_1 */
#define SBE_2T3E3_FRAMER_VAL_E3_RX_COFA_INTERRUPT_STATUS	0x10
#define SBE_2T3E3_FRAMER_VAL_E3_RX_OOF_INTERRUPT_STATUS		0x08
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LOF_INTERRUPT_STATUS		0x04
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LOS_INTERRUPT_STATUS		0x02
#define SBE_2T3E3_FRAMER_VAL_E3_RX_AIS_INTERRUPT_STATUS		0x01

/* E3_RX_INTERRUPT_STATUS_2 */
#define SBE_2T3E3_FRAMER_VAL_E3_RX_TTB_CHANGE_INTERRUPT_STATUS	0x40
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FEBE_INTERRUPT_STATUS	0x10
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FERF_INTERRUPT_STATUS	0x08
#define SBE_2T3E3_FRAMER_VAL_E3_RX_BIP8_ERROR_INTERRUPT_STATUS	0x04
#define SBE_2T3E3_FRAMER_VAL_E3_RX_BIP4_ERROR_INTERRUPT_STATUS	0x04
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FRAMING_BYTE_ERROR_INTERRUPT_STATUS 0x02
#define SBE_2T3E3_FRAMER_VAL_E3_RX_PAYLOAD_MISMATCH_INTERRUPT_STATUS 0x01

/* E3_RX_LAPD_CONTROL */
#define SBE_2T3E3_FRAMER_VAL_E3_RX_DL_FROM_NR			0x08
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LAPD_ENABLE			0x04
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LAPD_INTERRUPT_ENABLE	0x02
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LAPD_INTERRUPT_STATUS	0x01

/* E3_RX_LAPD_STATUS */
#define SBE_2T3E3_FRAMER_VAL_E3_RX_ABORT			0x40
#define SBE_2T3E3_FRAMER_VAL_E3_RX_LAPD_TYPE			0x30
#define SBE_2T3E3_FRAMER_VAL_E3_RX_CR_TYPE			0x08
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FCS_ERROR			0x04
#define SBE_2T3E3_FRAMER_VAL_E3_RX_END_OF_MESSAGE		0x02
#define SBE_2T3E3_FRAMER_VAL_E3_RX_FLAG_PRESENT			0x01

/* E3_TX_CONFIGURATION */
#define SBE_2T3E3_FRAMER_VAL_E3_TX_BIP4_ENABLE			0x80
#define SBE_2T3E3_FRAMER_VAL_E3_TX_A_SOURCE_SELECT		0x60
#define SBE_2T3E3_FRAMER_VAL_E3_TX_DL_IN_NR			0x10
#define SBE_2T3E3_FRAMER_VAL_E3_TX_N_SOURCE_SELECT		0x18
#define SBE_2T3E3_FRAMER_VAL_E3_TX_AIS_ENABLE			0x04
#define SBE_2T3E3_FRAMER_VAL_E3_TX_LOS_ENABLE			0x02
#define SBE_2T3E3_FRAMER_VAL_E3_TX_MA_RX			0x01
#define SBE_2T3E3_FRAMER_VAL_E3_TX_FAS_SOURCE_SELECT		0x01

/* E3_TX_LAPD_CONFIGURATION */
#define SBE_2T3E3_FRAMER_VAL_E3_TX_AUTO_RETRANSMIT		0x08
#define SBE_2T3E3_FRAMER_VAL_E3_TX_LAPD_MESSAGE_LENGTH		0x02
#define SBE_2T3E3_FRAMER_VAL_E3_TX_LAPD_ENABLE			0x01

/* E3_TX_LAPD_STATUS_INTERRUPT */
#define SBE_2T3E3_FRAMER_VAL_E3_TX_DL_START			0x08
#define SBE_2T3E3_FRAMER_VAL_E3_TX_DL_BUSY			0x04
#define SBE_2T3E3_FRAMER_VAL_E3_TX_LAPD_INTERRUPT_ENABLE	0x02
#define SBE_2T3E3_FRAMER_VAL_E3_TX_LAPD_INTERRUPT_STATUS	0x01






/**************************************************************
 *  LIU
 **************************************************************/

/* reg_map indexes */
#define SBE_2T3E3_LIU_REG_REG0			0
#define SBE_2T3E3_LIU_REG_REG1			1
#define SBE_2T3E3_LIU_REG_REG2			2
#define SBE_2T3E3_LIU_REG_REG3			3
#define SBE_2T3E3_LIU_REG_REG4			4

#define	SBE_2T3E3_LIU_REG_MAX			5

/**********/

/* REG0 */
#define SBE_2T3E3_LIU_VAL_RECEIVE_LOSS_OF_LOCK_STATUS		0x10
#define SBE_2T3E3_LIU_VAL_RECEIVE_LOSS_OF_SIGNAL_STATUS		0x08
#define SBE_2T3E3_LIU_VAL_ANALOG_LOSS_OF_SIGNAL_STATUS		0x04
#define SBE_2T3E3_LIU_VAL_DIGITAL_LOSS_OF_SIGNAL_STATUS		0x02
#define SBE_2T3E3_LIU_VAL_DMO_STATUS				0x01

/* REG1 */
#define SBE_2T3E3_LIU_VAL_TRANSMITTER_OFF			0x10
#define SBE_2T3E3_LIU_VAL_TRANSMIT_ALL_ONES			0x08
#define SBE_2T3E3_LIU_VAL_TRANSMIT_CLOCK_INVERT			0x04
#define SBE_2T3E3_LIU_VAL_TRANSMIT_LEVEL_SELECT			0x02
#define SBE_2T3E3_LIU_VAL_TRANSMIT_BINARY_DATA			0x01

/* REG2 */
#define SBE_2T3E3_LIU_VAL_DECODER_DISABLE			0x10
#define SBE_2T3E3_LIU_VAL_ENCODER_DISABLE			0x08
#define SBE_2T3E3_LIU_VAL_ANALOG_LOSS_OF_SIGNAL_DISABLE		0x04
#define SBE_2T3E3_LIU_VAL_DIGITAL_LOSS_OF_SIGNAL_DISABLE	0x02
#define SBE_2T3E3_LIU_VAL_RECEIVE_EQUALIZATION_DISABLE		0x01

/* REG3 */
#define SBE_2T3E3_LIU_VAL_RECEIVE_BINARY_DATA			0x10
#define SBE_2T3E3_LIU_VAL_RECOVERED_DATA_MUTING			0x08
#define SBE_2T3E3_LIU_VAL_RECEIVE_CLOCK_OUTPUT_2		0x04
#define SBE_2T3E3_LIU_VAL_INVERT_RECEIVE_CLOCK_2		0x02
#define SBE_2T3E3_LIU_VAL_INVERT_RECEIVE_CLOCK_1		0x01

/* REG4 */
#define SBE_2T3E3_LIU_VAL_T3_MODE_SELECT			0x00
#define SBE_2T3E3_LIU_VAL_E3_MODE_SELECT			0x04
#define SBE_2T3E3_LIU_VAL_LOCAL_LOOPBACK			0x02
#define SBE_2T3E3_LIU_VAL_REMOTE_LOOPBACK			0x01
#define SBE_2T3E3_LIU_VAL_LOOPBACK_OFF				0x00
#define SBE_2T3E3_LIU_VAL_LOOPBACK_REMOTE			0x01
#define SBE_2T3E3_LIU_VAL_LOOPBACK_ANALOG			0x02
#define SBE_2T3E3_LIU_VAL_LOOPBACK_DIGITAL			0x03

/**********************************************************************
 *
 * descriptor list and data buffer
 *
 **********************************************************************/
typedef struct {
	u32 rdes0;
	u32 rdes1;
	u32 rdes2;
	u32 rdes3;
} t3e3_rx_desc_t;

#define SBE_2T3E3_RX_DESC_RING_SIZE			64

/* RDES0 */
#define SBE_2T3E3_RX_DESC_21143_OWN			0X80000000
#define SBE_2T3E3_RX_DESC_FRAME_LENGTH			0x3fff0000
#define SBE_2T3E3_RX_DESC_FRAME_LENGTH_SHIFT		16
#define SBE_2T3E3_RX_DESC_ERROR_SUMMARY			0x00008000
#define SBE_2T3E3_RX_DESC_DESC_ERROR			0x00004000
#define SBE_2T3E3_RX_DESC_DATA_TYPE			0x00003000
#define SBE_2T3E3_RX_DESC_RUNT_FRAME			0x00000800
#define SBE_2T3E3_RX_DESC_FIRST_DESC			0x00000200
#define SBE_2T3E3_RX_DESC_LAST_DESC			0x00000100
#define SBE_2T3E3_RX_DESC_FRAME_TOO_LONG		0x00000080
#define SBE_2T3E3_RX_DESC_COLLISION_SEEN		0x00000040
#define SBE_2T3E3_RX_DESC_FRAME_TYPE			0x00000020
#define SBE_2T3E3_RX_DESC_RECEIVE_WATCHDOG		0x00000010
#define SBE_2T3E3_RX_DESC_MII_ERROR			0x00000008
#define SBE_2T3E3_RX_DESC_DRIBBLING_BIT			0x00000004
#define SBE_2T3E3_RX_DESC_CRC_ERROR			0x00000002

/* RDES1 */
#define SBE_2T3E3_RX_DESC_END_OF_RING			0x02000000
#define SBE_2T3E3_RX_DESC_SECOND_ADDRESS_CHAINED	0x01000000
#define SBE_2T3E3_RX_DESC_BUFFER_2_SIZE			0x003ff800
#define SBE_2T3E3_RX_DESC_BUFFER_1_SIZE			0x000007ff

/*********************/

typedef struct {
	u32 tdes0;
	u32 tdes1;
	u32 tdes2;
	u32 tdes3;
} t3e3_tx_desc_t;

#define SBE_2T3E3_TX_DESC_RING_SIZE			256

/* TDES0 */
#define SBE_2T3E3_TX_DESC_21143_OWN			0x80000000
#define SBE_2T3E3_TX_DESC_ERROR_SUMMARY			0x00008000
#define SBE_2T3E3_TX_DESC_TRANSMIT_JABBER_TIMEOUT	0x00004000
#define SBE_2T3E3_TX_DESC_LOSS_OF_CARRIER		0x00000800
#define SBE_2T3E3_TX_DESC_NO_CARRIER			0x00000400
#define SBE_2T3E3_TX_DESC_LINK_FAIL_REPORT		0x00000004
#define SBE_2T3E3_TX_DESC_UNDERFLOW_ERROR		0x00000002
#define SBE_2T3E3_TX_DESC_DEFFERED			0x00000001

/* TDES1 */
#define SBE_2T3E3_TX_DESC_INTERRUPT_ON_COMPLETION	0x80000000
#define SBE_2T3E3_TX_DESC_LAST_SEGMENT			0x40000000
#define SBE_2T3E3_TX_DESC_FIRST_SEGMENT			0x20000000
#define SBE_2T3E3_TX_DESC_CRC_DISABLE			0x04000000
#define SBE_2T3E3_TX_DESC_END_OF_RING			0x02000000
#define SBE_2T3E3_TX_DESC_SECOND_ADDRESS_CHAINED	0x01000000
#define SBE_2T3E3_TX_DESC_DISABLE_PADDING		0x00800000
#define SBE_2T3E3_TX_DESC_BUFFER_2_SIZE			0x003ff800
#define SBE_2T3E3_TX_DESC_BUFFER_1_SIZE			0x000007ff


#define SBE_2T3E3_MTU					1600
#define SBE_2T3E3_CRC16_LENGTH				2
#define SBE_2T3E3_CRC32_LENGTH				4

#define MCLBYTES (SBE_2T3E3_MTU + 128)

struct channel {
	struct pci_dev *pdev;
	struct net_device *dev;
	struct card *card;
	unsigned long addr;	/* DECchip */

	int leds;

	/* pci specific */
	struct {
		u32 slot;           /* should be 0 or 1 */
		u32 command;
		u8 cache_size;
	} h;

	/* statistics */
	t3e3_stats_t s;

	/* running */
	struct {
		u32 flags;
	} r;

	/* parameters */
	t3e3_param_t p;

	u32 liu_regs[SBE_2T3E3_LIU_REG_MAX];	   /* LIU registers */
	u32 framer_regs[SBE_2T3E3_FRAMER_REG_MAX]; /* Framer registers */

	/* Ethernet Controller */
	struct {
		u_int16_t card_serial_number[3];

		u32 reg[SBE_2T3E3_21143_REG_MAX]; /* registers i.e. CSR */

		u32 interrupt_enable_mask;

		/* receive chain/ring */
		t3e3_rx_desc_t *rx_ring;
		struct sk_buff *rx_data[SBE_2T3E3_RX_DESC_RING_SIZE];
		u32 rx_ring_current_read;

		/* transmit chain/ring */
		t3e3_tx_desc_t *tx_ring;
		struct sk_buff *tx_data[SBE_2T3E3_TX_DESC_RING_SIZE];
		u32 tx_ring_current_read;
		u32 tx_ring_current_write;
		int tx_full;
		int tx_free_cnt;
		spinlock_t tx_lock;
	} ether;

	int32_t interrupt_active;
	int32_t rcv_count;
};

struct card {
	spinlock_t bootrom_lock;
	unsigned long bootrom_addr;
	struct timer_list timer; /* for updating LEDs */
	struct channel channels[0];
};

#define SBE_2T3E3_FLAG_NETWORK_UP		0x00000001
#define SBE_2T3E3_FLAG_NO_ERROR_MESSAGES	0x00000002

extern const u32 cpld_reg_map[][2];
extern const u32 cpld_val_map[][2];
extern const u32 t3e3_framer_reg_map[];
extern const u32 t3e3_liu_reg_map[];

void t3e3_init(struct channel *);
void t3e3_if_up(struct channel *);
void t3e3_if_down(struct channel *);
int t3e3_if_start_xmit(struct sk_buff *skb, struct net_device *dev);
void t3e3_if_config(struct channel *, u32, char *,
		    t3e3_resp_t *, int *);
void t3e3_set_frame_type(struct channel *, u32);
u32 t3e3_eeprom_read_word(struct channel *, u32);
void t3e3_read_card_serial_number(struct channel *);

/* interrupt handlers */
irqreturn_t t3e3_intr(int irq, void *dev_instance);
void dc_intr(struct channel *);
void dc_intr_rx(struct channel *);
void dc_intr_tx(struct channel *);
void dc_intr_tx_underflow(struct channel *);
void exar7250_intr(struct channel *);
void exar7250_E3_intr(struct channel *, u32);
void exar7250_T3_intr(struct channel *, u32);

/* Ethernet controller */
u32 bootrom_read(struct channel *, u32);
void bootrom_write(struct channel *, u32, u32);
void dc_init(struct channel *);
void dc_start(struct channel *);
void dc_stop(struct channel *);
void dc_start_intr(struct channel *);
void dc_stop_intr(struct channel *);
void dc_reset(struct channel *);
void dc_restart(struct channel *);
void dc_receiver_onoff(struct channel *, u32);
void dc_transmitter_onoff(struct channel *, u32);
void dc_set_loopback(struct channel *, u32);
void dc_clear_descriptor_list(struct channel *);
void dc_drop_descriptor_list(struct channel *);
void dc_set_output_port(struct channel *);
void t3e3_sc_init(struct channel *);

/* CPLD */
void cpld_init(struct channel *sc);
u32 cpld_read(struct channel *sc, u32 reg);
void cpld_set_crc(struct channel *, u32);
void cpld_start_intr(struct channel *);
void cpld_stop_intr(struct channel *);
void cpld_set_clock(struct channel *sc, u32 mode);
void cpld_set_scrambler(struct channel *, u32);
void cpld_select_panel(struct channel *, u32);
void cpld_set_frame_mode(struct channel *, u32);
void cpld_set_frame_type(struct channel *, u32);
void cpld_set_pad_count(struct channel *, u32);
void cpld_set_fractional_mode(struct channel *, u32, u32, u32);
void cpld_LOS_update(struct channel *);

/* Framer */
extern u32 exar7250_read(struct channel *, u32);
extern void exar7250_write(struct channel *, u32, u32);
void exar7250_init(struct channel *);
void exar7250_start_intr(struct channel *, u32);
void exar7250_stop_intr(struct channel *, u32);
void exar7250_set_frame_type(struct channel *, u32);
void exar7250_set_loopback(struct channel *, u32);
void exar7250_unipolar_onoff(struct channel *, u32);

/* LIU */
u32 exar7300_read(struct channel *, u32);
void exar7300_write(struct channel *, u32, u32);
void exar7300_init(struct channel *);
void exar7300_line_build_out_onoff(struct channel *, u32);
void exar7300_set_frame_type(struct channel *, u32);
void exar7300_set_loopback(struct channel *, u32);
void exar7300_transmit_all_ones_onoff(struct channel *, u32);
void exar7300_receive_equalization_onoff(struct channel *, u32);
void exar7300_unipolar_onoff(struct channel *, u32);

void update_led(struct channel *, int);
int setup_device(struct net_device *dev, struct channel *sc);

static inline int has_two_ports(struct pci_dev *pdev)
{
	return pdev->subsystem_device == PCI_SUBDEVICE_ID_SBE_2T3E3_P0;
}

#define dev_to_priv(dev) (*(struct channel **) ((hdlc_device*)(dev) + 1))

static inline u32 dc_read(unsigned long addr, u32 reg)
{
	return inl(addr + (reg << 3));
}

static inline void dc_write(unsigned long addr, u32 reg, u32 val)
{
	outl(val, addr + (reg << 3));
}

static inline void dc_set_bits(unsigned long addr, u32 reg, u32 bits)
{
	dc_write(addr, reg, dc_read(addr, reg) | bits);
}

static inline void dc_clear_bits(unsigned long addr, u32 reg, u32 bits)
{
	dc_write(addr, reg, dc_read(addr, reg) & ~bits);
}

#define CPLD_MAP_REG(reg, sc)	(cpld_reg_map[(reg)][(sc)->h.slot])

static inline void cpld_write(struct channel *channel, unsigned reg, u32 val)
{
	unsigned long flags;
	spin_lock_irqsave(&channel->card->bootrom_lock, flags);
	bootrom_write(channel, CPLD_MAP_REG(reg, channel), val);
	spin_unlock_irqrestore(&channel->card->bootrom_lock, flags);
}

#define exar7250_set_bit(sc, reg, bit)			\
	exar7250_write((sc), (reg),			\
		       exar7250_read(sc, reg) | (bit))

#define exar7250_clear_bit(sc, reg, bit)		\
	exar7250_write((sc), (reg),			\
		       exar7250_read(sc, reg) & ~(bit))

#define exar7300_set_bit(sc, reg, bit)			\
	exar7300_write((sc), (reg),			\
		       exar7300_read(sc, reg) | (bit))

#define exar7300_clear_bit(sc, reg, bit)		\
	exar7300_write((sc), (reg),			\
		       exar7300_read(sc, reg) & ~(bit))


#endif /* T3E3_H */
