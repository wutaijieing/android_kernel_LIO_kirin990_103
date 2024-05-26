// SPDX-License-Identifier: GPL-2.0
/*
 * battery_1s2p.c
 *
 * huawei parallel battery driver
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
#include <chipset_common/hwpower/battery/battery_capacity_public.h>
#include <chipset_common/hwpower/battery/battery_temp.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/coul/coul_interface.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/common_module/power_dsm.h>

#define HWLOG_TAG bat_1s2p
HWLOG_REGIST();

#define BAT_1S2P_PROP_NAME          64
#define BAT_1S2P_RETRY_TIMES        20
#define BAT_1S2P_SOC_FAST           10
#define BAT_1S2P_INTERVAL_NOR       (10 * 1000)
#define BAT_1S2P_INTERVAL_F1        (5 * 1000)
#define BAT_1S2P_INTERVAL_RETRY     (1000 / 2)
#define BAT_1S2P_CAP_DIFF_THR       5
#define BAT_1S2P_DSM_BUF_SIZE       256

#define BAT_1S2P_COUNT              2
#define BAT_MAIN                    0
#define BAT_AUX                     1

#define CAP_RATIO_MAX               1000
#define WEIGHT_FACTOR_MAX           100000
#define WEIGHT_FACTOR_DIV           1000
#define WEIGHT_FACTOR_COUNT         4
#define SOC_PRECISION               100

#define VOLT_THRESHOLD_COUNT        3

#define VOLT_LOW_THRESHOLD          2700

struct battery_info {
	int soc;
	int volt;
	int curr;
	u32 weight_dischg[WEIGHT_FACTOR_COUNT];
	u32 weight_chg[WEIGHT_FACTOR_COUNT];
	u32 volt_low;
	u32 cap_ratio;
	u32 weight_now;
	int is_exist;
	int is_ready;
};

struct bat_1s2p_device {
	struct device *dev;
	struct delayed_work monitor;
	struct wakeup_source *wakelock;
	struct mutex lock;
	struct battery_info bat_info[BAT_1S2P_COUNT];
	u32 volt_th_dischg[VOLT_THRESHOLD_COUNT];
	u32 volt_th_chg[VOLT_THRESHOLD_COUNT];
	unsigned int interval;
	int vol_type;
	int cycle_type;
	int last_cap_type;
	int temp_type;
	int soc;
	int retry_times;
	int cap_diff_thr;
};

static struct bat_1s2p_device *g_bat_1s2p_dev;

static void bat_1s2p_update_bat_info(struct bat_1s2p_device *di, int id);
static int bat_1s2p_info_mixing(struct bat_1s2p_device *di);

static void bat_1s2p_init_info(struct bat_1s2p_device *di)
{
	bat_1s2p_update_bat_info(di, COUL_TYPE_MAIN);
	bat_1s2p_update_bat_info(di, COUL_TYPE_AUX);
	di->soc = bat_1s2p_info_mixing(di);
	hwlog_info("%s soc:%d\n", __func__, di->soc);
}

static bool bat_1s2p_waiting_ready(struct bat_1s2p_device *di)
{
	if (di->retry_times < BAT_1S2P_RETRY_TIMES &&
		(!di->bat_info[BAT_MAIN].is_ready ||
		!di->bat_info[BAT_AUX].is_ready)) {
		hwlog_info("%s need waiting\n", __func__);
		return true;
	}

	if (di->retry_times < BAT_1S2P_RETRY_TIMES)
		di->retry_times = BAT_1S2P_RETRY_TIMES + 1;
	return false;
}

static int bat_1s2p_is_ready(void *dev_data)
{
	int ret;
	struct bat_1s2p_device *di = g_bat_1s2p_dev;

	if (!di)
		return 0;

	mutex_lock(&di->lock);
	ret = di->bat_info[BAT_MAIN].is_ready &&
		di->bat_info[BAT_AUX].is_ready;
	mutex_unlock(&di->lock);

	return ret;
}

static int bat_1s2p_is_battery_exist(void *dev_data)
{
	return coul_interface_is_battery_exist(COUL_TYPE_MAIN) ||
		coul_interface_is_battery_exist(COUL_TYPE_AUX);
}

static int bat_1s2p_read_battery_soc(void *dev_data)
{
	struct bat_1s2p_device *di = g_bat_1s2p_dev;

	if (!di)
		return coul_interface_get_battery_capacity(COUL_TYPE_MAIN);

	mutex_lock(&di->lock);
	if (bat_1s2p_waiting_ready(di)) /* try again */
		bat_1s2p_init_info(di);
	mutex_unlock(&di->lock);
	return di->soc;
}

static int bat_1s2p_read_battery_vol(void *dev_data)
{
	struct bat_1s2p_device *di = g_bat_1s2p_dev;
	int type = COUL_TYPE_MAIN;

	if (di)
		type = di->vol_type;

	return coul_interface_get_battery_voltage(type);
}

static int bat_1s2p_read_battery_current(void *dev_data)
{
	return coul_interface_get_battery_current(COUL_TYPE_MAIN) +
		coul_interface_get_battery_current(COUL_TYPE_AUX);
}

static int bat_1s2p_read_battery_avg_current(void *dev_data)
{
	return coul_interface_get_battery_avg_current(COUL_TYPE_MAIN) +
		coul_interface_get_battery_avg_current(COUL_TYPE_AUX);
}

static int bat_1s2p_read_battery_fcc(void *dev_data)
{
	int fcc0 = coul_interface_get_battery_fcc(COUL_TYPE_MAIN);
	int fcc1 = coul_interface_get_battery_fcc(COUL_TYPE_AUX);
	int sum = 0;

	if (fcc0 < 0 && fcc1 < 0)
		return -EPERM;
	if (fcc0 >= 0)
		sum += fcc0;
	if (fcc1 >= 0)
		sum += fcc1;
	return sum;
}

static int bat_1s2p_read_battery_cycle(void *dev_data)
{
	struct bat_1s2p_device *di = g_bat_1s2p_dev;
	int main_cycle, aux_cycle, cycle;
	int type = COUL_TYPE_MAIN;

	if (di)
		type = di->cycle_type;

	if (type == COUL_TYPE_1S2P) {
		main_cycle = coul_interface_get_battery_cycle(COUL_TYPE_MAIN);
		aux_cycle = coul_interface_get_battery_cycle(COUL_TYPE_AUX);
		cycle = main_cycle > aux_cycle ? main_cycle : aux_cycle;
	} else {
		cycle = coul_interface_get_battery_cycle(type);
	}

	return cycle;
}

static int bat_1s2p_read_battery_rm(void *dev_data)
{
	int rm0 = coul_interface_get_battery_rm(COUL_TYPE_MAIN);
	int rm1 = coul_interface_get_battery_rm(COUL_TYPE_AUX);
	int sum = 0;

	if (rm0 < 0 && rm1 < 0)
		return -EPERM;
	if (rm0 >= 0)
		sum += rm0;
	if (rm1 >= 0)
		sum += rm1;
	return sum;
}

static int bat_1s2p_set_battery_low_voltage(int val, void *dev_data)
{
	int ret;

	ret = coul_interface_set_battery_low_voltage(COUL_TYPE_MAIN, val);
	ret += coul_interface_set_battery_low_voltage(COUL_TYPE_AUX, val);
	return ret;
}

static int bat_1s2p_set_last_capacity(int capacity, void *dev_data)
{
	struct bat_1s2p_device *di = g_bat_1s2p_dev;
	int type = COUL_TYPE_MAIN;
	int ret;

	if (di)
		type = di->last_cap_type;

	if (type == COUL_TYPE_1S2P) {
		ret = coul_interface_set_battery_last_capacity(COUL_TYPE_MAIN,
			capacity);
		ret += coul_interface_set_battery_last_capacity(COUL_TYPE_AUX,
			capacity);
	} else {
		ret = coul_interface_set_battery_last_capacity(type, capacity);
	}
	hwlog_info("%s capacity %d\n", __func__, capacity);
	return ret;
}

static int bat_1s2p_get_last_capacity(void *dev_data)
{
	struct bat_1s2p_device *di = g_bat_1s2p_dev;
	int type;
	int last0, last1;

	if (!di)
		return -1; /* -1: not ready flag */

	type = di->last_cap_type;

	if (type != COUL_TYPE_1S2P)
		return coul_interface_get_battery_last_capacity(type);

	last0 = coul_interface_get_battery_last_capacity(COUL_TYPE_MAIN);
	last1 = coul_interface_get_battery_last_capacity(COUL_TYPE_AUX);
	hwlog_info("%s last0 %d last1 %d\n", __func__, last0, last1);
	if (last0 != last1)
		return bat_1s2p_read_battery_soc(di);
	return last0;
}

static int bat_1s2p_set_vterm_dec(int vterm, void *dev_data)
{
	int ret;

	ret = coul_interface_set_vterm_dec(COUL_TYPE_MAIN, vterm);
	ret += coul_interface_set_vterm_dec(COUL_TYPE_AUX, vterm);
	hwlog_info("%s vterm %d\n", __func__, vterm);
	return ret;
}

static int bat_1s2p_read_battery_temperature(void *dev_data)
{
	struct bat_1s2p_device *di = g_bat_1s2p_dev;
	int temp;
	int type = COUL_TYPE_MAIN;

	if (di)
		type = di->temp_type;

	if (type == COUL_TYPE_1S2P)
		bat_temp_get_temperature(BAT_TEMP_MIXED, &temp);
	else
		temp = coul_interface_get_battery_temperature(type);

	return temp;
}

static struct coul_interface_ops g_1s2p_ops = {
	.type_name = "1s2p",
	.is_coul_ready = bat_1s2p_is_ready,
	.is_battery_exist = bat_1s2p_is_battery_exist,
	.get_battery_capacity = bat_1s2p_read_battery_soc,
	.get_battery_voltage = bat_1s2p_read_battery_vol,
	.get_battery_current = bat_1s2p_read_battery_current,
	.get_battery_avg_current = bat_1s2p_read_battery_avg_current,
	.get_battery_temperature = bat_1s2p_read_battery_temperature,
	.get_battery_fcc = bat_1s2p_read_battery_fcc,
	.get_battery_cycle = bat_1s2p_read_battery_cycle,
	.set_battery_low_voltage = bat_1s2p_set_battery_low_voltage,
	.set_battery_last_capacity = bat_1s2p_set_last_capacity,
	.get_battery_last_capacity = bat_1s2p_get_last_capacity,
	.get_battery_rm = bat_1s2p_read_battery_rm,
	.set_vterm_dec = bat_1s2p_set_vterm_dec,
};

static void bat_1s2p_select_work_interval(struct bat_1s2p_device *di, int cap)
{
	if (bat_1s2p_waiting_ready(di)) {
		di->interval = BAT_1S2P_INTERVAL_RETRY;
		di->retry_times++;
		return;
	}

	/* select polling interval by SOC */
	if (cap < BAT_1S2P_SOC_FAST)
		di->interval = BAT_1S2P_INTERVAL_F1;
	else
		di->interval = BAT_1S2P_INTERVAL_NOR;
}

static int bat_1s2p_info_mixing(struct bat_1s2p_device *di)
{
	bool is_charge = (di->bat_info[BAT_MAIN].curr +
		di->bat_info[BAT_AUX].curr) > 0 ? true : false;
	u32 *p_weight = NULL;
	u32 *p_vol = NULL;
	int i, j, min_soc, max_soc;
	int weight_soc = 0;
	s64 ratio_soc;

	if (!di->bat_info[BAT_MAIN].is_exist &&
		!di->bat_info[BAT_AUX].is_exist)
		return coul_interface_get_battery_capacity(COUL_TYPE_MAIN);

	/* if any battery soc is 0 and voltage is low, return 0 */
	if ((di->bat_info[BAT_MAIN].soc <= 0) &&
		(di->bat_info[BAT_MAIN].volt <
		di->bat_info[BAT_MAIN].volt_low)) {
		hwlog_info("%s BAT_MAIN is low %d cmp %d\n", __func__,
			di->bat_info[BAT_MAIN].volt,
			di->bat_info[BAT_MAIN].volt_low);
		return 0;
	}

	if ((di->bat_info[BAT_AUX].soc <= 0) &&
		(di->bat_info[BAT_AUX].volt <
		di->bat_info[BAT_AUX].volt_low)) {
		hwlog_info("%s BAT_AUX is low %d cmp %d\n", __func__,
			di->bat_info[BAT_AUX].volt,
			di->bat_info[BAT_AUX].volt_low);
		return 0;
	}

	if (is_charge)
		p_vol = di->volt_th_chg;
	else
		p_vol = di->volt_th_dischg;

	for (i = 0; i < BAT_1S2P_COUNT; i++) {
		if (is_charge)
			p_weight = di->bat_info[i].weight_chg;
		else
			p_weight = di->bat_info[i].weight_dischg;

		for (j = 0; j < VOLT_THRESHOLD_COUNT; j++) {
			if (di->bat_info[i].volt <= p_vol[j])
				break;
		}

		di->bat_info[i].weight_now = p_weight[j];

		ratio_soc = di->bat_info[i].soc *
			(int)di->bat_info[i].cap_ratio *
			(int)di->bat_info[i].weight_now;
		ratio_soc /= ((CAP_RATIO_MAX * WEIGHT_FACTOR_DIV) / SOC_PRECISION);
		weight_soc += ratio_soc;
		hwlog_info("%s sub%d %d soc%d r_soc %d ratio %d weight %d\n",
			__func__, i, ratio_soc, weight_soc,
			di->bat_info[i].soc, di->bat_info[i].cap_ratio,
			di->bat_info[i].weight_now);
	}

	weight_soc = DIV_ROUND_CLOSEST(weight_soc, SOC_PRECISION);
	hwlog_info("%s raw mixed_soc %d\n", __func__, weight_soc);
	min_soc = min(di->bat_info[BAT_MAIN].soc, di->bat_info[BAT_AUX].soc);
	max_soc = max(di->bat_info[BAT_MAIN].soc, di->bat_info[BAT_AUX].soc);
	weight_soc = max(weight_soc, min_soc);
	weight_soc = min(weight_soc, max_soc);
	hwlog_info("%s mixed_soc %d, min_max %d %d\n",
		__func__, weight_soc, min_soc, max_soc);
	return weight_soc;
}

static void bat_1s2p_update_bat_info(struct bat_1s2p_device *di, int id)
{
	int index;

	if (id == COUL_TYPE_MAIN)
		index = BAT_MAIN;
	else if (id == COUL_TYPE_AUX)
		index = BAT_AUX;
	else
		return;

	di->bat_info[index].is_ready = coul_interface_is_coul_ready(id);
	di->bat_info[index].is_exist = coul_interface_is_battery_exist(id);
	di->bat_info[index].volt = coul_interface_get_battery_voltage(id);
	if (di->bat_info[index].is_exist) {
		di->bat_info[index].curr =
			coul_interface_get_battery_current(id);
		di->bat_info[index].soc =
			coul_interface_get_battery_capacity(id);
	} else {
		hwlog_info("%s id %d not exist\n", __func__, index);
		di->bat_info[index].curr = 0;
		di->bat_info[index].soc = 0;
	}
}

static int bat_1s2p_soc_smooth(struct bat_1s2p_device *di, int soc)
{
	int out = di->soc;
	int delta = out - soc;

	/* 1: soc change step is 1 */
	if (abs(delta) > 1)
		delta = delta > 0 ? 1 : -1;
	out -= delta;

	return out;
}

static void bat_1s2p_detect_fault_send_dmd(struct bat_1s2p_device *di)
{
	char tmp_buf[BAT_1S2P_DSM_BUF_SIZE] = { 0 };

	if ((!di->bat_info[BAT_MAIN].is_exist) || (!di->bat_info[BAT_AUX].is_exist)) {
		snprintf(tmp_buf, BAT_1S2P_DSM_BUF_SIZE, "bat not exist,cap_main is %d, aux is %d\n",
			di->bat_info[BAT_MAIN].is_exist, di->bat_info[BAT_AUX].is_exist);
		power_dsm_report_dmd(POWER_DSM_BATTERY_DETECT,
			DSM_DUAL_BATTERY_OUT_OF_POSITION_DETECTION, tmp_buf);
		return;
	}

	if (abs(di->bat_info[BAT_MAIN].soc - di->bat_info[BAT_AUX].soc) > di->cap_diff_thr) {
		snprintf(tmp_buf, BAT_1S2P_DSM_BUF_SIZE,
			"capacity diff out range, cur vol cap info:main-%d,%d,%d,aux-%d,%d,%d cap_th:%d\n",
			di->bat_info[BAT_MAIN].curr, di->bat_info[BAT_MAIN].volt, di->bat_info[BAT_MAIN].soc,
			di->bat_info[BAT_AUX].curr, di->bat_info[BAT_AUX].volt, di->bat_info[BAT_AUX].soc,
			di->cap_diff_thr);
		power_dsm_report_dmd(POWER_DSM_BATTERY_DETECT,
			DSM_DUAL_BATTERY_CAPACITY_DIFFERENT_DETECT, tmp_buf);
	}
}

static void bat_1s2p_update(struct bat_1s2p_device *di)
{
	int soc;
	bool is_waiting = bat_1s2p_waiting_ready(di);

	bat_1s2p_update_bat_info(di, COUL_TYPE_MAIN);
	bat_1s2p_update_bat_info(di, COUL_TYPE_AUX);
	soc = bat_1s2p_info_mixing(di);
	if (is_waiting) {
		di->soc = soc;
	} else {
		di->soc = bat_1s2p_soc_smooth(di, soc);
		bat_1s2p_detect_fault_send_dmd(di);
	}
}

static void bat_1s2p_monitor_work(struct work_struct *work)
{
	struct bat_1s2p_device *di = container_of(work,
		struct bat_1s2p_device, monitor.work);

	__pm_wakeup_event(di->wakelock, jiffies_to_msecs(HZ));
	mutex_lock(&di->lock);
	bat_1s2p_update(di);
	bat_1s2p_select_work_interval(di,
		min(di->bat_info[BAT_MAIN].soc, di->bat_info[BAT_AUX].soc));
	mutex_unlock(&di->lock);
	power_wakeup_unlock(di->wakelock, false);

	hwlog_info("%s soc %d, interval %d\n",
		__func__, di->soc, di->interval);
	queue_delayed_work(system_power_efficient_wq,
		&di->monitor, msecs_to_jiffies(di->interval));
}

static int bat_1s2p_validate_param(struct bat_1s2p_device *di)
{
	struct battery_info *bat_info = NULL;
	int i, j;
	int cap_ratio_total = 0;

	for (i = 0; i < BAT_1S2P_COUNT; i++) {
		bat_info = di->bat_info + i;

		/* Check capacity ratio value */
		if (bat_info->cap_ratio > CAP_RATIO_MAX)
			return -EINVAL;

		cap_ratio_total += bat_info->cap_ratio;

		/* Check each weight factor value */
		for (j = 0; j < WEIGHT_FACTOR_COUNT; j++) {
			if (bat_info->weight_dischg[j] > WEIGHT_FACTOR_MAX)
				return -EINVAL;
			if (bat_info->weight_chg[j] > WEIGHT_FACTOR_MAX)
				return -EINVAL;
		}
	}

	if (cap_ratio_total > CAP_RATIO_MAX)
		return -EINVAL;

	return 0;
}

static int bat_1s2p_parse_dts(struct bat_1s2p_device *di)
{
	struct device_node *np = di->dev->of_node;
	char prop_name[BAT_1S2P_PROP_NAME];
	struct battery_info *bat_info = NULL;
	int i, ret;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"vol_type", &di->vol_type, COUL_TYPE_MAIN);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"cycle_type", &di->cycle_type, COUL_TYPE_1S2P);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"last_cap_type", &di->last_cap_type, COUL_TYPE_MAIN);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"temp_type", &di->temp_type, COUL_TYPE_MAIN);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"cap_diff_thr", &di->cap_diff_thr, BAT_1S2P_CAP_DIFF_THR);

	ret = power_dts_read_u32_array(power_dts_tag(HWLOG_TAG),
		np, "volt_th_dischg",
		di->volt_th_dischg, VOLT_THRESHOLD_COUNT);
	if (ret)
		return ret;

	ret = power_dts_read_u32_array(power_dts_tag(HWLOG_TAG),
		np, "volt_th_chg",
		di->volt_th_chg, VOLT_THRESHOLD_COUNT);
	if (ret)
		return ret;

	for (i = 0; i < BAT_1S2P_COUNT; i++) {
		bat_info = di->bat_info + i;

		/* initial weight */
		bat_info->weight_now = WEIGHT_FACTOR_DIV;

		snprintf(prop_name, BAT_1S2P_PROP_NAME, "cap_ratio_%d", i);
		/* 2: for dual battery ratio */
		ret = power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
			prop_name, &bat_info->cap_ratio, CAP_RATIO_MAX / 2);
		if (ret)
			return ret;

		snprintf(prop_name, BAT_1S2P_PROP_NAME,
			"weight_factor_chg_%d", i);
		ret = power_dts_read_u32_array(power_dts_tag(HWLOG_TAG),
			np, prop_name, bat_info->weight_chg,
			WEIGHT_FACTOR_COUNT);
		if (ret)
			return ret;

		snprintf(prop_name, BAT_1S2P_PROP_NAME,
			"weight_factor_dischg_%d", i);
		ret = power_dts_read_u32_array(power_dts_tag(HWLOG_TAG),
			np, prop_name, bat_info->weight_dischg,
			WEIGHT_FACTOR_COUNT);
		if (ret)
			return ret;

		(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
			"volt_low", &bat_info->volt_low, VOLT_LOW_THRESHOLD);
	}

	return bat_1s2p_validate_param(di);
}

static int bat_1s2p_probe(struct platform_device *pdev)
{
	struct bat_1s2p_device *di = NULL;
	int ret;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &pdev->dev;

	ret = bat_1s2p_parse_dts(di);
	if (ret) {
		hwlog_err("parse dts err\n");
		return ret;
	}

	di->wakelock = power_wakeup_source_register(di->dev, "bat_1s2p");
	if (!di->wakelock)
		return -ENODEV;
	mutex_init(&di->lock);
	INIT_DELAYED_WORK(&di->monitor, bat_1s2p_monitor_work);
	g_bat_1s2p_dev = di;
	platform_set_drvdata(pdev, di);
	bat_1s2p_init_info(di);
	bat_1s2p_select_work_interval(di,
		min(di->bat_info[BAT_MAIN].soc, di->bat_info[BAT_AUX].soc));
	queue_delayed_work(system_power_efficient_wq,
		&di->monitor, msecs_to_jiffies(di->interval));
	coul_interface_ops_register(&g_1s2p_ops);

	return 0;
}

static int bat_1s2p_remove(struct platform_device *pdev)
{
	struct bat_1s2p_device *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;
	mutex_destroy(&di->lock);
	power_wakeup_source_unregister(di->wakelock);
	g_bat_1s2p_dev = NULL;
	return 0;
}

static int bat_1s2p_resume(struct platform_device *pdev)
{
	struct bat_1s2p_device *di = platform_get_drvdata(pdev);

	if (!di)
		return 0;

	queue_delayed_work(system_power_efficient_wq, &di->monitor, 0);
	return 0;
}

static int bat_1s2p_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct bat_1s2p_device *di = platform_get_drvdata(pdev);

	if (!di)
		return 0;

	cancel_delayed_work(&di->monitor);
	return 0;
}

static const struct of_device_id bat_1s2p_match_table[] = {
	{
		.compatible = "huawei,battery_1s2p",
		.data = NULL,
	},
	{},
};

static struct platform_driver bat_1s2p_driver = {
	.probe = bat_1s2p_probe,
	.remove = bat_1s2p_remove,
	.resume = bat_1s2p_resume,
	.suspend = bat_1s2p_suspend,
	.driver = {
		.name = "huawei,battery_1s2p",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(bat_1s2p_match_table),
	},
};

static int __init bat_1s2p_init(void)
{
	return platform_driver_register(&bat_1s2p_driver);
}

static void __exit bat_1s2p_exit(void)
{
	platform_driver_unregister(&bat_1s2p_driver);
}

rootfs_initcall(bat_1s2p_init);
module_exit(bat_1s2p_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("parallel battery driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
