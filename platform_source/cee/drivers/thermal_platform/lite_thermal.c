/* lite_thermal.c
 *
 * thermal lite framework
 *
 * Copyright (C) 2017-2021 Huawei Technologies Co., Ltd.
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

#include "ipa_thermal.h"
#include <linux/cpu_cooling.h>
#include <linux/thermal.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/sched/task.h>
#include <uapi/linux/sched/types.h>
#include <securec.h>
#include <linux/of_platform.h>
#include <linux/lpm_thermal.h>

#ifdef CONFIG_THERMAL_SUPPORT_LITE
bool lite_check_cdev_exist(char *type)
{
	int oncpucluster[MAX_THERMAL_CLUSTER_NUM] = {0};
	int cpuclustercnt, gpuclustercnt, npuclustercnt;

	get_clustermask_cnt(oncpucluster, MAX_THERMAL_CLUSTER_NUM, &cpuclustercnt,
			    &gpuclustercnt, &npuclustercnt);

	if (!strncmp(type, "thermal-devfreq-0", 17) && gpuclustercnt == 0)
		return false;
	else if (!strncmp(type, "npu_pm", 6) && npuclustercnt == 0)
		return false;
	else if (!strncmp(type, "thermal-cpufreq-0", 17) && (cpuclustercnt < 1))
		return false;
	else if (!strncmp(type, "thermal-cpufreq-1", 17) && (cpuclustercnt < 2))
		return false;
	else if (!strncmp(type, "thermal-cpufreq-2", 17)  && (cpuclustercnt < 3))
		return false;
	return true;
}

void get_clustermask_cnt(int *oncpucluster, int len, int *cpuclustercnt, int *gpuclustercnt,
			 int *npuclustercnt)
{
	struct device_node *gpunp = NULL;
	struct device_node *npunp = NULL;
	int clustermask[MAX_THERMAL_CLUSTER_NUM] = {0};

	*cpuclustercnt = 0;
	*gpuclustercnt = 0;
	*npuclustercnt = 0;
	ipa_get_clustermask(clustermask, len, oncpucluster, cpuclustercnt);
	gpunp = of_find_compatible_node(NULL, NULL, "arm,mali-midgard");
	if (gpunp)
		*gpuclustercnt = 1;

	npunp = of_find_compatible_node(NULL, NULL, "lpm,npu-pm");
	if (npunp)
		*npuclustercnt = 1;

	return;
}
EXPORT_SYMBOL(get_clustermask_cnt);

void lite_powerhal_proc(int *oncpucluster, int len,
			int cpuclustercnt, int gpuclustercnt)
{
	int i;
	int j = 0;

	(void)len;
	for (i = 0; i < g_powerhal_actor_num; i++) {
		if (i < g_cluster_num && oncpucluster[i] == 1) {
			g_powerhal_profiles[0][j] = g_powerhal_profiles[0][i];
			g_powerhal_profiles[1][j] = g_powerhal_profiles[1][i];
			g_powerhal_profiles[2][j] = g_powerhal_profiles[2][i];
			j++;
		}
	}
	if (gpuclustercnt != 0) {
		g_powerhal_profiles[0][j] = g_powerhal_profiles[0][i - 1];
		g_powerhal_profiles[1][j] = g_powerhal_profiles[1][i - 1];
		g_powerhal_profiles[2][j] = g_powerhal_profiles[2][i - 1];
		j++;
	}
	g_powerhal_actor_num = cpuclustercnt + gpuclustercnt;
	return;
}

void lite_weight_proc(int *oncpucluster, int len,
		      int cpuclustercnt, int gpuclustercnt)
{
	int i;
	int j = 0;

	for (i = 0; i < g_ipa_actor_num; i++) {
		if (i < g_cluster_num && oncpucluster[i] == 1) {
			g_weights_profiles[0][j] = g_weights_profiles[0][i];
			g_weights_profiles[1][j] = g_weights_profiles[1][i];
			j++;
		}
	}

	if (gpuclustercnt != 0) {
		g_weights_profiles[0][j] = g_weights_profiles[0][i - 1];
		g_weights_profiles[1][j] = g_weights_profiles[1][i - 1];
		j++;
	}
	g_ipa_actor_num = cpuclustercnt + gpuclustercnt;
	return;
}
#endif
