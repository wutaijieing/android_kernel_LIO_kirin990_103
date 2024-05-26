/* SPDX-License-Identifier: GPL-2.0 */
/*
 * power_pinctrl.h
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

#ifndef _POWER_PINCTRL_H_
#define _POWER_PINCTRL_H_

#include <linux/device.h>
#include <linux/pinctrl/consumer.h>

#define POWER_PINCTRL_COL      1

struct pinctrl *power_pinctrl_get(struct device *dev);
struct pinctrl_state *power_pinctrl_lookup_state(struct pinctrl *pinctrl, const char *prop);
int power_pinctrl_select_state(struct pinctrl *pinctrl, struct pinctrl_state *state);
int power_pinctrl_config(struct device *dev, const char *prop, u32 len);
int power_pinctrl_config_state(struct device *dev, const char *prop);

#endif /* _POWER_PINCTRL_H_ */
