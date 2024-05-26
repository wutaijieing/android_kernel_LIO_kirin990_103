// SPDX-License-Identifier: GPL-2.0
/*
 * power_pinctrl.c
 *
 * pinctrl interface for power module
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <chipset_common/hwpower/common_module/power_pinctrl.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG power_pinctrl
HWLOG_REGIST();

struct pinctrl *power_pinctrl_get(struct device *dev)
{
	struct pinctrl *pinctrl = NULL;

	if (!dev) {
		hwlog_err("dev is null\n");
		return NULL;
	}

	pinctrl = pinctrl_get(dev);
	if (IS_ERR(pinctrl)) {
		hwlog_err("get failed\n");
		return NULL;
	}

	return pinctrl;
}

struct pinctrl_state *power_pinctrl_lookup_state(struct pinctrl *pinctrl, const char *prop)
{
	struct pinctrl_state *state = NULL;

	if (!pinctrl || !prop) {
		hwlog_err("pinctrl or prop is null\n");
		return NULL;
	}

	state = pinctrl_lookup_state(pinctrl, prop);
	if (IS_ERR(state)) {
		hwlog_err("lookup %s state failed\n", prop);
		return NULL;
	}

	return state;
}

int power_pinctrl_select_state(struct pinctrl *pinctrl, struct pinctrl_state *state)
{
	if (!pinctrl || !state) {
		hwlog_err("pinctrl or state is null\n");
		return -EINVAL;
	}

	if (pinctrl_select_state(pinctrl, state)) {
		hwlog_err("select_state failed\n");
		return -EINVAL;
	}

	return 0;
}

int power_pinctrl_config_state(struct device *dev, const char *prop)
{
	struct pinctrl_state *state = NULL;
	struct pinctrl *pinctrl = NULL;
	int ret;

	if (!dev || !prop) {
		hwlog_err("dev or prop is null\n");
		return -EINVAL;
	}

	pinctrl = power_pinctrl_get(dev);
	if (!pinctrl)
		return -EINVAL;

	state = power_pinctrl_lookup_state(pinctrl, prop);
	if (!state) {
		hwlog_err("could not get prop state\n");
		return -EINVAL;
	}

	ret = power_pinctrl_select_state(pinctrl, state);
	if (ret < 0) {
		hwlog_err("could not set pins to this state\n");
		return -EINVAL;
	}

	return 0;
}

int power_pinctrl_config(struct device *dev, const char *prop, u32 len)
{
	int i, array_len;
	const char *tmp_string = NULL;
	struct pinctrl *pinctrl = NULL;
	struct pinctrl_state *state = NULL;

	if (!dev || !dev->of_node || !prop) {
		hwlog_err("dev or np or prop is null\n");
		return -EINVAL;
	}

	array_len = power_dts_read_count_strings("power_pinctrl", dev->of_node,
		prop, len, POWER_PINCTRL_COL);
	if (array_len <= 0)
		return 0;

	pinctrl = power_pinctrl_get(dev);
	if (!pinctrl)
		return -EINVAL;

	for (i = 0; i < array_len; i++) {
		if (power_dts_read_string_index("power_pinctrl",
			dev->of_node, prop, i, &tmp_string))
			return -EINVAL;

		state = power_pinctrl_lookup_state(pinctrl, tmp_string);
		if (!state)
			return -EINVAL;

		if (power_pinctrl_select_state(pinctrl, state))
			return -EINVAL;
	}

	return 0;
}
