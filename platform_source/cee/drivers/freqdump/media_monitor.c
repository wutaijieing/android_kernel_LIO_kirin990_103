/*
 * media_monitor.c
 *
 * media_monitor
 *
 * Copyright (C) 2017-2020 Huawei Technologies Co., Ltd.
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

#include <asm/compiler.h>
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
#include "media_monitor.h"
#include <linux/regulator/consumer.h>
#include "freqdump_kernel.h"
#include <platform_include/see/bl31_smc.h>

struct regulator *g_media1_dpm_regulator, *g_media2_dpm_regulator;
u32 g_media_info[INFO_MAX];
static struct semaphore g_media_monitor_sem;

static void hisi_sec_media_monitor_enable(unsigned int delay_value)
{
	(void)atfd_service_freqdump_smc((u64)MEDIA_MONITOR_SVC_ENABLE,
					     (u64)delay_value,
					     (u64)0, (u64)0);
}

/*
 * flag = 0: disable read g_media_info
 * flag = 1: enable read g_media_info
 */
static void hisi_sec_set_media_flag(int32_t flag)
{
	(void)atfd_service_freqdump_smc((u64)MEDIA_MONITOR_SVC_SET_FLAG,
					     (u64)(s64)flag, (u64)0, (u64)0);
}

int media_monitor_read(unsigned int dpm_period_media)
{
	int ret;

	down(&g_media_monitor_sem);

	ret = regulator_is_enabled(g_media1_dpm_regulator);
	/* return 1 -> is enabled */
	ret = (int)((unsigned int)ret |
		    (unsigned int)regulator_is_enabled(g_media2_dpm_regulator));
	if (ret == 0) { /* media is offline */
		hisi_sec_set_media_flag(NO_READ);
		sec_loadmonitor_data_read(PERI_MONITOR_EN);
	} else {
		/*
		 * vote to media pwrup
		 * return 0 -> enable success
		 */
		ret = regulator_enable(g_media1_dpm_regulator);
		ret = (int)((unsigned int)ret |
			    (unsigned int)regulator_enable(g_media2_dpm_regulator));
		if (ret != 0) {
			hisi_sec_set_media_flag(NO_READ);
			pr_err("hisi_dpm : media_dpm_regulator enable error.\n");
			up(&g_media_monitor_sem);
			return ret;
		}
		hisi_sec_set_media_flag(CAN_READ);

		/* enable media monitor */
		hisi_sec_media_monitor_enable(dpm_period_media);
		sec_loadmonitor_data_read(ALL_MONITOR_EN);
		/* return 0 -> disable success */
		ret = regulator_disable(g_media1_dpm_regulator);
		ret = (int)((unsigned int)ret |
			    (unsigned int)regulator_disable(g_media2_dpm_regulator));
		if (ret != 0) {
			hisi_sec_set_media_flag(NO_READ);
			pr_err("hisi_dpm : media_dpm_regulator disable error.\n");
			up(&g_media_monitor_sem);
			return ret;
		}
	}
	up(&g_media_monitor_sem);
	return ret;
}

#ifdef CONFIG_LIBLINUX
int media_dpm_probe(struct platform_device *pdev)
#else
int __init media_dpm_probe(struct platform_device *pdev)
#endif
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;

	ret = of_property_read_u32_array(np, "media-info",
					 &g_media_info[0],
					 (size_t)MEDIA_INFO_LEN);
	if (ret != 0) {
		pr_err("lm dpm get media-info node error.\n");
		return -ENODEV;
	}

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
	(void)pdev;

	devm_regulator_put(g_media1_dpm_regulator);
	devm_regulator_put(g_media2_dpm_regulator);

	return 0;
}

static const struct of_device_id media_dpm_of_match[] = {
	{.compatible = "dpm-params"},
	{},
};

MODULE_DEVICE_TABLE(of, media_dpm_of_match);

struct platform_driver g_media_dpm_driver = {
	.driver = {
		.name = "media-monitor-dpm",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(media_dpm_of_match),
	},
	.probe = media_dpm_probe,
	.remove = media_dpm_remove,
};
