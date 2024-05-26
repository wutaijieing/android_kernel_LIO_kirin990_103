/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: Generic device tree based pinctrl driver for one register per pin
 * type pinmux controllers.
 * Create: 2019-6-12
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#include <linux/irqchip/chained_irq.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>

#include "core.h"
#include "devicetree.h"
#include "pinconf.h"
#include "pinmux.h"

#define DRIVER_NAME			"pinctrl-v500"
#define PCH_OFF_DISABLED		~0U

/**
 * struct pch_func_vals - mux function register offset and value pair
 * @reg:	register virtual address
 * @val:	register value
 */
struct pch_func_vals {
	void __iomem *reg;
	void __iomem *mux_reg;
	unsigned mux_val;
	unsigned val;
	unsigned mask;
};

/**
 * struct pch_conf_vals - pinconf parameter, pinconf register offset
 * and value, enable, disable, mask
 * @param:	config parameter
 * @val:	user input bits in the pinconf register
 * @enable:	enable bits in the pinconf register
 * @disable:	disable bits in the pinconf register
 * @mask:	mask bits in the register value
 */
struct pch_conf_vals {
	enum pin_config_param param;
	unsigned val;
	unsigned enable;
	unsigned disable;
	unsigned mask;
};

/**
 * struct pch_conf_type - pinconf property name, pinconf param pair
 * @name:	property name in DTS file
 * @param:	config parameter
 */
struct pch_conf_type {
	const char *name;
	enum pin_config_param param;
};

/**
 * struct pch_function - pinctrl function
 * @name:	pinctrl function name
 * @vals:	register and vals array
 * @nvals:	number of entries in vals array
 * @pgnames:	array of pingroup names the function uses
 * @npgnames:	number of pingroup names the function uses
 * @node:	list node
 */
struct pch_function {
	const char *name;
	struct pch_func_vals *vals;
	unsigned nvals;
	const char **pgnames;
	int npgnames;
	struct pch_conf_vals *conf;
	unsigned int nconfs;
	struct list_head node;
};

/**
 * struct pch_gpiofunc_range - pin ranges with same mux value of gpio function
 * @offset:	offset base of pins
 * @npins:	number pins with the same mux value of gpio function
 * @gpiofunc:	mux value of gpio function
 * @node:	list node
 */
struct pch_gpiofunc_range {
	unsigned offset;
	unsigned npins;
	unsigned gpiofunc;
	struct list_head node;
};

/**
 * struct pch_data - wrapper for data needed by pinctrl framework
 * @pa:		pindesc array
 * @cur:	index to current element
 *
 * REVISIT: We should be able to drop this eventually by adding
 * support for registering pins individually in the pinctrl
 * framework for those drivers that don't need a static array.
 */
struct pch_data {
	struct pinctrl_pin_desc *pa;
	unsigned int cur;
};

/**
 * struct pch_soc_data - SoC specific settings
 * @flags:	initial SoC specific PCH_FEAT_xxx values
 */
struct pch_soc_data {
	unsigned flags;
};

/**
 * struct pch_device - pinctrl device instance
 * @res:	resources
 * @base:	virtual address of the controller
 * @size:	size of the ioremapped area
 * @dev:	device entry
 * @np:		device tree node
 * @pctl:	pin controller device
 * @flags:	mask of PCH_FEAT_xxx values
 * @missing_nr_pinctrl_cells: for legacy binding, may go away
 * @socdata:	soc specific data
 * @lock:	spinlock for register access
 * @mutex:	mutex protecting the lists
 * @width:	bits per mux register
 * @fmask:	function register mask
 * @fshift:	function register shift
 * @fmax:	max number of functions in fmask
 * @pins:	physical pins on the SoC
 * @gpiofuncs:	list of gpio functions
 * @desc:	pin controller descriptor
 * @read:	register read function to use
 * @write:	register write function to use
 */
struct pch_device {
	struct resource *res;
	void __iomem *base;
	unsigned size;
	struct device *dev;
	struct device_node *np;
	struct pinctrl_dev *pctl;
	unsigned flags;
#define PCH_FEAT_PINCONF	(1 << 0)
	struct property *missing_nr_pinctrl_cells;
	struct pch_soc_data socdata;
	raw_spinlock_t lock;
	struct mutex mutex;
	unsigned width;
	unsigned fmask;
	unsigned fshift;
	unsigned fmax;
	struct pch_data pins;
	struct list_head gpiofuncs;
	struct pinctrl_desc desc;
	unsigned (*read)(void __iomem *reg);
	void (*write)(unsigned val, void __iomem *reg);
};

#define IOC_ADDR_SIZE (0x4)
static int pch_pinconf_get(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *config);
static int pch_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *configs, unsigned num_configs);

static enum pin_config_param pch_bias[] = {
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
static unsigned pch_readl(void __iomem *reg)
{
	return readl(reg);
}

static void pch_writel(unsigned val, void __iomem *reg)
{
	writel(val, reg);
}

static void pch_pin_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *s,
					unsigned pin)
{
	struct pch_device *pch;
	unsigned val, mux_bytes;
	unsigned long offset;
	size_t pa;
#if defined CONFIG_HISI_PINCRTL_INFO
	struct pch_gpiofunc_range *frange = NULL;
	struct list_head *pos = NULL;
	struct list_head *tmp = NULL;
#endif
	pch = pinctrl_dev_get_drvdata(pctldev);
#if defined CONFIG_HISI_PINCRTL_INFO
	list_for_each_safe(pos, tmp, &pch->gpiofuncs) {
		frange = list_entry(pos, struct pch_gpiofunc_range, node);
		if (pin >= frange->offset + frange->npins
			|| pin < frange->offset)
			continue;
		mux_bytes = pch->width / BITS_PER_BYTE;
		val = (unsigned)pch->read(pch->base + pin * mux_bytes);
		seq_printf(s, "%08x %s " , val, DRIVER_NAME);
		break;
	}
#else

	mux_bytes = pch->width / BITS_PER_BYTE;
	offset = pin * mux_bytes;
	val = pch->read(pch->base + offset);
	pa = pch->res->start + offset;

	seq_printf(s, "%zx %08x %s ", pa, val, DRIVER_NAME);
#endif
}

static void pch_dt_free_map(struct pinctrl_dev *pctldev,
				struct pinctrl_map *map, unsigned num_maps)
{
	struct pch_device *pch;

	pch = pinctrl_dev_get_drvdata(pctldev);
	devm_kfree(pch->dev, map);
}

static int pch_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned *num_maps);

static const struct pinctrl_ops pch_pinctrl_ops = {
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
	.pin_dbg_show = pch_pin_dbg_show,
	.dt_node_to_map = pch_dt_node_to_map,
	.dt_free_map = pch_dt_free_map,
};

static int pch_get_function(struct pinctrl_dev *pctldev, unsigned pin,
			    struct pch_function **func)
{
	struct pch_device *pch = pinctrl_dev_get_drvdata(pctldev);
	struct pin_desc *pdesc = NULL;
	const struct pinctrl_setting_mux *setting;
	struct function_desc *function;
	unsigned fselector;

	pdesc = pin_desc_get(pctldev, pin);
	if (!pdesc)
		return -ENOTSUPP;
	/* If pin is not described in DTS & enabled, mux_setting is NULL. */
	setting = pdesc->mux_setting;
	if (!setting)
		return -ENOTSUPP;
	fselector = setting->func;
	function = pinmux_generic_get_function(pctldev, fselector);
	if (!function)
		return -ENOTSUPP;
	*func = function->data;
	if (!(*func)) {
		dev_err(pch->dev, "%s could not find function%i\n",
			__func__, fselector);
		return -ENOTSUPP;
	}
	return 0;
}

static int pch_set_mux(struct pinctrl_dev *pctldev, unsigned fselector,
	unsigned group)
{
	struct pch_device *pch;
	struct function_desc *function;
	struct pch_function *func;
	unsigned int i;

	pch = pinctrl_dev_get_drvdata(pctldev);
	/* If function mask is null, needn't enable it. */
	if (!pch->fmask)
		return 0;
	function = pinmux_generic_get_function(pctldev, fselector);
	if (!function)
		return -EINVAL;
	func = function->data;
	if (!func)
		return -EINVAL;

	dev_dbg(pch->dev, "enabling %s function%i\n",
		func->name, fselector);

	for (i = 0; i < func->nvals; i++) {
		struct pch_func_vals *vals;
		unsigned long flags;
		unsigned val, mux_val, mask;

		vals = &func->vals[i];
		mask = pch->fmask;
		raw_spin_lock_irqsave(&pch->lock, flags);
		if(vals->mux_reg) {
			mux_val = pch->read(vals->mux_reg);
			mux_val &= ~mask;
			mux_val |= (vals->mux_val & mask);
			pch->write(mux_val, vals->mux_reg);
		}
		val = pch->read(vals->reg);
		val &= ~mask;
		val |= (vals->val & mask);
		pch->write(val, vals->reg);
		raw_spin_unlock_irqrestore(&pch->lock, flags);
	}

	return 0;
}

static int pch_request_gpio(struct pinctrl_dev *pctldev,
			    struct pinctrl_gpio_range *range, unsigned pin)
{
	struct pch_device *pch = pinctrl_dev_get_drvdata(pctldev);
	struct pch_gpiofunc_range *frange = NULL;
	struct list_head *pos, *tmp;
	unsigned data, mux_bytes;

	/* If function mask is null, return directly. */
	if (!pch->fmask)
		return -ENOTSUPP;

	list_for_each_safe(pos, tmp, &pch->gpiofuncs) {
		frange = list_entry(pos, struct pch_gpiofunc_range, node);
		if (pin >= frange->offset + frange->npins
			|| pin < frange->offset)
			continue;
		mux_bytes = pch->width / BITS_PER_BYTE;
		data = pch->read(pch->base + pin * mux_bytes) & ~pch->fmask;
		data |= frange->gpiofunc;
		pch->write(data, pch->base + pin * mux_bytes);
		break;
	}
	return 0;
}

static const struct pinmux_ops pch_pinmux_ops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.set_mux = pch_set_mux,
	.gpio_request_enable = pch_request_gpio,
};

/* Clear BIAS value */
static void pch_pinconf_clear_bias(struct pinctrl_dev *pctldev, unsigned pin)
{
	unsigned long config;
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(pch_bias); i++) {
		config = pinconf_to_config_packed(pch_bias[i], 0);
		pch_pinconf_set(pctldev, pin, &config, 1);
	}
}

/*
 * Check whether PIN_CONFIG_BIAS_DISABLE is valid.
 * It's depend on that PULL_DOWN & PULL_UP configs are all invalid.
 */
static bool pch_pinconf_bias_disable(struct pinctrl_dev *pctldev, unsigned pin)
{
	unsigned long config;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pch_bias); i++) {
		config = pinconf_to_config_packed(pch_bias[i], 0);
		if (!pch_pinconf_get(pctldev, pin, &config))
			goto out;
	}
	return true;
out:
	return false;
}

static int pch_pinconf_get(struct pinctrl_dev *pctldev,
				unsigned pin, unsigned long *config)
{
	struct pch_device *pch = pinctrl_dev_get_drvdata(pctldev);
	struct pch_function *func;
	enum pin_config_param param;
	unsigned offset = 0, data = 0, i, j;
	int ret;

	ret = pch_get_function(pctldev, pin, &func);
	if (ret)
		return ret;

	for (i = 0; i < func->nconfs; i++) {
		param = pinconf_to_config_param(*config);
		if (param == PIN_CONFIG_BIAS_DISABLE) {
			if (pch_pinconf_bias_disable(pctldev, pin)) {
				*config = 0;
				return 0;
			} else {
				return -ENOTSUPP;
			}
		} else if (param != func->conf[i].param) {
			continue;
		}

		offset = pin * (pch->width / BITS_PER_BYTE);
		data = pch->read(pch->base + offset) & func->conf[i].mask;
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
		case PIN_CONFIG_LOW_POWER_MODE:
		default:
			*config = data;
			break;
		}
		return 0;
	}
	return -ENOTSUPP;
}

static int pch_pinconf_set(struct pinctrl_dev *pctldev,
				unsigned pin, unsigned long *configs,
				unsigned num_configs)
{
	struct pch_device *pch = pinctrl_dev_get_drvdata(pctldev);
	struct pch_function *func;
	unsigned offset = 0, shift = 0, i, j, data;
	u32 arg;
	int ret;

	ret = pch_get_function(pctldev, pin, &func);
	if (ret)
		return ret;

	for (j = 0; j < num_configs; j++) {
		for (i = 0; i < func->nconfs; i++) {
			if (pinconf_to_config_param(configs[j])
				!= func->conf[i].param)
				continue;

			offset = pin * (pch->width / BITS_PER_BYTE);
			data = pch->read(pch->base + offset);
			arg = pinconf_to_config_argument(configs[j]);
			switch (func->conf[i].param) {
			/* 2 parameters */
			case PIN_CONFIG_INPUT_SCHMITT:
			case PIN_CONFIG_DRIVE_STRENGTH:
			case PIN_CONFIG_SLEW_RATE:
			case PIN_CONFIG_LOW_POWER_MODE:
				shift = (unsigned int)ffs(func->conf[i].mask) - 1;
				data &= ~func->conf[i].mask;
				data |= (arg << shift) & func->conf[i].mask;
				break;
			/* 4 parameters */
			case PIN_CONFIG_BIAS_DISABLE:
				pch_pinconf_clear_bias(pctldev, pin);
				break;
			case PIN_CONFIG_BIAS_PULL_DOWN:
			case PIN_CONFIG_BIAS_PULL_UP:
				if (arg)
					pch_pinconf_clear_bias(pctldev, pin);
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
			pch->write(data, pch->base + offset);

			break;
		}
		if (i >= func->nconfs)
			return -ENOTSUPP;
	} /* for each config */

	return 0;
}

static int pch_pinconf_group_get(struct pinctrl_dev *pctldev,
				unsigned group, unsigned long *config)
{
	const unsigned *pins;
	unsigned i, npins = 0;
	unsigned long old = 0;
	int ret;

	ret = pinctrl_generic_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;
	for (i = 0; i < npins; i++) {
		if (pch_pinconf_get(pctldev, pins[i], config))
			return -ENOTSUPP;
		/* configs do not match between two pins */
		if (i && (old != *config))
			return -ENOTSUPP;
		old = *config;
	}
	return 0;
}

static int pch_pinconf_group_set(struct pinctrl_dev *pctldev,
				unsigned group, unsigned long *configs,
				unsigned num_configs)
{
	const unsigned *pins;
	unsigned npins, i;
	int ret;

	ret = pinctrl_generic_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;
	for (i = 0; i < npins; i++) {
		if (pch_pinconf_set(pctldev, pins[i], configs, num_configs))
			return -ENOTSUPP;
	}
	return 0;
}

static void pch_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				struct seq_file *s, unsigned pin)
{
}

static void pch_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
				struct seq_file *s, unsigned selector)
{
}

static void pch_pinconf_config_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *s,
					unsigned long config)
{
	pinconf_generic_dump_config(pctldev, s, config);
}

static const struct pinconf_ops pch_pinconf_ops = {
	.pin_config_get = pch_pinconf_get,
	.pin_config_set = pch_pinconf_set,
	.pin_config_group_get = pch_pinconf_group_get,
	.pin_config_group_set = pch_pinconf_group_set,
	.pin_config_dbg_show = pch_pinconf_dbg_show,
	.pin_config_group_dbg_show = pch_pinconf_group_dbg_show,
	.pin_config_config_dbg_show = pch_pinconf_config_dbg_show,
	.is_generic = true,
};

/**
 * pch_add_pin() - add a pin to the static per controller pin array
 * @pch: pch driver instance
 * @offset: register offset from base
 */
static int pch_add_pin(struct pch_device *pch)
{
	struct pinctrl_pin_desc *pin;
	unsigned int i;

	i = pch->pins.cur;
	if (i >= pch->desc.npins) {
		dev_err(pch->dev, "too many pins, max %i\n",
			pch->desc.npins);
		return -ENOMEM;
	}

	pin = &pch->pins.pa[i];
	pin->number = i;
	pch->pins.cur++;

	return i;
}

/**
 * pch_allocate_pin_table() - adds all the pins for the pinctrl driver
 * @pch: pch driver instance
 *
 * In case of errors, resources are freed in pch_free_resources.
 *
 * If your hardware needs holes in the address space, then just set
 * up multiple driver instances.
 */
static int pch_allocate_pin_table(struct pch_device *pch)
{
	unsigned int mux_bytes, nr_pins, i;
	int res;

	mux_bytes = pch->width / BITS_PER_BYTE;
	nr_pins = pch->size / mux_bytes;
	dev_dbg(pch->dev, "allocating %i pins\n", nr_pins);
	pch->pins.pa = devm_kzalloc(pch->dev,
				sizeof(*pch->pins.pa) * nr_pins,
				GFP_KERNEL);
	if (!pch->pins.pa)
		return -ENOMEM;

	pch->desc.pins = pch->pins.pa;
	pch->desc.npins = nr_pins;
	for (i = 0; i < pch->desc.npins; i++) {
		res = pch_add_pin(pch);
		if (res < 0) {
			dev_err(pch->dev, "error adding pins: %i\n", res);
			return res;
		}
	}

	return 0;
}

/**
 * pch_add_function() - adds a new function to the function list
 * @pch: pch driver instance
 * @fcn: new function allocated
 * @name: name of the function
 * @vals: array of mux register value pairs used by the function
 * @nvals: number of mux register value pairs
 * @pgnames: array of pingroup names for the function
 * @npgnames: number of pingroup names
 *
 * Caller must take care of locking.
 */
static int pch_add_function(struct pch_device *pch,
			    struct pch_function **fcn,
			    const char *name,
			    struct pch_func_vals *vals,
			    unsigned int nvals,
			    const char **pgnames,
			    unsigned int npgnames)
{
	struct pch_function *function;
	int selector;

	function = devm_kzalloc(pch->dev, sizeof(*function), GFP_KERNEL);
	if (!function)
		return -ENOMEM;

	function->vals = vals;
	function->nvals = nvals;

	selector = pinmux_generic_add_function(pch->pctl, name,
					       pgnames, npgnames,
					       function);
	if (selector < 0) {
		devm_kfree(pch->dev, function);
		*fcn = NULL;
	} else {
		*fcn = function;
	}

	return selector;
}

/**
 * pch_get_pin_by_offset() - get a pin index based on the register offset
 * @pch: pch driver instance
 * @offset: register offset from the base
 *
 * Note that this is OK as long as the pins are in a static array.
 */
static int pch_get_pin_by_offset(struct pch_device *pch, unsigned offset)
{
	unsigned index;

	if (offset >= pch->size) {
		dev_err(pch->dev, "mux offset out of range: 0x%x (0x%x)\n",
			offset, pch->size);
		return -EINVAL;
	}

	index = offset / (pch->width / BITS_PER_BYTE);
	return index;
}

/*
 * check whether data matches enable bits or disable bits
 * Return value: 1 for matching enable bits, 0 for matching disable bits,
 *               and negative value for matching failure.
 */
static int pch_config_match(unsigned data, unsigned enable, unsigned disable)
{
	int ret = -EINVAL;

	if (data == enable)
		ret = 1;
	else if (data == disable)
		ret = 0;
	return ret;
}

static void add_config(struct pch_conf_vals **conf, enum pin_config_param param,
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
static void pch_add_conf2(struct pch_device *pch, struct device_node *np,
			  const char *name, enum pin_config_param param,
			  struct pch_conf_vals **conf, unsigned long **settings)
{
	unsigned value[2], shift;
	int ret;

	ret = of_property_read_u32_array(np, name, value, 2);
	if (ret)
		return;
	/* set value & mask */
	value[0] &= value[1];
	shift = (unsigned int)ffs(value[1]) - 1;
	/* skip enable & disable */
	add_config(conf, param, value[0], 0, 0, value[1]);
	add_setting(settings, param, value[0] >> shift);
}

/* add pinconf setting with 4 parameters */
static void pch_add_conf4(struct pch_device *pch, struct device_node *np,
			  const char *name, enum pin_config_param param,
			  struct pch_conf_vals **conf, unsigned long **settings)
{
	unsigned value[4];
	int ret;

	/* value to set, enable, disable, mask */
	ret = of_property_read_u32_array(np, name, value, 4);
	if (ret)
		return;
	if (!value[3]) {
		dev_err(pch->dev, "mask field of the property can't be 0\n");
		return;
	}
	value[0] &= value[3];
	value[1] &= value[3];
	value[2] &= value[3];
	ret = pch_config_match(value[0], value[1], value[2]);
	if (ret < 0)
		dev_dbg(pch->dev, "failed to match enable or disable bits\n");
	add_config(conf, param, value[0], value[1], value[2], value[3]);
	add_setting(settings, param, (unsigned int)ret);
}

static int pch_parse_pinconf(struct pch_device *pch, struct device_node *np,
			     struct pch_function *func,
			     struct pinctrl_map **map)

{
	struct pinctrl_map *m = *map;
	unsigned int i = 0, nconfs = 0;
	unsigned long *settings = NULL, *s = NULL;
	struct pch_conf_vals *conf = NULL;
	struct pch_conf_type prop2[] = {
		{ "pinctrl-v500,drive-strength", PIN_CONFIG_DRIVE_STRENGTH, },
		{ "pinctrl-v500,slew-rate", PIN_CONFIG_SLEW_RATE, },
		{ "pinctrl-v500,input-schmitt", PIN_CONFIG_INPUT_SCHMITT, },
		{ "pinctrl-v500,low-power-mode", PIN_CONFIG_LOW_POWER_MODE, },
	};
	struct pch_conf_type prop4[] = {
		{ "pinctrl-v500,bias-pullup", PIN_CONFIG_BIAS_PULL_UP, },
		{ "pinctrl-v500,bias-pulldown", PIN_CONFIG_BIAS_PULL_DOWN, },
		{ "pinctrl-v500,input-schmitt-enable",
			PIN_CONFIG_INPUT_SCHMITT_ENABLE, },
	};

	/* If pinconf isn't supported, don't parse properties in below. */
	if (!(pch->flags & PCH_FEAT_PINCONF))
		return -ENOTSUPP;

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
		return -ENOTSUPP;

	func->conf = devm_kzalloc(pch->dev,
				  sizeof(struct pch_conf_vals) * nconfs,
				  GFP_KERNEL);
	if (!func->conf)
		return -ENOMEM;
	func->nconfs = nconfs;
	conf = &(func->conf[0]);
	m++;
	settings = devm_kzalloc(pch->dev, sizeof(unsigned long) * nconfs,
				GFP_KERNEL);
	if (!settings)
		return -ENOMEM;
	s = &settings[0];

	for (i = 0; i < ARRAY_SIZE(prop2); i++)
		pch_add_conf2(pch, np, prop2[i].name, prop2[i].param,
			      &conf, &s);
	for (i = 0; i < ARRAY_SIZE(prop4); i++)
		pch_add_conf4(pch, np, prop4[i].name, prop4[i].param,
			      &conf, &s);
	m->type = PIN_MAP_TYPE_CONFIGS_GROUP;
	m->data.configs.group_or_pin = np->name;
	m->data.configs.configs = settings;
	m->data.configs.num_configs = nconfs;
	return 0;
}

/**
 * smux_parse_one_pinctrl_entry() - parses a device tree mux entry
 * @pctldev: pin controller device
 * @pch: pinctrl driver instance
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
static int pch_parse_one_pinctrl_entry(struct pch_device *pch,
						struct device_node *np,
						struct pinctrl_map **map,
						unsigned *num_maps,
						const char **pgnames)
{
	const char *name = "pinctrl-v500,pad";
	struct pch_func_vals *vals;
	int rows, *pins, found = 0, res = -ENOMEM, i, fsel, gsel;
	struct pch_function *function = NULL;

	rows = pinctrl_count_index_with_args(np, name);
	if (rows <= 0) {
		dev_err(pch->dev, "Invalid number of rows: %d\n", rows);
		return -EINVAL;
	}

	vals = devm_kzalloc(pch->dev, sizeof(*vals) * rows, GFP_KERNEL);
	if (!vals)
		return -ENOMEM;

	pins = devm_kzalloc(pch->dev, sizeof(*pins) * rows, GFP_KERNEL);
	if (!pins)
		goto free_vals;

	for (i = 0; i < rows; i++) {
		struct of_phandle_args pad_spec, mux_spec;
		unsigned int offset;
		int pin;

		res = pinctrl_parse_index_with_args(np, name, i, &pad_spec);
		if (res)
			return res;

		if (pad_spec.args_count < 2) {
			dev_err(pch->dev, "invalid args_count for spec: %i\n",
				pad_spec.args_count);
			break;
		}

		/* Index plus one value cell */
		offset = pad_spec.args[0];
		vals[found].reg = pch->base + offset;
		vals[found].val = pad_spec.args[1];
		vals[found].mux_reg = NULL;
		res = pinctrl_parse_index_with_args(np, "pinctrl-v500,mux", i, &mux_spec);
		if(!res) {
			vals[found].mux_reg = ioremap(mux_spec.args[0], IOC_ADDR_SIZE);
			vals[found].mux_val = mux_spec.args[1];
		}
		dev_dbg(pch->dev, "%s index: 0x%x value: 0x%x\n",
			pad_spec.np->name, offset, pad_spec.args[1]);

		pin = pch_get_pin_by_offset(pch, offset);
		if (pin < 0) {
			dev_err(pch->dev,
				"could not add functions for %s %ux\n",
				np->name, offset);
			break;
		}
		pins[found++] = pin;
	}

	pgnames[0] = np->name;
	mutex_lock(&pch->mutex);
	fsel = pch_add_function(pch, &function, np->name, vals, found,
				pgnames, 1);
	if (fsel < 0) {
		res = fsel;
		goto free_pins;
	}

	gsel = pinctrl_generic_add_group(pch->pctl, np->name, pins, found, pch);
	if (gsel < 0) {
		res = gsel;
		goto free_function;
	}

	(*map)->type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)->data.mux.group = np->name;
	(*map)->data.mux.function = np->name;

	if ((pch->flags & PCH_FEAT_PINCONF) && function) {
		res = pch_parse_pinconf(pch, np, function, map);
		if (res == 0)
			*num_maps = 2;
		else if (res == -ENOTSUPP)
			*num_maps = 1;
		else
			goto free_pingroups;
	} else {
		*num_maps = 1;
	}
	mutex_unlock(&pch->mutex);

	return 0;

free_pingroups:
	pinctrl_generic_remove_group(pch->pctl, gsel);
	*num_maps = 1;
free_function:
	pinmux_generic_remove_function(pch->pctl, fsel);
free_pins:
	mutex_unlock(&pch->mutex);
	devm_kfree(pch->dev, pins);

free_vals:
	devm_kfree(pch->dev, vals);

	return res;
}

/**
 * pch_dt_node_to_map() - allocates and parses pinctrl maps
 * @pctldev: pinctrl instance
 * @np_config: device tree pinmux entry
 * @map: array of map entries
 * @num_maps: number of maps
 */
static int pch_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned *num_maps)
{
	struct pch_device *pch;
	const char **pgnames;
	int ret;

	pch = pinctrl_dev_get_drvdata(pctldev);

	/* create 2 maps. One is for pinmux, and the other is for pinconf. */
	*map = devm_kzalloc(pch->dev, sizeof(**map) * 2, GFP_KERNEL);
	if (!*map)
		return -ENOMEM;

	*num_maps = 0;

	pgnames = devm_kzalloc(pch->dev, sizeof(*pgnames), GFP_KERNEL);
	if (!pgnames) {
		ret = -ENOMEM;
		goto free_map;
	}

	ret = pch_parse_one_pinctrl_entry(pch, np_config, map,
			num_maps, pgnames);
	if (ret < 0) {
		dev_err(pch->dev, "no pins entries for %s\n",
			np_config->name);
		goto free_pgnames;
	}

	return 0;

free_pgnames:
	devm_kfree(pch->dev, pgnames);
free_map:
	devm_kfree(pch->dev, *map);

	return ret;
}

/**
 * pch_free_resources() - free memory used by this driver
 * @pch: pch driver instance
 */
static void pch_free_resources(struct pch_device *pch)
{
	pinctrl_unregister(pch->pctl);

	if (pch->missing_nr_pinctrl_cells)
		of_remove_property(pch->np, pch->missing_nr_pinctrl_cells);
}

static int pch_add_gpio_func(struct device_node *node, struct pch_device *pch)
{
	const char *propname = "pinctrl-v500,gpio-range";
	const char *cellname = "#pinctrl-v500,gpio-range-cells";
	struct of_phandle_args gpiospec;
	struct pch_gpiofunc_range *range;
	int ret, i;

	for (i = 0; ; i++) {
		ret = of_parse_phandle_with_args(node, propname, cellname,
						 i, &gpiospec);
		/* Do not treat it as error. Only treat it as end condition. */
		if (ret) {
			ret = 0;
			break;
		}
		range = devm_kzalloc(pch->dev, sizeof(*range), GFP_KERNEL);
		if (!range) {
			ret = -ENOMEM;
			break;
		}
		range->offset = gpiospec.args[0];
		range->npins = gpiospec.args[1];
		range->gpiofunc = gpiospec.args[2];
		mutex_lock(&pch->mutex);
		list_add_tail(&range->node, &pch->gpiofuncs);
		mutex_unlock(&pch->mutex);
	}
	return ret;
}

#ifdef CONFIG_PM
static int pinctrl_v500_suspend(struct platform_device *pdev,
					pm_message_t state)
{
	struct pch_device *pch;

	pch = platform_get_drvdata(pdev);
	if (!pch)
		return -EINVAL;

	return pinctrl_force_sleep(pch->pctl);
}

static int pinctrl_v500_resume(struct platform_device *pdev)
{
	struct pch_device *pch;

	pch = platform_get_drvdata(pdev);
	if (!pch)
		return -EINVAL;

	return pinctrl_force_default(pch->pctl);
}
#endif

/**
 * pch_quirk_missing_pinctrl_cells - handle legacy binding
 * @pch: pinctrl driver instance
 * @np: device tree node
 * @cells: number of cells
 *
 * Handle legacy binding with no #pinctrl-cells. This should be
 * always two pinctrl-v500,bit-per-mux and one for others.
 * At some point we may want to consider removing this.
 */
static int pch_quirk_missing_pinctrl_cells(struct pch_device *pch,
					   struct device_node *np,
					   int cells)
{
	struct property *p;
	const char *name = "#pinctrl-cells";
	int error;
	u32 val;

	error = of_property_read_u32(np, name, &val);
	if (!error)
		return 0;

	dev_warn(pch->dev, "please update dts to use %s = <%i>\n",
		 name, cells);

	p = devm_kzalloc(pch->dev, sizeof(*p), GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	p->length = (int)sizeof(__be32);
	p->value = devm_kzalloc(pch->dev, sizeof(__be32), GFP_KERNEL);
	if (!p->value)
		return -ENOMEM;
	*(__be32 *)p->value = cpu_to_be32(cells);

	p->name = devm_kstrdup(pch->dev, name, GFP_KERNEL);
	if (!p->name)
		return -ENOMEM;

	pch->missing_nr_pinctrl_cells = p;

	error = of_add_property(np, pch->missing_nr_pinctrl_cells);

	return error;
}

static int pch_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	struct pch_device *pch;
	const struct pch_soc_data *soc;
	int ret;

	soc = of_device_get_match_data(&pdev->dev);
	if (WARN_ON(!soc))
		return -EINVAL;

	pch = devm_kzalloc(&pdev->dev, sizeof(*pch), GFP_KERNEL);
	if (!pch) {
		dev_err(&pdev->dev, "could not allocate\n");
		return -ENOMEM;
	}
	pch->dev = &pdev->dev;
	pch->np = np;
	raw_spin_lock_init(&pch->lock);
	mutex_init(&pch->mutex);
	INIT_LIST_HEAD(&pch->gpiofuncs);
	pch->flags = soc->flags;
	memcpy(&pch->socdata, soc, sizeof(*soc));

	ret = of_property_read_u32(np, "pinctrl-v500,register-width",
				   &pch->width);
	if (ret) {
		dev_err(pch->dev, "register width not specified\n");

		return ret;
	}

	ret = of_property_read_u32(np, "pinctrl-v500,function-mask",
				   &pch->fmask);
	if (!ret) {
		pch->fshift = __ffs(pch->fmask);
		pch->fmax = pch->fmask >> pch->fshift;
	} else {
		/* If mask property doesn't exist, function mux is invalid. */
		pch->fmask = 0;
		pch->fshift = 0;
		pch->fmax = 0;
	}

	ret = pch_quirk_missing_pinctrl_cells(pch, np, 1);
	if (ret) {
		dev_err(&pdev->dev, "unable to patch #pinctrl-cells\n");

		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(pch->dev, "could not get resource\n");
		return -ENODEV;
	}

	pch->res = devm_request_mem_region(pch->dev, res->start,
			resource_size(res), DRIVER_NAME);
	if (!pch->res) {
		dev_err(pch->dev, "could not get mem_region\n");
		return -EBUSY;
	}

	pch->size = resource_size(pch->res);
	pch->base = devm_ioremap(pch->dev, pch->res->start, pch->size);
	if (!pch->base) {
		dev_err(pch->dev, "could not ioremap\n");
		return -ENODEV;
	}

	platform_set_drvdata(pdev, pch);

	pch->read = pch_readl;
	pch->write = pch_writel;
	pch->desc.name = DRIVER_NAME;
	pch->desc.pctlops = &pch_pinctrl_ops;
	pch->desc.pmxops = &pch_pinmux_ops;
	if (pch->flags & PCH_FEAT_PINCONF)
		pch->desc.confops = &pch_pinconf_ops;
	pch->desc.owner = THIS_MODULE;

	ret = pch_allocate_pin_table(pch);
	if (ret < 0)
		goto free;

	ret = pinctrl_register_and_init(&pch->desc, pch->dev, pch, &pch->pctl);
	if (ret) {
		dev_err(pch->dev, "could not register single pinctrl driver\n");
		goto free;
	}

	ret = pch_add_gpio_func(np, pch);
	if (ret < 0)
		goto free;

	dev_info(pch->dev, "%i pins at pa %p size %u\n",
		 pch->desc.npins, pch->base, pch->size);

	return pinctrl_enable(pch->pctl);

free:
	pch_free_resources(pch);

	return ret;
}

static int pch_remove(struct platform_device *pdev)
{
	struct pch_device *pch = platform_get_drvdata(pdev);

	if (!pch)
		return 0;

	pch_free_resources(pch);

	return 0;
}

static const struct pch_soc_data pinctrl_v500 = {
};

static const struct pch_soc_data pinconf_v500 = {
	.flags = PCH_FEAT_PINCONF,
};

static const struct of_device_id pch_of_match[] = {
	{ .compatible = "pinctrl-v500", .data = &pinctrl_v500 },
	{ .compatible = "pinconf-v500", .data = &pinconf_v500 },
	{ },
};
MODULE_DEVICE_TABLE(of, pch_of_match);

static struct platform_driver pch_driver = {
	.probe		= pch_probe,
	.remove		= pch_remove,
	.driver = {
		.name		= DRIVER_NAME,
		.of_match_table	= pch_of_match,
	},
#ifdef CONFIG_PM
	.suspend = pinctrl_v500_suspend,
	.resume = pinctrl_v500_resume,
#endif
};

static int __init pinctrl_v500_init(void)
{
	return platform_driver_register(&pch_driver);
}
static void __exit pinctrl_v500_exit(void)
{
	platform_driver_unregister(&pch_driver);
}
arch_initcall(pinctrl_v500_init);
module_exit(pinctrl_v500_exit);
