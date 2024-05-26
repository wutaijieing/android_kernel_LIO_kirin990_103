/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>

#include "hisi_disp_config.h"
#include "hisi_disp_iommu.h"

struct hisi_dpu_config g_dpu_config;
uint32_t g_dm_input_data_st_addr[DPU_SCENE_MAX] = {
	/* online */
	DACC_OFFSET + DM_INPUTDATA_ST_ADDR0, DACC_OFFSET + DM_INPUTDATA_ST_ADDR1, DACC_OFFSET + DM_INPUTDATA_ST_ADDR2,
	DACC_OFFSET + DM_INPUTDATA_ST_ADDR3,

	/* offline */
	DACC_OFFSET + DM_INPUTDATA_ST_ADDR4, DACC_OFFSET + DM_INPUTDATA_ST_ADDR5, DACC_OFFSET + DM_INPUTDATA_ST_ADDR6,

	DACC_OFFSET + DM_DPP_INITAIL_ST_ADDR, DACC_OFFSET + DM_SECURITY_ST_ADDR
};

static void _hisi_config_get_ip_base(struct device_node *np, char __iomem *ip_base_addrs[], uint32_t ip_count)
{
	uint32_t i;
	disp_pr_info("+++");
	if (ip_count > DISP_IP_MAX)
		return;

	for (i = 0; i < ip_count; i++) {
		ip_base_addrs[i] = of_iomap(np, i);
		disp_pr_info("ip_base_addrs:0x%x", ip_base_addrs[i]);
		if (!ip_base_addrs[i])
			disp_pr_err("get ip %u base addr fail", i);
	}
}

static void _hisi_config_get_irq_no(struct device_node *np, uint32_t *irq_signals, uint32_t irq_count)
{
	uint32_t i;

	if (irq_count > DISP_IRQ_SIGNAL_MAX)
		return;

	for (i = 0; i < irq_count; i++) {
		irq_signals[i] = irq_of_parse_and_map(np, i);
		disp_pr_info("irq_signals:%d", irq_signals[i]);
		if (irq_signals[i] == 0)
			disp_pr_err("get irq no fail, signal = %u", i);
	}
}

static void _hisi_config_get_clks(struct platform_device *device, struct device_node *np, struct clk *clks[], uint32_t clk_count)
{
	uint32_t i;
	int ret;
	const char *clk_name = NULL;

	if (clk_count > PM_CLK_MAX)
		return;

	for (i = 0; i < clk_count; i++) {
		ret = of_property_read_string_index(np, "clock-names", i, &clk_name);
		if (ret)
			continue;
		disp_pr_info("clk_name:%s", clk_name);
		clks[i] = devm_clk_get(&device->dev, clk_name);
		if (!clks[i]) {
			disp_pr_err("get clk %s fail", clk_name);
		}
	}
}

static void _hisi_config_get_operators(struct device_node *np, union operator_type_desc operator_types[], uint32_t type_count)
{
	struct property *prop;
	const __be32 *cur;
	u32 val;
	uint16_t type;
	uint16_t count;

	of_property_for_each_u32(np, "operators", prop, cur, val) {
		type = get_operator_type(val);
		count = get_operator_count(val);
		disp_pr_info("get operator %s, count = %u", hisi_config_get_operator_name(type), count);

		if (type > type_count)
			continue;

		if (operator_types[type].type_desc != 0)
			disp_pr_warn("type %d have read again, count =%d", hisi_config_get_operator_name(type), count);

		operator_types[type].type_desc = val;
	}
}

static inline void _hisi_config_get_enable_fastboot_display(struct device_node *np, uint32_t *enable_fastboot_display_flag)
{
	 of_property_read_u32(np, "enable_fastboot_display", enable_fastboot_display_flag);
	 // *enable_fastboot_display_flag = 0;
}

static void _hisi_config_get_others_property(struct device_node *np)
{
	 of_property_read_u32(np, "fpga_flag", &g_dpu_config.fpga_flag);
	 of_property_read_u32(np, "v7_tag", &g_dpu_config.v7_tag);
}

int hisi_disp_init_dpu_config(struct platform_device *device)
{
	struct device_node *np = NULL;

	disp_pr_info("+++");
	np = of_find_compatible_node(NULL, NULL, DPU_DTS_COMPATIBLE_NAME);
	if (!np) {
		disp_pr_err("get %s np fail", DPU_DTS_COMPATIBLE_NAME);
		return -1;
	}
	/* iova init */
	hisi_dss_iommu_enable(device);

	_hisi_config_get_ip_base(np, g_dpu_config.ip_base_addrs, DISP_IP_MAX);
	_hisi_config_get_irq_no(np, g_dpu_config.irq_signals, DISP_IRQ_SIGNAL_MAX);
	hisi_config_get_regulators(device, g_dpu_config.regulators, PM_REGULATOR_MAX);
	_hisi_config_get_clks(device, np, g_dpu_config.clks, PM_CLK_MAX);
	_hisi_config_get_operators(np, g_dpu_config.operator_types, COMP_OPS_TYPE_MAX);
	_hisi_config_get_enable_fastboot_display(np, &g_dpu_config.enable_fastboot_display);
	_hisi_config_get_others_property(np);
	return 0;
}
