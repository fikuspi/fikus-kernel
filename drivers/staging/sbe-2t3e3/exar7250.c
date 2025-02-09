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

#include "2t3e3.h"
#include "ctrl.h"

void exar7250_init(struct channel *sc)
{
	exar7250_write(sc, SBE_2T3E3_FRAMER_REG_OPERATING_MODE,
		       SBE_2T3E3_FRAMER_VAL_T3_CBIT |
		       SBE_2T3E3_FRAMER_VAL_INTERRUPT_ENABLE_RESET |
		       SBE_2T3E3_FRAMER_VAL_TIMING_ASYNCH_TXINCLK);

	exar7250_write(sc, SBE_2T3E3_FRAMER_REG_IO_CONTROL,
		       SBE_2T3E3_FRAMER_VAL_DISABLE_TX_LOSS_OF_CLOCK |
		       SBE_2T3E3_FRAMER_VAL_DISABLE_RX_LOSS_OF_CLOCK |
		       SBE_2T3E3_FRAMER_VAL_AMI_LINE_CODE |
		       SBE_2T3E3_FRAMER_VAL_RX_LINE_CLOCK_INVERT);

	exar7250_set_frame_type(sc, SBE_2T3E3_FRAME_TYPE_T3_CBIT);
}

void exar7250_set_frame_type(struct channel *sc, u32 type)
{
	u32 val;

	switch (type) {
	case SBE_2T3E3_FRAME_TYPE_E3_G751:
	case SBE_2T3E3_FRAME_TYPE_E3_G832:
	case SBE_2T3E3_FRAME_TYPE_T3_CBIT:
	case SBE_2T3E3_FRAME_TYPE_T3_M13:
		break;
	default:
		return;
	}

	exar7250_stop_intr(sc, type);

	val = exar7250_read(sc, SBE_2T3E3_FRAMER_REG_OPERATING_MODE);
	val &= ~(SBE_2T3E3_FRAMER_VAL_LOCAL_LOOPBACK_MODE |
		 SBE_2T3E3_FRAMER_VAL_T3_E3_SELECT |
		 SBE_2T3E3_FRAMER_VAL_FRAME_FORMAT_SELECT);
	switch (type) {
	case SBE_2T3E3_FRAME_TYPE_E3_G751:
		val |= SBE_2T3E3_FRAMER_VAL_E3_G751;
		break;
	case SBE_2T3E3_FRAME_TYPE_E3_G832:
		val |= SBE_2T3E3_FRAMER_VAL_E3_G832;
		break;
	case SBE_2T3E3_FRAME_TYPE_T3_CBIT:
		val |= SBE_2T3E3_FRAMER_VAL_T3_CBIT;
		break;
	case SBE_2T3E3_FRAME_TYPE_T3_M13:
		val |= SBE_2T3E3_FRAMER_VAL_T3_M13;
		break;
	default:
		return;
	}
	exar7250_write(sc, SBE_2T3E3_FRAMER_REG_OPERATING_MODE, val);
	exar7250_start_intr(sc, type);
}


void exar7250_start_intr(struct channel *sc, u32 type)
{
	u32 val;

	switch (type) {
	case SBE_2T3E3_FRAME_TYPE_E3_G751:
	case SBE_2T3E3_FRAME_TYPE_E3_G832:
		val = exar7250_read(sc, SBE_2T3E3_FRAMER_REG_E3_RX_CONFIGURATION_STATUS_2);

		cpld_LOS_update(sc);

		sc->s.OOF = val & SBE_2T3E3_FRAMER_VAL_E3_RX_OOF ? 1 : 0;
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_STATUS_1);
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_ENABLE_1,
			       SBE_2T3E3_FRAMER_VAL_E3_RX_OOF_INTERRUPT_ENABLE |
			       SBE_2T3E3_FRAMER_VAL_E3_RX_LOS_INTERRUPT_ENABLE);

		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_STATUS_2);
		break;

	case SBE_2T3E3_FRAME_TYPE_T3_CBIT:
	case SBE_2T3E3_FRAME_TYPE_T3_M13:
		val = exar7250_read(sc, SBE_2T3E3_FRAMER_REG_T3_RX_CONFIGURATION_STATUS);

		cpld_LOS_update(sc);

		sc->s.OOF = val & SBE_2T3E3_FRAMER_VAL_T3_RX_OOF ? 1 : 0;

		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_T3_RX_INTERRUPT_STATUS);
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_T3_RX_INTERRUPT_ENABLE,
			       SBE_2T3E3_FRAMER_VAL_T3_RX_LOS_INTERRUPT_ENABLE |
			       SBE_2T3E3_FRAMER_VAL_T3_RX_OOF_INTERRUPT_ENABLE);

		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_T3_RX_FEAC_INTERRUPT_ENABLE_STATUS);

		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_T3_RX_LAPD_CONTROL, 0);
		break;

	default:
		return;
	}

	exar7250_read(sc, SBE_2T3E3_FRAMER_REG_BLOCK_INTERRUPT_STATUS);
	exar7250_write(sc, SBE_2T3E3_FRAMER_REG_BLOCK_INTERRUPT_ENABLE,
		       SBE_2T3E3_FRAMER_VAL_RX_INTERRUPT_ENABLE |
		       SBE_2T3E3_FRAMER_VAL_TX_INTERRUPT_ENABLE);
}


void exar7250_stop_intr(struct channel *sc, u32 type)
{
	exar7250_write(sc, SBE_2T3E3_FRAMER_REG_BLOCK_INTERRUPT_ENABLE, 0);
	exar7250_read(sc, SBE_2T3E3_FRAMER_REG_BLOCK_INTERRUPT_STATUS);

	switch (type) {
	case SBE_2T3E3_FRAME_TYPE_E3_G751:
	case SBE_2T3E3_FRAME_TYPE_E3_G832:
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_ENABLE_1, 0);
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_STATUS_1);
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_ENABLE_2, 0);
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_E3_RX_INTERRUPT_STATUS_2);
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_E3_RX_LAPD_CONTROL, 0);
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_E3_RX_LAPD_CONTROL);
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_E3_TX_LAPD_STATUS, 0);
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_E3_TX_LAPD_STATUS);
		break;

	case SBE_2T3E3_FRAME_TYPE_T3_CBIT:
	case SBE_2T3E3_FRAME_TYPE_T3_M13:
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_T3_RX_INTERRUPT_ENABLE, 0);
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_T3_RX_INTERRUPT_STATUS);
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_T3_RX_FEAC_INTERRUPT_ENABLE_STATUS, 0);
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_T3_RX_FEAC_INTERRUPT_ENABLE_STATUS);
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_T3_RX_LAPD_CONTROL, 0);
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_T3_RX_LAPD_CONTROL);
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_T3_TX_FEAC_CONFIGURATION_STATUS, 0);
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_T3_TX_FEAC_CONFIGURATION_STATUS);
		exar7250_write(sc, SBE_2T3E3_FRAMER_REG_T3_TX_LAPD_STATUS, 0);
		exar7250_read(sc, SBE_2T3E3_FRAMER_REG_T3_TX_LAPD_STATUS);
		break;
	}
}




void exar7250_unipolar_onoff(struct channel *sc, u32 mode)
{
	switch (mode) {
	case SBE_2T3E3_OFF:
		exar7300_clear_bit(sc, SBE_2T3E3_FRAMER_REG_IO_CONTROL,
				   SBE_2T3E3_FRAMER_VAL_UNIPOLAR);
		break;
	case SBE_2T3E3_ON:
		exar7300_set_bit(sc, SBE_2T3E3_FRAMER_REG_IO_CONTROL,
				 SBE_2T3E3_FRAMER_VAL_UNIPOLAR);
		break;
	}
}

void exar7250_set_loopback(struct channel *sc, u32 mode)
{
	switch (mode) {
	case SBE_2T3E3_FRAMER_VAL_LOOPBACK_OFF:
		exar7300_clear_bit(sc, SBE_2T3E3_FRAMER_REG_OPERATING_MODE,
				   SBE_2T3E3_FRAMER_VAL_LOCAL_LOOPBACK_MODE);
		break;
	case SBE_2T3E3_FRAMER_VAL_LOOPBACK_ON:
		exar7300_set_bit(sc, SBE_2T3E3_FRAMER_REG_OPERATING_MODE,
				 SBE_2T3E3_FRAMER_VAL_LOCAL_LOOPBACK_MODE);
		break;
	}
}
