/*
 * ddrlink_qos_driver.c
 *
 * gpu freq link ddr freq driver
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

#include <linux/platform_drivers/platform_qos.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/limits.h>
#include <linux/printk.h>
#include "gpuflink.h"

static struct platform_qos_request g_qos_req_ddr;
static int g_rate;

static void rate_init(void)
{
	struct device_node *np = NULL;
	int ret;
	unsigned int rate;
	unsigned int utilization;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,ddr_devfreq_latency");
	if (np == NULL)
		return;

	ret = of_property_read_u32_index(np, "pm_qos_data_reg", 0, &rate);
	if (ret != 0) {
		pr_err("%s read ddr transformer rate failed %d\n", __func__, ret);
		of_node_put(np);
		return;
	}

	ret = of_property_read_u32_index(np, "pm_qos_data_reg", 1, &utilization);
	if (ret != 0) {
		pr_err("%s read ddr transformer utilization failed %d\n", __func__, ret);
		of_node_put(np);
		return;
	}

	of_node_put(np);

	if (rate == 0) {
		g_rate = 0;
		return;
	}

	if (utilization < INT_MAX / rate)
		g_rate = (int)(utilization * rate);
	else
		g_rate = INT_MAX;
}

static void ddrqos_update_request(unsigned int freq)
{
	int bandwidth;

	if (!platform_qos_request_active(&g_qos_req_ddr) || g_rate == 0)
		return;

	if (freq < INT_MAX / g_rate)
		bandwidth = (int)(freq * g_rate / 100);
	else
		bandwidth = INT_MAX;

	platform_qos_update_request(&g_qos_req_ddr, bandwidth);
}

struct gpuflink_driver g_ddrlink_driver = {
	.name = "ddrlink_qos_driver",
	.set_target = ddrqos_update_request,
	.governor_name = "ddr_link_governor",
};

static int __init ddrlink_driver_init(void)
{
	rate_init();
	platform_qos_add_request(&g_qos_req_ddr, PLATFORM_QOS_MEMORY_LATENCY, 0);
	return gpuflink_add_driver(&g_ddrlink_driver);
}

late_initcall(ddrlink_driver_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("gpufreq link ddrfreq driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");