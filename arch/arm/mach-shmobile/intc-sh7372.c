/*
 * sh7372 processor support - INTC hardware block
 *
 * Copyright (C) 2010  Magnus Damm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/interrupt.h>
#include <fikus/module.h>
#include <fikus/irq.h>
#include <fikus/io.h>
#include <fikus/sh_intc.h>
#include <mach/intc.h>
#include <mach/irqs.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

enum {
	UNUSED_INTCA = 0,

	/* interrupt sources INTCA */
	DIRC,
	CRYPT_STD,
	IIC1_ALI1, IIC1_TACKI1, IIC1_WAITI1, IIC1_DTEI1,
	AP_ARM_IRQPMU, AP_ARM_COMMTX, AP_ARM_COMMRX,
	MFI_MFIM, MFI_MFIS,
	BBIF1, BBIF2,
	USBHSDMAC0_USHDMI,
	_3DG_SGX540,
	CMT1_CMT10, CMT1_CMT11, CMT1_CMT12, CMT1_CMT13, CMT2, CMT3,
	KEYSC_KEY,
	SCIFA0, SCIFA1, SCIFA2, SCIFA3,
	MSIOF2, MSIOF1,
	SCIFA4, SCIFA5, SCIFB,
	FLCTL_FLSTEI, FLCTL_FLTENDI, FLCTL_FLTREQ0I, FLCTL_FLTREQ1I,
	SDHI0_SDHI0I0, SDHI0_SDHI0I1, SDHI0_SDHI0I2, SDHI0_SDHI0I3,
	SDHI1_SDHI1I0, SDHI1_SDHI1I1, SDHI1_SDHI1I2,
	IRREM,
	IRDA,
	TPU0,
	TTI20,
	DDM,
	SDHI2_SDHI2I0, SDHI2_SDHI2I1, SDHI2_SDHI2I2, SDHI2_SDHI2I3,
	RWDT0,
	DMAC1_1_DEI0, DMAC1_1_DEI1, DMAC1_1_DEI2, DMAC1_1_DEI3,
	DMAC1_2_DEI4, DMAC1_2_DEI5, DMAC1_2_DADERR,
	DMAC2_1_DEI0, DMAC2_1_DEI1, DMAC2_1_DEI2, DMAC2_1_DEI3,
	DMAC2_2_DEI4, DMAC2_2_DEI5, DMAC2_2_DADERR,
	DMAC3_1_DEI0, DMAC3_1_DEI1, DMAC3_1_DEI2, DMAC3_1_DEI3,
	DMAC3_2_DEI4, DMAC3_2_DEI5, DMAC3_2_DADERR,
	SHWYSTAT_RT, SHWYSTAT_HS, SHWYSTAT_COM,
	HDMI,
	SPU2_SPU0, SPU2_SPU1,
	FSI, FMSI,
	MIPI_HSI,
	IPMMU_IPMMUD,
	CEC_1, CEC_2,
	AP_ARM_CTIIRQ, AP_ARM_DMAEXTERRIRQ, AP_ARM_DMAIRQ, AP_ARM_DMASIRQ,
	MFIS2,
	CPORTR2S,
	CMT14, CMT15,
	MMC_MMC_ERR, MMC_MMC_NOR,
	IIC4_ALI4, IIC4_TACKI4, IIC4_WAITI4, IIC4_DTEI4,
	IIC3_ALI3, IIC3_TACKI3, IIC3_WAITI3, IIC3_DTEI3,
	USB0_USB0I1, USB0_USB0I0,
	USB1_USB1I1, USB1_USB1I0,
	USBHSDMAC1_USHDMI,

	/* interrupt groups INTCA */
	DMAC1_1, DMAC1_2, DMAC2_1, DMAC2_2, DMAC3_1, DMAC3_2, SHWYSTAT,
	AP_ARM1, AP_ARM2, SPU2, FLCTL, IIC1, SDHI0, SDHI1, SDHI2
};

static struct intc_vect intca_vectors[] __initdata = {
	INTC_VECT(DIRC, 0x0560),
	INTC_VECT(CRYPT_STD, 0x0700),
	INTC_VECT(IIC1_ALI1, 0x0780), INTC_VECT(IIC1_TACKI1, 0x07a0),
	INTC_VECT(IIC1_WAITI1, 0x07c0), INTC_VECT(IIC1_DTEI1, 0x07e0),
	INTC_VECT(AP_ARM_IRQPMU, 0x0800), INTC_VECT(AP_ARM_COMMTX, 0x0840),
	INTC_VECT(AP_ARM_COMMRX, 0x0860),
	INTC_VECT(MFI_MFIM, 0x0900), INTC_VECT(MFI_MFIS, 0x0920),
	INTC_VECT(BBIF1, 0x0940), INTC_VECT(BBIF2, 0x0960),
	INTC_VECT(USBHSDMAC0_USHDMI, 0x0a00),
	INTC_VECT(_3DG_SGX540, 0x0a60),
	INTC_VECT(CMT1_CMT10, 0x0b00), INTC_VECT(CMT1_CMT11, 0x0b20),
	INTC_VECT(CMT1_CMT12, 0x0b40), INTC_VECT(CMT1_CMT13, 0x0b60),
	INTC_VECT(CMT2, 0x0b80), INTC_VECT(CMT3, 0x0ba0),
	INTC_VECT(KEYSC_KEY, 0x0be0),
	INTC_VECT(SCIFA0, 0x0c00), INTC_VECT(SCIFA1, 0x0c20),
	INTC_VECT(SCIFA2, 0x0c40), INTC_VECT(SCIFA3, 0x0c60),
	INTC_VECT(MSIOF2, 0x0c80), INTC_VECT(MSIOF1, 0x0d00),
	INTC_VECT(SCIFA4, 0x0d20), INTC_VECT(SCIFA5, 0x0d40),
	INTC_VECT(SCIFB, 0x0d60),
	INTC_VECT(FLCTL_FLSTEI, 0x0d80), INTC_VECT(FLCTL_FLTENDI, 0x0da0),
	INTC_VECT(FLCTL_FLTREQ0I, 0x0dc0), INTC_VECT(FLCTL_FLTREQ1I, 0x0de0),
	INTC_VECT(SDHI0_SDHI0I0, 0x0e00), INTC_VECT(SDHI0_SDHI0I1, 0x0e20),
	INTC_VECT(SDHI0_SDHI0I2, 0x0e40), INTC_VECT(SDHI0_SDHI0I3, 0x0e60),
	INTC_VECT(SDHI1_SDHI1I0, 0x0e80), INTC_VECT(SDHI1_SDHI1I1, 0x0ea0),
	INTC_VECT(SDHI1_SDHI1I2, 0x0ec0),
	INTC_VECT(IRREM, 0x0f60),
	INTC_VECT(IRDA, 0x0480),
	INTC_VECT(TPU0, 0x04a0),
	INTC_VECT(TTI20, 0x1100),
	INTC_VECT(DDM, 0x1140),
	INTC_VECT(SDHI2_SDHI2I0, 0x1200), INTC_VECT(SDHI2_SDHI2I1, 0x1220),
	INTC_VECT(SDHI2_SDHI2I2, 0x1240), INTC_VECT(SDHI2_SDHI2I3, 0x1260),
	INTC_VECT(RWDT0, 0x1280),
	INTC_VECT(DMAC1_1_DEI0, 0x2000), INTC_VECT(DMAC1_1_DEI1, 0x2020),
	INTC_VECT(DMAC1_1_DEI2, 0x2040), INTC_VECT(DMAC1_1_DEI3, 0x2060),
	INTC_VECT(DMAC1_2_DEI4, 0x2080), INTC_VECT(DMAC1_2_DEI5, 0x20a0),
	INTC_VECT(DMAC1_2_DADERR, 0x20c0),
	INTC_VECT(DMAC2_1_DEI0, 0x2100), INTC_VECT(DMAC2_1_DEI1, 0x2120),
	INTC_VECT(DMAC2_1_DEI2, 0x2140), INTC_VECT(DMAC2_1_DEI3, 0x2160),
	INTC_VECT(DMAC2_2_DEI4, 0x2180), INTC_VECT(DMAC2_2_DEI5, 0x21a0),
	INTC_VECT(DMAC2_2_DADERR, 0x21c0),
	INTC_VECT(DMAC3_1_DEI0, 0x2200), INTC_VECT(DMAC3_1_DEI1, 0x2220),
	INTC_VECT(DMAC3_1_DEI2, 0x2240), INTC_VECT(DMAC3_1_DEI3, 0x2260),
	INTC_VECT(DMAC3_2_DEI4, 0x2280), INTC_VECT(DMAC3_2_DEI5, 0x22a0),
	INTC_VECT(DMAC3_2_DADERR, 0x22c0),
	INTC_VECT(SHWYSTAT_RT, 0x1300), INTC_VECT(SHWYSTAT_HS, 0x1320),
	INTC_VECT(SHWYSTAT_COM, 0x1340),
	INTC_VECT(HDMI, 0x17e0),
	INTC_VECT(SPU2_SPU0, 0x1800), INTC_VECT(SPU2_SPU1, 0x1820),
	INTC_VECT(FSI, 0x1840),
	INTC_VECT(FMSI, 0x1860),
	INTC_VECT(MIPI_HSI, 0x18e0),
	INTC_VECT(IPMMU_IPMMUD, 0x1920),
	INTC_VECT(CEC_1, 0x1940), INTC_VECT(CEC_2, 0x1960),
	INTC_VECT(AP_ARM_CTIIRQ, 0x1980),
	INTC_VECT(AP_ARM_DMAEXTERRIRQ, 0x19a0),
	INTC_VECT(AP_ARM_DMAIRQ, 0x19c0),
	INTC_VECT(AP_ARM_DMASIRQ, 0x19e0),
	INTC_VECT(MFIS2, 0x1a00),
	INTC_VECT(CPORTR2S, 0x1a20),
	INTC_VECT(CMT14, 0x1a40), INTC_VECT(CMT15, 0x1a60),
	INTC_VECT(MMC_MMC_ERR, 0x1ac0), INTC_VECT(MMC_MMC_NOR, 0x1ae0),
	INTC_VECT(IIC4_ALI4, 0x1b00), INTC_VECT(IIC4_TACKI4, 0x1b20),
	INTC_VECT(IIC4_WAITI4, 0x1b40), INTC_VECT(IIC4_DTEI4, 0x1b60),
	INTC_VECT(IIC3_ALI3, 0x1b80), INTC_VECT(IIC3_TACKI3, 0x1ba0),
	INTC_VECT(IIC3_WAITI3, 0x1bc0), INTC_VECT(IIC3_DTEI3, 0x1be0),
	INTC_VECT(USB0_USB0I1, 0x1c80), INTC_VECT(USB0_USB0I0, 0x1ca0),
	INTC_VECT(USB1_USB1I1, 0x1cc0), INTC_VECT(USB1_USB1I0, 0x1ce0),
	INTC_VECT(USBHSDMAC1_USHDMI, 0x1d00),
};

static struct intc_group intca_groups[] __initdata = {
	INTC_GROUP(DMAC1_1, DMAC1_1_DEI0,
		   DMAC1_1_DEI1, DMAC1_1_DEI2, DMAC1_1_DEI3),
	INTC_GROUP(DMAC1_2, DMAC1_2_DEI4,
		   DMAC1_2_DEI5, DMAC1_2_DADERR),
	INTC_GROUP(DMAC2_1, DMAC2_1_DEI0,
		   DMAC2_1_DEI1, DMAC2_1_DEI2, DMAC2_1_DEI3),
	INTC_GROUP(DMAC2_2, DMAC2_2_DEI4,
		   DMAC2_2_DEI5, DMAC2_2_DADERR),
	INTC_GROUP(DMAC3_1, DMAC3_1_DEI0,
		   DMAC3_1_DEI1, DMAC3_1_DEI2, DMAC3_1_DEI3),
	INTC_GROUP(DMAC3_2, DMAC3_2_DEI4,
		   DMAC3_2_DEI5, DMAC3_2_DADERR),
	INTC_GROUP(AP_ARM1, AP_ARM_IRQPMU, AP_ARM_COMMTX, AP_ARM_COMMRX),
	INTC_GROUP(AP_ARM2, AP_ARM_CTIIRQ, AP_ARM_DMAEXTERRIRQ,
		   AP_ARM_DMAIRQ, AP_ARM_DMASIRQ),
	INTC_GROUP(SPU2, SPU2_SPU0, SPU2_SPU1),
	INTC_GROUP(FLCTL, FLCTL_FLSTEI, FLCTL_FLTENDI,
		   FLCTL_FLTREQ0I, FLCTL_FLTREQ1I),
	INTC_GROUP(IIC1, IIC1_ALI1, IIC1_TACKI1, IIC1_WAITI1, IIC1_DTEI1),
	INTC_GROUP(SDHI0, SDHI0_SDHI0I0, SDHI0_SDHI0I1,
		   SDHI0_SDHI0I2, SDHI0_SDHI0I3),
	INTC_GROUP(SDHI1, SDHI1_SDHI1I0, SDHI1_SDHI1I1,
		   SDHI1_SDHI1I2),
	INTC_GROUP(SDHI2, SDHI2_SDHI2I0, SDHI2_SDHI2I1,
		   SDHI2_SDHI2I2, SDHI2_SDHI2I3),
	INTC_GROUP(SHWYSTAT, SHWYSTAT_RT, SHWYSTAT_HS, SHWYSTAT_COM),
};

static struct intc_mask_reg intca_mask_registers[] __initdata = {
	{ 0xe6940080, 0xe69400c0, 8, /* IMR0A / IMCR0A */
	  { DMAC2_1_DEI3, DMAC2_1_DEI2, DMAC2_1_DEI1, DMAC2_1_DEI0,
	    AP_ARM_IRQPMU, 0, AP_ARM_COMMTX, AP_ARM_COMMRX } },
	{ 0xe6940084, 0xe69400c4, 8, /* IMR1A / IMCR1A */
	  { 0, CRYPT_STD, DIRC, 0,
	    DMAC1_1_DEI3, DMAC1_1_DEI2, DMAC1_1_DEI1, DMAC1_1_DEI0 } },
	{ 0xe6940088, 0xe69400c8, 8, /* IMR2A / IMCR2A */
	  { 0, 0, 0, 0,
	    BBIF1, BBIF2, MFI_MFIS, MFI_MFIM } },
	{ 0xe694008c, 0xe69400cc, 8, /* IMR3A / IMCR3A */
	  { DMAC3_1_DEI3, DMAC3_1_DEI2, DMAC3_1_DEI1, DMAC3_1_DEI0,
	    DMAC3_2_DADERR, DMAC3_2_DEI5, DMAC3_2_DEI4, IRDA } },
	{ 0xe6940090, 0xe69400d0, 8, /* IMR4A / IMCR4A */
	  { DDM, 0, 0, 0,
	    0, 0, 0, 0 } },
	{ 0xe6940094, 0xe69400d4, 8, /* IMR5A / IMCR5A */
	  { KEYSC_KEY, DMAC1_2_DADERR, DMAC1_2_DEI5, DMAC1_2_DEI4,
	    SCIFA3, SCIFA2, SCIFA1, SCIFA0 } },
	{ 0xe6940098, 0xe69400d8, 8, /* IMR6A / IMCR6A */
	  { SCIFB, SCIFA5, SCIFA4, MSIOF1,
	    0, 0, MSIOF2, 0 } },
	{ 0xe694009c, 0xe69400dc, 8, /* IMR7A / IMCR7A */
	  { SDHI0_SDHI0I3, SDHI0_SDHI0I2, SDHI0_SDHI0I1, SDHI0_SDHI0I0,
	    FLCTL_FLTREQ1I, FLCTL_FLTREQ0I, FLCTL_FLTENDI, FLCTL_FLSTEI } },
	{ 0xe69400a0, 0xe69400e0, 8, /* IMR8A / IMCR8A */
	  { 0, SDHI1_SDHI1I2, SDHI1_SDHI1I1, SDHI1_SDHI1I0,
	    TTI20, USBHSDMAC0_USHDMI, 0, 0 } },
	{ 0xe69400a4, 0xe69400e4, 8, /* IMR9A / IMCR9A */
	  { CMT1_CMT13, CMT1_CMT12, CMT1_CMT11, CMT1_CMT10,
	    CMT2, 0, 0, _3DG_SGX540 } },
	{ 0xe69400a8, 0xe69400e8, 8, /* IMR10A / IMCR10A */
	  { 0, DMAC2_2_DADERR, DMAC2_2_DEI5, DMAC2_2_DEI4,
	    0, 0, 0, 0 } },
	{ 0xe69400ac, 0xe69400ec, 8, /* IMR11A / IMCR11A */
	  { IIC1_DTEI1, IIC1_WAITI1, IIC1_TACKI1, IIC1_ALI1,
	    0, 0, IRREM, 0 } },
	{ 0xe69400b0, 0xe69400f0, 8, /* IMR12A / IMCR12A */
	  { 0, 0, TPU0, 0,
	    0, 0, 0, 0 } },
	{ 0xe69400b4, 0xe69400f4, 8, /* IMR13A / IMCR13A */
	  { SDHI2_SDHI2I3, SDHI2_SDHI2I2, SDHI2_SDHI2I1, SDHI2_SDHI2I0,
	    0, CMT3, 0, RWDT0 } },
	{ 0xe6950080, 0xe69500c0, 8, /* IMR0A3 / IMCR0A3 */
	  { SHWYSTAT_RT, SHWYSTAT_HS, SHWYSTAT_COM, 0,
	    0, 0, 0, 0 } },
	{ 0xe6950090, 0xe69500d0, 8, /* IMR4A3 / IMCR4A3 */
	  { 0, 0, 0, 0,
	    0, 0, 0, HDMI } },
	{ 0xe6950094, 0xe69500d4, 8, /* IMR5A3 / IMCR5A3 */
	  { SPU2_SPU0, SPU2_SPU1, FSI, FMSI,
	    0, 0, 0, MIPI_HSI } },
	{ 0xe6950098, 0xe69500d8, 8, /* IMR6A3 / IMCR6A3 */
	  { 0, IPMMU_IPMMUD, CEC_1, CEC_2,
	    AP_ARM_CTIIRQ, AP_ARM_DMAEXTERRIRQ,
	    AP_ARM_DMAIRQ, AP_ARM_DMASIRQ } },
	{ 0xe695009c, 0xe69500dc, 8, /* IMR7A3 / IMCR7A3 */
	  { MFIS2, CPORTR2S, CMT14, CMT15,
	    0, 0, MMC_MMC_ERR, MMC_MMC_NOR } },
	{ 0xe69500a0, 0xe69500e0, 8, /* IMR8A3 / IMCR8A3 */
	  { IIC4_ALI4, IIC4_TACKI4, IIC4_WAITI4, IIC4_DTEI4,
	    IIC3_ALI3, IIC3_TACKI3, IIC3_WAITI3, IIC3_DTEI3 } },
	{ 0xe69500a4, 0xe69500e4, 8, /* IMR9A3 / IMCR9A3 */
	  { 0, 0, 0, 0,
	    USB0_USB0I1, USB0_USB0I0, USB1_USB1I1, USB1_USB1I0 } },
	{ 0xe69500a8, 0xe69500e8, 8, /* IMR10A3 / IMCR10A3 */
	  { USBHSDMAC1_USHDMI, 0, 0, 0,
	    0, 0, 0, 0 } },
};

static struct intc_prio_reg intca_prio_registers[] __initdata = {
	{ 0xe6940000, 0, 16, 4, /* IPRAA */ { DMAC3_1, DMAC3_2, CMT2, 0 } },
	{ 0xe6940004, 0, 16, 4, /* IPRBA */ { IRDA, 0, BBIF1, BBIF2 } },
	{ 0xe6940008, 0, 16, 4, /* IPRCA */ { 0, CRYPT_STD,
					      CMT1_CMT11, AP_ARM1 } },
	{ 0xe694000c, 0, 16, 4, /* IPRDA */ { 0, 0,
					      CMT1_CMT12, 0 } },
	{ 0xe6940010, 0, 16, 4, /* IPREA */ { DMAC1_1, MFI_MFIS,
					      MFI_MFIM, 0 } },
	{ 0xe6940014, 0, 16, 4, /* IPRFA */ { KEYSC_KEY, DMAC1_2,
					      _3DG_SGX540, CMT1_CMT10 } },
	{ 0xe6940018, 0, 16, 4, /* IPRGA */ { SCIFA0, SCIFA1,
					      SCIFA2, SCIFA3 } },
	{ 0xe694001c, 0, 16, 4, /* IPRGH */ { MSIOF2, USBHSDMAC0_USHDMI,
					      FLCTL, SDHI0 } },
	{ 0xe6940020, 0, 16, 4, /* IPRIA */ { MSIOF1, SCIFA4,
					      0/* MSU */, IIC1 } },
	{ 0xe6940024, 0, 16, 4, /* IPRJA */ { DMAC2_1, DMAC2_2,
					      0/* MSUG */, TTI20 } },
	{ 0xe6940028, 0, 16, 4, /* IPRKA */ { 0, CMT1_CMT13, IRREM, SDHI1 } },
	{ 0xe694002c, 0, 16, 4, /* IPRLA */ { TPU0, 0, 0, 0 } },
	{ 0xe6940030, 0, 16, 4, /* IPRMA */ { 0, CMT3, 0, RWDT0 } },
	{ 0xe6940034, 0, 16, 4, /* IPRNA */ { SCIFB, SCIFA5, 0, DDM } },
	{ 0xe6940038, 0, 16, 4, /* IPROA */ { 0, 0, DIRC, SDHI2 } },
	{ 0xe6950000, 0, 16, 4, /* IPRAA3 */ { SHWYSTAT, 0, 0, 0 } },
	{ 0xe6950024, 0, 16, 4, /* IPRJA3 */ { 0, 0, 0, HDMI } },
	{ 0xe6950028, 0, 16, 4, /* IPRKA3 */ { SPU2, 0, FSI, FMSI } },
	{ 0xe695002c, 0, 16, 4, /* IPRLA3 */ { 0, 0, 0, MIPI_HSI } },
	{ 0xe6950030, 0, 16, 4, /* IPRMA3 */ { IPMMU_IPMMUD, 0,
					       CEC_1, CEC_2 } },
	{ 0xe6950034, 0, 16, 4, /* IPRNA3 */ { AP_ARM2, 0, 0, 0 } },
	{ 0xe6950038, 0, 16, 4, /* IPROA3 */ { MFIS2, CPORTR2S,
					       CMT14, CMT15 } },
	{ 0xe695003c, 0, 16, 4, /* IPRPA3 */ { 0, 0,
					       MMC_MMC_ERR, MMC_MMC_NOR } },
	{ 0xe6950040, 0, 16, 4, /* IPRQA3 */ { IIC4_ALI4, IIC4_TACKI4,
					       IIC4_WAITI4, IIC4_DTEI4 } },
	{ 0xe6950044, 0, 16, 4, /* IPRRA3 */ { IIC3_ALI3, IIC3_TACKI3,
					       IIC3_WAITI3, IIC3_DTEI3 } },
	{ 0xe6950048, 0, 16, 4, /* IPRSA3 */ { 0/*ERI*/, 0/*RXI*/,
					       0/*TXI*/, 0/*TEI*/} },
	{ 0xe695004c, 0, 16, 4, /* IPRTA3 */ { USB0_USB0I1, USB0_USB0I0,
					       USB1_USB1I1, USB1_USB1I0 } },
	{ 0xe6950050, 0, 16, 4, /* IPRUA3 */ { USBHSDMAC1_USHDMI, 0, 0, 0 } },
};

static DECLARE_INTC_DESC(intca_desc, "sh7372-intca",
			 intca_vectors, intca_groups,
			 intca_mask_registers, intca_prio_registers,
			 NULL);

INTC_IRQ_PINS_16(intca_irq_pins_lo, 0xe6900000,
		 INTC_VECT, "sh7372-intca-irq-lo");

INTC_IRQ_PINS_16H(intca_irq_pins_hi, 0xe6900000,
		 INTC_VECT, "sh7372-intca-irq-hi");

enum {
	UNUSED_INTCS = 0,
	ENABLED_INTCS,

	/* interrupt sources INTCS */

	/* IRQ0S - IRQ31S */
	VEU_VEU0, VEU_VEU1, VEU_VEU2, VEU_VEU3,
	RTDMAC_1_DEI0, RTDMAC_1_DEI1, RTDMAC_1_DEI2, RTDMAC_1_DEI3,
	CEU, BEU_BEU0, BEU_BEU1, BEU_BEU2,
	/* MFI */
	/* BBIF2 */
	VPU,
	TSIF1,
	/* 3DG */
	_2DDMAC,
	IIC2_ALI2, IIC2_TACKI2, IIC2_WAITI2, IIC2_DTEI2,
	IPMMU_IPMMUR, IPMMU_IPMMUR2,
	RTDMAC_2_DEI4, RTDMAC_2_DEI5, RTDMAC_2_DADERR,
	/* KEYSC */
	/* TTI20 */
	MSIOF,
	IIC0_ALI0, IIC0_TACKI0, IIC0_WAITI0, IIC0_DTEI0,
	TMU_TUNI0, TMU_TUNI1, TMU_TUNI2,
	CMT0,
	TSIF0,
	/* CMT2 */
	LMB,
	CTI,
	/* RWDT0 */
	ICB,
	JPU_JPEG,
	LCDC,
	LCRC,
	RTDMAC2_1_DEI0, RTDMAC2_1_DEI1, RTDMAC2_1_DEI2, RTDMAC2_1_DEI3,
	RTDMAC2_2_DEI4, RTDMAC2_2_DEI5, RTDMAC2_2_DADERR,
	ISP,
	LCDC1,
	CSIRX,
	DSITX_DSITX0,
	DSITX_DSITX1,
	/* SPU2 */
	/* FSI */
	/* FMSI */
	/* HDMI */
	TMU1_TUNI0, TMU1_TUNI1, TMU1_TUNI2,
	CMT4,
	DSITX1_DSITX1_0,
	DSITX1_DSITX1_1,
	MFIS2_INTCS, /* Priority always enabled using ENABLED_INTCS */
	CPORTS2R,
	/* CEC */
	JPU6E,

	/* interrupt groups INTCS */
	RTDMAC_1, RTDMAC_2, VEU, BEU, IIC0, IPMMU, IIC2,
	RTDMAC2_1, RTDMAC2_2, TMU1, DSITX,
};

static struct intc_vect intcs_vectors[] = {
	/* IRQ0S - IRQ31S */
	INTCS_VECT(VEU_VEU0, 0x700), INTCS_VECT(VEU_VEU1, 0x720),
	INTCS_VECT(VEU_VEU2, 0x740), INTCS_VECT(VEU_VEU3, 0x760),
	INTCS_VECT(RTDMAC_1_DEI0, 0x800), INTCS_VECT(RTDMAC_1_DEI1, 0x820),
	INTCS_VECT(RTDMAC_1_DEI2, 0x840), INTCS_VECT(RTDMAC_1_DEI3, 0x860),
	INTCS_VECT(CEU, 0x880), INTCS_VECT(BEU_BEU0, 0x8a0),
	INTCS_VECT(BEU_BEU1, 0x8c0), INTCS_VECT(BEU_BEU2, 0x8e0),
	/* MFI */
	/* BBIF2 */
	INTCS_VECT(VPU, 0x980),
	INTCS_VECT(TSIF1, 0x9a0),
	/* 3DG */
	INTCS_VECT(_2DDMAC, 0xa00),
	INTCS_VECT(IIC2_ALI2, 0xa80), INTCS_VECT(IIC2_TACKI2, 0xaa0),
	INTCS_VECT(IIC2_WAITI2, 0xac0), INTCS_VECT(IIC2_DTEI2, 0xae0),
	INTCS_VECT(IPMMU_IPMMUR, 0xb00), INTCS_VECT(IPMMU_IPMMUR2, 0xb20),
	INTCS_VECT(RTDMAC_2_DEI4, 0xb80), INTCS_VECT(RTDMAC_2_DEI5, 0xba0),
	INTCS_VECT(RTDMAC_2_DADERR, 0xbc0),
	/* KEYSC */
	/* TTI20 */
	INTCS_VECT(MSIOF, 0x0d20),
	INTCS_VECT(IIC0_ALI0, 0xe00), INTCS_VECT(IIC0_TACKI0, 0xe20),
	INTCS_VECT(IIC0_WAITI0, 0xe40), INTCS_VECT(IIC0_DTEI0, 0xe60),
	INTCS_VECT(TMU_TUNI0, 0xe80), INTCS_VECT(TMU_TUNI1, 0xea0),
	INTCS_VECT(TMU_TUNI2, 0xec0),
	INTCS_VECT(CMT0, 0xf00),
	INTCS_VECT(TSIF0, 0xf20),
	/* CMT2 */
	INTCS_VECT(LMB, 0xf60),
	INTCS_VECT(CTI, 0x400),
	/* RWDT0 */
	INTCS_VECT(ICB, 0x480),
	INTCS_VECT(JPU_JPEG, 0x560),
	INTCS_VECT(LCDC, 0x580),
	INTCS_VECT(LCRC, 0x5a0),
	INTCS_VECT(RTDMAC2_1_DEI0, 0x1300), INTCS_VECT(RTDMAC2_1_DEI1, 0x1320),
	INTCS_VECT(RTDMAC2_1_DEI2, 0x1340), INTCS_VECT(RTDMAC2_1_DEI3, 0x1360),
	INTCS_VECT(RTDMAC2_2_DEI4, 0x1380), INTCS_VECT(RTDMAC2_2_DEI5, 0x13a0),
	INTCS_VECT(RTDMAC2_2_DADERR, 0x13c0),
	INTCS_VECT(ISP, 0x1720),
	INTCS_VECT(LCDC1, 0x1780),
	INTCS_VECT(CSIRX, 0x17a0),
	INTCS_VECT(DSITX_DSITX0, 0x17c0),
	INTCS_VECT(DSITX_DSITX1, 0x17e0),
	/* SPU2 */
	/* FSI */
	/* FMSI */
	/* HDMI */
	INTCS_VECT(TMU1_TUNI0, 0x1900), INTCS_VECT(TMU1_TUNI1, 0x1920),
	INTCS_VECT(TMU1_TUNI2, 0x1940),
	INTCS_VECT(CMT4, 0x1980),
	INTCS_VECT(DSITX1_DSITX1_0, 0x19a0),
	INTCS_VECT(DSITX1_DSITX1_1, 0x19c0),
	INTCS_VECT(MFIS2_INTCS, 0x1a00),
	INTCS_VECT(CPORTS2R, 0x1a20),
	/* CEC */
	INTCS_VECT(JPU6E, 0x1a80),
};

static struct intc_group intcs_groups[] __initdata = {
	INTC_GROUP(RTDMAC_1, RTDMAC_1_DEI0, RTDMAC_1_DEI1,
		   RTDMAC_1_DEI2, RTDMAC_1_DEI3),
	INTC_GROUP(RTDMAC_2, RTDMAC_2_DEI4, RTDMAC_2_DEI5, RTDMAC_2_DADERR),
	INTC_GROUP(VEU, VEU_VEU0, VEU_VEU1, VEU_VEU2, VEU_VEU3),
	INTC_GROUP(BEU, BEU_BEU0, BEU_BEU1, BEU_BEU2),
	INTC_GROUP(IIC0, IIC0_ALI0, IIC0_TACKI0, IIC0_WAITI0, IIC0_DTEI0),
	INTC_GROUP(IPMMU, IPMMU_IPMMUR, IPMMU_IPMMUR2),
	INTC_GROUP(IIC2, IIC2_ALI2, IIC2_TACKI2, IIC2_WAITI2, IIC2_DTEI2),
	INTC_GROUP(RTDMAC2_1, RTDMAC2_1_DEI0, RTDMAC2_1_DEI1,
		   RTDMAC2_1_DEI2, RTDMAC2_1_DEI3),
	INTC_GROUP(RTDMAC2_2, RTDMAC2_2_DEI4,
		   RTDMAC2_2_DEI5, RTDMAC2_2_DADERR),
	INTC_GROUP(TMU1, TMU1_TUNI2, TMU1_TUNI1, TMU1_TUNI0),
	INTC_GROUP(DSITX, DSITX_DSITX0, DSITX_DSITX1),
};

static struct intc_mask_reg intcs_mask_registers[] = {
	{ 0xffd20184, 0xffd201c4, 8, /* IMR1SA / IMCR1SA */
	  { BEU_BEU2, BEU_BEU1, BEU_BEU0, CEU,
	    VEU_VEU3, VEU_VEU2, VEU_VEU1, VEU_VEU0 } },
	{ 0xffd20188, 0xffd201c8, 8, /* IMR2SA / IMCR2SA */
	  { 0, 0, 0, VPU,
	    0, 0, 0, 0 } },
	{ 0xffd2018c, 0xffd201cc, 8, /* IMR3SA / IMCR3SA */
	  { 0, 0, 0, _2DDMAC,
	    0, 0, 0, ICB } },
	{ 0xffd20190, 0xffd201d0, 8, /* IMR4SA / IMCR4SA */
	  { 0, 0, 0, CTI,
	    JPU_JPEG, 0, LCRC, LCDC } },
	{ 0xffd20194, 0xffd201d4, 8, /* IMR5SA / IMCR5SA */
	  { 0, RTDMAC_2_DADERR, RTDMAC_2_DEI5, RTDMAC_2_DEI4,
	    RTDMAC_1_DEI3, RTDMAC_1_DEI2, RTDMAC_1_DEI1, RTDMAC_1_DEI0 } },
	{ 0xffd20198, 0xffd201d8, 8, /* IMR6SA / IMCR6SA */
	  { 0, 0, MSIOF, 0,
	    0, 0, 0, 0 } },
	{ 0xffd2019c, 0xffd201dc, 8, /* IMR7SA / IMCR7SA */
	  { 0, TMU_TUNI2, TMU_TUNI1, TMU_TUNI0,
	    0, 0, 0, 0 } },
	{ 0xffd201a4, 0xffd201e4, 8, /* IMR9SA / IMCR9SA */
	  { 0, 0, 0, CMT0,
	    IIC2_DTEI2, IIC2_WAITI2, IIC2_TACKI2, IIC2_ALI2 } },
	{ 0xffd201a8, 0xffd201e8, 8, /* IMR10SA / IMCR10SA */
	  { 0, 0, IPMMU_IPMMUR2, IPMMU_IPMMUR,
	    0, 0, 0, 0 } },
	{ 0xffd201ac, 0xffd201ec, 8, /* IMR11SA / IMCR11SA */
	  { IIC0_DTEI0, IIC0_WAITI0, IIC0_TACKI0, IIC0_ALI0,
	    0, TSIF1, LMB, TSIF0 } },
	{ 0xffd50180, 0xffd501c0, 8, /* IMR0SA3 / IMCR0SA3 */
	  { 0, RTDMAC2_2_DADERR, RTDMAC2_2_DEI5, RTDMAC2_2_DEI4,
	    RTDMAC2_1_DEI3, RTDMAC2_1_DEI2, RTDMAC2_1_DEI1, RTDMAC2_1_DEI0 } },
	{ 0xffd50190, 0xffd501d0, 8, /* IMR4SA3 / IMCR4SA3 */
	  { 0, ISP, 0, 0,
	    LCDC1, CSIRX, DSITX_DSITX0, DSITX_DSITX1 } },
	{ 0xffd50198, 0xffd501d8, 8, /* IMR6SA3 / IMCR6SA3 */
	  { 0, TMU1_TUNI2, TMU1_TUNI1, TMU1_TUNI0,
	    CMT4, DSITX1_DSITX1_0, DSITX1_DSITX1_1, 0 } },
	{ 0xffd5019c, 0xffd501dc, 8, /* IMR7SA3 / IMCR7SA3 */
	  { MFIS2_INTCS, CPORTS2R, 0, 0,
	    JPU6E, 0, 0, 0 } },
};

/* Priority is needed for INTCA to receive the INTCS interrupt */
static struct intc_prio_reg intcs_prio_registers[] = {
	{ 0xffd20000, 0, 16, 4, /* IPRAS */ { CTI, 0, _2DDMAC, ICB } },
	{ 0xffd20004, 0, 16, 4, /* IPRBS */ { JPU_JPEG, LCDC, 0, LCRC } },
	{ 0xffd20010, 0, 16, 4, /* IPRES */ { RTDMAC_1, CEU, 0, VPU } },
	{ 0xffd20014, 0, 16, 4, /* IPRFS */ { 0, RTDMAC_2, 0, CMT0 } },
	{ 0xffd20018, 0, 16, 4, /* IPRGS */ { TMU_TUNI0, TMU_TUNI1,
					      TMU_TUNI2, TSIF1 } },
	{ 0xffd2001c, 0, 16, 4, /* IPRHS */ { 0, 0, VEU, BEU } },
	{ 0xffd20020, 0, 16, 4, /* IPRIS */ { 0, MSIOF, TSIF0, IIC0 } },
	{ 0xffd20028, 0, 16, 4, /* IPRKS */ { 0, 0, LMB, 0 } },
	{ 0xffd2002c, 0, 16, 4, /* IPRLS */ { IPMMU, 0, 0, 0 } },
	{ 0xffd20030, 0, 16, 4, /* IPRMS */ { IIC2, 0, 0, 0 } },
	{ 0xffd50000, 0, 16, 4, /* IPRAS3 */ { RTDMAC2_1, 0, 0, 0 } },
	{ 0xffd50004, 0, 16, 4, /* IPRBS3 */ { RTDMAC2_2, 0, 0, 0 } },
	{ 0xffd50020, 0, 16, 4, /* IPRIS3 */ { 0, ISP, 0, 0 } },
	{ 0xffd50024, 0, 16, 4, /* IPRJS3 */ { LCDC1, CSIRX, DSITX, 0 } },
	{ 0xffd50030, 0, 16, 4, /* IPRMS3 */ { TMU1, 0, 0, 0 } },
	{ 0xffd50034, 0, 16, 4, /* IPRNS3 */ { CMT4, DSITX1_DSITX1_0,
					       DSITX1_DSITX1_1, 0 } },
	{ 0xffd50038, 0, 16, 4, /* IPROS3 */ { ENABLED_INTCS, CPORTS2R,
					       0, 0 } },
	{ 0xffd5003c, 0, 16, 4, /* IPRPS3 */ { JPU6E, 0, 0, 0 } },
};

static struct resource intcs_resources[] __initdata = {
	[0] = {
		.start	= 0xffd20000,
		.end	= 0xffd201ff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= 0xffd50000,
		.end	= 0xffd501ff,
		.flags	= IORESOURCE_MEM,
	}
};

static struct intc_desc intcs_desc __initdata = {
	.name = "sh7372-intcs",
	.force_enable = ENABLED_INTCS,
	.skip_syscore_suspend = true,
	.resource = intcs_resources,
	.num_resources = ARRAY_SIZE(intcs_resources),
	.hw = INTC_HW_DESC(intcs_vectors, intcs_groups, intcs_mask_registers,
			   intcs_prio_registers, NULL, NULL),
};

static void intcs_demux(unsigned int irq, struct irq_desc *desc)
{
	void __iomem *reg = (void *)irq_get_handler_data(irq);
	unsigned int evtcodeas = ioread32(reg);

	generic_handle_irq(intcs_evt2irq(evtcodeas));
}

static void __iomem *intcs_ffd2;
static void __iomem *intcs_ffd5;

void __init sh7372_init_irq(void)
{
	void __iomem *intevtsa;
	int n;

	intcs_ffd2 = ioremap_nocache(0xffd20000, PAGE_SIZE);
	intevtsa = intcs_ffd2 + 0x100;
	intcs_ffd5 = ioremap_nocache(0xffd50000, PAGE_SIZE);

	register_intc_controller(&intca_desc);
	register_intc_controller(&intca_irq_pins_lo_desc);
	register_intc_controller(&intca_irq_pins_hi_desc);
	register_intc_controller(&intcs_desc);

	/* setup dummy cascade chip for INTCS */
	n = evt2irq(0xf80);
	irq_alloc_desc_at(n, numa_node_id());
	irq_set_chip_and_handler_name(n, &dummy_irq_chip,
				      handle_level_irq, "level");
	set_irq_flags(n, IRQF_VALID); /* yuck */

	/* demux using INTEVTSA */
	irq_set_handler_data(n, (void *)intevtsa);
	irq_set_chained_handler(n, intcs_demux);

	/* unmask INTCS in INTAMASK */
	iowrite16(0, intcs_ffd2 + 0x104);
}

static unsigned short ffd2[0x200];
static unsigned short ffd5[0x100];

void sh7372_intcs_suspend(void)
{
	int k;

	for (k = 0x00; k <= 0x30; k += 4)
		ffd2[k] = __raw_readw(intcs_ffd2 + k);

	for (k = 0x80; k <= 0xb0; k += 4)
		ffd2[k] = __raw_readb(intcs_ffd2 + k);

	for (k = 0x180; k <= 0x188; k += 4)
		ffd2[k] = __raw_readb(intcs_ffd2 + k);

	for (k = 0x00; k <= 0x3c; k += 4)
		ffd5[k] = __raw_readw(intcs_ffd5 + k);

	for (k = 0x80; k <= 0x9c; k += 4)
		ffd5[k] = __raw_readb(intcs_ffd5 + k);
}

void sh7372_intcs_resume(void)
{
	int k;

	for (k = 0x00; k <= 0x30; k += 4)
		__raw_writew(ffd2[k], intcs_ffd2 + k);

	for (k = 0x80; k <= 0xb0; k += 4)
		__raw_writeb(ffd2[k], intcs_ffd2 + k);

	for (k = 0x180; k <= 0x188; k += 4)
		__raw_writeb(ffd2[k], intcs_ffd2 + k);

	for (k = 0x00; k <= 0x3c; k += 4)
		__raw_writew(ffd5[k], intcs_ffd5 + k);

	for (k = 0x80; k <= 0x9c; k += 4)
		__raw_writeb(ffd5[k], intcs_ffd5 + k);
}

#define E694_BASE IOMEM(0xe6940000)
#define E695_BASE IOMEM(0xe6950000)

static unsigned short e694[0x200];
static unsigned short e695[0x200];

void sh7372_intca_suspend(void)
{
	int k;

	for (k = 0x00; k <= 0x38; k += 4)
		e694[k] = __raw_readw(E694_BASE + k);

	for (k = 0x80; k <= 0xb4; k += 4)
		e694[k] = __raw_readb(E694_BASE + k);

	for (k = 0x180; k <= 0x1b4; k += 4)
		e694[k] = __raw_readb(E694_BASE + k);

	for (k = 0x00; k <= 0x50; k += 4)
		e695[k] = __raw_readw(E695_BASE + k);

	for (k = 0x80; k <= 0xa8; k += 4)
		e695[k] = __raw_readb(E695_BASE + k);

	for (k = 0x180; k <= 0x1a8; k += 4)
		e695[k] = __raw_readb(E695_BASE + k);
}

void sh7372_intca_resume(void)
{
	int k;

	for (k = 0x00; k <= 0x38; k += 4)
		__raw_writew(e694[k], E694_BASE + k);

	for (k = 0x80; k <= 0xb4; k += 4)
		__raw_writeb(e694[k], E694_BASE + k);

	for (k = 0x180; k <= 0x1b4; k += 4)
		__raw_writeb(e694[k], E694_BASE + k);

	for (k = 0x00; k <= 0x50; k += 4)
		__raw_writew(e695[k], E695_BASE + k);

	for (k = 0x80; k <= 0xa8; k += 4)
		__raw_writeb(e695[k], E695_BASE + k);

	for (k = 0x180; k <= 0x1a8; k += 4)
		__raw_writeb(e695[k], E695_BASE + k);
}
