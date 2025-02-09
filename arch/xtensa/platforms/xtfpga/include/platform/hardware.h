/*
 * arch/xtensa/platform/xtavnet/include/platform/hardware.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Tensilica Inc.
 */

/*
 * This file contains the hardware configuration of the XTAVNET boards.
 */

#ifndef __XTENSA_XTAVNET_HARDWARE_H
#define __XTENSA_XTAVNET_HARDWARE_H

/* By default NO_IRQ is defined to 0 in Fikus, but we use the
   interrupt 0 for UART... */
#define NO_IRQ                 -1

/* Memory configuration. */

#define PLATFORM_DEFAULT_MEM_START 0x00000000
#define PLATFORM_DEFAULT_MEM_SIZE  0x04000000

/* Interrupt configuration. */

#define PLATFORM_NR_IRQS	10

/* Default assignment of LX60 devices to external interrupts. */

#ifdef CONFIG_ARCH_HAS_SMP
#define DUART16552_INTNUM	XCHAL_EXTINT3_NUM
#define OETH_IRQ		XCHAL_EXTINT4_NUM
#else
#define DUART16552_INTNUM	XCHAL_EXTINT0_NUM
#define OETH_IRQ		XCHAL_EXTINT1_NUM
#endif

/*
 *  Device addresses and parameters.
 */

/* UART */
#define DUART16552_PADDR	(XCHAL_KIO_PADDR + 0x0D050020)
/* LCD instruction and data addresses. */
#define LCD_INSTR_ADDR		((char *)IOADDR(0x0D040000))
#define LCD_DATA_ADDR		((char *)IOADDR(0x0D040004))

/* Misc. */
#define XTFPGA_FPGAREGS_VADDR	IOADDR(0x0D020000)
/* Clock frequency in Hz (read-only):  */
#define XTFPGA_CLKFRQ_VADDR	(XTFPGA_FPGAREGS_VADDR + 0x04)
/* Setting of 8 DIP switches:  */
#define DIP_SWITCHES_VADDR	(XTFPGA_FPGAREGS_VADDR + 0x0C)
/* Software reset (write 0xdead):  */
#define XTFPGA_SWRST_VADDR	(XTFPGA_FPGAREGS_VADDR + 0x10)

/*  OpenCores Ethernet controller:  */
				/* regs + RX/TX descriptors */
#define OETH_REGS_PADDR		(XCHAL_KIO_PADDR + 0x0D030000)
#define OETH_REGS_SIZE		0x1000
#define OETH_SRAMBUFF_PADDR	(XCHAL_KIO_PADDR + 0x0D800000)

				/* 5*rx buffs + 5*tx buffs */
#define OETH_SRAMBUFF_SIZE	(5 * 0x600 + 5 * 0x600)

#endif /* __XTENSA_XTAVNET_HARDWARE_H */
