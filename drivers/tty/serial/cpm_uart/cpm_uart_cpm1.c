/*
 *  Driver for CPM (SCC/SMC) serial ports; CPM1 definitions
 *
 *  Maintainer: Kumar Gala (galak@kernel.crashing.org) (CPM2)
 *              Pantelis Antoniou (panto@intracom.gr) (CPM1)
 *
 *  Copyright (C) 2004 Freescale Semiconductor, Inc.
 *            (C) 2004 Intracom, S.A.
 *            (C) 2006 MontaVista Software, Inc.
 *		Vitaly Bordug <vbordug@ru.mvista.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <fikus/module.h>
#include <fikus/tty.h>
#include <fikus/gfp.h>
#include <fikus/ioport.h>
#include <fikus/init.h>
#include <fikus/serial.h>
#include <fikus/console.h>
#include <fikus/sysrq.h>
#include <fikus/device.h>
#include <fikus/bootmem.h>
#include <fikus/dma-mapping.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/fs_pd.h>

#include <fikus/serial_core.h>
#include <fikus/kernel.h>

#include <fikus/of.h>

#include "cpm_uart.h"

/**************************************************************/

void cpm_line_cr_cmd(struct uart_cpm_port *port, int cmd)
{
	cpm_command(port->command, cmd);
}

void __iomem *cpm_uart_map_pram(struct uart_cpm_port *port,
				struct device_node *np)
{
	return of_iomap(np, 1);
}

void cpm_uart_unmap_pram(struct uart_cpm_port *port, void __iomem *pram)
{
	iounmap(pram);
}

/*
 * Allocate DP-Ram and memory buffers. We need to allocate a transmit and
 * receive buffer descriptors from dual port ram, and a character
 * buffer area from host mem. If we are allocating for the console we need
 * to do it from bootmem
 */
int cpm_uart_allocbuf(struct uart_cpm_port *pinfo, unsigned int is_con)
{
	int dpmemsz, memsz;
	u8 *dp_mem;
	unsigned long dp_offset;
	u8 *mem_addr;
	dma_addr_t dma_addr = 0;

	pr_debug("CPM uart[%d]:allocbuf\n", pinfo->port.line);

	dpmemsz = sizeof(cbd_t) * (pinfo->rx_nrfifos + pinfo->tx_nrfifos);
	dp_offset = cpm_dpalloc(dpmemsz, 8);
	if (IS_ERR_VALUE(dp_offset)) {
		printk(KERN_ERR
		       "cpm_uart_cpm1.c: could not allocate buffer descriptors\n");
		return -ENOMEM;
	}
	dp_mem = cpm_dpram_addr(dp_offset);

	memsz = L1_CACHE_ALIGN(pinfo->rx_nrfifos * pinfo->rx_fifosize) +
	    L1_CACHE_ALIGN(pinfo->tx_nrfifos * pinfo->tx_fifosize);
	if (is_con) {
		/* was hostalloc but changed cause it blows away the */
		/* large tlb mapping when pinning the kernel area    */
		mem_addr = (u8 *) cpm_dpram_addr(cpm_dpalloc(memsz, 8));
		dma_addr = (u32)cpm_dpram_phys(mem_addr);
	} else
		mem_addr = dma_alloc_coherent(pinfo->port.dev, memsz, &dma_addr,
					      GFP_KERNEL);

	if (mem_addr == NULL) {
		cpm_dpfree(dp_offset);
		printk(KERN_ERR
		       "cpm_uart_cpm1.c: could not allocate coherent memory\n");
		return -ENOMEM;
	}

	pinfo->dp_addr = dp_offset;
	pinfo->mem_addr = mem_addr;             /*  virtual address*/
	pinfo->dma_addr = dma_addr;             /*  physical address*/
	pinfo->mem_size = memsz;

	pinfo->rx_buf = mem_addr;
	pinfo->tx_buf = pinfo->rx_buf + L1_CACHE_ALIGN(pinfo->rx_nrfifos
						       * pinfo->rx_fifosize);

	pinfo->rx_bd_base = (cbd_t __iomem __force *)dp_mem;
	pinfo->tx_bd_base = pinfo->rx_bd_base + pinfo->rx_nrfifos;

	return 0;
}

void cpm_uart_freebuf(struct uart_cpm_port *pinfo)
{
	dma_free_coherent(pinfo->port.dev, L1_CACHE_ALIGN(pinfo->rx_nrfifos *
							  pinfo->rx_fifosize) +
			  L1_CACHE_ALIGN(pinfo->tx_nrfifos *
					 pinfo->tx_fifosize), pinfo->mem_addr,
			  pinfo->dma_addr);

	cpm_dpfree(pinfo->dp_addr);
}
