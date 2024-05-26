/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "dkmd_log.h"
#include "dkmd_object.h"
#include "dpu_config_utils.h"

struct dpu_config g_dpu_config_data;
EXPORT_SYMBOL(g_dpu_config_data);

static void _config_get_ip_base(struct device_node *np, char __iomem *ip_base_addrs[], uint32_t ip_count)
{
	uint32_t i;

	for (i = 0; i < ip_count; i++) {
		ip_base_addrs[i] = of_iomap(np, i);
		if (!ip_base_addrs[i])
			dpu_pr_err("get ip %u base addr fail", i);

		dpu_pr_debug("ip %u, ip_base_addrs:0x%llx", i, ip_base_addrs[i]);
	}
}

static void _config_get_lbuf_size(struct device_node *np)
{
	int ret;
	uint32_t lbuf_size = 0;

	ret = of_property_read_u32(np, "linebuf_size", &lbuf_size);
	if (ret != 0) {
		dpu_pr_err("failed to get linebuf size.\n");
		return;
	}
	dpu_pr_debug("lbuf_size = %u K", lbuf_size / SIZE_1K);
	g_dpu_config_data.lbuf_size = lbuf_size;
}

static int32_t _config_get_scene_id_array(struct device_node *np, const char *node_name, uint32_t max_len,
	uint32_t *scene_ids, uint32_t len)
{
	uint32_t i;

	if (len < max_len) {
		dpu_pr_err("scene_id array len=%u is smaller than %u", len, max_len);
		return -1;
	}

	for (i = 0; i < max_len; ++i) {
		if (of_property_read_u32_index(np, node_name, i, &scene_ids[i])) {
			dpu_pr_info("read offline_scene_id fail");
			break;
		}
		dpu_pr_info("get offline_scene_id %u", scene_ids[i]);
		++g_dpu_config_data.offline_scene_id_count;
	}
	dpu_pr_info("get offline_scene_id size=%u", g_dpu_config_data.offline_scene_id_count);

	return 0;
}

static int32_t _config_get_offline_scene_id_array(struct device_node *np)
{
	return _config_get_scene_id_array(np, "offline_scene_id", DTS_OFFLINE_SCENE_ID_MAX,
		g_dpu_config_data.offline_scene_ids, ARRAY_SIZE(g_dpu_config_data.offline_scene_ids));
}

static void _config_get_dpu_version_value(struct device_node *np)
{
	int ret;
	uint32_t is_fpga = 0;

	ret = of_property_read_u32(np, "fpga_flag", &is_fpga);
	if (ret != 0)
		dpu_pr_err("failed to get fpga_flag.\n");

	g_dpu_config_data.version.info.version = dpu_config_get_version();
	g_dpu_config_data.version.info.hw_type = (is_fpga == 0) ? DPU_HW_TYPE_ASIC : DPU_HW_TYPE_FPGA;
	dpu_pr_debug("dpu version 0x%llx", g_dpu_config_data.version.value);
}

static void _config_get_dvfs_config(struct device_node *np)
{
	int ret;
	uint32_t pmctrl_dvfs_enable = 0;

	ret = of_property_read_u32(np, "pmctrl_dvfs_enable", &pmctrl_dvfs_enable);
	if (ret != 0)
		dpu_pr_info("failed to get pmctrl_dvfs_enable!");

	g_dpu_config_data.pmctrl_dvfs_enable = pmctrl_dvfs_enable;
	dpu_pr_info("dpu pmctrl_dvfs_enable %u", g_dpu_config_data.pmctrl_dvfs_enable);

	g_dpu_config_data.clk_gate_edc = of_clk_get(np, 0);
	if (IS_ERR(g_dpu_config_data.clk_gate_edc))
		dpu_pr_err("failed to get clk_gate_edc!");
}

void dpu_config_dvfs_vote_exec(uint64_t clk_rate)
{
	int ret = 0;

	if (g_dpu_config_data.clk_gate_edc == NULL)
		return;

	ret = clk_set_rate(g_dpu_config_data.clk_gate_edc, clk_rate);
	if (ret)
		dpu_pr_err("set edc clk rate=%llu failed, error=%d!", clk_rate, ret);

	dpu_pr_info("set clk rate=%llu",
			clk_get_rate(g_dpu_config_data.clk_gate_edc));
}

int32_t dpu_init_config(struct platform_device *device)
{
	struct device_node *np = NULL;

	if (!device)
		return -1;

	np = device->dev.of_node;
	if (!np) {
		dpu_pr_err("platfodpu device of node is null");
		return -1;
	}

	_config_get_ip_base(np, g_dpu_config_data.ip_base_addrs, DISP_IP_MAX);
	_config_get_lbuf_size(np);
	_config_get_dpu_version_value(np);
	_config_get_dvfs_config(np);
	return _config_get_offline_scene_id_array(np);
}

MODULE_LICENSE("GPL");
