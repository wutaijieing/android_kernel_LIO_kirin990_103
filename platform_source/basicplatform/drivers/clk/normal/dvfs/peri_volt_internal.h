/*
 * peri_volt_internal.h
 *
 * Hisilicon clock driver
 *
 * Copyright (c) 2017-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef __PERIVOLT_POLL_INTERNAL_H_
#define __PERIVOLT_POLL_INTERNAL_H_

#include "platform_include/basicplatform/linux/peri_volt_poll.h"

#define PERI_GET_VOLT_NOCACHE		0x1

struct of_device_id;
extern const struct of_device_id __perivolt_of_table[];

typedef void (*of_perivolt_init_cb_t) (struct device_node *);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
#define PERIVOLT_OF_DECLARE(name, compat, fn)                        \
	static const struct of_device_id __perivolt_of_table_##name  \
		__used __section("__perivolt_of_table")                \
		= { .compatible = (compat), .data = (fn) }
#else
#define PERIVOLT_OF_DECLARE(name, compat, fn)                        \
	static const struct of_device_id __perivolt_of_table_##name  \
		__used __section(__perivolt_of_table)                \
		= { .compatible = (compat), .data = (fn) }
#endif

int perivolt_register(struct device *dev, struct peri_volt_poll *pvp);

#endif /* __PERIVOLT_POLL_INTERNAL_H */
