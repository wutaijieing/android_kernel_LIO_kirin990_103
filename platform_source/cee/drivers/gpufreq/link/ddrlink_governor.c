/*
 * ddrlink_governor.c
 *
 * governor for gpu link ddr freq
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

#include <linux/of.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/platform_drivers/perf_mode.h>
#include "gpuflink.h"

struct ddrlink_profile {
	u64 *gpufreq;
	u32 (*ddrfreq)[MAX_PERF_MODE];
	int freq_pair_num;
};

static struct ddrlink_profile g_prof;

static int parse_gpu_prof(struct device_node *prof_node, int freq_num)
{
	unsigned int i = 0;
	struct device_node *child = NULL;
	int ret;

	g_prof.gpufreq = kzalloc(freq_num * sizeof(unsigned long), GFP_KERNEL);
	if (!g_prof.gpufreq)
		return -ENOMEM;

	for_each_available_child_of_node(prof_node, child) {
		ret = of_property_read_u64(child, "opp-hz", &g_prof.gpufreq[i]);
		if (ret) {
			of_node_put(child);
			kfree(g_prof.gpufreq);
			g_prof.gpufreq = NULL;
			return -ENOENT;
		}
		of_node_put(child);
		i++;
	}

	return 0;
}

static int parse_ddr_prof(struct device_node *prof_node, int freq_num, const char *prop_name)
{
	unsigned int i = 0;
	struct device_node *child = NULL;
	int ret;

	g_prof.ddrfreq = kzalloc(freq_num * MAX_PERF_MODE * sizeof(unsigned int), GFP_KERNEL);
	if (!g_prof.ddrfreq)
		return -ENOMEM;

	for_each_available_child_of_node(prof_node, child) {
		ret = of_property_read_u32_array(child, prop_name, (u32 *)&g_prof.ddrfreq[i],
						 ARRAY_SIZE(g_prof.ddrfreq[0]));
		if (ret) {
			of_node_put(child);
			kfree(g_prof.ddrfreq);
			g_prof.ddrfreq = NULL;
			return -ENOENT;
		}
		of_node_put(child);
		i++;
	}
	return 0;
}

static int ddrlink_gov_init(void)
{
	struct device_node *gov_node = NULL;
	struct device_node *opp = NULL;
	int ret = 0;
	int freq_pair_num;
	unsigned int index;
	const char *prop_name = NULL;

	gov_node = of_find_compatible_node(NULL, NULL, "gpufreq,ddrlink_governor");
	if (!gov_node)
		return -ENOMEM;

	opp = of_parse_phandle(gov_node, "profile", 0);
	if (!opp) {
		of_node_put(gov_node);
		return -ENOENT;
	}

	freq_pair_num = of_get_available_child_count(opp);
	if (freq_pair_num <= 0) {
		ret = -EPERM;
		goto node_put;
	}

	ret = parse_gpu_prof(opp, freq_pair_num);
	if (ret)
		goto node_put;

	ret = of_property_read_u32(gov_node, "ddr-type", &index);
	if (ret)
		goto gpu_prof_free;

	ret = of_property_read_string_index(gov_node, "prop-name", index, &prop_name);
	if (ret)
		goto gpu_prof_free;

	ret = parse_ddr_prof(opp, freq_pair_num, prop_name);
	if (ret)
		goto gpu_prof_free;

	g_prof.freq_pair_num = freq_pair_num;

	of_node_put(opp);
	of_node_put(gov_node);

	return 0;

gpu_prof_free:
	kfree(g_prof.gpufreq);
	g_prof.gpufreq = NULL;
node_put:
	of_node_put(opp);
	of_node_put(gov_node);

	return ret;
}

static int find_gpufreq_ceil(unsigned long gpufreq)
{
	int i;

	for (i = 0; i < g_prof.freq_pair_num - 1; i++) {
		if (g_prof.gpufreq[i] >= gpufreq)
			break;
	}

	return i;
}

static int ddrlink_gov_get_target(unsigned long gpufreq, int mode)
{
	int index;

	index = find_gpufreq_ceil(gpufreq);
	return g_prof.ddrfreq[index][mode];
}

static int ddrlink_gov_exit(void)
{
	g_prof.freq_pair_num = 0;
	kfree(g_prof.gpufreq);
	g_prof.gpufreq = NULL;
	kfree(g_prof.ddrfreq);
	g_prof.ddrfreq = NULL;

	return 0;
}

struct gpuflink_governor g_ddrlink_governor = {
	.name = "ddr_link_governor",
	.gov_init = ddrlink_gov_init,
	.get_target = ddrlink_gov_get_target,
	.gov_exit = ddrlink_gov_exit,
};

static int __init ddrlink_governor_init(void)
{
	return gpuflink_add_governor(&g_ddrlink_governor);
}

late_initcall(ddrlink_governor_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("gpu freq link ddr governor");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");