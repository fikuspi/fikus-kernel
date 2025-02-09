/*
 * OF helpers for DMA request / controller
 *
 * Based on of_gpio.h
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __FIKUS_OF_DMA_H
#define __FIKUS_OF_DMA_H

#include <fikus/of.h>
#include <fikus/dmaengine.h>

struct device_node;

struct of_dma {
	struct list_head	of_dma_controllers;
	struct device_node	*of_node;
	struct dma_chan		*(*of_dma_xlate)
				(struct of_phandle_args *, struct of_dma *);
	void			*of_dma_data;
};

struct of_dma_filter_info {
	dma_cap_mask_t	dma_cap;
	dma_filter_fn	filter_fn;
};

#ifdef CONFIG_OF
extern int of_dma_controller_register(struct device_node *np,
		struct dma_chan *(*of_dma_xlate)
		(struct of_phandle_args *, struct of_dma *),
		void *data);
extern void of_dma_controller_free(struct device_node *np);
extern struct dma_chan *of_dma_request_slave_channel(struct device_node *np,
						     const char *name);
extern struct dma_chan *of_dma_simple_xlate(struct of_phandle_args *dma_spec,
		struct of_dma *ofdma);
#else
static inline int of_dma_controller_register(struct device_node *np,
		struct dma_chan *(*of_dma_xlate)
		(struct of_phandle_args *, struct of_dma *),
		void *data)
{
	return -ENODEV;
}

static inline void of_dma_controller_free(struct device_node *np)
{
}

static inline struct dma_chan *of_dma_request_slave_channel(struct device_node *np,
						     const char *name)
{
	return NULL;
}

static inline struct dma_chan *of_dma_simple_xlate(struct of_phandle_args *dma_spec,
		struct of_dma *ofdma)
{
	return NULL;
}

#endif

#endif /* __FIKUS_OF_DMA_H */
