// SPDX-License-Identifier: GPL-2.0
/*
 * battery_balance.c
 *
 * huawei parallel battery balance driver
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_supply_interface.h>
#include <chipset_common/hwpower/common_module/power_log.h>
#include <chipset_common/hwpower/battery/battery_capacity_public.h>
#include <chipset_common/hwpower/battery/battery_temp.h>
#include <chipset_common/hwpower/battery/battery_parallel.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/coul/coul_interface.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/common_module/power_dsm.h>

#define HWLOG_TAG battery_balance
HWLOG_REGIST();

#define BAT_MAIN                0
#define BAT_AUX                 1
#define BAL_CCCV_TAB_LEN        10
#define BAL_CCCV_TAB_COL        2
#define BAL_TEMP_TAB_LEN        10
#define BAL_TEMP_TAB_COL        2
#define BAL_UNBAL_TAB_LEN       2
#define BAL_PRECISION           1000
#define BAL_DETECT_CYCLE        5
#define BAL_RATIO_ERR_COUNT     3
#define BAL_DSM_BUF_SIZE        256
#define BAL_DETECT_CUR_TH       5600
#define BAL_UNBLANCE_MAX        1
#define BAL_UNBLANCE_MIN        0

struct bat_temp_cccv {
	int temp;
	int len;
	struct bat_cccv_node cccv_tab[BAL_CCCV_TAB_LEN];
};

struct bat_param {
	u32 weight;
	int len;
	struct bat_temp_cccv temp_tab[BAL_TEMP_TAB_LEN];
};

struct bat_bal_device {
	struct device *dev;
	struct bat_param param_tab[BAT_PARALLEL_COUNT];
	int unbalance_th[BAL_UNBAL_TAB_LEN];
	int req_cur[BAT_PARALLEL_COUNT];
	int bal_cur;
	int ratio_err_cnt;
	int detect_cycle;
};

static struct bat_bal_device *g_bat_bal_dev;

static int bat_bal_get_cccv_node(int bat_id, struct bat_balance_info info,
	struct bat_cccv_node *node)
{
	int i;
	struct bat_bal_device *di = g_bat_bal_dev;
	struct bat_param *param = NULL;
	struct bat_temp_cccv *temp_cccv = NULL;

	if (!di)
		return -ENODEV;

	param = &di->param_tab[bat_id];
	for (i = 0; i < param->len; i++) {
		if (info.temp < param->temp_tab[i].temp)
			break;
	}
	if (i >= param->len)
		i = param->len - 1;
	temp_cccv = &param->temp_tab[i];
	for (i = 0; i < temp_cccv->len; i++) {
		if (info.vol < temp_cccv->cccv_tab[i].vol)
			break;
	}
	if (i >= temp_cccv->len)
		i = temp_cccv->len - 1;
	*node = temp_cccv->cccv_tab[i];
	return 0;
}

static void bat_bal_requst_current(int cur, int other_cur,
	struct bat_cccv_node cccv,
	struct bat_cccv_node other_cccv,
	int *own, int *other)
{
	int delta;

	if (cur <= 0) {
		*own = cccv.cur;
		*other = other_cccv.cur;
	} else {
		*own = cccv.cur;
		delta = cccv.cur - cur;
		if (other_cur <= 0) {
			*other = 0;
		} else {
			*other = other_cur + delta * other_cur / cur;
			if (*other < 0)
				*other = 0;
		}
	}
}

static void bat_bal_current_bias_detect_dmd(struct bat_balance_info *info,
	struct bat_bal_device *di)
{
	int cur_ratio = info[BAT_MAIN].cur * 1000 / info[BAT_AUX].cur; /* multiplied by 1000 to calc ratio */
	char tmp_buf[BAL_DSM_BUF_SIZE] = { 0 };
	int info_total_cur = info[BAT_MAIN].cur + info[BAT_AUX].cur;

	if (info_total_cur < BAL_DETECT_CUR_TH) {
		di->detect_cycle = 0;
		di->ratio_err_cnt = 0;
		return;
	}

	if ((cur_ratio < di->unbalance_th[BAL_UNBLANCE_MIN]) ||
		(cur_ratio > di->unbalance_th[BAL_UNBLANCE_MAX])) {
		di->ratio_err_cnt++;
		if (di->ratio_err_cnt == BAL_RATIO_ERR_COUNT) {
			snprintf(tmp_buf, BAL_DSM_BUF_SIZE,
				"%s: main-%d, %d, %d, aux-%d, %d, %d cur_ratio: %d ratio_range:%d %d\n",
				"cur_ratio out range, cur vol temp info",
				info[BAT_MAIN].cur, info[BAT_MAIN].vol, info[BAT_MAIN].temp,
				info[BAT_AUX].cur, info[BAT_AUX].vol, info[BAT_AUX].temp, cur_ratio,
				di->unbalance_th[BAL_UNBLANCE_MIN], di->unbalance_th[BAL_UNBLANCE_MAX]);
			power_dsm_report_dmd(POWER_DSM_BATTERY_DETECT,
				POWER_DSM_DUAL_BATTERY_CURRENT_BIAS_DETECT, tmp_buf);
		}
	}

	di->detect_cycle++;
	if (di->detect_cycle >= BAL_DETECT_CYCLE) {
		di->detect_cycle = 0;
		di->ratio_err_cnt = 0;
	}
}

int bat_balance_get_cur_info(struct bat_balance_info *info, u32 info_len,
	struct bat_balance_cur *result)
{
	struct bat_bal_device *di = g_bat_bal_dev;
	struct bat_cccv_node cccv_node[BAT_PARALLEL_COUNT];
	int own_cur[BAT_PARALLEL_COUNT];
	int other_cur[BAT_PARALLEL_COUNT];
	int request_cur[BAT_PARALLEL_COUNT];
	int is_exist[BAT_PARALLEL_COUNT] = { 0 };

	if (!di)
		return -EINVAL;

	if (!info || !result) {
		hwlog_err("info, result error\n");
		return -EINVAL;
	}

	if (info_len != BAT_PARALLEL_COUNT) {
		hwlog_err("info_len error\n");
		return -EINVAL;
	}

	bat_bal_current_bias_detect_dmd(info, di);

	hwlog_info("%s info main %d %d %d, aux %d %d %d\n", __func__,
		info[BAT_MAIN].temp, info[BAT_MAIN].vol, info[BAT_MAIN].cur,
		info[BAT_AUX].temp, info[BAT_AUX].vol, info[BAT_AUX].cur);
	bat_bal_get_cccv_node(BAT_MAIN, info[BAT_MAIN], &cccv_node[BAT_MAIN]);
	bat_bal_get_cccv_node(BAT_AUX, info[BAT_AUX], &cccv_node[BAT_AUX]);
	result->cccv[BAT_MAIN] = cccv_node[BAT_MAIN];
	result->cccv[BAT_AUX] = cccv_node[BAT_AUX];
	hwlog_info("%s cccv main %d %d, aux %d %d\n", __func__,
		cccv_node[BAT_MAIN].vol, cccv_node[BAT_MAIN].cur,
		cccv_node[BAT_AUX].vol, cccv_node[BAT_AUX].cur);

	power_supply_get_int_property_value("battery_gauge",
		POWER_SUPPLY_PROP_PRESENT, &is_exist[BAT_MAIN]);
	power_supply_get_int_property_value("battery_gauge_aux",
		POWER_SUPPLY_PROP_PRESENT, &is_exist[BAT_AUX]);
	if (!is_exist[BAT_MAIN] && !is_exist[BAT_AUX]) {
		hwlog_err("battery main and aux not exist, do not balance\n");
		result->total_cur =
			cccv_node[BAT_MAIN].cur + cccv_node[BAT_AUX].cur;
		di->bal_cur = result->total_cur;
		return 0;
	} else if (!is_exist[BAT_MAIN]) {
		hwlog_err("battery main not exist, do not balance\n");
		result->total_cur = cccv_node[BAT_AUX].cur;
		di->bal_cur = result->total_cur;
		return 0;
	} else if (!is_exist[BAT_AUX]) {
		hwlog_err("battery aux not exist, do not balance\n");
		result->total_cur = cccv_node[BAT_MAIN].cur;
		di->bal_cur = result->total_cur;
		return 0;
	}

	bat_bal_requst_current(info[BAT_MAIN].cur, info[BAT_AUX].cur,
		cccv_node[BAT_MAIN], cccv_node[BAT_AUX],
		&own_cur[BAT_MAIN], &other_cur[BAT_MAIN]);
	bat_bal_requst_current(info[BAT_AUX].cur, info[BAT_MAIN].cur,
		cccv_node[BAT_AUX], cccv_node[BAT_MAIN],
		&own_cur[BAT_AUX], &other_cur[BAT_AUX]);
	hwlog_info("%s own_cur %d %d, other_cur %d %d\n", __func__,
		own_cur[BAT_MAIN], own_cur[BAT_AUX],
		other_cur[BAT_MAIN], other_cur[BAT_AUX]);

	request_cur[BAT_MAIN] = own_cur[BAT_MAIN] <= other_cur[BAT_AUX] ?
		own_cur[BAT_MAIN] : other_cur[BAT_AUX];
	request_cur[BAT_AUX] = own_cur[BAT_AUX] <= other_cur[BAT_MAIN] ?
		own_cur[BAT_AUX] : other_cur[BAT_MAIN];

	result->total_cur = request_cur[BAT_MAIN] + request_cur[BAT_AUX];
	hwlog_info("%s total_cur %d request_cur %d %d\n", __func__,
		result->total_cur,
		request_cur[BAT_MAIN], request_cur[BAT_AUX]);
	di->bal_cur = result->total_cur;
	di->req_cur[BAT_MAIN] = request_cur[BAT_MAIN];
	di->req_cur[BAT_AUX] = request_cur[BAT_AUX];

	return 0;
}

static int bat_bal_get_log_head(char *buffer, int size, void *dev_data)
{
	struct bat_bal_device *di = dev_data;

	if (!di || !buffer)
		return -EPERM;

	snprintf(buffer, size, " bal_cur req_cur0 req_cur1 ");

	return 0;
}

static int bat_bal_dump_log_data(char *buffer, int size, void *dev_data)
{
	struct bat_bal_device *di = dev_data;

	if (!di || !buffer)
		return -EPERM;

	snprintf(buffer, size, "  %-8d%-9d%-10d ",
		di->bal_cur, di->req_cur[BAT_MAIN], di->req_cur[BAT_AUX]);

	return 0;
}

static struct power_log_ops bat_bal_log_ops = {
	.dev_name = "bat_1s2p_balance",
	.dump_log_head = bat_bal_get_log_head,
	.dump_log_content = bat_bal_dump_log_data,
};

static int bat_bal_cccv_tab_dts(struct device_node *node,
	const char *tab_name,
	struct bat_temp_cccv *temp_cccv)
{
	int i, len;
	u32 data;

	len = power_dts_read_u32_count(power_dts_tag(HWLOG_TAG), node,
			tab_name, BAL_CCCV_TAB_LEN, BAL_CCCV_TAB_COL);
	if (len <= 0) {
		hwlog_err("cccv_tab_dts get failed\n");
		return -ENODEV;
	}
	temp_cccv->len = len / BAL_CCCV_TAB_COL;
	for (i = 0; i < len; i++) {
		if (power_dts_read_u32_index(power_dts_tag(HWLOG_TAG),
			node, tab_name, i, &data))
			return -ENODEV;
		if ((i % BAL_CCCV_TAB_COL) == 0)
			temp_cccv->cccv_tab[i / BAL_CCCV_TAB_COL].vol = data;
		else
			temp_cccv->cccv_tab[i / BAL_CCCV_TAB_COL].cur = data;
	}
	return 0;
}

static int bat_bal_parse_bat_dts(int bat_id, struct bat_bal_device *di,
	struct device_node *node)
{
	int ret;
	int i, len;
	int idata = 0;
	const char *string = NULL;

	ret = power_dts_read_u32(power_dts_tag(HWLOG_TAG), node,
		"weight", &di->param_tab[bat_id].weight, 2000); /* 2000mAH default */
	if (ret) {
		hwlog_err("batt %d weight failed\n", bat_id);
		return ret;
	}
	len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), node,
		"temp_tab", BAL_TEMP_TAB_LEN, BAL_TEMP_TAB_COL);
	if (len < 0)
		return -ENODEV;
	di->param_tab[bat_id].len = len / BAL_TEMP_TAB_COL;
	for (i = 0; i < len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG),
			node, "temp_tab", i, &string))
			return -ENODEV;

		if ((i % BAL_TEMP_TAB_COL) == 0) {
			if (kstrtoint(string, POWER_BASE_DEC, &idata))
				return -ENODEV;
			di->param_tab[bat_id].temp_tab[i / BAL_TEMP_TAB_COL].temp =
				idata;
			hwlog_info("bat_id %d temp_tab temp:%d\n", bat_id, idata);
		} else {
			if (bat_bal_cccv_tab_dts(node, string,
				&di->param_tab[bat_id].temp_tab[i / BAL_TEMP_TAB_COL]))
				return -ENODEV;
		}
	}
	return 0;
}

static int bat_bal_parse_dts(struct bat_bal_device *di)
{
	struct device_node *np = di->dev->of_node;
	int ret;
	struct device_node *bat_node = NULL;

	ret = power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"unbalance_th", di->unbalance_th, BAL_UNBAL_TAB_LEN);
	if (ret)
		return ret;

	bat_node = of_find_node_by_name(np, "battery0");
	if (!bat_node) {
		hwlog_err("batt0 find failed\n");
		return -ENODEV;
	}
	ret = bat_bal_parse_bat_dts(BAT_MAIN, di, bat_node);
	if (ret) {
		hwlog_err("batt0 para read failed\n");
		return ret;
	}

	bat_node = of_find_node_by_name(np, "battery1");
	if (!bat_node) {
		hwlog_err("batt1 find failed\n");
		return -ENODEV;
	}
	ret = bat_bal_parse_bat_dts(BAT_AUX, di, bat_node);
	if (ret) {
		hwlog_err("batt1 para read failed\n");
		return ret;
	}
	return 0;
}

static int bat_bal_probe(struct platform_device *pdev)
{
	struct bat_bal_device *di = NULL;
	int ret;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;
	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;
	di->dev = &pdev->dev;
	ret = bat_bal_parse_dts(di);
	if (ret) {
		hwlog_err("parse dts err\n");
		return ret;
	}
	g_bat_bal_dev = di;

	bat_bal_log_ops.dev_data = (void *)di;
	power_log_ops_register(&bat_bal_log_ops);

	hwlog_info("%s ok\n", __func__);
	return 0;
}

static int bat_bal_remove(struct platform_device *pdev)
{
	struct bat_bal_device *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;
	g_bat_bal_dev = NULL;
	return 0;
}

static const struct of_device_id bat_bal_match_table[] = {
	{
		.compatible = "huawei,battery_1s2p_bal",
		.data = NULL,
	},
	{},
};

static struct platform_driver bat_bal_driver = {
	.probe = bat_bal_probe,
	.remove = bat_bal_remove,
	.driver = {
		.name = "huawei,battery_balance",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(bat_bal_match_table),
	},
};

static int __init bat_bal_init(void)
{
	return platform_driver_register(&bat_bal_driver);
}

static void __exit bat_bal_exit(void)
{
	platform_driver_unregister(&bat_bal_driver);
}

late_initcall_sync(bat_bal_init);
module_exit(bat_bal_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("parallel battery balance driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
