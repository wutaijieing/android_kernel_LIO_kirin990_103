// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_ic_icali.c
 *
 * current calibration for wireless ic
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

#include <chipset_common/hwpower/common_module/power_calibration.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_nv.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/charger/charger_common_interface.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <chipset_common/hwpower/hardware_ic/wireless_ic_icali.h>
#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>
#include <chipset_common/hwpower/common_module/power_delay.h>

#define HWLOG_TAG wireless_ic_icali
HWLOG_REGIST();

enum wltrx_icali_set {
	IC_TYPE_OFFSET = 0,
	INIT_CHIP_OFFSET,
	CUR_Y1_OFFSET,
	CUR_Y2_OFFSET,
	ICALI_OFFSET,
	ICALI_SET_MAX,
};

enum wltrx_icali_get {
	GET_SUPPORT_OFFSET = 0,
	GET_PARA_A_OFFSET,
	GET_PARA_B_OFFSET,
	ICALI_GET_MAX,
};

static struct wltrx_icali_info *g_wltrx_icali_di;

int wltrx_icali_ops_register(struct wltrx_icali_ops *ops,
	unsigned int type)
{
	struct wltrx_icali_info *di = g_wltrx_icali_di;

	if (!di || !ops || !wltrx_is_drv_type_valid(type)) {
		hwlog_err("ops_register: invalid para\n");
		return -EINVAL;
	}

	if (di->ops[type]) {
		hwlog_err("ops_register: type=%u, already registered\n", type);
		return -EEXIST;
	}

	di->ops[type] = ops;
	hwlog_info("[ops_register] succ, type=%u\n", type);
	return 0;
}

static struct wltrx_icali_ops *wltrx_icali_get_ops(struct wltrx_icali_info *di)
{
	if (!wltrx_ic_is_type_valid(di->ic_type)) {
		hwlog_err("get_ops: ic_type=%u err\n", di->ic_type);
		return NULL;
	}

	return di->ops[di->ic_type];
}

static int wltrx_icali_get_avg_y(int *data, int len,
	int *avg, int *index_max, int *index_min)
{
	int i;
	int max, min;
	int sum = 0;

	max = data[0];
	min = data[0];
	*index_max = 0;
	*index_min = 0;
	for (i = 0; i < len; i++) {
		if (max < data[i]) {
			*index_max = i;
			max = data[i];
		}
		if (min > data[i]) {
			*index_min = i;
			min = data[i];
		}
		sum += data[i];
	}

	hwlog_info("[get_avg_y] sum=%d len=%d max=%d min=%d\n",
		sum, len, max, min);
	/* 2: delete the maximum and minimum */
	if (len - 2 <= 0)
		return -EINVAL;
	*avg = (sum - max - min) / (len - 2);
	return 0;
}

static int wltrx_icali_get_avg_x(int *data, int len,
	int *avg, int index_max, int index_min)
{
	int i;
	int sum = 0;
	int max = data[index_max];
	int min = data[index_min];

	for (i = 0; i < len; i++)
		sum += data[i];

	hwlog_info("[get_avg_x] sum=%d len=%d max=%d min=%d\n",
		sum, len, max, min);
	/* 2: delete the maximum and minimum */
	if (len - 2 <= 0)
		return -EINVAL;
	*avg = (sum - max - min) / (len - 2);
	return 0;
}

static int wltrx_icali_get_point(struct wltrx_icali_info *di, struct wltrx_icali_point *point)
{
	int ret;
	int index_max = 0;
	int index_min = 0;

	if (di->curr_index1 != di->curr_index2) {
		hwlog_err("[get_point] index error\n");
		return -EINVAL;
	}

	ret = wltrx_icali_get_avg_y(di->y1, di->curr_index1, &point->y1,
		&index_max, &index_min);
	ret += wltrx_icali_get_avg_x(di->x1, di->curr_index1, &point->x1,
		index_max, index_min);
	ret += wltrx_icali_get_avg_y(di->y2, di->curr_index2, &point->y2,
		&index_max, &index_min);
	ret += wltrx_icali_get_avg_x(di->x2, di->curr_index2, &point->x2,
		index_max, index_min);
	hwlog_info("[get_point] x1=%d y1=%d x2=%d y2=%d\n",
		point->x1, point->y1, point->x2, point->y2);
	return ret;
}

static void wltrx_icali_init_data(struct wltrx_icali_info *di)
{
	di->curr_index1 = 0;
	di->curr_index2 = 0;
	memset(di->x1, 0, sizeof(di->x1));
	memset(di->x2, 0, sizeof(di->x2));
	memset(di->y1, 0, sizeof(di->y1));
	memset(di->y2, 0, sizeof(di->y2));
}

static int wltrx_icali_handle_y1_offset(struct wltrx_icali_info *di,
	struct wltrx_icali_ops *ops, int value)
{
	int curr = 0;
	int ret;

	if (di->curr_index1 >= WLTRX_ICALI_COL) {
		hwlog_err("handle_y1_offset: index out of range, index=%d\n",
			di->curr_index1);
		return 0;
	}

	ret = ops->get_current(&curr, ops->dev_data);
	di->y1[di->curr_index1] = value;
	di->x1[di->curr_index1] = curr;
	hwlog_info("[handle_y1_offset] index=%d x1=%d, y1=%d\n",
		di->curr_index1, curr, value);
	di->curr_index1++;
	return ret;
}

static int wltrx_icali_handle_y2_offset(struct wltrx_icali_info *di,
	struct wltrx_icali_ops *ops, int value)
{
	int curr = 0;
	int ret;

	if (di->curr_index2 >= WLTRX_ICALI_COL) {
		hwlog_err("handle_y2_offset: index out of range, index=%d\n", di->curr_index2);
		return 0;
	}
	ret = ops->get_current(&curr, ops->dev_data);
	di->y2[di->curr_index2] = value;
	di->x2[di->curr_index2] = curr;
	hwlog_info("[handle_y2_offset] index=%d x2=%d, y2=%d\n",
		di->curr_index2, curr, value);
	di->curr_index2++;

	return ret;
}

static int wltrx_icali_handle_set(struct wltrx_icali_info *di,
	struct wltrx_icali_ops *ops, unsigned int offset, int value)
{
	struct wltrx_icali_point point = { 0 };
	int ret;

	switch (offset) {
	case INIT_CHIP_OFFSET:
		wltrx_icali_init_data(di);
		ret = ops->init(ops->dev_data);
		break;
	case CUR_Y1_OFFSET:
		ret = wltrx_icali_handle_y1_offset(di, ops, value);
		break;
	case CUR_Y2_OFFSET:
		ret = wltrx_icali_handle_y2_offset(di, ops, value);
		break;
	case ICALI_OFFSET:
		wltrx_icali_get_point(di, &point);
		ret = ops->calibration(point, ops->dev_data);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static bool wltrx_icali_ops_valid(struct wltrx_icali_ops *ops)
{
	if (!ops || !ops->init || !ops->get_current ||
		!ops->calibration || !ops->exit || !ops->get_current_gain ||
		!ops->get_current_offset) {
		hwlog_err("ops_valid: invalid ops, not support calibration\n");
		return false;
	}

	return true;
}

static int wltrx_icali_set_data(unsigned int offset, int value, void *dev_data)
{
	int ret;
	struct wltrx_icali_info *di = dev_data;
	struct wltrx_icali_ops *ops = NULL;

	if (!di) {
		hwlog_err("set_data: invalid di\n");
		return -ENODEV;
	}

	hwlog_info("[set_data] offset=%u, value=%d\n", offset, value);
	if (offset == IC_TYPE_OFFSET) {
		di->ic_type = value;
		return 0;
	}

	ops = wltrx_icali_get_ops(di);
	if (!wltrx_icali_ops_valid(ops))
		return 0;

	ret = wltrx_icali_handle_set(di, ops, offset, value);
	if (ret || (offset == ICALI_OFFSET)) {
		ops->exit(ops->dev_data);
		hwlog_err("set_data: exit, ret=%d\n", ret);
	}

	return ret;
}

static int wltrx_icali_get_data(unsigned int offset, int *value, void *dev_data)
{
	int ret;
	struct wltrx_icali_info *di = dev_data;
	struct wltrx_icali_ops *ops = NULL;

	if (!di || !value) {
		hwlog_err("get_data: invalid di\n");
		return -ENODEV;
	}

	hwlog_info("[get_data] offset=%u\n", offset);
	ops = wltrx_icali_get_ops(di);
	if (!wltrx_icali_ops_valid(ops)) {
		if (offset == GET_SUPPORT_OFFSET)
			*value = 0;
		return 0;
	}

	switch (offset) {
	case GET_SUPPORT_OFFSET:
		*value = 1;
		ret = 0;
		break;
	case GET_PARA_A_OFFSET:
		ret = ops->get_current_gain(value, ops->dev_data);
		break;
	case GET_PARA_B_OFFSET:
		ret = ops->get_current_offset(value, ops->dev_data);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret) {
		ops->exit(ops->dev_data);
		hwlog_err("get_data: failed\n");
	}

	return ret;
}

static struct power_cali_ops power_cali_wl_ops = {
	.name = "wltrx_curr",
	.get_data = wltrx_icali_get_data,
	.set_data = wltrx_icali_set_data,
	.save_data = NULL,
	.show_data = NULL,
};

static int __init wltrx_icali_init(void)
{
	struct wltrx_icali_info *di = NULL;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	g_wltrx_icali_di = di;
	power_cali_wl_ops.dev_data = di;
	power_cali_ops_register(&power_cali_wl_ops);
	return 0;
}

static void __exit wltrx_icali_exit(void)
{
	kfree(g_wltrx_icali_di);
	g_wltrx_icali_di = NULL;
}

subsys_initcall(wltrx_icali_init);
module_exit(wltrx_icali_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("calibration for direct charge module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
