/*
 *  Driver for Motorola IMX serial ports
 *
 *  Based on drivers/char/serial.c, by John Torvalds, Theodore Ts'o.
 *
 *  Author: Sascha Hauer <sascha@saschahauer.de>
 *  Copyright (C) 2004 Pengutronix
 *
 *  Copyright (C) 2009 emlix GmbH
 *  Author: Fabian Godehardt (added IrDA support for iMX)
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
 * [29-Mar-2005] Mike Lee
 * Added hardware handshake
 */

#if defined(CONFIG_SERIAL_IMX_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <fikus/module.h>
#include <fikus/ioport.h>
#include <fikus/init.h>
#include <fikus/console.h>
#include <fikus/sysrq.h>
#include <fikus/platform_device.h>
#include <fikus/tty.h>
#include <fikus/tty_flip.h>
#include <fikus/serial_core.h>
#include <fikus/serial.h>
#include <fikus/clk.h>
#include <fikus/delay.h>
#include <fikus/rational.h>
#include <fikus/slab.h>
#include <fikus/of.h>
#include <fikus/of_device.h>
#include <fikus/io.h>
#include <fikus/dma-mapping.h>

#include <asm/irq.h>
#include <fikus/platform_data/serial-imx.h>
#include <fikus/platform_data/dma-imx.h>

/* Register definitions */
#define URXD0 0x0  /* Receiver Register */
#define URTX0 0x40 /* Transmitter Register */
#define UCR1  0x80 /* Control Register 1 */
#define UCR2  0x84 /* Control Register 2 */
#define UCR3  0x88 /* Control Register 3 */
#define UCR4  0x8c /* Control Register 4 */
#define UFCR  0x90 /* FIFO Control Register */
#define USR1  0x94 /* Status Register 1 */
#define USR2  0x98 /* Status Register 2 */
#define UESC  0x9c /* Escape Character Register */
#define UTIM  0xa0 /* Escape Timer Register */
#define UBIR  0xa4 /* BRM Incremental Register */
#define UBMR  0xa8 /* BRM Modulator Register */
#define UBRC  0xac /* Baud Rate Count Register */
#define IMX21_ONEMS 0xb0 /* One Millisecond register */
#define IMX1_UTS 0xd0 /* UART Test Register on i.mx1 */
#define IMX21_UTS 0xb4 /* UART Test Register on all other i.mx*/

/* UART Control Register Bit Fields.*/
#define URXD_CHARRDY	(1<<15)
#define URXD_ERR	(1<<14)
#define URXD_OVRRUN	(1<<13)
#define URXD_FRMERR	(1<<12)
#define URXD_BRK	(1<<11)
#define URXD_PRERR	(1<<10)
#define UCR1_ADEN	(1<<15) /* Auto detect interrupt */
#define UCR1_ADBR	(1<<14) /* Auto detect baud rate */
#define UCR1_TRDYEN	(1<<13) /* Transmitter ready interrupt enable */
#define UCR1_IDEN	(1<<12) /* Idle condition interrupt */
#define UCR1_ICD_REG(x) (((x) & 3) << 10) /* idle condition detect */
#define UCR1_RRDYEN	(1<<9)	/* Recv ready interrupt enable */
#define UCR1_RDMAEN	(1<<8)	/* Recv ready DMA enable */
#define UCR1_IREN	(1<<7)	/* Infrared interface enable */
#define UCR1_TXMPTYEN	(1<<6)	/* Transimitter empty interrupt enable */
#define UCR1_RTSDEN	(1<<5)	/* RTS delta interrupt enable */
#define UCR1_SNDBRK	(1<<4)	/* Send break */
#define UCR1_TDMAEN	(1<<3)	/* Transmitter ready DMA enable */
#define IMX1_UCR1_UARTCLKEN (1<<2) /* UART clock enabled, i.mx1 only */
#define UCR1_ATDMAEN    (1<<2)  /* Aging DMA Timer Enable */
#define UCR1_DOZE	(1<<1)	/* Doze */
#define UCR1_UARTEN	(1<<0)	/* UART enabled */
#define UCR2_ESCI	(1<<15)	/* Escape seq interrupt enable */
#define UCR2_IRTS	(1<<14)	/* Ignore RTS pin */
#define UCR2_CTSC	(1<<13)	/* CTS pin control */
#define UCR2_CTS	(1<<12)	/* Clear to send */
#define UCR2_ESCEN	(1<<11)	/* Escape enable */
#define UCR2_PREN	(1<<8)	/* Parity enable */
#define UCR2_PROE	(1<<7)	/* Parity odd/even */
#define UCR2_STPB	(1<<6)	/* Stop */
#define UCR2_WS		(1<<5)	/* Word size */
#define UCR2_RTSEN	(1<<4)	/* Request to send interrupt enable */
#define UCR2_ATEN	(1<<3)	/* Aging Timer Enable */
#define UCR2_TXEN	(1<<2)	/* Transmitter enabled */
#define UCR2_RXEN	(1<<1)	/* Receiver enabled */
#define UCR2_SRST	(1<<0)	/* SW reset */
#define UCR3_DTREN	(1<<13) /* DTR interrupt enable */
#define UCR3_PARERREN	(1<<12) /* Parity enable */
#define UCR3_FRAERREN	(1<<11) /* Frame error interrupt enable */
#define UCR3_DSR	(1<<10) /* Data set ready */
#define UCR3_DCD	(1<<9)	/* Data carrier detect */
#define UCR3_RI		(1<<8)	/* Ring indicator */
#define UCR3_TIMEOUTEN	(1<<7)	/* Timeout interrupt enable */
#define UCR3_RXDSEN	(1<<6)	/* Receive status interrupt enable */
#define UCR3_AIRINTEN	(1<<5)	/* Async IR wake interrupt enable */
#define UCR3_AWAKEN	(1<<4)	/* Async wake interrupt enable */
#define IMX21_UCR3_RXDMUXSEL	(1<<2)	/* RXD Muxed Input Select */
#define UCR3_INVT	(1<<1)	/* Inverted Infrared transmission */
#define UCR3_BPEN	(1<<0)	/* Preset registers enable */
#define UCR4_CTSTL_SHF	10	/* CTS trigger level shift */
#define UCR4_CTSTL_MASK	0x3F	/* CTS trigger is 6 bits wide */
#define UCR4_INVR	(1<<9)	/* Inverted infrared reception */
#define UCR4_ENIRI	(1<<8)	/* Serial infrared interrupt enable */
#define UCR4_WKEN	(1<<7)	/* Wake interrupt enable */
#define UCR4_REF16	(1<<6)	/* Ref freq 16 MHz */
#define UCR4_IDDMAEN    (1<<6)  /* DMA IDLE Condition Detected */
#define UCR4_IRSC	(1<<5)	/* IR special case */
#define UCR4_TCEN	(1<<3)	/* Transmit complete interrupt enable */
#define UCR4_BKEN	(1<<2)	/* Break condition interrupt enable */
#define UCR4_OREN	(1<<1)	/* Receiver overrun interrupt enable */
#define UCR4_DREN	(1<<0)	/* Recv data ready interrupt enable */
#define UFCR_RXTL_SHF	0	/* Receiver trigger level shift */
#define UFCR_DCEDTE	(1<<6)	/* DCE/DTE mode select */
#define UFCR_RFDIV	(7<<7)	/* Reference freq divider mask */
#define UFCR_RFDIV_REG(x)	(((x) < 7 ? 6 - (x) : 6) << 7)
#define UFCR_TXTL_SHF	10	/* Transmitter trigger level shift */
#define USR1_PARITYERR	(1<<15) /* Parity error interrupt flag */
#define USR1_RTSS	(1<<14) /* RTS pin status */
#define USR1_TRDY	(1<<13) /* Transmitter ready interrupt/dma flag */
#define USR1_RTSD	(1<<12) /* RTS delta */
#define USR1_ESCF	(1<<11) /* Escape seq interrupt flag */
#define USR1_FRAMERR	(1<<10) /* Frame error interrupt flag */
#define USR1_RRDY	(1<<9)	 /* Receiver ready interrupt/dma flag */
#define USR1_TIMEOUT	(1<<7)	 /* Receive timeout interrupt status */
#define USR1_RXDS	 (1<<6)	 /* Receiver idle interrupt flag */
#define USR1_AIRINT	 (1<<5)	 /* Async IR wake interrupt flag */
#define USR1_AWAKE	 (1<<4)	 /* Aysnc wake interrupt flag */
#define USR2_ADET	 (1<<15) /* Auto baud rate detect complete */
#define USR2_TXFE	 (1<<14) /* Transmit buffer FIFO empty */
#define USR2_DTRF	 (1<<13) /* DTR edge interrupt flag */
#define USR2_IDLE	 (1<<12) /* Idle condition */
#define USR2_IRINT	 (1<<8)	 /* Serial infrared interrupt flag */
#define USR2_WAKE	 (1<<7)	 /* Wake */
#define USR2_RTSF	 (1<<4)	 /* RTS edge interrupt flag */
#define USR2_TXDC	 (1<<3)	 /* Transmitter complete */
#define USR2_BRCD	 (1<<2)	 /* Break condition */
#define USR2_ORE	(1<<1)	 /* Overrun error */
#define USR2_RDR	(1<<0)	 /* Recv data ready */
#define UTS_FRCPERR	(1<<13) /* Force parity error */
#define UTS_LOOP	(1<<12)	 /* Loop tx and rx */
#define UTS_TXEMPTY	 (1<<6)	 /* TxFIFO empty */
#define UTS_RXEMPTY	 (1<<5)	 /* RxFIFO empty */
#define UTS_TXFULL	 (1<<4)	 /* TxFIFO full */
#define UTS_RXFULL	 (1<<3)	 /* RxFIFO full */
#define UTS_SOFTRST	 (1<<0)	 /* Software reset */

/* We've been assigned a range on the "Low-density serial ports" major */
#define SERIAL_IMX_MAJOR	207
#define MINOR_START		16
#define DEV_NAME		"ttymxc"

/*
 * This determines how often we check the modem status signals
 * for any change.  They generally aren't connected to an IRQ
 * so we have to poll them.  We also check immediately before
 * filling the TX fifo incase CTS has been dropped.
 */
#define MCTRL_TIMEOUT	(250*HZ/1000)

#define DRIVER_NAME "IMX-uart"

#define UART_NR 8

/* i.mx21 type uart runs on all i.mx except i.mx1 */
enum imx_uart_type {
	IMX1_UART,
	IMX21_UART,
	IMX6Q_UART,
};

/* device type dependent stuff */
struct imx_uart_data {
	unsigned uts_reg;
	enum imx_uart_type devtype;
};

struct imx_port {
	struct uart_port	port;
	struct timer_list	timer;
	unsigned int		old_status;
	int			txirq, rxirq, rtsirq;
	unsigned int		have_rtscts:1;
	unsigned int		dte_mode:1;
	unsigned int		use_irda:1;
	unsigned int		irda_inv_rx:1;
	unsigned int		irda_inv_tx:1;
	unsigned short		trcv_delay; /* transceiver delay */
	struct clk		*clk_ipg;
	struct clk		*clk_per;
	const struct imx_uart_data *devdata;

	/* DMA fields */
	unsigned int		dma_is_inited:1;
	unsigned int		dma_is_enabled:1;
	unsigned int		dma_is_rxing:1;
	unsigned int		dma_is_txing:1;
	struct dma_chan		*dma_chan_rx, *dma_chan_tx;
	struct scatterlist	rx_sgl, tx_sgl[2];
	void			*rx_buf;
	unsigned int		rx_bytes, tx_bytes;
	struct work_struct	tsk_dma_rx, tsk_dma_tx;
	unsigned int		dma_tx_nents;
	wait_queue_head_t	dma_wait;
};

struct imx_port_ucrs {
	unsigned int	ucr1;
	unsigned int	ucr2;
	unsigned int	ucr3;
};

#ifdef CONFIG_IRDA
#define USE_IRDA(sport)	((sport)->use_irda)
#else
#define USE_IRDA(sport)	(0)
#endif

static struct imx_uart_data imx_uart_devdata[] = {
	[IMX1_UART] = {
		.uts_reg = IMX1_UTS,
		.devtype = IMX1_UART,
	},
	[IMX21_UART] = {
		.uts_reg = IMX21_UTS,
		.devtype = IMX21_UART,
	},
	[IMX6Q_UART] = {
		.uts_reg = IMX21_UTS,
		.devtype = IMX6Q_UART,
	},
};

static struct platform_device_id imx_uart_devtype[] = {
	{
		.name = "imx1-uart",
		.driver_data = (kernel_ulong_t) &imx_uart_devdata[IMX1_UART],
	}, {
		.name = "imx21-uart",
		.driver_data = (kernel_ulong_t) &imx_uart_devdata[IMX21_UART],
	}, {
		.name = "imx6q-uart",
		.driver_data = (kernel_ulong_t) &imx_uart_devdata[IMX6Q_UART],
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(platform, imx_uart_devtype);

static struct of_device_id imx_uart_dt_ids[] = {
	{ .compatible = "fsl,imx6q-uart", .data = &imx_uart_devdata[IMX6Q_UART], },
	{ .compatible = "fsl,imx1-uart", .data = &imx_uart_devdata[IMX1_UART], },
	{ .compatible = "fsl,imx21-uart", .data = &imx_uart_devdata[IMX21_UART], },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_uart_dt_ids);

static inline unsigned uts_reg(struct imx_port *sport)
{
	return sport->devdata->uts_reg;
}

static inline int is_imx1_uart(struct imx_port *sport)
{
	return sport->devdata->devtype == IMX1_UART;
}

static inline int is_imx21_uart(struct imx_port *sport)
{
	return sport->devdata->devtype == IMX21_UART;
}

static inline int is_imx6q_uart(struct imx_port *sport)
{
	return sport->devdata->devtype == IMX6Q_UART;
}
/*
 * Save and restore functions for UCR1, UCR2 and UCR3 registers
 */
#if defined(CONFIG_CONSOLE_POLL) || defined(CONFIG_SERIAL_IMX_CONSOLE)
static void imx_port_ucrs_save(struct uart_port *port,
			       struct imx_port_ucrs *ucr)
{
	/* save control registers */
	ucr->ucr1 = readl(port->membase + UCR1);
	ucr->ucr2 = readl(port->membase + UCR2);
	ucr->ucr3 = readl(port->membase + UCR3);
}

static void imx_port_ucrs_restore(struct uart_port *port,
				  struct imx_port_ucrs *ucr)
{
	/* restore control registers */
	writel(ucr->ucr1, port->membase + UCR1);
	writel(ucr->ucr2, port->membase + UCR2);
	writel(ucr->ucr3, port->membase + UCR3);
}
#endif

/*
 * Handle any change of modem status signal since we were last called.
 */
static void imx_mctrl_check(struct imx_port *sport)
{
	unsigned int status, changed;

	status = sport->port.ops->get_mctrl(&sport->port);
	changed = status ^ sport->old_status;

	if (changed == 0)
		return;

	sport->old_status = status;

	if (changed & TIOCM_RI)
		sport->port.icount.rng++;
	if (changed & TIOCM_DSR)
		sport->port.icount.dsr++;
	if (changed & TIOCM_CAR)
		uart_handle_dcd_change(&sport->port, status & TIOCM_CAR);
	if (changed & TIOCM_CTS)
		uart_handle_cts_change(&sport->port, status & TIOCM_CTS);

	wake_up_interruptible(&sport->port.state->port.delta_msr_wait);
}

/*
 * This is our per-port timeout handler, for checking the
 * modem status signals.
 */
static void imx_timeout(unsigned long data)
{
	struct imx_port *sport = (struct imx_port *)data;
	unsigned long flags;

	if (sport->port.state) {
		spin_lock_irqsave(&sport->port.lock, flags);
		imx_mctrl_check(sport);
		spin_unlock_irqrestore(&sport->port.lock, flags);

		mod_timer(&sport->timer, jiffies + MCTRL_TIMEOUT);
	}
}

/*
 * interrupts disabled on entry
 */
static void imx_stop_tx(struct uart_port *port)
{
	struct imx_port *sport = (struct imx_port *)port;
	unsigned long temp;

	if (USE_IRDA(sport)) {
		/* half duplex - wait for end of transmission */
		int n = 256;
		while ((--n > 0) &&
		      !(readl(sport->port.membase + USR2) & USR2_TXDC)) {
			udelay(5);
			barrier();
		}
		/*
		 * irda transceiver - wait a bit more to avoid
		 * cutoff, hardware dependent
		 */
		udelay(sport->trcv_delay);

		/*
		 * half duplex - reactivate receive mode,
		 * flush receive pipe echo crap
		 */
		if (readl(sport->port.membase + USR2) & USR2_TXDC) {
			temp = readl(sport->port.membase + UCR1);
			temp &= ~(UCR1_TXMPTYEN | UCR1_TRDYEN);
			writel(temp, sport->port.membase + UCR1);

			temp = readl(sport->port.membase + UCR4);
			temp &= ~(UCR4_TCEN);
			writel(temp, sport->port.membase + UCR4);

			while (readl(sport->port.membase + URXD0) &
			       URXD_CHARRDY)
				barrier();

			temp = readl(sport->port.membase + UCR1);
			temp |= UCR1_RRDYEN;
			writel(temp, sport->port.membase + UCR1);

			temp = readl(sport->port.membase + UCR4);
			temp |= UCR4_DREN;
			writel(temp, sport->port.membase + UCR4);
		}
		return;
	}

	/*
	 * We are maybe in the SMP context, so if the DMA TX thread is running
	 * on other cpu, we have to wait for it to finish.
	 */
	if (sport->dma_is_enabled && sport->dma_is_txing)
		return;

	temp = readl(sport->port.membase + UCR1);
	writel(temp & ~UCR1_TXMPTYEN, sport->port.membase + UCR1);
}

/*
 * interrupts disabled on entry
 */
static void imx_stop_rx(struct uart_port *port)
{
	struct imx_port *sport = (struct imx_port *)port;
	unsigned long temp;

	/*
	 * We are maybe in the SMP context, so if the DMA TX thread is running
	 * on other cpu, we have to wait for it to finish.
	 */
	if (sport->dma_is_enabled && sport->dma_is_rxing)
		return;

	temp = readl(sport->port.membase + UCR2);
	writel(temp & ~UCR2_RXEN, sport->port.membase + UCR2);
}

/*
 * Set the modem control timer to fire immediately.
 */
static void imx_enable_ms(struct uart_port *port)
{
	struct imx_port *sport = (struct imx_port *)port;

	mod_timer(&sport->timer, jiffies);
}

static inline void imx_transmit_buffer(struct imx_port *sport)
{
	struct circ_buf *xmit = &sport->port.state->xmit;

	while (!uart_circ_empty(xmit) &&
			!(readl(sport->port.membase + uts_reg(sport))
				& UTS_TXFULL)) {
		/* send xmit->buf[xmit->tail]
		 * out the port here */
		writel(xmit->buf[xmit->tail], sport->port.membase + URTX0);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		sport->port.icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&sport->port);

	if (uart_circ_empty(xmit))
		imx_stop_tx(&sport->port);
}

static void dma_tx_callback(void *data)
{
	struct imx_port *sport = data;
	struct scatterlist *sgl = &sport->tx_sgl[0];
	struct circ_buf *xmit = &sport->port.state->xmit;
	unsigned long flags;

	dma_unmap_sg(sport->port.dev, sgl, sport->dma_tx_nents, DMA_TO_DEVICE);

	sport->dma_is_txing = 0;

	/* update the stat */
	spin_lock_irqsave(&sport->port.lock, flags);
	xmit->tail = (xmit->tail + sport->tx_bytes) & (UART_XMIT_SIZE - 1);
	sport->port.icount.tx += sport->tx_bytes;
	spin_unlock_irqrestore(&sport->port.lock, flags);

	dev_dbg(sport->port.dev, "we finish the TX DMA.\n");

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&sport->port);

	if (waitqueue_active(&sport->dma_wait)) {
		wake_up(&sport->dma_wait);
		dev_dbg(sport->port.dev, "exit in %s.\n", __func__);
		return;
	}

	schedule_work(&sport->tsk_dma_tx);
}

static void dma_tx_work(struct work_struct *w)
{
	struct imx_port *sport = container_of(w, struct imx_port, tsk_dma_tx);
	struct circ_buf *xmit = &sport->port.state->xmit;
	struct scatterlist *sgl = sport->tx_sgl;
	struct dma_async_tx_descriptor *desc;
	struct dma_chan	*chan = sport->dma_chan_tx;
	struct device *dev = sport->port.dev;
	enum dma_status status;
	unsigned long flags;
	int ret;

	status = chan->device->device_tx_status(chan, (dma_cookie_t)0, NULL);
	if (DMA_IN_PROGRESS == status)
		return;

	spin_lock_irqsave(&sport->port.lock, flags);
	sport->tx_bytes = uart_circ_chars_pending(xmit);
	if (sport->tx_bytes == 0) {
		spin_unlock_irqrestore(&sport->port.lock, flags);
		return;
	}

	if (xmit->tail > xmit->head) {
		sport->dma_tx_nents = 2;
		sg_init_table(sgl, 2);
		sg_set_buf(sgl, xmit->buf + xmit->tail,
				UART_XMIT_SIZE - xmit->tail);
		sg_set_buf(sgl + 1, xmit->buf, xmit->head);
	} else {
		sport->dma_tx_nents = 1;
		sg_init_one(sgl, xmit->buf + xmit->tail, sport->tx_bytes);
	}
	spin_unlock_irqrestore(&sport->port.lock, flags);

	ret = dma_map_sg(dev, sgl, sport->dma_tx_nents, DMA_TO_DEVICE);
	if (ret == 0) {
		dev_err(dev, "DMA mapping error for TX.\n");
		return;
	}
	desc = dmaengine_prep_slave_sg(chan, sgl, sport->dma_tx_nents,
					DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT);
	if (!desc) {
		dev_err(dev, "We cannot prepare for the TX slave dma!\n");
		return;
	}
	desc->callback = dma_tx_callback;
	desc->callback_param = sport;

	dev_dbg(dev, "TX: prepare to send %lu bytes by DMA.\n",
			uart_circ_chars_pending(xmit));
	/* fire it */
	sport->dma_is_txing = 1;
	dmaengine_submit(desc);
	dma_async_issue_pending(chan);
	return;
}

/*
 * interrupts disabled on entry
 */
static void imx_start_tx(struct uart_port *port)
{
	struct imx_port *sport = (struct imx_port *)port;
	unsigned long temp;

	if (USE_IRDA(sport)) {
		/* half duplex in IrDA mode; have to disable receive mode */
		temp = readl(sport->port.membase + UCR4);
		temp &= ~(UCR4_DREN);
		writel(temp, sport->port.membase + UCR4);

		temp = readl(sport->port.membase + UCR1);
		temp &= ~(UCR1_RRDYEN);
		writel(temp, sport->port.membase + UCR1);
	}
	/* Clear any pending ORE flag before enabling interrupt */
	temp = readl(sport->port.membase + USR2);
	writel(temp | USR2_ORE, sport->port.membase + USR2);

	temp = readl(sport->port.membase + UCR4);
	temp |= UCR4_OREN;
	writel(temp, sport->port.membase + UCR4);

	if (!sport->dma_is_enabled) {
		temp = readl(sport->port.membase + UCR1);
		writel(temp | UCR1_TXMPTYEN, sport->port.membase + UCR1);
	}

	if (USE_IRDA(sport)) {
		temp = readl(sport->port.membase + UCR1);
		temp |= UCR1_TRDYEN;
		writel(temp, sport->port.membase + UCR1);

		temp = readl(sport->port.membase + UCR4);
		temp |= UCR4_TCEN;
		writel(temp, sport->port.membase + UCR4);
	}

	if (sport->dma_is_enabled) {
		/*
		 * We may in the interrupt context, so arise a work_struct to
		 * do the real job.
		 */
		schedule_work(&sport->tsk_dma_tx);
		return;
	}

	if (readl(sport->port.membase + uts_reg(sport)) & UTS_TXEMPTY)
		imx_transmit_buffer(sport);
}

static irqreturn_t imx_rtsint(int irq, void *dev_id)
{
	struct imx_port *sport = dev_id;
	unsigned int val;
	unsigned long flags;

	spin_lock_irqsave(&sport->port.lock, flags);

	writel(USR1_RTSD, sport->port.membase + USR1);
	val = readl(sport->port.membase + USR1) & USR1_RTSS;
	uart_handle_cts_change(&sport->port, !!val);
	wake_up_interruptible(&sport->port.state->port.delta_msr_wait);

	spin_unlock_irqrestore(&sport->port.lock, flags);
	return IRQ_HANDLED;
}

static irqreturn_t imx_txint(int irq, void *dev_id)
{
	struct imx_port *sport = dev_id;
	struct circ_buf *xmit = &sport->port.state->xmit;
	unsigned long flags;

	spin_lock_irqsave(&sport->port.lock, flags);
	if (sport->port.x_char) {
		/* Send next char */
		writel(sport->port.x_char, sport->port.membase + URTX0);
		goto out;
	}

	if (uart_circ_empty(xmit) || uart_tx_stopped(&sport->port)) {
		imx_stop_tx(&sport->port);
		goto out;
	}

	imx_transmit_buffer(sport);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&sport->port);

out:
	spin_unlock_irqrestore(&sport->port.lock, flags);
	return IRQ_HANDLED;
}

static irqreturn_t imx_rxint(int irq, void *dev_id)
{
	struct imx_port *sport = dev_id;
	unsigned int rx, flg, ignored = 0;
	struct tty_port *port = &sport->port.state->port;
	unsigned long flags, temp;

	spin_lock_irqsave(&sport->port.lock, flags);

	while (readl(sport->port.membase + USR2) & USR2_RDR) {
		flg = TTY_NORMAL;
		sport->port.icount.rx++;

		rx = readl(sport->port.membase + URXD0);

		temp = readl(sport->port.membase + USR2);
		if (temp & USR2_BRCD) {
			writel(USR2_BRCD, sport->port.membase + USR2);
			if (uart_handle_break(&sport->port))
				continue;
		}

		if (uart_handle_sysrq_char(&sport->port, (unsigned char)rx))
			continue;

		if (unlikely(rx & URXD_ERR)) {
			if (rx & URXD_BRK)
				sport->port.icount.brk++;
			else if (rx & URXD_PRERR)
				sport->port.icount.parity++;
			else if (rx & URXD_FRMERR)
				sport->port.icount.frame++;
			if (rx & URXD_OVRRUN)
				sport->port.icount.overrun++;

			if (rx & sport->port.ignore_status_mask) {
				if (++ignored > 100)
					goto out;
				continue;
			}

			rx &= sport->port.read_status_mask;

			if (rx & URXD_BRK)
				flg = TTY_BREAK;
			else if (rx & URXD_PRERR)
				flg = TTY_PARITY;
			else if (rx & URXD_FRMERR)
				flg = TTY_FRAME;
			if (rx & URXD_OVRRUN)
				flg = TTY_OVERRUN;

#ifdef SUPPORT_SYSRQ
			sport->port.sysrq = 0;
#endif
		}

		tty_insert_flip_char(port, rx, flg);
	}

out:
	spin_unlock_irqrestore(&sport->port.lock, flags);
	tty_flip_buffer_push(port);
	return IRQ_HANDLED;
}

/*
 * If the RXFIFO is filled with some data, and then we
 * arise a DMA operation to receive them.
 */
static void imx_dma_rxint(struct imx_port *sport)
{
	unsigned long temp;

	temp = readl(sport->port.membase + USR2);
	if ((temp & USR2_RDR) && !sport->dma_is_rxing) {
		sport->dma_is_rxing = 1;

		/* disable the `Recerver Ready Interrrupt` */
		temp = readl(sport->port.membase + UCR1);
		temp &= ~(UCR1_RRDYEN);
		writel(temp, sport->port.membase + UCR1);

		/* tell the DMA to receive the data. */
		schedule_work(&sport->tsk_dma_rx);
	}
}

static irqreturn_t imx_int(int irq, void *dev_id)
{
	struct imx_port *sport = dev_id;
	unsigned int sts;
	unsigned int sts2;

	sts = readl(sport->port.membase + USR1);

	if (sts & USR1_RRDY) {
		if (sport->dma_is_enabled)
			imx_dma_rxint(sport);
		else
			imx_rxint(irq, dev_id);
	}

	if (sts & USR1_TRDY &&
			readl(sport->port.membase + UCR1) & UCR1_TXMPTYEN)
		imx_txint(irq, dev_id);

	if (sts & USR1_RTSD)
		imx_rtsint(irq, dev_id);

	if (sts & USR1_AWAKE)
		writel(USR1_AWAKE, sport->port.membase + USR1);

	sts2 = readl(sport->port.membase + USR2);
	if (sts2 & USR2_ORE) {
		dev_err(sport->port.dev, "Rx FIFO overrun\n");
		sport->port.icount.overrun++;
		writel(sts2 | USR2_ORE, sport->port.membase + USR2);
	}

	return IRQ_HANDLED;
}

/*
 * Return TIOCSER_TEMT when transmitter is not busy.
 */
static unsigned int imx_tx_empty(struct uart_port *port)
{
	struct imx_port *sport = (struct imx_port *)port;

	return (readl(sport->port.membase + USR2) & USR2_TXDC) ?  TIOCSER_TEMT : 0;
}

/*
 * We have a modem side uart, so the meanings of RTS and CTS are inverted.
 */
static unsigned int imx_get_mctrl(struct uart_port *port)
{
	struct imx_port *sport = (struct imx_port *)port;
	unsigned int tmp = TIOCM_DSR | TIOCM_CAR;

	if (readl(sport->port.membase + USR1) & USR1_RTSS)
		tmp |= TIOCM_CTS;

	if (readl(sport->port.membase + UCR2) & UCR2_CTS)
		tmp |= TIOCM_RTS;

	return tmp;
}

static void imx_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct imx_port *sport = (struct imx_port *)port;
	unsigned long temp;

	temp = readl(sport->port.membase + UCR2) & ~UCR2_CTS;

	if (mctrl & TIOCM_RTS)
		if (!sport->dma_is_enabled)
			temp |= UCR2_CTS;

	writel(temp, sport->port.membase + UCR2);
}

/*
 * Interrupts always disabled.
 */
static void imx_break_ctl(struct uart_port *port, int break_state)
{
	struct imx_port *sport = (struct imx_port *)port;
	unsigned long flags, temp;

	spin_lock_irqsave(&sport->port.lock, flags);

	temp = readl(sport->port.membase + UCR1) & ~UCR1_SNDBRK;

	if (break_state != 0)
		temp |= UCR1_SNDBRK;

	writel(temp, sport->port.membase + UCR1);

	spin_unlock_irqrestore(&sport->port.lock, flags);
}

#define TXTL 2 /* reset default */
#define RXTL 1 /* reset default */

static int imx_setup_ufcr(struct imx_port *sport, unsigned int mode)
{
	unsigned int val;

	/* set receiver / transmitter trigger level */
	val = readl(sport->port.membase + UFCR) & (UFCR_RFDIV | UFCR_DCEDTE);
	val |= TXTL << UFCR_TXTL_SHF | RXTL;
	writel(val, sport->port.membase + UFCR);
	return 0;
}

#define RX_BUF_SIZE	(PAGE_SIZE)
static int start_rx_dma(struct imx_port *sport);
static void dma_rx_work(struct work_struct *w)
{
	struct imx_port *sport = container_of(w, struct imx_port, tsk_dma_rx);
	struct tty_port *port = &sport->port.state->port;

	if (sport->rx_bytes) {
		tty_insert_flip_string(port, sport->rx_buf, sport->rx_bytes);
		tty_flip_buffer_push(port);
		sport->rx_bytes = 0;
	}

	if (sport->dma_is_rxing)
		start_rx_dma(sport);
}

static void imx_rx_dma_done(struct imx_port *sport)
{
	unsigned long temp;

	/* Enable this interrupt when the RXFIFO is empty. */
	temp = readl(sport->port.membase + UCR1);
	temp |= UCR1_RRDYEN;
	writel(temp, sport->port.membase + UCR1);

	sport->dma_is_rxing = 0;

	/* Is the shutdown waiting for us? */
	if (waitqueue_active(&sport->dma_wait))
		wake_up(&sport->dma_wait);
}

/*
 * There are three kinds of RX DMA interrupts(such as in the MX6Q):
 *   [1] the RX DMA buffer is full.
 *   [2] the Aging timer expires(wait for 8 bytes long)
 *   [3] the Idle Condition Detect(enabled the UCR4_IDDMAEN).
 *
 * The [2] is trigger when a character was been sitting in the FIFO
 * meanwhile [3] can wait for 32 bytes long when the RX line is
 * on IDLE state and RxFIFO is empty.
 */
static void dma_rx_callback(void *data)
{
	struct imx_port *sport = data;
	struct dma_chan	*chan = sport->dma_chan_rx;
	struct scatterlist *sgl = &sport->rx_sgl;
	struct dma_tx_state state;
	enum dma_status status;
	unsigned int count;

	/* unmap it first */
	dma_unmap_sg(sport->port.dev, sgl, 1, DMA_FROM_DEVICE);

	status = chan->device->device_tx_status(chan, (dma_cookie_t)0, &state);
	count = RX_BUF_SIZE - state.residue;
	dev_dbg(sport->port.dev, "We get %d bytes.\n", count);

	if (count) {
		sport->rx_bytes = count;
		schedule_work(&sport->tsk_dma_rx);
	} else
		imx_rx_dma_done(sport);
}

static int start_rx_dma(struct imx_port *sport)
{
	struct scatterlist *sgl = &sport->rx_sgl;
	struct dma_chan	*chan = sport->dma_chan_rx;
	struct device *dev = sport->port.dev;
	struct dma_async_tx_descriptor *desc;
	int ret;

	sg_init_one(sgl, sport->rx_buf, RX_BUF_SIZE);
	ret = dma_map_sg(dev, sgl, 1, DMA_FROM_DEVICE);
	if (ret == 0) {
		dev_err(dev, "DMA mapping error for RX.\n");
		return -EINVAL;
	}
	desc = dmaengine_prep_slave_sg(chan, sgl, 1, DMA_DEV_TO_MEM,
					DMA_PREP_INTERRUPT);
	if (!desc) {
		dev_err(dev, "We cannot prepare for the RX slave dma!\n");
		return -EINVAL;
	}
	desc->callback = dma_rx_callback;
	desc->callback_param = sport;

	dev_dbg(dev, "RX: prepare for the DMA.\n");
	dmaengine_submit(desc);
	dma_async_issue_pending(chan);
	return 0;
}

static void imx_uart_dma_exit(struct imx_port *sport)
{
	if (sport->dma_chan_rx) {
		dma_release_channel(sport->dma_chan_rx);
		sport->dma_chan_rx = NULL;

		kfree(sport->rx_buf);
		sport->rx_buf = NULL;
	}

	if (sport->dma_chan_tx) {
		dma_release_channel(sport->dma_chan_tx);
		sport->dma_chan_tx = NULL;
	}

	sport->dma_is_inited = 0;
}

static int imx_uart_dma_init(struct imx_port *sport)
{
	struct dma_slave_config slave_config = {};
	struct device *dev = sport->port.dev;
	int ret;

	/* Prepare for RX : */
	sport->dma_chan_rx = dma_request_slave_channel(dev, "rx");
	if (!sport->dma_chan_rx) {
		dev_dbg(dev, "cannot get the DMA channel.\n");
		ret = -EINVAL;
		goto err;
	}

	slave_config.direction = DMA_DEV_TO_MEM;
	slave_config.src_addr = sport->port.mapbase + URXD0;
	slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	slave_config.src_maxburst = RXTL;
	ret = dmaengine_slave_config(sport->dma_chan_rx, &slave_config);
	if (ret) {
		dev_err(dev, "error in RX dma configuration.\n");
		goto err;
	}

	sport->rx_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!sport->rx_buf) {
		dev_err(dev, "cannot alloc DMA buffer.\n");
		ret = -ENOMEM;
		goto err;
	}
	sport->rx_bytes = 0;

	/* Prepare for TX : */
	sport->dma_chan_tx = dma_request_slave_channel(dev, "tx");
	if (!sport->dma_chan_tx) {
		dev_err(dev, "cannot get the TX DMA channel!\n");
		ret = -EINVAL;
		goto err;
	}

	slave_config.direction = DMA_MEM_TO_DEV;
	slave_config.dst_addr = sport->port.mapbase + URTX0;
	slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	slave_config.dst_maxburst = TXTL;
	ret = dmaengine_slave_config(sport->dma_chan_tx, &slave_config);
	if (ret) {
		dev_err(dev, "error in TX dma configuration.");
		goto err;
	}

	sport->dma_is_inited = 1;

	return 0;
err:
	imx_uart_dma_exit(sport);
	return ret;
}

static void imx_enable_dma(struct imx_port *sport)
{
	unsigned long temp;
	struct tty_port *port = &sport->port.state->port;

	port->low_latency = 1;
	INIT_WORK(&sport->tsk_dma_tx, dma_tx_work);
	INIT_WORK(&sport->tsk_dma_rx, dma_rx_work);
	init_waitqueue_head(&sport->dma_wait);

	/* set UCR1 */
	temp = readl(sport->port.membase + UCR1);
	temp |= UCR1_RDMAEN | UCR1_TDMAEN | UCR1_ATDMAEN |
		/* wait for 32 idle frames for IDDMA interrupt */
		UCR1_ICD_REG(3);
	writel(temp, sport->port.membase + UCR1);

	/* set UCR4 */
	temp = readl(sport->port.membase + UCR4);
	temp |= UCR4_IDDMAEN;
	writel(temp, sport->port.membase + UCR4);

	sport->dma_is_enabled = 1;
}

static void imx_disable_dma(struct imx_port *sport)
{
	unsigned long temp;
	struct tty_port *port = &sport->port.state->port;

	/* clear UCR1 */
	temp = readl(sport->port.membase + UCR1);
	temp &= ~(UCR1_RDMAEN | UCR1_TDMAEN | UCR1_ATDMAEN);
	writel(temp, sport->port.membase + UCR1);

	/* clear UCR2 */
	temp = readl(sport->port.membase + UCR2);
	temp &= ~(UCR2_CTSC | UCR2_CTS);
	writel(temp, sport->port.membase + UCR2);

	/* clear UCR4 */
	temp = readl(sport->port.membase + UCR4);
	temp &= ~UCR4_IDDMAEN;
	writel(temp, sport->port.membase + UCR4);

	sport->dma_is_enabled = 0;
	port->low_latency = 0;
}

/* half the RX buffer size */
#define CTSTL 16

static int imx_startup(struct uart_port *port)
{
	struct imx_port *sport = (struct imx_port *)port;
	int retval;
	unsigned long flags, temp;

	retval = clk_prepare_enable(sport->clk_per);
	if (retval)
		goto error_out1;
	retval = clk_prepare_enable(sport->clk_ipg);
	if (retval) {
		clk_disable_unprepare(sport->clk_per);
		goto error_out1;
	}

	imx_setup_ufcr(sport, 0);

	/* disable the DREN bit (Data Ready interrupt enable) before
	 * requesting IRQs
	 */
	temp = readl(sport->port.membase + UCR4);

	if (USE_IRDA(sport))
		temp |= UCR4_IRSC;

	/* set the trigger level for CTS */
	temp &= ~(UCR4_CTSTL_MASK << UCR4_CTSTL_SHF);
	temp |= CTSTL << UCR4_CTSTL_SHF;

	writel(temp & ~UCR4_DREN, sport->port.membase + UCR4);

	if (USE_IRDA(sport)) {
		/* reset fifo's and state machines */
		int i = 100;
		temp = readl(sport->port.membase + UCR2);
		temp &= ~UCR2_SRST;
		writel(temp, sport->port.membase + UCR2);
		while (!(readl(sport->port.membase + UCR2) & UCR2_SRST) &&
		    (--i > 0)) {
			udelay(1);
		}
	}

	/*
	 * Allocate the IRQ(s) i.MX1 has three interrupts whereas later
	 * chips only have one interrupt.
	 */
	if (sport->txirq > 0) {
		retval = request_irq(sport->rxirq, imx_rxint, 0,
				DRIVER_NAME, sport);
		if (retval)
			goto error_out1;

		retval = request_irq(sport->txirq, imx_txint, 0,
				DRIVER_NAME, sport);
		if (retval)
			goto error_out2;

		/* do not use RTS IRQ on IrDA */
		if (!USE_IRDA(sport)) {
			retval = request_irq(sport->rtsirq, imx_rtsint, 0,
					DRIVER_NAME, sport);
			if (retval)
				goto error_out3;
		}
	} else {
		retval = request_irq(sport->port.irq, imx_int, 0,
				DRIVER_NAME, sport);
		if (retval) {
			free_irq(sport->port.irq, sport);
			goto error_out1;
		}
	}

	spin_lock_irqsave(&sport->port.lock, flags);
	/*
	 * Finally, clear and enable interrupts
	 */
	writel(USR1_RTSD, sport->port.membase + USR1);

	temp = readl(sport->port.membase + UCR1);
	temp |= UCR1_RRDYEN | UCR1_RTSDEN | UCR1_UARTEN;

	if (USE_IRDA(sport)) {
		temp |= UCR1_IREN;
		temp &= ~(UCR1_RTSDEN);
	}

	writel(temp, sport->port.membase + UCR1);

	temp = readl(sport->port.membase + UCR2);
	temp |= (UCR2_RXEN | UCR2_TXEN);
	if (!sport->have_rtscts)
		temp |= UCR2_IRTS;
	writel(temp, sport->port.membase + UCR2);

	if (USE_IRDA(sport)) {
		/* clear RX-FIFO */
		int i = 64;
		while ((--i > 0) &&
			(readl(sport->port.membase + URXD0) & URXD_CHARRDY)) {
			barrier();
		}
	}

	if (!is_imx1_uart(sport)) {
		temp = readl(sport->port.membase + UCR3);
		temp |= IMX21_UCR3_RXDMUXSEL;
		writel(temp, sport->port.membase + UCR3);
	}

	if (USE_IRDA(sport)) {
		temp = readl(sport->port.membase + UCR4);
		if (sport->irda_inv_rx)
			temp |= UCR4_INVR;
		else
			temp &= ~(UCR4_INVR);
		writel(temp | UCR4_DREN, sport->port.membase + UCR4);

		temp = readl(sport->port.membase + UCR3);
		if (sport->irda_inv_tx)
			temp |= UCR3_INVT;
		else
			temp &= ~(UCR3_INVT);
		writel(temp, sport->port.membase + UCR3);
	}

	/*
	 * Enable modem status interrupts
	 */
	imx_enable_ms(&sport->port);
	spin_unlock_irqrestore(&sport->port.lock, flags);

	if (USE_IRDA(sport)) {
		struct imxuart_platform_data *pdata;
		pdata = dev_get_platdata(sport->port.dev);
		sport->irda_inv_rx = pdata->irda_inv_rx;
		sport->irda_inv_tx = pdata->irda_inv_tx;
		sport->trcv_delay = pdata->transceiver_delay;
		if (pdata->irda_enable)
			pdata->irda_enable(1);
	}

	return 0;

error_out3:
	if (sport->txirq)
		free_irq(sport->txirq, sport);
error_out2:
	if (sport->rxirq)
		free_irq(sport->rxirq, sport);
error_out1:
	return retval;
}

static void imx_shutdown(struct uart_port *port)
{
	struct imx_port *sport = (struct imx_port *)port;
	unsigned long temp;
	unsigned long flags;

	if (sport->dma_is_enabled) {
		/* We have to wait for the DMA to finish. */
		wait_event(sport->dma_wait,
			!sport->dma_is_rxing && !sport->dma_is_txing);
		imx_stop_rx(port);
		imx_disable_dma(sport);
		imx_uart_dma_exit(sport);
	}

	spin_lock_irqsave(&sport->port.lock, flags);
	temp = readl(sport->port.membase + UCR2);
	temp &= ~(UCR2_TXEN);
	writel(temp, sport->port.membase + UCR2);
	spin_unlock_irqrestore(&sport->port.lock, flags);

	if (USE_IRDA(sport)) {
		struct imxuart_platform_data *pdata;
		pdata = dev_get_platdata(sport->port.dev);
		if (pdata->irda_enable)
			pdata->irda_enable(0);
	}

	/*
	 * Stop our timer.
	 */
	del_timer_sync(&sport->timer);

	/*
	 * Free the interrupts
	 */
	if (sport->txirq > 0) {
		if (!USE_IRDA(sport))
			free_irq(sport->rtsirq, sport);
		free_irq(sport->txirq, sport);
		free_irq(sport->rxirq, sport);
	} else
		free_irq(sport->port.irq, sport);

	/*
	 * Disable all interrupts, port and break condition.
	 */

	spin_lock_irqsave(&sport->port.lock, flags);
	temp = readl(sport->port.membase + UCR1);
	temp &= ~(UCR1_TXMPTYEN | UCR1_RRDYEN | UCR1_RTSDEN | UCR1_UARTEN);
	if (USE_IRDA(sport))
		temp &= ~(UCR1_IREN);

	writel(temp, sport->port.membase + UCR1);
	spin_unlock_irqrestore(&sport->port.lock, flags);

	clk_disable_unprepare(sport->clk_per);
	clk_disable_unprepare(sport->clk_ipg);
}

static void
imx_set_termios(struct uart_port *port, struct ktermios *termios,
		   struct ktermios *old)
{
	struct imx_port *sport = (struct imx_port *)port;
	unsigned long flags;
	unsigned int ucr2, old_ucr1, old_txrxen, baud, quot;
	unsigned int old_csize = old ? old->c_cflag & CSIZE : CS8;
	unsigned int div, ufcr;
	unsigned long num, denom;
	uint64_t tdiv64;

	/*
	 * If we don't support modem control lines, don't allow
	 * these to be set.
	 */
	if (0) {
		termios->c_cflag &= ~(HUPCL | CRTSCTS | CMSPAR);
		termios->c_cflag |= CLOCAL;
	}

	/*
	 * We only support CS7 and CS8.
	 */
	while ((termios->c_cflag & CSIZE) != CS7 &&
	       (termios->c_cflag & CSIZE) != CS8) {
		termios->c_cflag &= ~CSIZE;
		termios->c_cflag |= old_csize;
		old_csize = CS8;
	}

	if ((termios->c_cflag & CSIZE) == CS8)
		ucr2 = UCR2_WS | UCR2_SRST | UCR2_IRTS;
	else
		ucr2 = UCR2_SRST | UCR2_IRTS;

	if (termios->c_cflag & CRTSCTS) {
		if (sport->have_rtscts) {
			ucr2 &= ~UCR2_IRTS;
			ucr2 |= UCR2_CTSC;

			/* Can we enable the DMA support? */
			if (is_imx6q_uart(sport) && !uart_console(port)
				&& !sport->dma_is_inited)
				imx_uart_dma_init(sport);
		} else {
			termios->c_cflag &= ~CRTSCTS;
		}
	}

	if (termios->c_cflag & CSTOPB)
		ucr2 |= UCR2_STPB;
	if (termios->c_cflag & PARENB) {
		ucr2 |= UCR2_PREN;
		if (termios->c_cflag & PARODD)
			ucr2 |= UCR2_PROE;
	}

	del_timer_sync(&sport->timer);

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port, termios, old, 50, port->uartclk / 16);
	quot = uart_get_divisor(port, baud);

	spin_lock_irqsave(&sport->port.lock, flags);

	sport->port.read_status_mask = 0;
	if (termios->c_iflag & INPCK)
		sport->port.read_status_mask |= (URXD_FRMERR | URXD_PRERR);
	if (termios->c_iflag & (BRKINT | PARMRK))
		sport->port.read_status_mask |= URXD_BRK;

	/*
	 * Characters to ignore
	 */
	sport->port.ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		sport->port.ignore_status_mask |= URXD_PRERR;
	if (termios->c_iflag & IGNBRK) {
		sport->port.ignore_status_mask |= URXD_BRK;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			sport->port.ignore_status_mask |= URXD_OVRRUN;
	}

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	/*
	 * disable interrupts and drain transmitter
	 */
	old_ucr1 = readl(sport->port.membase + UCR1);
	writel(old_ucr1 & ~(UCR1_TXMPTYEN | UCR1_RRDYEN | UCR1_RTSDEN),
			sport->port.membase + UCR1);

	while (!(readl(sport->port.membase + USR2) & USR2_TXDC))
		barrier();

	/* then, disable everything */
	old_txrxen = readl(sport->port.membase + UCR2);
	writel(old_txrxen & ~(UCR2_TXEN | UCR2_RXEN),
			sport->port.membase + UCR2);
	old_txrxen &= (UCR2_TXEN | UCR2_RXEN);

	if (USE_IRDA(sport)) {
		/*
		 * use maximum available submodule frequency to
		 * avoid missing short pulses due to low sampling rate
		 */
		div = 1;
	} else {
		/* custom-baudrate handling */
		div = sport->port.uartclk / (baud * 16);
		if (baud == 38400 && quot != div)
			baud = sport->port.uartclk / (quot * 16);

		div = sport->port.uartclk / (baud * 16);
		if (div > 7)
			div = 7;
		if (!div)
			div = 1;
	}

	rational_best_approximation(16 * div * baud, sport->port.uartclk,
		1 << 16, 1 << 16, &num, &denom);

	tdiv64 = sport->port.uartclk;
	tdiv64 *= num;
	do_div(tdiv64, denom * 16 * div);
	tty_termios_encode_baud_rate(termios,
				(speed_t)tdiv64, (speed_t)tdiv64);

	num -= 1;
	denom -= 1;

	ufcr = readl(sport->port.membase + UFCR);
	ufcr = (ufcr & (~UFCR_RFDIV)) | UFCR_RFDIV_REG(div);
	if (sport->dte_mode)
		ufcr |= UFCR_DCEDTE;
	writel(ufcr, sport->port.membase + UFCR);

	writel(num, sport->port.membase + UBIR);
	writel(denom, sport->port.membase + UBMR);

	if (!is_imx1_uart(sport))
		writel(sport->port.uartclk / div / 1000,
				sport->port.membase + IMX21_ONEMS);

	writel(old_ucr1, sport->port.membase + UCR1);

	/* set the parity, stop bits and data size */
	writel(ucr2 | old_txrxen, sport->port.membase + UCR2);

	if (UART_ENABLE_MS(&sport->port, termios->c_cflag))
		imx_enable_ms(&sport->port);

	if (sport->dma_is_inited && !sport->dma_is_enabled)
		imx_enable_dma(sport);
	spin_unlock_irqrestore(&sport->port.lock, flags);
}

static const char *imx_type(struct uart_port *port)
{
	struct imx_port *sport = (struct imx_port *)port;

	return sport->port.type == PORT_IMX ? "IMX" : NULL;
}

/*
 * Release the memory region(s) being used by 'port'.
 */
static void imx_release_port(struct uart_port *port)
{
	struct platform_device *pdev = to_platform_device(port->dev);
	struct resource *mmres;

	mmres = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mmres->start, resource_size(mmres));
}

/*
 * Request the memory region(s) being used by 'port'.
 */
static int imx_request_port(struct uart_port *port)
{
	struct platform_device *pdev = to_platform_device(port->dev);
	struct resource *mmres;
	void *ret;

	mmres = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mmres)
		return -ENODEV;

	ret = request_mem_region(mmres->start, resource_size(mmres), "imx-uart");

	return  ret ? 0 : -EBUSY;
}

/*
 * Configure/autoconfigure the port.
 */
static void imx_config_port(struct uart_port *port, int flags)
{
	struct imx_port *sport = (struct imx_port *)port;

	if (flags & UART_CONFIG_TYPE &&
	    imx_request_port(&sport->port) == 0)
		sport->port.type = PORT_IMX;
}

/*
 * Verify the new serial_struct (for TIOCSSERIAL).
 * The only change we allow are to the flags and type, and
 * even then only between PORT_IMX and PORT_UNKNOWN
 */
static int
imx_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	struct imx_port *sport = (struct imx_port *)port;
	int ret = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_IMX)
		ret = -EINVAL;
	if (sport->port.irq != ser->irq)
		ret = -EINVAL;
	if (ser->io_type != UPIO_MEM)
		ret = -EINVAL;
	if (sport->port.uartclk / 16 != ser->baud_base)
		ret = -EINVAL;
	if ((void *)sport->port.mapbase != ser->iomem_base)
		ret = -EINVAL;
	if (sport->port.iobase != ser->port)
		ret = -EINVAL;
	if (ser->hub6 != 0)
		ret = -EINVAL;
	return ret;
}

#if defined(CONFIG_CONSOLE_POLL)
static int imx_poll_get_char(struct uart_port *port)
{
	struct imx_port_ucrs old_ucr;
	unsigned int status;
	unsigned char c;

	/* save control registers */
	imx_port_ucrs_save(port, &old_ucr);

	/* disable interrupts */
	writel(UCR1_UARTEN, port->membase + UCR1);
	writel(old_ucr.ucr2 & ~(UCR2_ATEN | UCR2_RTSEN | UCR2_ESCI),
	       port->membase + UCR2);
	writel(old_ucr.ucr3 & ~(UCR3_DCD | UCR3_RI | UCR3_DTREN),
	       port->membase + UCR3);

	/* poll */
	do {
		status = readl(port->membase + USR2);
	} while (~status & USR2_RDR);

	/* read */
	c = readl(port->membase + URXD0);

	/* restore control registers */
	imx_port_ucrs_restore(port, &old_ucr);

	return c;
}

static void imx_poll_put_char(struct uart_port *port, unsigned char c)
{
	struct imx_port_ucrs old_ucr;
	unsigned int status;

	/* save control registers */
	imx_port_ucrs_save(port, &old_ucr);

	/* disable interrupts */
	writel(UCR1_UARTEN, port->membase + UCR1);
	writel(old_ucr.ucr2 & ~(UCR2_ATEN | UCR2_RTSEN | UCR2_ESCI),
	       port->membase + UCR2);
	writel(old_ucr.ucr3 & ~(UCR3_DCD | UCR3_RI | UCR3_DTREN),
	       port->membase + UCR3);

	/* drain */
	do {
		status = readl(port->membase + USR1);
	} while (~status & USR1_TRDY);

	/* write */
	writel(c, port->membase + URTX0);

	/* flush */
	do {
		status = readl(port->membase + USR2);
	} while (~status & USR2_TXDC);

	/* restore control registers */
	imx_port_ucrs_restore(port, &old_ucr);
}
#endif

static struct uart_ops imx_pops = {
	.tx_empty	= imx_tx_empty,
	.set_mctrl	= imx_set_mctrl,
	.get_mctrl	= imx_get_mctrl,
	.stop_tx	= imx_stop_tx,
	.start_tx	= imx_start_tx,
	.stop_rx	= imx_stop_rx,
	.enable_ms	= imx_enable_ms,
	.break_ctl	= imx_break_ctl,
	.startup	= imx_startup,
	.shutdown	= imx_shutdown,
	.set_termios	= imx_set_termios,
	.type		= imx_type,
	.release_port	= imx_release_port,
	.request_port	= imx_request_port,
	.config_port	= imx_config_port,
	.verify_port	= imx_verify_port,
#if defined(CONFIG_CONSOLE_POLL)
	.poll_get_char  = imx_poll_get_char,
	.poll_put_char  = imx_poll_put_char,
#endif
};

static struct imx_port *imx_ports[UART_NR];

#ifdef CONFIG_SERIAL_IMX_CONSOLE
static void imx_console_putchar(struct uart_port *port, int ch)
{
	struct imx_port *sport = (struct imx_port *)port;

	while (readl(sport->port.membase + uts_reg(sport)) & UTS_TXFULL)
		barrier();

	writel(ch, sport->port.membase + URTX0);
}

/*
 * Interrupts are disabled on entering
 */
static void
imx_console_write(struct console *co, const char *s, unsigned int count)
{
	struct imx_port *sport = imx_ports[co->index];
	struct imx_port_ucrs old_ucr;
	unsigned int ucr1;
	unsigned long flags = 0;
	int locked = 1;
	int retval;

	retval = clk_enable(sport->clk_per);
	if (retval)
		return;
	retval = clk_enable(sport->clk_ipg);
	if (retval) {
		clk_disable(sport->clk_per);
		return;
	}

	if (sport->port.sysrq)
		locked = 0;
	else if (oops_in_progress)
		locked = spin_trylock_irqsave(&sport->port.lock, flags);
	else
		spin_lock_irqsave(&sport->port.lock, flags);

	/*
	 *	First, save UCR1/2/3 and then disable interrupts
	 */
	imx_port_ucrs_save(&sport->port, &old_ucr);
	ucr1 = old_ucr.ucr1;

	if (is_imx1_uart(sport))
		ucr1 |= IMX1_UCR1_UARTCLKEN;
	ucr1 |= UCR1_UARTEN;
	ucr1 &= ~(UCR1_TXMPTYEN | UCR1_RRDYEN | UCR1_RTSDEN);

	writel(ucr1, sport->port.membase + UCR1);

	writel(old_ucr.ucr2 | UCR2_TXEN, sport->port.membase + UCR2);

	uart_console_write(&sport->port, s, count, imx_console_putchar);

	/*
	 *	Finally, wait for transmitter to become empty
	 *	and restore UCR1/2/3
	 */
	while (!(readl(sport->port.membase + USR2) & USR2_TXDC));

	imx_port_ucrs_restore(&sport->port, &old_ucr);

	if (locked)
		spin_unlock_irqrestore(&sport->port.lock, flags);

	clk_disable(sport->clk_ipg);
	clk_disable(sport->clk_per);
}

/*
 * If the port was already initialised (eg, by a boot loader),
 * try to determine the current setup.
 */
static void __init
imx_console_get_options(struct imx_port *sport, int *baud,
			   int *parity, int *bits)
{

	if (readl(sport->port.membase + UCR1) & UCR1_UARTEN) {
		/* ok, the port was enabled */
		unsigned int ucr2, ubir, ubmr, uartclk;
		unsigned int baud_raw;
		unsigned int ucfr_rfdiv;

		ucr2 = readl(sport->port.membase + UCR2);

		*parity = 'n';
		if (ucr2 & UCR2_PREN) {
			if (ucr2 & UCR2_PROE)
				*parity = 'o';
			else
				*parity = 'e';
		}

		if (ucr2 & UCR2_WS)
			*bits = 8;
		else
			*bits = 7;

		ubir = readl(sport->port.membase + UBIR) & 0xffff;
		ubmr = readl(sport->port.membase + UBMR) & 0xffff;

		ucfr_rfdiv = (readl(sport->port.membase + UFCR) & UFCR_RFDIV) >> 7;
		if (ucfr_rfdiv == 6)
			ucfr_rfdiv = 7;
		else
			ucfr_rfdiv = 6 - ucfr_rfdiv;

		uartclk = clk_get_rate(sport->clk_per);
		uartclk /= ucfr_rfdiv;

		{	/*
			 * The next code provides exact computation of
			 *   baud_raw = round(((uartclk/16) * (ubir + 1)) / (ubmr + 1))
			 * without need of float support or long long division,
			 * which would be required to prevent 32bit arithmetic overflow
			 */
			unsigned int mul = ubir + 1;
			unsigned int div = 16 * (ubmr + 1);
			unsigned int rem = uartclk % div;

			baud_raw = (uartclk / div) * mul;
			baud_raw += (rem * mul + div / 2) / div;
			*baud = (baud_raw + 50) / 100 * 100;
		}

		if (*baud != baud_raw)
			pr_info("Console IMX rounded baud rate from %d to %d\n",
				baud_raw, *baud);
	}
}

static int __init
imx_console_setup(struct console *co, char *options)
{
	struct imx_port *sport;
	int baud = 9600;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
	int retval;

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (co->index == -1 || co->index >= ARRAY_SIZE(imx_ports))
		co->index = 0;
	sport = imx_ports[co->index];
	if (sport == NULL)
		return -ENODEV;

	/* For setting the registers, we only need to enable the ipg clock. */
	retval = clk_prepare_enable(sport->clk_ipg);
	if (retval)
		goto error_console;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		imx_console_get_options(sport, &baud, &parity, &bits);

	imx_setup_ufcr(sport, 0);

	retval = uart_set_options(&sport->port, co, baud, parity, bits, flow);

	clk_disable(sport->clk_ipg);
	if (retval) {
		clk_unprepare(sport->clk_ipg);
		goto error_console;
	}

	retval = clk_prepare(sport->clk_per);
	if (retval)
		clk_disable_unprepare(sport->clk_ipg);

error_console:
	return retval;
}

static struct uart_driver imx_reg;
static struct console imx_console = {
	.name		= DEV_NAME,
	.write		= imx_console_write,
	.device		= uart_console_device,
	.setup		= imx_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &imx_reg,
};

#define IMX_CONSOLE	&imx_console
#else
#define IMX_CONSOLE	NULL
#endif

static struct uart_driver imx_reg = {
	.owner          = THIS_MODULE,
	.driver_name    = DRIVER_NAME,
	.dev_name       = DEV_NAME,
	.major          = SERIAL_IMX_MAJOR,
	.minor          = MINOR_START,
	.nr             = ARRAY_SIZE(imx_ports),
	.cons           = IMX_CONSOLE,
};

static int serial_imx_suspend(struct platform_device *dev, pm_message_t state)
{
	struct imx_port *sport = platform_get_drvdata(dev);
	unsigned int val;

	/* enable wakeup from i.MX UART */
	val = readl(sport->port.membase + UCR3);
	val |= UCR3_AWAKEN;
	writel(val, sport->port.membase + UCR3);

	uart_suspend_port(&imx_reg, &sport->port);

	return 0;
}

static int serial_imx_resume(struct platform_device *dev)
{
	struct imx_port *sport = platform_get_drvdata(dev);
	unsigned int val;

	/* disable wakeup from i.MX UART */
	val = readl(sport->port.membase + UCR3);
	val &= ~UCR3_AWAKEN;
	writel(val, sport->port.membase + UCR3);

	uart_resume_port(&imx_reg, &sport->port);

	return 0;
}

#ifdef CONFIG_OF
/*
 * This function returns 1 iff pdev isn't a device instatiated by dt, 0 iff it
 * could successfully get all information from dt or a negative errno.
 */
static int serial_imx_probe_dt(struct imx_port *sport,
		struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *of_id =
			of_match_device(imx_uart_dt_ids, &pdev->dev);
	int ret;

	if (!np)
		/* no device tree device */
		return 1;

	ret = of_alias_get_id(np, "serial");
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get alias id, errno %d\n", ret);
		return ret;
	}
	sport->port.line = ret;

	if (of_get_property(np, "fsl,uart-has-rtscts", NULL))
		sport->have_rtscts = 1;

	if (of_get_property(np, "fsl,irda-mode", NULL))
		sport->use_irda = 1;

	if (of_get_property(np, "fsl,dte-mode", NULL))
		sport->dte_mode = 1;

	sport->devdata = of_id->data;

	return 0;
}
#else
static inline int serial_imx_probe_dt(struct imx_port *sport,
		struct platform_device *pdev)
{
	return 1;
}
#endif

static void serial_imx_probe_pdata(struct imx_port *sport,
		struct platform_device *pdev)
{
	struct imxuart_platform_data *pdata = dev_get_platdata(&pdev->dev);

	sport->port.line = pdev->id;
	sport->devdata = (struct imx_uart_data	*) pdev->id_entry->driver_data;

	if (!pdata)
		return;

	if (pdata->flags & IMXUART_HAVE_RTSCTS)
		sport->have_rtscts = 1;

	if (pdata->flags & IMXUART_IRDA)
		sport->use_irda = 1;
}

static int serial_imx_probe(struct platform_device *pdev)
{
	struct imx_port *sport;
	struct imxuart_platform_data *pdata;
	void __iomem *base;
	int ret = 0;
	struct resource *res;

	sport = devm_kzalloc(&pdev->dev, sizeof(*sport), GFP_KERNEL);
	if (!sport)
		return -ENOMEM;

	ret = serial_imx_probe_dt(sport, pdev);
	if (ret > 0)
		serial_imx_probe_pdata(sport, pdev);
	else if (ret < 0)
		return ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	base = devm_ioremap(&pdev->dev, res->start, PAGE_SIZE);
	if (!base)
		return -ENOMEM;

	sport->port.dev = &pdev->dev;
	sport->port.mapbase = res->start;
	sport->port.membase = base;
	sport->port.type = PORT_IMX,
	sport->port.iotype = UPIO_MEM;
	sport->port.irq = platform_get_irq(pdev, 0);
	sport->rxirq = platform_get_irq(pdev, 0);
	sport->txirq = platform_get_irq(pdev, 1);
	sport->rtsirq = platform_get_irq(pdev, 2);
	sport->port.fifosize = 32;
	sport->port.ops = &imx_pops;
	sport->port.flags = UPF_BOOT_AUTOCONF;
	init_timer(&sport->timer);
	sport->timer.function = imx_timeout;
	sport->timer.data     = (unsigned long)sport;

	sport->clk_ipg = devm_clk_get(&pdev->dev, "ipg");
	if (IS_ERR(sport->clk_ipg)) {
		ret = PTR_ERR(sport->clk_ipg);
		dev_err(&pdev->dev, "failed to get ipg clk: %d\n", ret);
		return ret;
	}

	sport->clk_per = devm_clk_get(&pdev->dev, "per");
	if (IS_ERR(sport->clk_per)) {
		ret = PTR_ERR(sport->clk_per);
		dev_err(&pdev->dev, "failed to get per clk: %d\n", ret);
		return ret;
	}

	sport->port.uartclk = clk_get_rate(sport->clk_per);

	imx_ports[sport->port.line] = sport;

	pdata = dev_get_platdata(&pdev->dev);
	if (pdata && pdata->init) {
		ret = pdata->init(pdev);
		if (ret)
			return ret;
	}

	ret = uart_add_one_port(&imx_reg, &sport->port);
	if (ret)
		goto deinit;
	platform_set_drvdata(pdev, sport);

	return 0;
deinit:
	if (pdata && pdata->exit)
		pdata->exit(pdev);
	return ret;
}

static int serial_imx_remove(struct platform_device *pdev)
{
	struct imxuart_platform_data *pdata;
	struct imx_port *sport = platform_get_drvdata(pdev);

	pdata = dev_get_platdata(&pdev->dev);

	uart_remove_one_port(&imx_reg, &sport->port);

	if (pdata && pdata->exit)
		pdata->exit(pdev);

	return 0;
}

static struct platform_driver serial_imx_driver = {
	.probe		= serial_imx_probe,
	.remove		= serial_imx_remove,

	.suspend	= serial_imx_suspend,
	.resume		= serial_imx_resume,
	.id_table	= imx_uart_devtype,
	.driver		= {
		.name	= "imx-uart",
		.owner	= THIS_MODULE,
		.of_match_table = imx_uart_dt_ids,
	},
};

static int __init imx_serial_init(void)
{
	int ret;

	pr_info("Serial: IMX driver\n");

	ret = uart_register_driver(&imx_reg);
	if (ret)
		return ret;

	ret = platform_driver_register(&serial_imx_driver);
	if (ret != 0)
		uart_unregister_driver(&imx_reg);

	return ret;
}

static void __exit imx_serial_exit(void)
{
	platform_driver_unregister(&serial_imx_driver);
	uart_unregister_driver(&imx_reg);
}

module_init(imx_serial_init);
module_exit(imx_serial_exit);

MODULE_AUTHOR("Sascha Hauer");
MODULE_DESCRIPTION("IMX generic serial port driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:imx-uart");
