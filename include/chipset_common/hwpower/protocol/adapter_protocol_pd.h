/* SPDX-License-Identifier: GPL-2.0 */
/*
 * adapter_protocol_pd.h
 *
 * pd protocol driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#ifndef _ADAPTER_PROTOCOL_PD_H_
#define _ADAPTER_PROTOCOL_PD_H_

#include <linux/errno.h>

struct hwpd_ops {
	const char *chip_name;
	void *dev_data;
	void (*hard_reset_master)(void *dev_data);
	void (*set_output_voltage)(int volt, void *dev_data);
};

struct hwpd_dev {
	struct device *dev;
	struct hwpd_ops *p_ops;
	int volt;
	int dev_id;
};

#ifdef CONFIG_ADAPTER_PROTOCOL_PD
int hwpd_ops_register(struct hwpd_ops *ops);
#else
static inline int hwpd_ops_register(struct hwpd_ops *ops)
{
	return -EPERM;
}
#endif /* CONFIG_ADAPTER_PROTOCOL_PD */

#endif /* _ADAPTER_PROTOCOL_PD_H_ */
