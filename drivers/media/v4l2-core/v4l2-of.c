/*
 * V4L2 OF binding parsing library
 *
 * Copyright (C) 2012 - 2013 Samsung Electronics Co., Ltd.
 * Author: Sylwester Nawrocki <s.nawrocki@samsung.com>
 *
 * Copyright (C) 2012 Renesas Electronics Corp.
 * Author: Guennadi Liakhovetski <g.liakhovetski@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 */
#include <fikus/kernel.h>
#include <fikus/module.h>
#include <fikus/of.h>
#include <fikus/string.h>
#include <fikus/types.h>

#include <media/v4l2-of.h>

static void v4l2_of_parse_csi_bus(const struct device_node *node,
				  struct v4l2_of_endpoint *endpoint)
{
	struct v4l2_of_bus_mipi_csi2 *bus = &endpoint->bus.mipi_csi2;
	u32 data_lanes[ARRAY_SIZE(bus->data_lanes)];
	struct property *prop;
	bool have_clk_lane = false;
	unsigned int flags = 0;
	u32 v;

	prop = of_find_property(node, "data-lanes", NULL);
	if (prop) {
		const __be32 *lane = NULL;
		int i;

		for (i = 0; i < ARRAY_SIZE(data_lanes); i++) {
			lane = of_prop_next_u32(prop, lane, &data_lanes[i]);
			if (!lane)
				break;
		}
		bus->num_data_lanes = i;
		while (i--)
			bus->data_lanes[i] = data_lanes[i];
	}

	if (!of_property_read_u32(node, "clock-lanes", &v)) {
		bus->clock_lane = v;
		have_clk_lane = true;
	}

	if (of_get_property(node, "clock-noncontinuous", &v))
		flags |= V4L2_MBUS_CSI2_NONCONTINUOUS_CLOCK;
	else if (have_clk_lane || bus->num_data_lanes > 0)
		flags |= V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;

	bus->flags = flags;
	endpoint->bus_type = V4L2_MBUS_CSI2;
}

static void v4l2_of_parse_parallel_bus(const struct device_node *node,
				       struct v4l2_of_endpoint *endpoint)
{
	struct v4l2_of_bus_parallel *bus = &endpoint->bus.parallel;
	unsigned int flags = 0;
	u32 v;

	if (!of_property_read_u32(node, "hsync-active", &v))
		flags |= v ? V4L2_MBUS_HSYNC_ACTIVE_HIGH :
			V4L2_MBUS_HSYNC_ACTIVE_LOW;

	if (!of_property_read_u32(node, "vsync-active", &v))
		flags |= v ? V4L2_MBUS_VSYNC_ACTIVE_HIGH :
			V4L2_MBUS_VSYNC_ACTIVE_LOW;

	if (!of_property_read_u32(node, "pclk-sample", &v))
		flags |= v ? V4L2_MBUS_PCLK_SAMPLE_RISING :
			V4L2_MBUS_PCLK_SAMPLE_FALLING;

	if (!of_property_read_u32(node, "field-even-active", &v))
		flags |= v ? V4L2_MBUS_FIELD_EVEN_HIGH :
			V4L2_MBUS_FIELD_EVEN_LOW;
	if (flags)
		endpoint->bus_type = V4L2_MBUS_PARALLEL;
	else
		endpoint->bus_type = V4L2_MBUS_BT656;

	if (!of_property_read_u32(node, "data-active", &v))
		flags |= v ? V4L2_MBUS_DATA_ACTIVE_HIGH :
			V4L2_MBUS_DATA_ACTIVE_LOW;

	if (of_get_property(node, "slave-mode", &v))
		flags |= V4L2_MBUS_SLAVE;
	else
		flags |= V4L2_MBUS_MASTER;

	if (!of_property_read_u32(node, "bus-width", &v))
		bus->bus_width = v;

	if (!of_property_read_u32(node, "data-shift", &v))
		bus->data_shift = v;

	if (!of_property_read_u32(node, "sync-on-green-active", &v))
		flags |= v ? V4L2_MBUS_VIDEO_SOG_ACTIVE_HIGH :
			V4L2_MBUS_VIDEO_SOG_ACTIVE_LOW;

	bus->flags = flags;

}

/**
 * v4l2_of_parse_endpoint() - parse all endpoint node properties
 * @node: pointer to endpoint device_node
 * @endpoint: pointer to the V4L2 OF endpoint data structure
 *
 * All properties are optional. If none are found, we don't set any flags.
 * This means the port has a static configuration and no properties have
 * to be specified explicitly.
 * If any properties that identify the bus as parallel are found and
 * slave-mode isn't set, we set V4L2_MBUS_MASTER. Similarly, if we recognise
 * the bus as serial CSI-2 and clock-noncontinuous isn't set, we set the
 * V4L2_MBUS_CSI2_CONTINUOUS_CLOCK flag.
 * The caller should hold a reference to @node.
 */
void v4l2_of_parse_endpoint(const struct device_node *node,
			    struct v4l2_of_endpoint *endpoint)
{
	struct device_node *port_node = of_get_parent(node);

	memset(endpoint, 0, offsetof(struct v4l2_of_endpoint, head));

	endpoint->local_node = node;
	/*
	 * It doesn't matter whether the two calls below succeed.
	 * If they don't then the default value 0 is used.
	 */
	of_property_read_u32(port_node, "reg", &endpoint->port);
	of_property_read_u32(node, "reg", &endpoint->id);

	v4l2_of_parse_csi_bus(node, endpoint);
	/*
	 * Parse the parallel video bus properties only if none
	 * of the MIPI CSI-2 specific properties were found.
	 */
	if (endpoint->bus.mipi_csi2.flags == 0)
		v4l2_of_parse_parallel_bus(node, endpoint);

	of_node_put(port_node);
}
EXPORT_SYMBOL(v4l2_of_parse_endpoint);

/**
 * v4l2_of_get_next_endpoint() - get next endpoint node
 * @parent: pointer to the parent device node
 * @prev: previous endpoint node, or NULL to get first
 *
 * Return: An 'endpoint' node pointer with refcount incremented. Refcount
 * of the passed @prev node is not decremented, the caller have to use
 * of_node_put() on it when done.
 */
struct device_node *v4l2_of_get_next_endpoint(const struct device_node *parent,
					struct device_node *prev)
{
	struct device_node *endpoint;
	struct device_node *port = NULL;

	if (!parent)
		return NULL;

	if (!prev) {
		struct device_node *node;
		/*
		 * It's the first call, we have to find a port subnode
		 * within this node or within an optional 'ports' node.
		 */
		node = of_get_child_by_name(parent, "ports");
		if (node)
			parent = node;

		port = of_get_child_by_name(parent, "port");

		if (port) {
			/* Found a port, get an endpoint. */
			endpoint = of_get_next_child(port, NULL);
			of_node_put(port);
		} else {
			endpoint = NULL;
		}

		if (!endpoint)
			pr_err("%s(): no endpoint nodes specified for %s\n",
			       __func__, parent->full_name);
		of_node_put(node);
	} else {
		port = of_get_parent(prev);
		if (!port)
			/* Hm, has someone given us the root node ?... */
			return NULL;

		/* Avoid dropping prev node refcount to 0. */
		of_node_get(prev);
		endpoint = of_get_next_child(port, prev);
		if (endpoint) {
			of_node_put(port);
			return endpoint;
		}

		/* No more endpoints under this port, try the next one. */
		do {
			port = of_get_next_child(parent, port);
			if (!port)
				return NULL;
		} while (of_node_cmp(port->name, "port"));

		/* Pick up the first endpoint in this port. */
		endpoint = of_get_next_child(port, NULL);
		of_node_put(port);
	}

	return endpoint;
}
EXPORT_SYMBOL(v4l2_of_get_next_endpoint);

/**
 * v4l2_of_get_remote_port_parent() - get remote port's parent node
 * @node: pointer to a local endpoint device_node
 *
 * Return: Remote device node associated with remote endpoint node linked
 *	   to @node. Use of_node_put() on it when done.
 */
struct device_node *v4l2_of_get_remote_port_parent(
			       const struct device_node *node)
{
	struct device_node *np;
	unsigned int depth;

	/* Get remote endpoint node. */
	np = of_parse_phandle(node, "remote-endpoint", 0);

	/* Walk 3 levels up only if there is 'ports' node. */
	for (depth = 3; depth && np; depth--) {
		np = of_get_next_parent(np);
		if (depth == 2 && of_node_cmp(np->name, "ports"))
			break;
	}
	return np;
}
EXPORT_SYMBOL(v4l2_of_get_remote_port_parent);

/**
 * v4l2_of_get_remote_port() - get remote port node
 * @node: pointer to a local endpoint device_node
 *
 * Return: Remote port node associated with remote endpoint node linked
 *	   to @node. Use of_node_put() on it when done.
 */
struct device_node *v4l2_of_get_remote_port(const struct device_node *node)
{
	struct device_node *np;

	/* Get remote endpoint node. */
	np = of_parse_phandle(node, "remote-endpoint", 0);
	if (!np)
		return NULL;
	return of_get_parent(np);
}
EXPORT_SYMBOL(v4l2_of_get_remote_port);
