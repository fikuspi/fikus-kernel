/* 16550 serial driver for gdbstub I/O
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */
#include <fikus/string.h>
#include <fikus/kernel.h>
#include <fikus/signal.h>
#include <fikus/sched.h>
#include <fikus/mm.h>
#include <fikus/console.h>
#include <fikus/init.h>
#include <fikus/nmi.h>

#include <asm/pgtable.h>
#include <asm/gdb-stub.h>
#include <asm/exceptions.h>
#include <asm/serial-regs.h>
#include <unit/serial.h>
#include <asm/smp.h>

/*
 * initialise the GDB stub
 */
void gdbstub_io_init(void)
{
	u16 tmp;

	/* set up the serial port */
	GDBPORT_SERIAL_LCR = UART_LCR_WLEN8; /* 1N8 */
	GDBPORT_SERIAL_FCR = (UART_FCR_ENABLE_FIFO | UART_FCR_CLEAR_RCVR |
			      UART_FCR_CLEAR_XMIT);

	FLOWCTL_CLEAR(DTR);
	FLOWCTL_SET(RTS);

	gdbstub_io_set_baud(115200);

	/* we want to get serial receive interrupts */
	XIRQxICR(GDBPORT_SERIAL_IRQ) = 0;
	tmp = XIRQxICR(GDBPORT_SERIAL_IRQ);

#if   CONFIG_GDBSTUB_IRQ_LEVEL == 0
	IVAR0 = EXCEP_IRQ_LEVEL0;
#elif CONFIG_GDBSTUB_IRQ_LEVEL == 1
	IVAR1 = EXCEP_IRQ_LEVEL1;
#elif CONFIG_GDBSTUB_IRQ_LEVEL == 2
	IVAR2 = EXCEP_IRQ_LEVEL2;
#elif CONFIG_GDBSTUB_IRQ_LEVEL == 3
	IVAR3 = EXCEP_IRQ_LEVEL3;
#elif CONFIG_GDBSTUB_IRQ_LEVEL == 4
	IVAR4 = EXCEP_IRQ_LEVEL4;
#elif CONFIG_GDBSTUB_IRQ_LEVEL == 5
	IVAR5 = EXCEP_IRQ_LEVEL5;
#else
#error "Unknown irq level for gdbstub."
#endif

	set_intr_stub(NUM2EXCEP_IRQ_LEVEL(CONFIG_GDBSTUB_IRQ_LEVEL),
		gdbstub_io_rx_handler);

	XIRQxICR(GDBPORT_SERIAL_IRQ) &= ~GxICR_REQUEST;
	XIRQxICR(GDBPORT_SERIAL_IRQ) =
		GxICR_ENABLE | NUM2GxICR_LEVEL(CONFIG_GDBSTUB_IRQ_LEVEL);
	tmp = XIRQxICR(GDBPORT_SERIAL_IRQ);

	GDBPORT_SERIAL_IER = UART_IER_RDI | UART_IER_RLSI;

	/* permit level 0 IRQs to take place */
	arch_local_change_intr_mask_level(
		NUM2EPSW_IM(CONFIG_GDBSTUB_IRQ_LEVEL + 1));
}

/*
 * set up the GDB stub serial port baud rate timers
 */
void gdbstub_io_set_baud(unsigned baud)
{
	unsigned value;
	u8 lcr;

	value = 18432000 / 16 / baud;

	lcr = GDBPORT_SERIAL_LCR;
	GDBPORT_SERIAL_LCR |= UART_LCR_DLAB;
	GDBPORT_SERIAL_DLL = value & 0xff;
	GDBPORT_SERIAL_DLM = (value >> 8) & 0xff;
	GDBPORT_SERIAL_LCR = lcr;
}

/*
 * wait for a character to come from the debugger
 */
int gdbstub_io_rx_char(unsigned char *_ch, int nonblock)
{
	unsigned ix;
	u8 ch, st;
#if defined(CONFIG_MN10300_WD_TIMER)
	int cpu;
#endif

	*_ch = 0xff;

	if (gdbstub_rx_unget) {
		*_ch = gdbstub_rx_unget;
		gdbstub_rx_unget = 0;
		return 0;
	}

 try_again:
	/* pull chars out of the buffer */
	ix = gdbstub_rx_outp;
	barrier();
	if (ix == gdbstub_rx_inp) {
		if (nonblock)
			return -EAGAIN;
#ifdef CONFIG_MN10300_WD_TIMER
	for (cpu = 0; cpu < NR_CPUS; cpu++)
		watchdog_alert_counter[cpu] = 0;
#endif
		goto try_again;
	}

	ch = gdbstub_rx_buffer[ix++];
	st = gdbstub_rx_buffer[ix++];
	barrier();
	gdbstub_rx_outp = ix & 0x00000fff;

	if (st & UART_LSR_BI) {
		gdbstub_proto("### GDB Rx Break Detected ###\n");
		return -EINTR;
	} else if (st & (UART_LSR_FE | UART_LSR_OE | UART_LSR_PE)) {
		gdbstub_proto("### GDB Rx Error (st=%02x) ###\n", st);
		return -EIO;
	} else {
		gdbstub_proto("### GDB Rx %02x (st=%02x) ###\n", ch, st);
		*_ch = ch & 0x7f;
		return 0;
	}
}

/*
 * send a character to the debugger
 */
void gdbstub_io_tx_char(unsigned char ch)
{
	FLOWCTL_SET(DTR);
	LSR_WAIT_FOR(THRE);
	/* FLOWCTL_WAIT_FOR(CTS); */

	if (ch == 0x0a) {
		GDBPORT_SERIAL_TX = 0x0d;
		LSR_WAIT_FOR(THRE);
		/* FLOWCTL_WAIT_FOR(CTS); */
	}
	GDBPORT_SERIAL_TX = ch;

	FLOWCTL_CLEAR(DTR);
}

/*
 * send a character to the debugger
 */
void gdbstub_io_tx_flush(void)
{
	LSR_WAIT_FOR(TEMT);
	LSR_WAIT_FOR(THRE);
	FLOWCTL_CLEAR(DTR);
}
