/*
 * Freescale MXS SPI master driver
 *
 * Copyright 2012 DENX Software Engineering, GmbH.
 * Copyright 2012 Freescale Semiconductor, Inc.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
 *
 * Rework and transition to new API by:
 * Marek Vasut <marex@denx.de>
 *
 * Based on previous attempt by:
 * Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Based on code from U-Boot bootloader by:
 * Marek Vasut <marex@denx.de>
 *
 * Based on spi-stmp.c, which is:
 * Author: Dmitry Pervushin <dimka@embeddedalley.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <fikus/kernel.h>
#include <fikus/init.h>
#include <fikus/ioport.h>
#include <fikus/of.h>
#include <fikus/of_device.h>
#include <fikus/of_gpio.h>
#include <fikus/platform_device.h>
#include <fikus/delay.h>
#include <fikus/interrupt.h>
#include <fikus/dma-mapping.h>
#include <fikus/dmaengine.h>
#include <fikus/highmem.h>
#include <fikus/clk.h>
#include <fikus/err.h>
#include <fikus/completion.h>
#include <fikus/gpio.h>
#include <fikus/regulator/consumer.h>
#include <fikus/module.h>
#include <fikus/stmp_device.h>
#include <fikus/spi/spi.h>
#include <fikus/spi/mxs-spi.h>

#define DRIVER_NAME		"mxs-spi"

/* Use 10S timeout for very long transfers, it should suffice. */
#define SSP_TIMEOUT		10000

#define SG_MAXLEN		0xff00

struct mxs_spi {
	struct mxs_ssp		ssp;
	struct completion	c;
};

static int mxs_spi_setup_transfer(struct spi_device *dev,
				struct spi_transfer *t)
{
	struct mxs_spi *spi = spi_master_get_devdata(dev->master);
	struct mxs_ssp *ssp = &spi->ssp;
	uint32_t hz = 0;

	hz = dev->max_speed_hz;
	if (t && t->speed_hz)
		hz = min(hz, t->speed_hz);
	if (hz == 0) {
		dev_err(&dev->dev, "Cannot continue with zero clock\n");
		return -EINVAL;
	}

	mxs_ssp_set_clk_rate(ssp, hz);

	writel(BF_SSP_CTRL1_SSP_MODE(BV_SSP_CTRL1_SSP_MODE__SPI) |
		     BF_SSP_CTRL1_WORD_LENGTH
		     (BV_SSP_CTRL1_WORD_LENGTH__EIGHT_BITS) |
		     ((dev->mode & SPI_CPOL) ? BM_SSP_CTRL1_POLARITY : 0) |
		     ((dev->mode & SPI_CPHA) ? BM_SSP_CTRL1_PHASE : 0),
		     ssp->base + HW_SSP_CTRL1(ssp));

	writel(0x0, ssp->base + HW_SSP_CMD0);
	writel(0x0, ssp->base + HW_SSP_CMD1);

	return 0;
}

static int mxs_spi_setup(struct spi_device *dev)
{
	int err = 0;

	if (!dev->bits_per_word)
		dev->bits_per_word = 8;

	if (dev->mode & ~(SPI_CPOL | SPI_CPHA))
		return -EINVAL;

	err = mxs_spi_setup_transfer(dev, NULL);
	if (err) {
		dev_err(&dev->dev,
			"Failed to setup transfer, error = %d\n", err);
	}

	return err;
}

static uint32_t mxs_spi_cs_to_reg(unsigned cs)
{
	uint32_t select = 0;

	/*
	 * i.MX28 Datasheet: 17.10.1: HW_SSP_CTRL0
	 *
	 * The bits BM_SSP_CTRL0_WAIT_FOR_CMD and BM_SSP_CTRL0_WAIT_FOR_IRQ
	 * in HW_SSP_CTRL0 register do have multiple usage, please refer to
	 * the datasheet for further details. In SPI mode, they are used to
	 * toggle the chip-select lines (nCS pins).
	 */
	if (cs & 1)
		select |= BM_SSP_CTRL0_WAIT_FOR_CMD;
	if (cs & 2)
		select |= BM_SSP_CTRL0_WAIT_FOR_IRQ;

	return select;
}

static void mxs_spi_set_cs(struct mxs_spi *spi, unsigned cs)
{
	const uint32_t mask =
		BM_SSP_CTRL0_WAIT_FOR_CMD | BM_SSP_CTRL0_WAIT_FOR_IRQ;
	uint32_t select;
	struct mxs_ssp *ssp = &spi->ssp;

	writel(mask, ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_CLR);
	select = mxs_spi_cs_to_reg(cs);
	writel(select, ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);
}

static inline void mxs_spi_enable(struct mxs_spi *spi)
{
	struct mxs_ssp *ssp = &spi->ssp;

	writel(BM_SSP_CTRL0_LOCK_CS,
		ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);
	writel(BM_SSP_CTRL0_IGNORE_CRC,
		ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_CLR);
}

static inline void mxs_spi_disable(struct mxs_spi *spi)
{
	struct mxs_ssp *ssp = &spi->ssp;

	writel(BM_SSP_CTRL0_LOCK_CS,
		ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_CLR);
	writel(BM_SSP_CTRL0_IGNORE_CRC,
		ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);
}

static int mxs_ssp_wait(struct mxs_spi *spi, int offset, int mask, bool set)
{
	const unsigned long timeout = jiffies + msecs_to_jiffies(SSP_TIMEOUT);
	struct mxs_ssp *ssp = &spi->ssp;
	uint32_t reg;

	do {
		reg = readl_relaxed(ssp->base + offset);

		if (!set)
			reg = ~reg;

		reg &= mask;

		if (reg == mask)
			return 0;
	} while (time_before(jiffies, timeout));

	return -ETIMEDOUT;
}

static void mxs_ssp_dma_irq_callback(void *param)
{
	struct mxs_spi *spi = param;
	complete(&spi->c);
}

static irqreturn_t mxs_ssp_irq_handler(int irq, void *dev_id)
{
	struct mxs_ssp *ssp = dev_id;
	dev_err(ssp->dev, "%s[%i] CTRL1=%08x STATUS=%08x\n",
		__func__, __LINE__,
		readl(ssp->base + HW_SSP_CTRL1(ssp)),
		readl(ssp->base + HW_SSP_STATUS(ssp)));
	return IRQ_HANDLED;
}

static int mxs_spi_txrx_dma(struct mxs_spi *spi, int cs,
			    unsigned char *buf, int len,
			    int *first, int *last, int write)
{
	struct mxs_ssp *ssp = &spi->ssp;
	struct dma_async_tx_descriptor *desc = NULL;
	const bool vmalloced_buf = is_vmalloc_addr(buf);
	const int desc_len = vmalloced_buf ? PAGE_SIZE : SG_MAXLEN;
	const int sgs = DIV_ROUND_UP(len, desc_len);
	int sg_count;
	int min, ret;
	uint32_t ctrl0;
	struct page *vm_page;
	void *sg_buf;
	struct {
		uint32_t		pio[4];
		struct scatterlist	sg;
	} *dma_xfer;

	if (!len)
		return -EINVAL;

	dma_xfer = kzalloc(sizeof(*dma_xfer) * sgs, GFP_KERNEL);
	if (!dma_xfer)
		return -ENOMEM;

	INIT_COMPLETION(spi->c);

	ctrl0 = readl(ssp->base + HW_SSP_CTRL0);
	ctrl0 &= ~BM_SSP_CTRL0_XFER_COUNT;
	ctrl0 |= BM_SSP_CTRL0_DATA_XFER | mxs_spi_cs_to_reg(cs);

	if (*first)
		ctrl0 |= BM_SSP_CTRL0_LOCK_CS;
	if (!write)
		ctrl0 |= BM_SSP_CTRL0_READ;

	/* Queue the DMA data transfer. */
	for (sg_count = 0; sg_count < sgs; sg_count++) {
		min = min(len, desc_len);

		/* Prepare the transfer descriptor. */
		if ((sg_count + 1 == sgs) && *last)
			ctrl0 |= BM_SSP_CTRL0_IGNORE_CRC;

		if (ssp->devid == IMX23_SSP) {
			ctrl0 &= ~BM_SSP_CTRL0_XFER_COUNT;
			ctrl0 |= min;
		}

		dma_xfer[sg_count].pio[0] = ctrl0;
		dma_xfer[sg_count].pio[3] = min;

		if (vmalloced_buf) {
			vm_page = vmalloc_to_page(buf);
			if (!vm_page) {
				ret = -ENOMEM;
				goto err_vmalloc;
			}
			sg_buf = page_address(vm_page) +
				((size_t)buf & ~PAGE_MASK);
		} else {
			sg_buf = buf;
		}

		sg_init_one(&dma_xfer[sg_count].sg, sg_buf, min);
		ret = dma_map_sg(ssp->dev, &dma_xfer[sg_count].sg, 1,
			write ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

		len -= min;
		buf += min;

		/* Queue the PIO register write transfer. */
		desc = dmaengine_prep_slave_sg(ssp->dmach,
				(struct scatterlist *)dma_xfer[sg_count].pio,
				(ssp->devid == IMX23_SSP) ? 1 : 4,
				DMA_TRANS_NONE,
				sg_count ? DMA_PREP_INTERRUPT : 0);
		if (!desc) {
			dev_err(ssp->dev,
				"Failed to get PIO reg. write descriptor.\n");
			ret = -EINVAL;
			goto err_mapped;
		}

		desc = dmaengine_prep_slave_sg(ssp->dmach,
				&dma_xfer[sg_count].sg, 1,
				write ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

		if (!desc) {
			dev_err(ssp->dev,
				"Failed to get DMA data write descriptor.\n");
			ret = -EINVAL;
			goto err_mapped;
		}
	}

	/*
	 * The last descriptor must have this callback,
	 * to finish the DMA transaction.
	 */
	desc->callback = mxs_ssp_dma_irq_callback;
	desc->callback_param = spi;

	/* Start the transfer. */
	dmaengine_submit(desc);
	dma_async_issue_pending(ssp->dmach);

	ret = wait_for_completion_timeout(&spi->c,
				msecs_to_jiffies(SSP_TIMEOUT));
	if (!ret) {
		dev_err(ssp->dev, "DMA transfer timeout\n");
		ret = -ETIMEDOUT;
		dmaengine_terminate_all(ssp->dmach);
		goto err_vmalloc;
	}

	ret = 0;

err_vmalloc:
	while (--sg_count >= 0) {
err_mapped:
		dma_unmap_sg(ssp->dev, &dma_xfer[sg_count].sg, 1,
			write ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	}

	kfree(dma_xfer);

	return ret;
}

static int mxs_spi_txrx_pio(struct mxs_spi *spi, int cs,
			    unsigned char *buf, int len,
			    int *first, int *last, int write)
{
	struct mxs_ssp *ssp = &spi->ssp;

	if (*first)
		mxs_spi_enable(spi);

	mxs_spi_set_cs(spi, cs);

	while (len--) {
		if (*last && len == 0)
			mxs_spi_disable(spi);

		if (ssp->devid == IMX23_SSP) {
			writel(BM_SSP_CTRL0_XFER_COUNT,
				ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_CLR);
			writel(1,
				ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);
		} else {
			writel(1, ssp->base + HW_SSP_XFER_SIZE);
		}

		if (write)
			writel(BM_SSP_CTRL0_READ,
				ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_CLR);
		else
			writel(BM_SSP_CTRL0_READ,
				ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);

		writel(BM_SSP_CTRL0_RUN,
				ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);

		if (mxs_ssp_wait(spi, HW_SSP_CTRL0, BM_SSP_CTRL0_RUN, 1))
			return -ETIMEDOUT;

		if (write)
			writel(*buf, ssp->base + HW_SSP_DATA(ssp));

		writel(BM_SSP_CTRL0_DATA_XFER,
			     ssp->base + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);

		if (!write) {
			if (mxs_ssp_wait(spi, HW_SSP_STATUS(ssp),
						BM_SSP_STATUS_FIFO_EMPTY, 0))
				return -ETIMEDOUT;

			*buf = (readl(ssp->base + HW_SSP_DATA(ssp)) & 0xff);
		}

		if (mxs_ssp_wait(spi, HW_SSP_CTRL0, BM_SSP_CTRL0_RUN, 0))
			return -ETIMEDOUT;

		buf++;
	}

	if (len <= 0)
		return 0;

	return -ETIMEDOUT;
}

static int mxs_spi_transfer_one(struct spi_master *master,
				struct spi_message *m)
{
	struct mxs_spi *spi = spi_master_get_devdata(master);
	struct mxs_ssp *ssp = &spi->ssp;
	int first, last;
	struct spi_transfer *t, *tmp_t;
	int status = 0;
	int cs;

	first = last = 0;

	cs = m->spi->chip_select;

	list_for_each_entry_safe(t, tmp_t, &m->transfers, transfer_list) {

		status = mxs_spi_setup_transfer(m->spi, t);
		if (status)
			break;

		if (&t->transfer_list == m->transfers.next)
			first = 1;
		if (&t->transfer_list == m->transfers.prev)
			last = 1;
		if ((t->rx_buf && t->tx_buf) || (t->rx_dma && t->tx_dma)) {
			dev_err(ssp->dev,
				"Cannot send and receive simultaneously\n");
			status = -EINVAL;
			break;
		}

		/*
		 * Small blocks can be transfered via PIO.
		 * Measured by empiric means:
		 *
		 * dd if=/dev/mtdblock0 of=/dev/null bs=1024k count=1
		 *
		 * DMA only: 2.164808 seconds, 473.0KB/s
		 * Combined: 1.676276 seconds, 610.9KB/s
		 */
		if (t->len < 32) {
			writel(BM_SSP_CTRL1_DMA_ENABLE,
				ssp->base + HW_SSP_CTRL1(ssp) +
				STMP_OFFSET_REG_CLR);

			if (t->tx_buf)
				status = mxs_spi_txrx_pio(spi, cs,
						(void *)t->tx_buf,
						t->len, &first, &last, 1);
			if (t->rx_buf)
				status = mxs_spi_txrx_pio(spi, cs,
						t->rx_buf, t->len,
						&first, &last, 0);
		} else {
			writel(BM_SSP_CTRL1_DMA_ENABLE,
				ssp->base + HW_SSP_CTRL1(ssp) +
				STMP_OFFSET_REG_SET);

			if (t->tx_buf)
				status = mxs_spi_txrx_dma(spi, cs,
						(void *)t->tx_buf, t->len,
						&first, &last, 1);
			if (t->rx_buf)
				status = mxs_spi_txrx_dma(spi, cs,
						t->rx_buf, t->len,
						&first, &last, 0);
		}

		if (status) {
			stmp_reset_block(ssp->base);
			break;
		}

		m->actual_length += t->len;
		first = last = 0;
	}

	m->status = status;
	spi_finalize_current_message(master);

	return status;
}

static const struct of_device_id mxs_spi_dt_ids[] = {
	{ .compatible = "fsl,imx23-spi", .data = (void *) IMX23_SSP, },
	{ .compatible = "fsl,imx28-spi", .data = (void *) IMX28_SSP, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mxs_spi_dt_ids);

static int mxs_spi_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id =
			of_match_device(mxs_spi_dt_ids, &pdev->dev);
	struct device_node *np = pdev->dev.of_node;
	struct spi_master *master;
	struct mxs_spi *spi;
	struct mxs_ssp *ssp;
	struct resource *iores;
	struct clk *clk;
	void __iomem *base;
	int devid, clk_freq;
	int ret = 0, irq_err;

	/*
	 * Default clock speed for the SPI core. 160MHz seems to
	 * work reasonably well with most SPI flashes, so use this
	 * as a default. Override with "clock-frequency" DT prop.
	 */
	const int clk_freq_default = 160000000;

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq_err = platform_get_irq(pdev, 0);
	if (irq_err < 0)
		return -EINVAL;

	base = devm_ioremap_resource(&pdev->dev, iores);
	if (IS_ERR(base))
		return PTR_ERR(base);

	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	devid = (enum mxs_ssp_id) of_id->data;
	ret = of_property_read_u32(np, "clock-frequency",
				   &clk_freq);
	if (ret)
		clk_freq = clk_freq_default;

	master = spi_alloc_master(&pdev->dev, sizeof(*spi));
	if (!master)
		return -ENOMEM;

	master->transfer_one_message = mxs_spi_transfer_one;
	master->setup = mxs_spi_setup;
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->mode_bits = SPI_CPOL | SPI_CPHA;
	master->num_chipselect = 3;
	master->dev.of_node = np;
	master->flags = SPI_MASTER_HALF_DUPLEX;

	spi = spi_master_get_devdata(master);
	ssp = &spi->ssp;
	ssp->dev = &pdev->dev;
	ssp->clk = clk;
	ssp->base = base;
	ssp->devid = devid;

	init_completion(&spi->c);

	ret = devm_request_irq(&pdev->dev, irq_err, mxs_ssp_irq_handler, 0,
			       DRIVER_NAME, ssp);
	if (ret)
		goto out_master_free;

	ssp->dmach = dma_request_slave_channel(&pdev->dev, "rx-tx");
	if (!ssp->dmach) {
		dev_err(ssp->dev, "Failed to request DMA\n");
		ret = -ENODEV;
		goto out_master_free;
	}

	ret = clk_prepare_enable(ssp->clk);
	if (ret)
		goto out_dma_release;

	clk_set_rate(ssp->clk, clk_freq);
	ssp->clk_rate = clk_get_rate(ssp->clk) / 1000;

	ret = stmp_reset_block(ssp->base);
	if (ret)
		goto out_disable_clk;

	platform_set_drvdata(pdev, master);

	ret = spi_register_master(master);
	if (ret) {
		dev_err(&pdev->dev, "Cannot register SPI master, %d\n", ret);
		goto out_disable_clk;
	}

	return 0;

out_disable_clk:
	clk_disable_unprepare(ssp->clk);
out_dma_release:
	dma_release_channel(ssp->dmach);
out_master_free:
	spi_master_put(master);
	return ret;
}

static int mxs_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master;
	struct mxs_spi *spi;
	struct mxs_ssp *ssp;

	master = spi_master_get(platform_get_drvdata(pdev));
	spi = spi_master_get_devdata(master);
	ssp = &spi->ssp;

	spi_unregister_master(master);
	clk_disable_unprepare(ssp->clk);
	dma_release_channel(ssp->dmach);
	spi_master_put(master);

	return 0;
}

static struct platform_driver mxs_spi_driver = {
	.probe	= mxs_spi_probe,
	.remove	= mxs_spi_remove,
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = mxs_spi_dt_ids,
	},
};

module_platform_driver(mxs_spi_driver);

MODULE_AUTHOR("Marek Vasut <marex@denx.de>");
MODULE_DESCRIPTION("MXS SPI master driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:mxs-spi");
