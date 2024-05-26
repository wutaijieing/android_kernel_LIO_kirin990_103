/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Hisi fbex for "preloading" source file
 * Create : 2020/06/05
 */

#include "fbex_driver.h"

#include <linux/of.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <platform_include/see/fbe_ctrl.h>

#define MODULE_NAME  "hisi-fbex"
#define _unused      __attribute__((unused))

spinlock_t g_fbex_preaload_lock;
static bool g_msp_loading = false; /* working in two work, using spinlock */
static bool g_preload_start = false; /* working in two work, using spinlock */

bool hisi_fbex_is_preload(void)
{
	bool flag = false;

	spin_lock(&g_fbex_preaload_lock);
	flag = g_preload_start;
	spin_unlock(&g_fbex_preaload_lock);
	return flag;
}

void hisi_fbex_preloading_msp(bool flag)
{
	spin_lock(&g_fbex_preaload_lock);
	g_msp_loading = flag;
	spin_unlock(&g_fbex_preaload_lock);
}

void hisi_fbex_set_preload(bool flag)
{
	spin_lock(&g_fbex_preaload_lock);
	g_preload_start = flag;
	spin_unlock(&g_fbex_preaload_lock);
}

static int hisi_fbex_pm_prepare(struct device *pdev _unused)
{
	int ret = 0;

	/* if preload is working, break the suspend */
	spin_lock(&g_fbex_preaload_lock);
	if (unlikely(g_msp_loading)) {
		pr_err("%s, fbex is working\n", __func__);
		ret = -1;
	}
	g_preload_start = false;
	spin_unlock(&g_fbex_preaload_lock);

	return ret;
}

static const struct of_device_id g_fbex_of_table[] = {
	{ .compatible = "hisi,hisi-fbex" },
	{ },
};

/* Only register fbex_pm_prepare */
static const struct dev_pm_ops g_fbex_pm_ops = {
	.prepare = hisi_fbex_pm_prepare,
};

static struct platform_driver g_hisi_fbex_driver = {
	.driver = {
		.name           = MODULE_NAME,
		.owner          = THIS_MODULE,
		.pm             = &g_fbex_pm_ops,
		.of_match_table = of_match_ptr(g_fbex_of_table),
	},
};

void __exit hisi_fbex_preload_exit(void)
{
	platform_driver_unregister(&g_hisi_fbex_driver);
}

int __init hisi_fbex_preload_init(void)
{
	int ret;

	spin_lock_init(&g_fbex_preaload_lock);
	ret = platform_driver_register(&g_hisi_fbex_driver);
	if (ret)
		pr_err("%s: register hisi fbex preload fail\n", MODULE_NAME);
	return ret;
}

module_init(hisi_fbex_preload_init);
module_exit(hisi_fbex_preload_exit);

MODULE_DESCRIPTION("hisi fbex preload");
MODULE_ALIAS("hisi fbex preload");
MODULE_LICENSE("GPL");
