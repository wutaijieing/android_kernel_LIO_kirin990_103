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
#include <linux/of_address.h>
#include <soc_pmctrl_interface.h>
#include <soc_media1_crg_interface.h>

#include "dkmd_log.h"
#include "dkmd_object.h"
#include "dpu_config_utils.h"

static uint64_t s_dpu_core_rate_tbl[] = {
	105000000UL, /* L0_POWER_OFF */
	279000000UL, /* L1_060V */
	418000000UL, /* L2_065V */
	480000000UL, /* L3_070V */
	642000000UL  /* L4_080V */
};

static uint64_t dpu_config_get_core_rate(uint32_t level)
{
	if (level >= ARRAY_SIZE(s_dpu_core_rate_tbl))
		return s_dpu_core_rate_tbl[4];

	return s_dpu_core_rate_tbl[level];
}

bool is_dpu_dvfs_enable(void)
{
	return false;
}
EXPORT_SYMBOL(is_dpu_dvfs_enable);

void dpu_intra_frame_dvfs_vote(struct intra_frame_dvfs_info *info)
{
	void_unused(info);
}

void dpu_sw_dvfs_enable(void)
{
	void_unused(0);
}

void dpu_legacy_inter_frame_dvfs_vote(uint32_t level)
{
	dpu_config_dvfs_vote_exec(dpu_config_get_core_rate(level));
}
EXPORT_SYMBOL(dpu_legacy_inter_frame_dvfs_vote);

void dpu_enable_core_clock(void)
{
	dpu_pr_info("+");
	if (dpu_config_enable_pmctrl_dvfs()) {
		dpu_sw_dvfs_enable();
	} else {
		if (g_dpu_config_data.clk_gate_edc == NULL)
			return;

		dpu_legacy_inter_frame_dvfs_vote(1);
		if (clk_prepare_enable(g_dpu_config_data.clk_gate_edc) != 0)
			dpu_pr_err("enable clk_gate_edc failed!");
	}
}

void dpu_disable_core_clock(void)
{
	dpu_pr_info("+");

	dpu_legacy_inter_frame_dvfs_vote(0);

	if ((g_dpu_config_data.clk_gate_edc != NULL) &&
			(!dpu_config_enable_pmctrl_dvfs()))
		clk_disable_unprepare(g_dpu_config_data.clk_gate_edc);
}
