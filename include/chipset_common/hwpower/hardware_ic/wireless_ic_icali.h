/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_ic_icali.h
 *
 * curr calibration for wireless ic
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#ifndef _WL_CALIBRATION_H_
#define _WL_CALIBRATION_H_

#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>

#define WLTRX_ICALI_COL      10

struct wltrx_icali_point {
	/* x1 x2: ic adc samping y1 y2: equipment sampling */
	int x1;
	int x2;
	int y1;
	int y2;
};

struct wltrx_icali_ops {
	void *dev_data;
	int (*init)(void *dev_data);
	void (*exit)(void *dev_data);
	int (*calibration)(struct wltrx_icali_point point, void *dev_data);
	int (*get_current)(int *value, void *dev_data);
	int (*get_current_offset)(int *value, void *dev_data);
	int (*get_current_gain)(int *value, void *dev_data);
};

struct wltrx_icali_info {
	struct wltrx_icali_ops *ops[WLTRX_DRV_MAX];
	int x1[WLTRX_ICALI_COL];
	int y1[WLTRX_ICALI_COL];
	int x2[WLTRX_ICALI_COL];
	int y2[WLTRX_ICALI_COL];
	int curr_index1;
	int curr_index2;
	int ic_type;
};

int wltrx_icali_ops_register(struct wltrx_icali_ops *ops,
	unsigned int type);

#endif /* _WL_CALIBRATION_H_ */
