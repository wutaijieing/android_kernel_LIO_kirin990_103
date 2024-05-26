/*
 * media_monitor_k.c
 *
 * media monitor module
 *
 * Copyright (C) 2019-2020 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <media_monitor_k.h>
#include <linux/regulator/consumer.h>
#include <loadmonitor_common.h>
#include <platform_include/see/bl31_smc.h>

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt)		"[loadmonitor&freqdump]:" fmt

static struct regulator *g_media1_dpm_regulator;
static struct regulator *g_media2_dpm_regulator;
static struct semaphore g_media_monitor_sem;

int media0_monitor_read(unsigned int dpm_period_media)
{
	int ret;

	down(&g_media_monitor_sem);
	ret = regulator_is_enabled(g_media1_dpm_regulator);
	if (ret == 0) { /* media is offline no power up */
		pr_info("%s:is power off,is allowed\n", __func__);
	} else {
		/* vote to media pwrup */
		ret = regulator_enable(g_media1_dpm_regulator);
		if (ret != 0) {
			pr_err("%s:enable error,ret:%d.\n", __func__, ret);
			up(&g_media_monitor_sem);
			return ret;
		}

		/* enable media monitor */
		(void)atfd_service_freqdump_smc(
			(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE,
			(u64)g_monitor_delay_value.media0,
			(u64)0,
			(u64)ENABLE_MEDIA0);
		sec_loadmonitor_data_read(ENABLE_MEDIA0);
		pr_info("%s read success\n", __func__);

		/* return 0 -> disable success */
		ret = regulator_disable(g_media1_dpm_regulator);
		if (ret != 0) {
			pr_err("%s:disable error,ret:%d.\n", __func__, ret);
			up(&g_media_monitor_sem);
			return ret;
		}
	}
	up(&g_media_monitor_sem);
	return ret;
}

int media1_monitor_read(unsigned int dpm_period_media)
{
	int ret;

	down(&g_media_monitor_sem);
	/* return 1 -> is enabled */
	ret = regulator_is_enabled(g_media2_dpm_regulator);
	if (ret == 0) {
		/* media is offline no power up */
		pr_info("%s:is power off,is allowed,ret:%d\n", __func__, ret);
	} else {
		/*
		 * vote to media power up
		 * return 0 -> enable success
		 */
		ret = regulator_enable(g_media2_dpm_regulator);
		if (ret != 0) {
			pr_err("%s:enable error,ret:%d.\n", __func__, ret);
			up(&g_media_monitor_sem);
			return ret;
		}

		/* enable media monitor */
		(void)atfd_service_freqdump_smc(
			(u64)FREQDUMP_LOADMONITOR_SVC_ENABLE,
			(u64)g_monitor_delay_value.media1,
			(u64)0,
			(u64)ENABLE_MEDIA1);
		sec_loadmonitor_data_read(ENABLE_MEDIA1);
		pr_info("%s: success\n", __func__);

		ret = regulator_disable(g_media2_dpm_regulator);
		if (ret) {
			pr_err("%s:disable error,ret:%d.\n", __func__, ret);
			up(&g_media_monitor_sem);
			return ret;
		}
	}
	up(&g_media_monitor_sem);
	return ret;
}

int media_monitor_read(unsigned int dpm_period_media)
{
	int ret_media0, ret_media1;

	ret_media0 = media0_monitor_read(dpm_period_media);
	ret_media1 = media1_monitor_read(dpm_period_media);
	if (ret_media0 != 0 || ret_media1 != 0)
		return -EINVAL;
	else
		return 0;
}

static int __init media_dpm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	g_media1_dpm_regulator = devm_regulator_get(dev, "media1-dpm");
	if (IS_ERR(g_media1_dpm_regulator)) {
		pr_err("lm dpm get media1 regulator error.\n");
		return -ENODEV;
	}

	g_media2_dpm_regulator = devm_regulator_get(dev, "media2-dpm");
	if (IS_ERR(g_media2_dpm_regulator)) {
		pr_err("lm dpm get media2 regulator error.\n");
		devm_regulator_put(g_media1_dpm_regulator);
		return -ENODEV;
	}
	sema_init(&g_media_monitor_sem, 1);

	return 0;
}

static int media_dpm_remove(struct platform_device *pdev)
{
	devm_regulator_put(g_media1_dpm_regulator);
	devm_regulator_put(g_media2_dpm_regulator);

	return 0;
}

static const struct of_device_id media_dpm_of_match[] = {
	{ .compatible = "dpm-params" },
	{},
};

MODULE_DEVICE_TABLE(of, media_dpm_of_match);

struct platform_driver media_dpm_driver = {
	.driver = {
		.name = "media-monitor-dpm",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(media_dpm_of_match),
	},
	.probe = media_dpm_probe,
	.remove = media_dpm_remove,
};
