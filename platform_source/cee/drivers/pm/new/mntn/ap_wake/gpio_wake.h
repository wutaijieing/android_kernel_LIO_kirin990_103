/*
 * gpio_wake.h
 *
 * wakeup gpio
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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

#ifndef __H_PM_MNTN_GPIO_WAKE_H__
#define __H_PM_MNTN_GPIO_WAKE_H__

#include <linux/of.h>

int init_wake_gpio_table(const struct device_node *lp_dn);
int pm_get_gpio_by_irq(unsigned int irq);

#endif
