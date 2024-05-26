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

#ifndef HISI_DISP_CONFIG_H
#define HISI_DISP_CONFIG_H

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/of.h>
#include <soc_dte_define.h>

#include "hisi_disp.h"
#include "hisi_composer_operator.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_config_charlotte.h"   /* other platform need include other platform head file */

#define OPERATOR_TYPE_MASK 0xff
#define build_operator_type(type, count) ((type) | ((count) << OPERATOR_BIT_FIELD_LEN))
#define get_operator_type(desc)  ((desc) & OPERATOR_TYPE_MASK)
#define get_operator_count(desc) (((desc) >> OPERATOR_BIT_FIELD_LEN) & OPERATOR_TYPE_MASK)

#define DPU_DTS_COMPATIBLE_NAME       "hisilicon,dpu_resource"

union operator_type_desc {
	uint32_t type_desc;
	struct {
		uint32_t type : OPERATOR_BIT_FIELD_LEN;
		uint32_t count : OPERATOR_BIT_FIELD_LEN;
	} detail;
};

struct hisi_dpu_config {
	char __iomem *ip_base_addrs[DISP_IP_MAX];
	uint32_t irq_signals[DISP_IRQ_SIGNAL_MAX];
	struct regulator_bulk_data regulators[PM_REGULATOR_MAX];
	struct clk *clks[PM_CLK_MAX];
	union operator_type_desc operator_types[COMP_OPS_TYPE_MAX];
	uint32_t enable_fastboot_display;
	uint32_t fpga_flag;
	uint32_t v7_tag;
	/* TODO: other config about platform */
};

extern struct hisi_dpu_config g_dpu_config;
extern uint32_t g_dm_input_data_st_addr[DPU_SCENE_MAX];

int hisi_disp_init_dpu_config(struct platform_device *device);

static inline uint32_t dpu_get_dm_offset(uint32_t scene_id)
{
	return g_dm_input_data_st_addr[scene_id % DPU_SCENE_MAX];
}

static inline union operator_type_desc *hisi_disp_config_get_operators(void)
{
	return g_dpu_config.operator_types;
}

static inline struct regulator_bulk_data *hisi_disp_config_get_regulators(void)
{
	return g_dpu_config.regulators;
}

static inline struct clk **hisi_disp_config_get_clks(void)
{
	return g_dpu_config.clks;
}

static inline uint32_t *hisi_disp_config_get_irqs(void)
{
	return g_dpu_config.irq_signals;
}

static inline void hisi_config_init_ip_base_addr(char __iomem **out_ip_base_addrs, uint32_t count)
{
	uint32_t i;
	disp_pr_info(" ++++ ");
	for (i = 0; i < count; i++) {
		out_ip_base_addrs[i] = g_dpu_config.ip_base_addrs[i];
		disp_pr_info("ip %d base addr 0x%x", i, out_ip_base_addrs[i]);
	}
}

static inline uint32_t hisi_config_get_enable_fastboot_display_flag(void)
{
	return g_dpu_config.enable_fastboot_display;
}

static inline uint32_t hisi_config_get_fpga_flag(void)
{
	return g_dpu_config.fpga_flag;
}

static inline uint32_t hisi_config_get_v7_tag(void)
{
	return g_dpu_config.v7_tag;
}

static inline char __iomem *hisi_config_get_ip_base(uint32_t ip)
{
	return g_dpu_config.ip_base_addrs[ip % DISP_IP_MAX];
}

static inline uint32_t dpu_config_get_scl_count(void)
{
	return g_dpu_config.operator_types[COMP_OPS_VSCF].detail.count + g_dpu_config.operator_types[COMP_OPS_ARSR].detail.count;
}

static inline uint32_t dpu_config_get_vscf_count(void)
{
	return g_dpu_config.operator_types[COMP_OPS_VSCF].detail.count;
}

static inline uint32_t dpu_config_get_arsr_count(void)
{
	return g_dpu_config.operator_types[COMP_OPS_ARSR].detail.count;
}

#endif /* HISI_DISP_CONFIG_H */
