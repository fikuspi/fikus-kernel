/*
 * Generic device tree based pinctrl driver for one register per pin
 * type pinmux controllers
 *
 * Copyright (C) 2012 Texas Instruments, Inc.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <fikus/init.h>
#include <fikus/module.h>
#include <fikus/io.h>
#include <fikus/slab.h>
#include <fikus/err.h>
#include <fikus/list.h>

#include <fikus/of.h>
#include <fikus/of_device.h>
#include <fikus/of_address.h>

#include <fikus/pinctrl/pinctrl.h>
#include <fikus/pinctrl/pinmux.h>
#include <fikus/pinctrl/pinconf-generic.h>

#include "core.h"
#include "pinconf.h"

#define DRIVER_NAME			"pinctrl-single"
#define PCS_MUX_PINS_NAME		"pinctrl-single,pins"
#define PCS_MUX_BITS_NAME		"pinctrl-single,bits"
#define PCS_REG_NAME_LEN		((sizeof(unsigned long) * 2) + 3)
#define PCS_OFF_DISABLED		~0U

/**
 * struct pcs_pingroup - pingroups for a function
 * @np:		pingroup device node pointer
 * @name:	pingroup name
 * @gpins:	array of the pins in the group
 * @ngpins:	number of pins in the group
 * @node:	list node
 */
struct pcs_pingroup {
	struct device_node *np;
	const char *name;
	int *gpins;
	int ngpins;
	struct list_head node;
};

/**
 * struct pcs_func_vals - mux function register offset and value pair
 * @reg:	register virtual address
 * @val:	register value
 */
struct pcs_func_vals {
	void __iomem *reg;
	unsigned val;
	unsigned mask;
};

/**
 * struct pcs_conf_vals - pinconf parameter, pinconf register offset
 * and value, enable, disable, mask
 * @param:	config parameter
 * @val:	user input bits in the pinconf register
 * @enable:	enable bits in the pinconf register
 * @disable:	disable bits in the pinconf register
 * @mask:	mask bits in the register value
 */
struct pcs_conf_vals {
	enum pin_config_param param;
	unsigned val;
	unsigned enable;
	unsigned disable;
	unsigned mask;
};

/**
 * struct pcs_conf_type - pinconf property name, pinconf param pair
 * @name:	property name in DTS file
 * @param:	config parameter
 */
struct pcs_conf_type {
	const char *name;
	enum pin_config_param param;
};

/**
 * struct pcs_function - pinctrl function
 * @name:	pinctrl function name
 * @vals:	register and vals array
 * @nvals:	number of entries in vals array
 * @pgnames:	array of pingroup names the function uses
 * @npgnames:	number of pingroup names the function uses
 * @node:	list node
 */
struct pcs_function {
	const char *name;
	struct pcs_func_vals *vals;
	unsigned nvals;
	const char **pgnames;
	int npgnames;
	struct pcs_conf_vals *conf;
	int nconfs;
	struct list_head node;
};

/**
 * struct pcs_gpiofunc_range - pin ranges with same mux value of gpio function
 * @offset:	offset base of pins
 * @npins:	number pins with the same mux value of gpio function
 * @gpiofunc:	mux value of gpio function
 * @node:	list node
 */
struct pcs_gpiofunc_range {
	unsigned offset;
	unsigned npins;
	unsigned gpiofunc;
	struct list_head node;
};

/**
 * struct pcs_data - wrapper for data needed by pinctrl framework
 * @pa:		pindesc array
 * @cur:	index to current element
 *
 * REVISIT: We should be able to drop this eventually by adding
 * support for registering pins individually in the pinctrl
 * framework for those drivers that don't need a static array.
 */
struct pcs_data {
	struct pinctrl_pin_desc *pa;
	int cur;
};

/**
 * struct pcs_name - register name for a pin
 * @name:	name of the pinctrl register
 *
 * REVISIT: We may want to make names optional in the pinctrl
 * framework as some drivers may not care about pin names to
 * avoid kernel bloat. The pin names can be deciphered by user
 * space tools using debugfs based on the register address and
 * SoC packaging information.
 */
struct pcs_name {
	char name[PCS_REG_NAME_LEN];
};

/**
 * struct pcs_device - pinctrl device instance
 * @res:	resources
 * @base:	virtual address of the controller
 * @size:	size of the ioremapped area
 * @dev:	device entry
 * @pctl:	pin controller device
 * @mutex:	mutex protecting the lists
 * @width:	bits per mux register
 * @fmask:	function register mask
 * @fshift:	function register shift
 * @foff:	value to turn mux off
 * @fmax:	max number of functions in fmask
 * @is_pinconf:	whether supports pinconf
 * @bits_per_pin:number of bits per pin
 * @names:	array of register names for pins
 * @pins:	physical pins on the SoC
 * @pgtree:	pingroup index radix tree
 * @ftree:	function index radix tree
 * @pingroups:	list of pingroups
 * @functions:	list of functions
 * @gpiofuncs:	list of gpio functions
 * @ngroups:	number of pingroups
 * @nfuncs:	number of functions
 * @desc:	pin controller descriptor
 * @read:	register read function to use
 * @write:	register write function to use
 */
struct pcs_device {
	struct resource *res;
	void __iomem *base;
	unsigned size;
	struct device *dev;
	struct pinctrl_dev *pctl;
	struct mutex mutex;
	unsigned width;
	unsigned fmask;
	unsigned fshift;
	unsigned foff;
	unsigned fmax;
	bool bits_per_mux;
	bool is_pinconf;
	unsigned bits_per_pin;
	struct pcs_name *names;
	struct pcs_data pins;
	struct radix_tree_root pgtree;
	struct radix_tree_root ftree;
	struct list_head pingroups;
	struct list_head functions;
	struct list_head gpiofuncs;
	unsigned ngroups;
	unsigned nfuncs;
	struct pinctrl_desc desc;
	unsigned (*read)(void __iomem *reg);
	void (*write)(unsigned val, void __iomem *reg);
};

static int pcs_pinconf_get(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *config);
static int pcs_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *configs, unsigned num_configs);

static enum pin_config_param pcs_bias[] = {
	PIN_CONFIG_BIAS_PULL_DOWN,
	PIN_CONFIG_BIAS_PULL_UP,
};

/*
 * REVISIT: Reads and writes could eventually use regmap or something
 * generic. But at least on omaps, some mux registers are performance
 * critical as they may need to be remuxed every time before and after
 * idle. Adding tests for register access width for every read and
 * write like regmap is doing is not desired, and caching the registers
 * does not help in this case.
 */

static unsigned __maybe_unused pcs_readb(void __iomem *reg)
{
	return readb(reg);
}

static unsigned __maybe_unused pcs_readw(void __iomem *reg)
{
	return readw(reg);
}

static unsigned __maybe_unused pcs_readl(void __iomem *reg)
{
	return readl(reg);
}

static void __maybe_unused pcs_writeb(unsigned val, void __iomem *reg)
{
	writeb(val, reg);
}

static void __maybe_unused pcs_writew(unsigned val, void __iomem *reg)
{
	writew(val, reg);
}

static void __maybe_unused pcs_writel(unsigned val, void __iomem *reg)
{
	writel(val, reg);
}

static int pcs_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct pcs_device *pcs;

	pcs = pinctrl_dev_get_drvdata(pctldev);

	return pcs->ngroups;
}

static const char *pcs_get_group_name(struct pinctrl_dev *pctldev,
					unsigned gselector)
{
	struct pcs_device *pcs;
	struct pcs_pingroup *group;

	pcs = pinctrl_dev_get_drvdata(pctldev);
	group = radix_tree_lookup(&pcs->pgtree, gselector);
	if (!group) {
		dev_err(pcs->dev, "%s could not find pingroup%i\n",
			__func__, gselector);
		return NULL;
	}

	return group->name;
}

static int pcs_get_group_pins(struct pinctrl_dev *pctldev,
					unsigned gselector,
					const unsigned **pins,
					unsigned *npins)
{
	struct pcs_device *pcs;
	struct pcs_pingroup *group;

	pcs = pinctrl_dev_get_drvdata(pctldev);
	group = radix_tree_lookup(&pcs->pgtree, gselector);
	if (!group) {
		dev_err(pcs->dev, "%s could not find pingroup%i\n",
			__func__, gselector);
		return -EINVAL;
	}

	*pins = group->gpins;
	*npins = group->ngpins;

	return 0;
}

static void pcs_pin_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *s,
					unsigned pin)
{
	struct pcs_device *pcs;
	unsigned val, mux_bytes;

	pcs = pinctrl_dev_get_drvdata(pctldev);

	mux_bytes = pcs->width / BITS_PER_BYTE;
	val = pcs->read(pcs->base + pin * mux_bytes);

	seq_printf(s, "%08x %s " , val, DRIVER_NAME);
}

static void pcs_dt_free_map(struct pinctrl_dev *pctldev,
				struct pinctrl_map *map, unsigned num_maps)
{
	struct pcs_device *pcs;

	pcs = pinctrl_dev_get_drvdata(pctldev);
	devm_kfree(pcs->dev, map);
}

static int pcs_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned *num_maps);

static const struct pinctrl_ops pcs_pinctrl_ops = {
	.get_groups_count = pcs_get_groups_count,
	.get_group_name = pcs_get_group_name,
	.get_group_pins = pcs_get_group_pins,
	.pin_dbg_show = pcs_pin_dbg_show,
	.dt_node_to_map = pcs_dt_node_to_map,
	.dt_free_map = pcs_dt_free_map,
};

static int pcs_get_functions_count(struct pinctrl_dev *pctldev)
{
	struct pcs_device *pcs;

	pcs = pinctrl_dev_get_drvdata(pctldev);

	return pcs->nfuncs;
}

static const char *pcs_get_function_name(struct pinctrl_dev *pctldev,
						unsigned fselector)
{
	struct pcs_device *pcs;
	struct pcs_function *func;

	pcs = pinctrl_dev_get_drvdata(pctldev);
	func = radix_tree_lookup(&pcs->ftree, fselector);
	if (!func) {
		dev_err(pcs->dev, "%s could not find function%i\n",
			__func__, fselector);
		return NULL;
	}

	return func->name;
}

static int pcs_get_function_groups(struct pinctrl_dev *pctldev,
					unsigned fselector,
					const char * const **groups,
					unsigned * const ngroups)
{
	struct pcs_device *pcs;
	struct pcs_function *func;

	pcs = pinctrl_dev_get_drvdata(pctldev);
	func = radix_tree_lookup(&pcs->ftree, fselector);
	if (!func) {
		dev_err(pcs->dev, "%s could not find function%i\n",
			__func__, fselector);
		return -EINVAL;
	}
	*groups = func->pgnames;
	*ngroups = func->npgnames;

	return 0;
}

static int pcs_get_function(struct pinctrl_dev *pctldev, unsigned pin,
			    struct pcs_function **func)
{
	struct pcs_device *pcs = pinctrl_dev_get_drvdata(pctldev);
	struct pin_desc *pdesc = pin_desc_get(pctldev, pin);
	const struct pinctrl_setting_mux *setting;
	unsigned fselector;

	/* If pin is not described in DTS & enabled, mux_setting is NULL. */
	setting = pdesc->mux_setting;
	if (!setting)
		return -ENOTSUPP;
	fselector = setting->func;
	*func = radix_tree_lookup(&pcs->ftree, fselector);
	if (!(*func)) {
		dev_err(pcs->dev, "%s could not find function%i\n",
			__func__, fselector);
		return -ENOTSUPP;
	}
	return 0;
}

static int pcs_enable(struct pinctrl_dev *pctldev, unsigned fselector,
	unsigned group)
{
	struct pcs_device *pcs;
	struct pcs_function *func;
	int i;

	pcs = pinctrl_dev_get_drvdata(pctldev);
	/* If function mask is null, needn't enable it. */
	if (!pcs->fmask)
		return 0;
	func = radix_tree_lookup(&pcs->ftree, fselector);
	if (!func)
		return -EINVAL;

	dev_dbg(pcs->dev, "enabling %s function%i\n",
		func->name, fselector);

	for (i = 0; i < func->nvals; i++) {
		struct pcs_func_vals *vals;
		unsigned val, mask;

		vals = &func->vals[i];
		val = pcs->read(vals->reg);

		if (pcs->bits_per_mux)
			mask = vals->mask;
		else
			mask = pcs->fmask;

		val &= ~mask;
		val |= (vals->val & mask);
		pcs->write(val, vals->reg);
	}

	return 0;
}

static void pcs_disable(struct pinctrl_dev *pctldev, unsigned fselector,
					unsigned group)
{
	struct pcs_device *pcs;
	struct pcs_function *func;
	int i;

	pcs = pinctrl_dev_get_drvdata(pctldev);
	/* If function mask is null, needn't disable it. */
	if (!pcs->fmask)
		return;

	func = radix_tree_lookup(&pcs->ftree, fselector);
	if (!func) {
		dev_err(pcs->dev, "%s could not find function%i\n",
			__func__, fselector);
		return;
	}

	/*
	 * Ignore disable if function-off is not specified. Some hardware
	 * does not have clearly defined disable function. For pin specific
	 * off modes, you can use alternate named states as described in
	 * pinctrl-bindings.txt.
	 */
	if (pcs->foff == PCS_OFF_DISABLED) {
		dev_dbg(pcs->dev, "ignoring disable for %s function%i\n",
			func->name, fselector);
		return;
	}

	dev_dbg(pcs->dev, "disabling function%i %s\n",
		fselector, func->name);

	for (i = 0; i < func->nvals; i++) {
		struct pcs_func_vals *vals;
		unsigned val;

		vals = &func->vals[i];
		val = pcs->read(vals->reg);
		val &= ~pcs->fmask;
		val |= pcs->foff << pcs->fshift;
		pcs->write(val, vals->reg);
	}
}

static int pcs_request_gpio(struct pinctrl_dev *pctldev,
			    struct pinctrl_gpio_range *range, unsigned pin)
{
	struct pcs_device *pcs = pinctrl_dev_get_drvdata(pctldev);
	struct pcs_gpiofunc_range *frange = NULL;
	struct list_head *pos, *tmp;
	int mux_bytes = 0;
	unsigned data;

	/* If function mask is null, return directly. */
	if (!pcs->fmask)
		return -ENOTSUPP;

	list_for_each_safe(pos, tmp, &pcs->gpiofuncs) {
		frange = list_entry(pos, struct pcs_gpiofunc_range, node);
		if (pin >= frange->offset + frange->npins
			|| pin < frange->offset)
			continue;
		mux_bytes = pcs->width / BITS_PER_BYTE;
		data = pcs->read(pcs->base + pin * mux_bytes) & ~pcs->fmask;
		data |= frange->gpiofunc;
		pcs->write(data, pcs->base + pin * mux_bytes);
		break;
	}
	return 0;
}

static const struct pinmux_ops pcs_pinmux_ops = {
	.get_functions_count = pcs_get_functions_count,
	.get_function_name = pcs_get_function_name,
	.get_function_groups = pcs_get_function_groups,
	.enable = pcs_enable,
	.disable = pcs_disable,
	.gpio_request_enable = pcs_request_gpio,
};

/* Clear BIAS value */
static void pcs_pinconf_clear_bias(struct pinctrl_dev *pctldev, unsigned pin)
{
	unsigned long config;
	int i;
	for (i = 0; i < ARRAY_SIZE(pcs_bias); i++) {
		config = pinconf_to_config_packed(pcs_bias[i], 0);
		pcs_pinconf_set(pctldev, pin, &config, 1);
	}
}

/*
 * Check whether PIN_CONFIG_BIAS_DISABLE is valid.
 * It's depend on that PULL_DOWN & PULL_UP configs are all invalid.
 */
static bool pcs_pinconf_bias_disable(struct pinctrl_dev *pctldev, unsigned pin)
{
	unsigned long config;
	int i;

	for (i = 0; i < ARRAY_SIZE(pcs_bias); i++) {
		config = pinconf_to_config_packed(pcs_bias[i], 0);
		if (!pcs_pinconf_get(pctldev, pin, &config))
			goto out;
	}
	return true;
out:
	return false;
}

static int pcs_pinconf_get(struct pinctrl_dev *pctldev,
				unsigned pin, unsigned long *config)
{
	struct pcs_device *pcs = pinctrl_dev_get_drvdata(pctldev);
	struct pcs_function *func;
	enum pin_config_param param;
	unsigned offset = 0, data = 0, i, j, ret;

	ret = pcs_get_function(pctldev, pin, &func);
	if (ret)
		return ret;

	for (i = 0; i < func->nconfs; i++) {
		param = pinconf_to_config_param(*config);
		if (param == PIN_CONFIG_BIAS_DISABLE) {
			if (pcs_pinconf_bias_disable(pctldev, pin)) {
				*config = 0;
				return 0;
			} else {
				return -ENOTSUPP;
			}
		} else if (param != func->conf[i].param) {
			continue;
		}

		offset = pin * (pcs->width / BITS_PER_BYTE);
		data = pcs->read(pcs->base + offset) & func->conf[i].mask;
		switch (func->conf[i].param) {
		/* 4 parameters */
		case PIN_CONFIG_BIAS_PULL_DOWN:
		case PIN_CONFIG_BIAS_PULL_UP:
		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			if ((data != func->conf[i].enable) ||
			    (data == func->conf[i].disable))
				return -ENOTSUPP;
			*config = 0;
			break;
		/* 2 parameters */
		case PIN_CONFIG_INPUT_SCHMITT:
			for (j = 0; j < func->nconfs; j++) {
				switch (func->conf[j].param) {
				case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
					if (data != func->conf[j].enable)
						return -ENOTSUPP;
					break;
				default:
					break;
				}
			}
			*config = data;
			break;
		case PIN_CONFIG_DRIVE_STRENGTH:
		case PIN_CONFIG_SLEW_RATE:
		default:
			*config = data;
			break;
		}
		return 0;
	}
	return -ENOTSUPP;
}

static int pcs_pinconf_set(struct pinctrl_dev *pctldev,
				unsigned pin, unsigned long *configs,
				unsigned num_configs)
{
	struct pcs_device *pcs = pinctrl_dev_get_drvdata(pctldev);
	struct pcs_function *func;
	unsigned offset = 0, shift = 0, i, data, ret;
	u16 arg;
	int j;

	ret = pcs_get_function(pctldev, pin, &func);
	if (ret)
		return ret;

	for (j = 0; j < num_configs; j++) {
		for (i = 0; i < func->nconfs; i++) {
			if (pinconf_to_config_param(configs[j])
				!= func->conf[i].param)
				continue;

			offset = pin * (pcs->width / BITS_PER_BYTE);
			data = pcs->read(pcs->base + offset);
			arg = pinconf_to_config_argument(configs[j]);
			switch (func->conf[i].param) {
			/* 2 parameters */
			case PIN_CONFIG_INPUT_SCHMITT:
			case PIN_CONFIG_DRIVE_STRENGTH:
			case PIN_CONFIG_SLEW_RATE:
				shift = ffs(func->conf[i].mask) - 1;
				data &= ~func->conf[i].mask;
				data |= (arg << shift) & func->conf[i].mask;
				break;
			/* 4 parameters */
			case PIN_CONFIG_BIAS_DISABLE:
				pcs_pinconf_clear_bias(pctldev, pin);
				break;
			case PIN_CONFIG_BIAS_PULL_DOWN:
			case PIN_CONFIG_BIAS_PULL_UP:
				if (arg)
					pcs_pinconf_clear_bias(pctldev, pin);
				/* fall through */
			case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
				data &= ~func->conf[i].mask;
				if (arg)
					data |= func->conf[i].enable;
				else
					data |= func->conf[i].disable;
				break;
			default:
				return -ENOTSUPP;
			}
			pcs->write(data, pcs->base + offset);

			break;
		}
		if (i >= func->nconfs)
			return -ENOTSUPP;
	} /* for each config */

	return 0;
}

static int pcs_pinconf_group_get(struct pinctrl_dev *pctldev,
				unsigned group, unsigned long *config)
{
	const unsigned *pins;
	unsigned npins, old = 0;
	int i, ret;

	ret = pcs_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;
	for (i = 0; i < npins; i++) {
		if (pcs_pinconf_get(pctldev, pins[i], config))
			return -ENOTSUPP;
		/* configs do not match between two pins */
		if (i && (old != *config))
			return -ENOTSUPP;
		old = *config;
	}
	return 0;
}

static int pcs_pinconf_group_set(struct pinctrl_dev *pctldev,
				unsigned group, unsigned long *configs,
				unsigned num_configs)
{
	const unsigned *pins;
	unsigned npins;
	int i, ret;

	ret = pcs_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;
	for (i = 0; i < npins; i++) {
		if (pcs_pinconf_set(pctldev, pins[i], configs, num_configs))
			return -ENOTSUPP;
	}
	return 0;
}

static void pcs_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				struct seq_file *s, unsigned pin)
{
}

static void pcs_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
				struct seq_file *s, unsigned selector)
{
}

static void pcs_pinconf_config_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *s,
					unsigned long config)
{
	pinconf_generic_dump_config(pctldev, s, config);
}

static const struct pinconf_ops pcs_pinconf_ops = {
	.pin_config_get = pcs_pinconf_get,
	.pin_config_set = pcs_pinconf_set,
	.pin_config_group_get = pcs_pinconf_group_get,
	.pin_config_group_set = pcs_pinconf_group_set,
	.pin_config_dbg_show = pcs_pinconf_dbg_show,
	.pin_config_group_dbg_show = pcs_pinconf_group_dbg_show,
	.pin_config_config_dbg_show = pcs_pinconf_config_dbg_show,
	.is_generic = true,
};

/**
 * pcs_add_pin() - add a pin to the static per controller pin array
 * @pcs: pcs driver instance
 * @offset: register offset from base
 */
static int pcs_add_pin(struct pcs_device *pcs, unsigned offset,
		unsigned pin_pos)
{
	struct pinctrl_pin_desc *pin;
	struct pcs_name *pn;
	int i;

	i = pcs->pins.cur;
	if (i >= pcs->desc.npins) {
		dev_err(pcs->dev, "too many pins, max %i\n",
			pcs->desc.npins);
		return -ENOMEM;
	}

	pin = &pcs->pins.pa[i];
	pn = &pcs->names[i];
	sprintf(pn->name, "%lx.%d",
		(unsigned long)pcs->res->start + offset, pin_pos);
	pin->name = pn->name;
	pin->number = i;
	pcs->pins.cur++;

	return i;
}

/**
 * pcs_allocate_pin_table() - adds all the pins for the pinctrl driver
 * @pcs: pcs driver instance
 *
 * In case of errors, resources are freed in pcs_free_resources.
 *
 * If your hardware needs holes in the address space, then just set
 * up multiple driver instances.
 */
static int pcs_allocate_pin_table(struct pcs_device *pcs)
{
	int mux_bytes, nr_pins, i;
	int num_pins_in_register = 0;

	mux_bytes = pcs->width / BITS_PER_BYTE;

	if (pcs->bits_per_mux) {
		pcs->bits_per_pin = fls(pcs->fmask);
		nr_pins = (pcs->size * BITS_PER_BYTE) / pcs->bits_per_pin;
		num_pins_in_register = pcs->width / pcs->bits_per_pin;
	} else {
		nr_pins = pcs->size / mux_bytes;
	}

	dev_dbg(pcs->dev, "allocating %i pins\n", nr_pins);
	pcs->pins.pa = devm_kzalloc(pcs->dev,
				sizeof(*pcs->pins.pa) * nr_pins,
				GFP_KERNEL);
	if (!pcs->pins.pa)
		return -ENOMEM;

	pcs->names = devm_kzalloc(pcs->dev,
				sizeof(struct pcs_name) * nr_pins,
				GFP_KERNEL);
	if (!pcs->names)
		return -ENOMEM;

	pcs->desc.pins = pcs->pins.pa;
	pcs->desc.npins = nr_pins;

	for (i = 0; i < pcs->desc.npins; i++) {
		unsigned offset;
		int res;
		int byte_num;
		int pin_pos = 0;

		if (pcs->bits_per_mux) {
			byte_num = (pcs->bits_per_pin * i) / BITS_PER_BYTE;
			offset = (byte_num / mux_bytes) * mux_bytes;
			pin_pos = i % num_pins_in_register;
		} else {
			offset = i * mux_bytes;
		}
		res = pcs_add_pin(pcs, offset, pin_pos);
		if (res < 0) {
			dev_err(pcs->dev, "error adding pins: %i\n", res);
			return res;
		}
	}

	return 0;
}

/**
 * pcs_add_function() - adds a new function to the function list
 * @pcs: pcs driver instance
 * @np: device node of the mux entry
 * @name: name of the function
 * @vals: array of mux register value pairs used by the function
 * @nvals: number of mux register value pairs
 * @pgnames: array of pingroup names for the function
 * @npgnames: number of pingroup names
 */
static struct pcs_function *pcs_add_function(struct pcs_device *pcs,
					struct device_node *np,
					const char *name,
					struct pcs_func_vals *vals,
					unsigned nvals,
					const char **pgnames,
					unsigned npgnames)
{
	struct pcs_function *function;

	function = devm_kzalloc(pcs->dev, sizeof(*function), GFP_KERNEL);
	if (!function)
		return NULL;

	function->name = name;
	function->vals = vals;
	function->nvals = nvals;
	function->pgnames = pgnames;
	function->npgnames = npgnames;

	mutex_lock(&pcs->mutex);
	list_add_tail(&function->node, &pcs->functions);
	radix_tree_insert(&pcs->ftree, pcs->nfuncs, function);
	pcs->nfuncs++;
	mutex_unlock(&pcs->mutex);

	return function;
}

static void pcs_remove_function(struct pcs_device *pcs,
				struct pcs_function *function)
{
	int i;

	mutex_lock(&pcs->mutex);
	for (i = 0; i < pcs->nfuncs; i++) {
		struct pcs_function *found;

		found = radix_tree_lookup(&pcs->ftree, i);
		if (found == function)
			radix_tree_delete(&pcs->ftree, i);
	}
	list_del(&function->node);
	mutex_unlock(&pcs->mutex);
}

/**
 * pcs_add_pingroup() - add a pingroup to the pingroup list
 * @pcs: pcs driver instance
 * @np: device node of the mux entry
 * @name: name of the pingroup
 * @gpins: array of the pins that belong to the group
 * @ngpins: number of pins in the group
 */
static int pcs_add_pingroup(struct pcs_device *pcs,
					struct device_node *np,
					const char *name,
					int *gpins,
					int ngpins)
{
	struct pcs_pingroup *pingroup;

	pingroup = devm_kzalloc(pcs->dev, sizeof(*pingroup), GFP_KERNEL);
	if (!pingroup)
		return -ENOMEM;

	pingroup->name = name;
	pingroup->np = np;
	pingroup->gpins = gpins;
	pingroup->ngpins = ngpins;

	mutex_lock(&pcs->mutex);
	list_add_tail(&pingroup->node, &pcs->pingroups);
	radix_tree_insert(&pcs->pgtree, pcs->ngroups, pingroup);
	pcs->ngroups++;
	mutex_unlock(&pcs->mutex);

	return 0;
}

/**
 * pcs_get_pin_by_offset() - get a pin index based on the register offset
 * @pcs: pcs driver instance
 * @offset: register offset from the base
 *
 * Note that this is OK as long as the pins are in a static array.
 */
static int pcs_get_pin_by_offset(struct pcs_device *pcs, unsigned offset)
{
	unsigned index;

	if (offset >= pcs->size) {
		dev_err(pcs->dev, "mux offset out of range: 0x%x (0x%x)\n",
			offset, pcs->size);
		return -EINVAL;
	}

	if (pcs->bits_per_mux)
		index = (offset * BITS_PER_BYTE) / pcs->bits_per_pin;
	else
		index = offset / (pcs->width / BITS_PER_BYTE);

	return index;
}

/*
 * check whether data matches enable bits or disable bits
 * Return value: 1 for matching enable bits, 0 for matching disable bits,
 *               and negative value for matching failure.
 */
static int pcs_config_match(unsigned data, unsigned enable, unsigned disable)
{
	int ret = -EINVAL;

	if (data == enable)
		ret = 1;
	else if (data == disable)
		ret = 0;
	return ret;
}

static void add_config(struct pcs_conf_vals **conf, enum pin_config_param param,
		       unsigned value, unsigned enable, unsigned disable,
		       unsigned mask)
{
	(*conf)->param = param;
	(*conf)->val = value;
	(*conf)->enable = enable;
	(*conf)->disable = disable;
	(*conf)->mask = mask;
	(*conf)++;
}

static void add_setting(unsigned long **setting, enum pin_config_param param,
			unsigned arg)
{
	**setting = pinconf_to_config_packed(param, arg);
	(*setting)++;
}

/* add pinconf setting with 2 parameters */
static void pcs_add_conf2(struct pcs_device *pcs, struct device_node *np,
			  const char *name, enum pin_config_param param,
			  struct pcs_conf_vals **conf, unsigned long **settings)
{
	unsigned value[2], shift;
	int ret;

	ret = of_property_read_u32_array(np, name, value, 2);
	if (ret)
		return;
	/* set value & mask */
	value[0] &= value[1];
	shift = ffs(value[1]) - 1;
	/* skip enable & disable */
	add_config(conf, param, value[0], 0, 0, value[1]);
	add_setting(settings, param, value[0] >> shift);
}

/* add pinconf setting with 4 parameters */
static void pcs_add_conf4(struct pcs_device *pcs, struct device_node *np,
			  const char *name, enum pin_config_param param,
			  struct pcs_conf_vals **conf, unsigned long **settings)
{
	unsigned value[4];
	int ret;

	/* value to set, enable, disable, mask */
	ret = of_property_read_u32_array(np, name, value, 4);
	if (ret)
		return;
	if (!value[3]) {
		dev_err(pcs->dev, "mask field of the property can't be 0\n");
		return;
	}
	value[0] &= value[3];
	value[1] &= value[3];
	value[2] &= value[3];
	ret = pcs_config_match(value[0], value[1], value[2]);
	if (ret < 0)
		dev_dbg(pcs->dev, "failed to match enable or disable bits\n");
	add_config(conf, param, value[0], value[1], value[2], value[3]);
	add_setting(settings, param, ret);
}

static int pcs_parse_pinconf(struct pcs_device *pcs, struct device_node *np,
			     struct pcs_function *func,
			     struct pinctrl_map **map)

{
	struct pinctrl_map *m = *map;
	int i = 0, nconfs = 0;
	unsigned long *settings = NULL, *s = NULL;
	struct pcs_conf_vals *conf = NULL;
	struct pcs_conf_type prop2[] = {
		{ "pinctrl-single,drive-strength", PIN_CONFIG_DRIVE_STRENGTH, },
		{ "pinctrl-single,slew-rate", PIN_CONFIG_SLEW_RATE, },
		{ "pinctrl-single,input-schmitt", PIN_CONFIG_INPUT_SCHMITT, },
	};
	struct pcs_conf_type prop4[] = {
		{ "pinctrl-single,bias-pullup", PIN_CONFIG_BIAS_PULL_UP, },
		{ "pinctrl-single,bias-pulldown", PIN_CONFIG_BIAS_PULL_DOWN, },
		{ "pinctrl-single,input-schmitt-enable",
			PIN_CONFIG_INPUT_SCHMITT_ENABLE, },
	};

	/* If pinconf isn't supported, don't parse properties in below. */
	if (!pcs->is_pinconf)
		return 0;

	/* cacluate how much properties are supported in current node */
	for (i = 0; i < ARRAY_SIZE(prop2); i++) {
		if (of_find_property(np, prop2[i].name, NULL))
			nconfs++;
	}
	for (i = 0; i < ARRAY_SIZE(prop4); i++) {
		if (of_find_property(np, prop4[i].name, NULL))
			nconfs++;
	}
	if (!nconfs)
		return 0;

	func->conf = devm_kzalloc(pcs->dev,
				  sizeof(struct pcs_conf_vals) * nconfs,
				  GFP_KERNEL);
	if (!func->conf)
		return -ENOMEM;
	func->nconfs = nconfs;
	conf = &(func->conf[0]);
	m++;
	settings = devm_kzalloc(pcs->dev, sizeof(unsigned long) * nconfs,
				GFP_KERNEL);
	if (!settings)
		return -ENOMEM;
	s = &settings[0];

	for (i = 0; i < ARRAY_SIZE(prop2); i++)
		pcs_add_conf2(pcs, np, prop2[i].name, prop2[i].param,
			      &conf, &s);
	for (i = 0; i < ARRAY_SIZE(prop4); i++)
		pcs_add_conf4(pcs, np, prop4[i].name, prop4[i].param,
			      &conf, &s);
	m->type = PIN_MAP_TYPE_CONFIGS_GROUP;
	m->data.configs.group_or_pin = np->name;
	m->data.configs.configs = settings;
	m->data.configs.num_configs = nconfs;
	return 0;
}

static void pcs_free_pingroups(struct pcs_device *pcs);

/**
 * smux_parse_one_pinctrl_entry() - parses a device tree mux entry
 * @pcs: pinctrl driver instance
 * @np: device node of the mux entry
 * @map: map entry
 * @num_maps: number of map
 * @pgnames: pingroup names
 *
 * Note that this binding currently supports only sets of one register + value.
 *
 * Also note that this driver tries to avoid understanding pin and function
 * names because of the extra bloat they would cause especially in the case of
 * a large number of pins. This driver just sets what is specified for the board
 * in the .dts file. Further user space debugging tools can be developed to
 * decipher the pin and function names using debugfs.
 *
 * If you are concerned about the boot time, set up the static pins in
 * the bootloader, and only set up selected pins as device tree entries.
 */
static int pcs_parse_one_pinctrl_entry(struct pcs_device *pcs,
						struct device_node *np,
						struct pinctrl_map **map,
						unsigned *num_maps,
						const char **pgnames)
{
	struct pcs_func_vals *vals;
	const __be32 *mux;
	int size, rows, *pins, index = 0, found = 0, res = -ENOMEM;
	struct pcs_function *function;

	mux = of_get_property(np, PCS_MUX_PINS_NAME, &size);
	if ((!mux) || (size < sizeof(*mux) * 2)) {
		dev_err(pcs->dev, "bad data for mux %s\n",
			np->name);
		return -EINVAL;
	}

	size /= sizeof(*mux);	/* Number of elements in array */
	rows = size / 2;

	vals = devm_kzalloc(pcs->dev, sizeof(*vals) * rows, GFP_KERNEL);
	if (!vals)
		return -ENOMEM;

	pins = devm_kzalloc(pcs->dev, sizeof(*pins) * rows, GFP_KERNEL);
	if (!pins)
		goto free_vals;

	while (index < size) {
		unsigned offset, val;
		int pin;

		offset = be32_to_cpup(mux + index++);
		val = be32_to_cpup(mux + index++);
		vals[found].reg = pcs->base + offset;
		vals[found].val = val;

		pin = pcs_get_pin_by_offset(pcs, offset);
		if (pin < 0) {
			dev_err(pcs->dev,
				"could not add functions for %s %ux\n",
				np->name, offset);
			break;
		}
		pins[found++] = pin;
	}

	pgnames[0] = np->name;
	function = pcs_add_function(pcs, np, np->name, vals, found, pgnames, 1);
	if (!function)
		goto free_pins;

	res = pcs_add_pingroup(pcs, np, np->name, pins, found);
	if (res < 0)
		goto free_function;

	(*map)->type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)->data.mux.group = np->name;
	(*map)->data.mux.function = np->name;

	if (pcs->is_pinconf) {
		res = pcs_parse_pinconf(pcs, np, function, map);
		if (res)
			goto free_pingroups;
		*num_maps = 2;
	} else {
		*num_maps = 1;
	}
	return 0;

free_pingroups:
	pcs_free_pingroups(pcs);
	*num_maps = 1;
free_function:
	pcs_remove_function(pcs, function);

free_pins:
	devm_kfree(pcs->dev, pins);

free_vals:
	devm_kfree(pcs->dev, vals);

	return res;
}

#define PARAMS_FOR_BITS_PER_MUX 3

static int pcs_parse_bits_in_pinctrl_entry(struct pcs_device *pcs,
						struct device_node *np,
						struct pinctrl_map **map,
						unsigned *num_maps,
						const char **pgnames)
{
	struct pcs_func_vals *vals;
	const __be32 *mux;
	int size, rows, *pins, index = 0, found = 0, res = -ENOMEM;
	int npins_in_row;
	struct pcs_function *function;

	mux = of_get_property(np, PCS_MUX_BITS_NAME, &size);

	if (!mux) {
		dev_err(pcs->dev, "no valid property for %s\n", np->name);
		return -EINVAL;
	}

	if (size < (sizeof(*mux) * PARAMS_FOR_BITS_PER_MUX)) {
		dev_err(pcs->dev, "bad data for %s\n", np->name);
		return -EINVAL;
	}

	/* Number of elements in array */
	size /= sizeof(*mux);

	rows = size / PARAMS_FOR_BITS_PER_MUX;
	npins_in_row = pcs->width / pcs->bits_per_pin;

	vals = devm_kzalloc(pcs->dev, sizeof(*vals) * rows * npins_in_row,
			GFP_KERNEL);
	if (!vals)
		return -ENOMEM;

	pins = devm_kzalloc(pcs->dev, sizeof(*pins) * rows * npins_in_row,
			GFP_KERNEL);
	if (!pins)
		goto free_vals;

	while (index < size) {
		unsigned offset, val;
		unsigned mask, bit_pos, val_pos, mask_pos, submask;
		unsigned pin_num_from_lsb;
		int pin;

		offset = be32_to_cpup(mux + index++);
		val = be32_to_cpup(mux + index++);
		mask = be32_to_cpup(mux + index++);

		/* Parse pins in each row from LSB */
		while (mask) {
			bit_pos = ffs(mask);
			pin_num_from_lsb = bit_pos / pcs->bits_per_pin;
			mask_pos = ((pcs->fmask) << (bit_pos - 1));
			val_pos = val & mask_pos;
			submask = mask & mask_pos;
			mask &= ~mask_pos;

			if (submask != mask_pos) {
				dev_warn(pcs->dev,
						"Invalid submask 0x%x for %s at 0x%x\n",
						submask, np->name, offset);
				continue;
			}

			vals[found].mask = submask;
			vals[found].reg = pcs->base + offset;
			vals[found].val = val_pos;

			pin = pcs_get_pin_by_offset(pcs, offset);
			if (pin < 0) {
				dev_err(pcs->dev,
					"could not add functions for %s %ux\n",
					np->name, offset);
				break;
			}
			pins[found++] = pin + pin_num_from_lsb;
		}
	}

	pgnames[0] = np->name;
	function = pcs_add_function(pcs, np, np->name, vals, found, pgnames, 1);
	if (!function)
		goto free_pins;

	res = pcs_add_pingroup(pcs, np, np->name, pins, found);
	if (res < 0)
		goto free_function;

	(*map)->type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)->data.mux.group = np->name;
	(*map)->data.mux.function = np->name;

	if (pcs->is_pinconf) {
		dev_err(pcs->dev, "pinconf not supported\n");
		goto free_pingroups;
	}

	*num_maps = 1;
	return 0;

free_pingroups:
	pcs_free_pingroups(pcs);
	*num_maps = 1;
free_function:
	pcs_remove_function(pcs, function);

free_pins:
	devm_kfree(pcs->dev, pins);

free_vals:
	devm_kfree(pcs->dev, vals);

	return res;
}
/**
 * pcs_dt_node_to_map() - allocates and parses pinctrl maps
 * @pctldev: pinctrl instance
 * @np_config: device tree pinmux entry
 * @map: array of map entries
 * @num_maps: number of maps
 */
static int pcs_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned *num_maps)
{
	struct pcs_device *pcs;
	const char **pgnames;
	int ret;

	pcs = pinctrl_dev_get_drvdata(pctldev);

	/* create 2 maps. One is for pinmux, and the other is for pinconf. */
	*map = devm_kzalloc(pcs->dev, sizeof(**map) * 2, GFP_KERNEL);
	if (!*map)
		return -ENOMEM;

	*num_maps = 0;

	pgnames = devm_kzalloc(pcs->dev, sizeof(*pgnames), GFP_KERNEL);
	if (!pgnames) {
		ret = -ENOMEM;
		goto free_map;
	}

	if (pcs->bits_per_mux) {
		ret = pcs_parse_bits_in_pinctrl_entry(pcs, np_config, map,
				num_maps, pgnames);
		if (ret < 0) {
			dev_err(pcs->dev, "no pins entries for %s\n",
				np_config->name);
			goto free_pgnames;
		}
	} else {
		ret = pcs_parse_one_pinctrl_entry(pcs, np_config, map,
				num_maps, pgnames);
		if (ret < 0) {
			dev_err(pcs->dev, "no pins entries for %s\n",
				np_config->name);
			goto free_pgnames;
		}
	}

	return 0;

free_pgnames:
	devm_kfree(pcs->dev, pgnames);
free_map:
	devm_kfree(pcs->dev, *map);

	return ret;
}

/**
 * pcs_free_funcs() - free memory used by functions
 * @pcs: pcs driver instance
 */
static void pcs_free_funcs(struct pcs_device *pcs)
{
	struct list_head *pos, *tmp;
	int i;

	mutex_lock(&pcs->mutex);
	for (i = 0; i < pcs->nfuncs; i++) {
		struct pcs_function *func;

		func = radix_tree_lookup(&pcs->ftree, i);
		if (!func)
			continue;
		radix_tree_delete(&pcs->ftree, i);
	}
	list_for_each_safe(pos, tmp, &pcs->functions) {
		struct pcs_function *function;

		function = list_entry(pos, struct pcs_function, node);
		list_del(&function->node);
	}
	mutex_unlock(&pcs->mutex);
}

/**
 * pcs_free_pingroups() - free memory used by pingroups
 * @pcs: pcs driver instance
 */
static void pcs_free_pingroups(struct pcs_device *pcs)
{
	struct list_head *pos, *tmp;
	int i;

	mutex_lock(&pcs->mutex);
	for (i = 0; i < pcs->ngroups; i++) {
		struct pcs_pingroup *pingroup;

		pingroup = radix_tree_lookup(&pcs->pgtree, i);
		if (!pingroup)
			continue;
		radix_tree_delete(&pcs->pgtree, i);
	}
	list_for_each_safe(pos, tmp, &pcs->pingroups) {
		struct pcs_pingroup *pingroup;

		pingroup = list_entry(pos, struct pcs_pingroup, node);
		list_del(&pingroup->node);
	}
	mutex_unlock(&pcs->mutex);
}

/**
 * pcs_free_resources() - free memory used by this driver
 * @pcs: pcs driver instance
 */
static void pcs_free_resources(struct pcs_device *pcs)
{
	if (pcs->pctl)
		pinctrl_unregister(pcs->pctl);

	pcs_free_funcs(pcs);
	pcs_free_pingroups(pcs);
}

#define PCS_GET_PROP_U32(name, reg, err)				\
	do {								\
		ret = of_property_read_u32(np, name, reg);		\
		if (ret) {						\
			dev_err(pcs->dev, err);				\
			return ret;					\
		}							\
	} while (0);

static struct of_device_id pcs_of_match[];

static int pcs_add_gpio_func(struct device_node *node, struct pcs_device *pcs)
{
	const char *propname = "pinctrl-single,gpio-range";
	const char *cellname = "#pinctrl-single,gpio-range-cells";
	struct of_phandle_args gpiospec;
	struct pcs_gpiofunc_range *range;
	int ret, i;

	for (i = 0; ; i++) {
		ret = of_parse_phandle_with_args(node, propname, cellname,
						 i, &gpiospec);
		/* Do not treat it as error. Only treat it as end condition. */
		if (ret) {
			ret = 0;
			break;
		}
		range = devm_kzalloc(pcs->dev, sizeof(*range), GFP_KERNEL);
		if (!range) {
			ret = -ENOMEM;
			break;
		}
		range->offset = gpiospec.args[0];
		range->npins = gpiospec.args[1];
		range->gpiofunc = gpiospec.args[2];
		mutex_lock(&pcs->mutex);
		list_add_tail(&range->node, &pcs->gpiofuncs);
		mutex_unlock(&pcs->mutex);
	}
	return ret;
}

#ifdef CONFIG_PM
static int pinctrl_single_suspend(struct platform_device *pdev,
					pm_message_t state)
{
	struct pcs_device *pcs;

	pcs = platform_get_drvdata(pdev);
	if (!pcs)
		return -EINVAL;

	return pinctrl_force_sleep(pcs->pctl);
}

static int pinctrl_single_resume(struct platform_device *pdev)
{
	struct pcs_device *pcs;

	pcs = platform_get_drvdata(pdev);
	if (!pcs)
		return -EINVAL;

	return pinctrl_force_default(pcs->pctl);
}
#endif

static int pcs_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *match;
	struct resource *res;
	struct pcs_device *pcs;
	int ret;

	match = of_match_device(pcs_of_match, &pdev->dev);
	if (!match)
		return -EINVAL;

	pcs = devm_kzalloc(&pdev->dev, sizeof(*pcs), GFP_KERNEL);
	if (!pcs) {
		dev_err(&pdev->dev, "could not allocate\n");
		return -ENOMEM;
	}
	pcs->dev = &pdev->dev;
	mutex_init(&pcs->mutex);
	INIT_LIST_HEAD(&pcs->pingroups);
	INIT_LIST_HEAD(&pcs->functions);
	INIT_LIST_HEAD(&pcs->gpiofuncs);
	pcs->is_pinconf = match->data;

	PCS_GET_PROP_U32("pinctrl-single,register-width", &pcs->width,
			 "register width not specified\n");

	ret = of_property_read_u32(np, "pinctrl-single,function-mask",
				   &pcs->fmask);
	if (!ret) {
		pcs->fshift = ffs(pcs->fmask) - 1;
		pcs->fmax = pcs->fmask >> pcs->fshift;
	} else {
		/* If mask property doesn't exist, function mux is invalid. */
		pcs->fmask = 0;
		pcs->fshift = 0;
		pcs->fmax = 0;
	}

	ret = of_property_read_u32(np, "pinctrl-single,function-off",
					&pcs->foff);
	if (ret)
		pcs->foff = PCS_OFF_DISABLED;

	pcs->bits_per_mux = of_property_read_bool(np,
						  "pinctrl-single,bit-per-mux");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(pcs->dev, "could not get resource\n");
		return -ENODEV;
	}

	pcs->res = devm_request_mem_region(pcs->dev, res->start,
			resource_size(res), DRIVER_NAME);
	if (!pcs->res) {
		dev_err(pcs->dev, "could not get mem_region\n");
		return -EBUSY;
	}

	pcs->size = resource_size(pcs->res);
	pcs->base = devm_ioremap(pcs->dev, pcs->res->start, pcs->size);
	if (!pcs->base) {
		dev_err(pcs->dev, "could not ioremap\n");
		return -ENODEV;
	}

	INIT_RADIX_TREE(&pcs->pgtree, GFP_KERNEL);
	INIT_RADIX_TREE(&pcs->ftree, GFP_KERNEL);
	platform_set_drvdata(pdev, pcs);

	switch (pcs->width) {
	case 8:
		pcs->read = pcs_readb;
		pcs->write = pcs_writeb;
		break;
	case 16:
		pcs->read = pcs_readw;
		pcs->write = pcs_writew;
		break;
	case 32:
		pcs->read = pcs_readl;
		pcs->write = pcs_writel;
		break;
	default:
		break;
	}

	pcs->desc.name = DRIVER_NAME;
	pcs->desc.pctlops = &pcs_pinctrl_ops;
	pcs->desc.pmxops = &pcs_pinmux_ops;
	if (pcs->is_pinconf)
		pcs->desc.confops = &pcs_pinconf_ops;
	pcs->desc.owner = THIS_MODULE;

	ret = pcs_allocate_pin_table(pcs);
	if (ret < 0)
		goto free;

	pcs->pctl = pinctrl_register(&pcs->desc, pcs->dev, pcs);
	if (!pcs->pctl) {
		dev_err(pcs->dev, "could not register single pinctrl driver\n");
		ret = -EINVAL;
		goto free;
	}

	ret = pcs_add_gpio_func(np, pcs);
	if (ret < 0)
		goto free;

	dev_info(pcs->dev, "%i pins at pa %p size %u\n",
		 pcs->desc.npins, pcs->base, pcs->size);

	return 0;

free:
	pcs_free_resources(pcs);

	return ret;
}

static int pcs_remove(struct platform_device *pdev)
{
	struct pcs_device *pcs = platform_get_drvdata(pdev);

	if (!pcs)
		return 0;

	pcs_free_resources(pcs);

	return 0;
}

static struct of_device_id pcs_of_match[] = {
	{ .compatible = "pinctrl-single", .data = (void *)false },
	{ .compatible = "pinconf-single", .data = (void *)true },
	{ },
};
MODULE_DEVICE_TABLE(of, pcs_of_match);

static struct platform_driver pcs_driver = {
	.probe		= pcs_probe,
	.remove		= pcs_remove,
	.driver = {
		.owner		= THIS_MODULE,
		.name		= DRIVER_NAME,
		.of_match_table	= pcs_of_match,
	},
#ifdef CONFIG_PM
	.suspend = pinctrl_single_suspend,
	.resume = pinctrl_single_resume,
#endif
};

module_platform_driver(pcs_driver);

MODULE_AUTHOR("Tony Lindgren <tony@atomide.com>");
MODULE_DESCRIPTION("One-register-per-pin type device tree based pinctrl driver");
MODULE_LICENSE("GPL v2");
