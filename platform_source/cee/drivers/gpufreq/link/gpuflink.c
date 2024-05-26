
/*
 * gpuflink.c
 *
 * gpu freq link core
 *
 * Copyright (C) 2021-2021 Huawei Technologies Co., Ltd.
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


#include <linux/list.h>
#include <linux/devfreq.h>
#include <linux/platform_drivers/perf_mode.h>
#include <linux/mutex.h>
#include "gpuflink.h"
#include <linux/platform_drivers/gpufreq.h>

static LIST_HEAD(g_governor_list);
static LIST_HEAD(g_driver_list);
static DEFINE_MUTEX(g_list_lock);
static struct devfreq *g_devfreq;
static int g_mode;

static struct gpuflink_governor *find_governor(const char *name)
{
	struct gpuflink_governor *gov = NULL;

	if (!name)
		return NULL;

	list_for_each_entry(gov, &g_governor_list, node) {
		if (!strcmp(gov->name, name))
			return gov;
	}
	return NULL;
}

static struct gpuflink_driver *find_driver(const char *name)
{
	struct gpuflink_driver *drv = NULL;

	if (!name)
		return NULL;

	list_for_each_entry(drv, &g_driver_list, node) {
		if (!strcmp(drv->name, name))
			return drv;
	}
	return NULL;
}

int gpuflink_add_governor(struct gpuflink_governor *governor)
{
	struct gpuflink_governor *gov = NULL;
	struct gpuflink_driver *drv = NULL;
	int ret = 0;

	if (!governor)
		return -EINVAL;

	mutex_lock(&g_list_lock);
	gov = find_governor(governor->name);
	if (gov) {
		ret = -EEXIST;
		goto unlock;
	}

	list_add(&governor->node, &g_governor_list);

	list_for_each_entry(drv, &g_driver_list, node) {
		if (!strcmp(drv->governor_name, governor->name)) {
			ret = governor->gov_init();
			if (ret) {
				pr_err("[%s], %s governor init failed\n", __func__, governor->name);
				goto unlock;
			}
			drv->governor = governor;
		}
	}
unlock:
	mutex_unlock(&g_list_lock);
	return ret;
}

int gpuflink_remove_governor(struct gpuflink_governor *governor)
{
	struct gpuflink_governor *gov = NULL;
	struct gpuflink_driver *drv = NULL;
	int ret = 0;

	if (!governor)
		return -EINVAL;

	mutex_lock(&g_list_lock);
	gov = find_governor(governor->name);
	if (!gov) {
		ret = -ENOENT;
		goto unlock;
	}

	list_for_each_entry(drv, &g_driver_list, node) {
		if (!strcmp(drv->governor_name, governor->name)) {
			if (!drv->governor)
				continue;

			ret = drv->governor->gov_exit();
			if (ret) {
				pr_err("[%s], %s gov remove failed\n", __func__, governor->name);
				goto unlock;
			}
			drv->governor = NULL;
		}
	}

	list_del(&governor->node);
unlock:
	mutex_unlock(&g_list_lock);
	return ret;
}

int gpuflink_add_driver(struct gpuflink_driver *driver)
{
	struct gpuflink_driver *drv = NULL;
	struct gpuflink_governor *gov = NULL;
	int ret = 0;

	if (!driver)
		return -EINVAL;

	mutex_lock(&g_list_lock);
	drv = find_driver(driver->name);
	if (drv) {
		ret = -EEXIST;
		goto unlock;
	}

	list_add(&driver->node, &g_driver_list);

	gov = find_governor(driver->governor_name);
	if (gov){
		ret = gov->gov_init();
		if (ret) {
			pr_err("[%s], %s gov init failed\n", __func__, driver->governor_name);
			goto unlock;
		}
		driver->governor = gov;
	}

unlock:
	mutex_unlock(&g_list_lock);
	return ret;
}

int gpuflink_remove_driver(struct gpuflink_driver *driver)
{
	struct gpuflink_driver *drv = NULL;
	int ret = 0;

	if (!driver)
		return -EINVAL;

	mutex_lock(&g_list_lock);
	drv = find_driver(driver->name);
	if (!drv) {
		ret = -ENOENT;
		goto unlock;
	}

	list_del(&driver->node);
unlock:
	mutex_unlock(&g_list_lock);
	return ret;
}

/* Hz, invoke by devfreq callback func */
void gpuflink_notifiy(unsigned long gpufreq)
{
	struct gpuflink_driver *drv = NULL;
	int target;

	mutex_lock(&g_list_lock);
	list_for_each_entry(drv, &g_driver_list, node) {
		if (!drv->governor)
			continue;
		target = drv->governor->get_target(gpufreq, g_mode);
		drv->set_target(target);
	}
	mutex_unlock(&g_list_lock);
}


static int set_gpuflink_mode(int mode)
{
	if (mode >= MAX_PERF_MODE || mode < PERF_MODE_0)
		return -EINVAL;

	g_mode = mode;

	gpuflink_notifiy(g_devfreq->previous_freq);

	return 0;
}

static int gpuflink_mode_notifier_call(struct notifier_block *nb, unsigned long val, void *v)
{
	int ret;

	ret = set_gpuflink_mode((int)val);
	if (ret != 0)
		return NOTIFY_BAD;

	return NOTIFY_OK;
}

static struct notifier_block gpufreq_freq_link_notifier = {
	.notifier_call = gpuflink_mode_notifier_call,
};

int gpuflink_init(struct devfreq *devfreq)
{
	if (!devfreq)
		return -EINVAL;

	g_mode = PERF_MODE_2;
	g_devfreq = devfreq;

	return perf_mode_register_notifier(&gpufreq_freq_link_notifier);
}

void gpuflink_suspend(struct devfreq *devfreq)
{
	struct gpuflink_driver *drv = NULL;

	if (!devfreq)
		return;

	mutex_lock(&g_list_lock);
	list_for_each_entry(drv, &g_driver_list, node) {
		if (drv->governor)
			drv->set_target(0);
	}
	mutex_unlock(&g_list_lock);
}

void gpuflink_resume(struct devfreq *devfreq)
{
	struct gpuflink_driver *drv = NULL;
	int target;
	unsigned long gpufreq;

	if (!devfreq)
		return;

	mutex_lock(&g_list_lock);
	gpufreq = devfreq->previous_freq;
	list_for_each_entry(drv, &g_driver_list, node) {
		if (!drv->governor)
			continue;
		target = drv->governor->get_target(gpufreq, g_mode);
		drv->set_target(target);
	}
	mutex_unlock(&g_list_lock);
}
