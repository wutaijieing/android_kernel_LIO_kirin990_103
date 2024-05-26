/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: npu dvfs
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

#include <linux/version.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/pm_opp.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/thermal.h>
#include <linux/hwspinlock.h>
#include <npu_pm.h>
#include "npu_pm_private.h"
#include "npu_pm_smc.h"

static struct npu_pm_dvfs_data g_dvfs_data = {
	.dvfs_enable  = false,
	.tz_dev       = NULL,
	.tz_num       = 0,
	.test         = 0,
};

#define NPU_PM_BUCK_STABLE_MAX	5000 /* us */
#define NPU_PM_TEMP_NORMAL	40

#define NPU_PM_LOCK_TIMEOUT	300 /* ms */
#define NPU_PM_LOCK_ID		64

static struct hwspinlock *g_npu_pm_hwlock;

static int npu_pm_get_temp(void)
{
#ifdef CONFIG_THERMAL
	int temp = 0;
	int ret;
#endif

#ifdef CONFIG_NPU_PM_DEBUG
	if ((g_dvfs_data.test & BIT(NPU_LOW_TEMP_DEBUG)) > 0)
		return g_dvfs_data.temp_test;
#endif

#ifdef CONFIG_THERMAL
	if (g_dvfs_data.tz_dev == NULL)
		return NPU_PM_TEMP_NORMAL;

	ret = thermal_zone_get_temp(g_dvfs_data.tz_dev, &temp);
	if (ret != 0) {
		pr_err("get npu temp failed%d\n", ret);
		return NPU_PM_TEMP_NORMAL;
	}

	return temp;
#else
	return NPU_PM_TEMP_NORMAL;
#endif
}

static int npu_pm_get_tzone(int temp)
{
	int i;

	if (g_dvfs_data.tzone == NULL || g_dvfs_data.tz_num == 0)
		return 0;

	for (i = 0; i < g_dvfs_data.tz_num; i++) {
		if (temp <= g_dvfs_data.tzone[i])
			break;
	}

	return min_t(int, g_dvfs_data.tz_num - 1, i);
}

int npu_pm_dvfs_hal(unsigned long target_freq)
{
	unsigned int last_freq;
	ktime_t in_ktime;
	unsigned long delta_time;
	unsigned int tz_last;
	int cur_temp;
	int ret;

	if (!g_dvfs_data.dvfs_enable)
		return 0;

	in_ktime = ktime_get();

	target_freq = target_freq / KHZ;

	last_freq = g_dvfs_data.current_freq;
	g_dvfs_data.current_freq = target_freq;

	tz_last = (unsigned int)g_dvfs_data.tz_cur;
	/* called out of hwspin_lock */
	cur_temp = npu_pm_get_temp();
	g_dvfs_data.tz_cur = npu_pm_get_tzone(cur_temp);

	if (last_freq == target_freq &&
	    tz_last == (unsigned int)g_dvfs_data.tz_cur)
		return 0;

	ret = hwspin_lock_timeout(g_npu_pm_hwlock, NPU_PM_LOCK_TIMEOUT);
	if (ret != 0) {
		pr_err("%s, apply npu pm lock fail\n", __func__);
		return -ENODEV;
	}

	ret = npu_pm_dvfs_request_smc_request(target_freq, cur_temp);

	hwspin_unlock(g_npu_pm_hwlock);

	if (ret < 0) {
		pr_err("[npu dvfs] %s dvfs request fail%d\n", __func__, ret);
		return ret;
	}

	delta_time = (unsigned long)ktime_to_ns(ktime_sub(ktime_get(), in_ktime));
	if (delta_time > g_dvfs_data.max_dvfs_time)
		g_dvfs_data.max_dvfs_time = delta_time;

	return 0;
}

int npu_pm_pwr_on(unsigned long target_freq)
{
	int ret;
	int temp;

	target_freq = target_freq / KHZ;
	g_dvfs_data.current_freq = target_freq;
	/* called out of hwspin_lock */
	temp = npu_pm_get_temp();
	g_dvfs_data.tz_cur = npu_pm_get_tzone(temp);

	ret = hwspin_lock_timeout(g_npu_pm_hwlock, NPU_PM_LOCK_TIMEOUT);
	if (ret != 0) {
		pr_err("%s, apply npu pm lock fail\n", __func__);
		ret = -ENODEV;
		goto err_power_on;
	}

	ret = npu_pm_pwron_smc_request(target_freq, temp);

	hwspin_unlock(g_npu_pm_hwlock);

err_power_on:
	return ret;
}

int npu_pm_pwr_off(void)
{
	int ret;
	int temp;

	/* called out of hwspin_lock */
	temp = npu_pm_get_temp();

	ret = hwspin_lock_timeout(g_npu_pm_hwlock, NPU_PM_LOCK_TIMEOUT);
	if (ret != 0) {
		pr_err("%s, apply npu pm lock fail\n", __func__);
		ret = -ENODEV;
		goto err_power_off;
	}

	ret = npu_pm_pwroff_smc_request(temp);

	hwspin_unlock(g_npu_pm_hwlock);

err_power_off:
	return ret;
}

static int npu_pm_freq_init(struct device *dev)
{
	struct device_node *np = dev->of_node;
	unsigned int value = 0;
	int ret;

	if (np == NULL) {
		dev_err(dev, "npu dts not found\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "initial_freq", &value);
	if (ret != 0) {
		dev_err(dev, "parse npu initial frequency fail%d\n", ret);
		return -EINVAL;
	}
	g_dvfs_data.current_freq = value;

	return 0;
}

static void npu_pm_thermal_dev_init(struct device *dev)
{
#ifdef CONFIG_THERMAL
	struct device_node *np = dev->of_node;
	struct thermal_zone_device *tz = NULL;
	const char *tz_name = NULL;
	int ret;

	if (np == NULL) {
		dev_err(dev, "npu dts not found\n");
		return;
	}

	ret = of_property_read_string(np, "thermal_zone_name", &tz_name);
	if (ret != 0) {
		dev_err(dev, "get thermal zone name failed%d\n", ret);
		return;
	}

	tz = thermal_zone_get_zone_by_name(tz_name);
	if (tz == NULL) {
		dev_err(dev, "get thermal zone failed\n");
		return;
	}
	g_dvfs_data.tz_dev = tz;
#endif
}

static void npu_pm_thermal_zone_init(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int *tz = NULL;
	int tz_num;
	int ret;

	if (np == NULL) {
		dev_err(dev, "npu dts not found\n");
		return;
	}

	ret = of_property_count_u32_elems(np, "thermal_zone");
	if (ret <= 0) {
		dev_err(dev, "get thermal zone failed%d\n", ret);
		return;
	}
	tz_num = ret;

	tz = devm_kzalloc(dev, sizeof(int) * tz_num, GFP_KERNEL);
	if (tz == NULL)
		return;

	ret = of_property_read_u32_array(np, "thermal_zone", (u32 *)tz, tz_num);
	if (ret != 0) {
		dev_err(dev, "get thermal zone failed\n");
		return;
	}
	g_dvfs_data.tzone = tz;
	g_dvfs_data.tz_num = tz_num;
}

static int npu_pm_hwspinlock_init(struct device *dev)
{
	g_npu_pm_hwlock = hwspin_lock_request_specific(NPU_PM_LOCK_ID);
	if (g_npu_pm_hwlock == NULL) {
		dev_err(dev, "npu pm get hwspinlock fail\n");
		return -ENOMEM;
	}

	return 0;
}

int npu_pm_dvfs_init(struct npu_pm_device *pmdev)
{
	struct device *dev = pmdev->dev;
	int ret;

	npu_pm_thermal_dev_init(dev);
	npu_pm_thermal_zone_init(dev);

	ret = npu_pm_freq_init(dev);
	if (ret != 0) {
		dev_err(dev, "%s npu profile data parse failed\n", __func__);
		return ret;
	}

	ret = npu_pm_hwspinlock_init(dev);
	if (ret != 0)
		return ret;

	pmdev->dvfs_data = &g_dvfs_data;

	return ret;
}

