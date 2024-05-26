/* Copyright (c) 2021-2021, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "dpu_dpe_utils.h"

struct dss_vote_cmd g_vote_cmd;

#ifdef SUPPORT_LOW_TEMPERATURE_PROTECT
bool check_low_temperature(struct dpu_fb_data_type *dpufd)
{
	uint32_t perictrl4;

	dpu_check_and_return(!dpufd, FALSE, ERR, "dpufd is NULL\n");

	perictrl4 = inp32(dpufd->pmctrl_base + MIDIA_PERI_CTRL4);
	perictrl4 &= PMCTRL_PERI_CTRL4_TEMPERATURE_MASK;
	perictrl4 = (perictrl4 >> PMCTRL_PERI_CTRL4_TEMPERATURE_SHIFT);
	DPU_FB_DEBUG("Get current temperature: %d\n", perictrl4);

	return (perictrl4 != NORMAL_TEMPRATURE) ? true : false;
}

void dpufb_low_temperature_clk_protect(struct dpu_fb_data_type *dpufd)
{
	struct dss_vote_cmd vote_cmd;
	int ret;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	if (!check_low_temperature(dpufd))
		return;

	down(&g_dpufb_dss_clk_vote_sem);

	vote_cmd = dpufd->dss_vote_cmd;
	/* clk_rate_l4@low_temperature enable would be fail,
	 * so clk config change to clk_rate_l3 before enter idle.
	 */
	if (vote_cmd.dss_pri_clk_rate == DEFAULT_DSS_CORE_CLK_RATE_L4) {
		g_vote_cmd = dpufd->dss_vote_cmd;
		vote_cmd.dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L3;
		vote_cmd.dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L3;
		ret = dpufb_set_edc_mmbuf_clk(dpufd, vote_cmd);
		if (ret < 0)
			DPU_FB_INFO("set edc mmbuf clk failed\n");
	}

	up(&g_dpufb_dss_clk_vote_sem);
}

void dpufb_low_temperature_clk_restore(struct dpu_fb_data_type *dpufd)
{
	int ret;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	if (!g_vote_cmd.dss_pri_clk_rate)
		return;

	down(&g_dpufb_dss_clk_vote_sem);

	/* Recovery is required only if the two are equal.
	 * Otherwise, it means that there is already a process(HWC etc.) to vote clk.
	 */
	if (g_vote_cmd.dss_pri_clk_rate == dpufd->dss_vote_cmd.dss_pri_clk_rate) {
		ret = dpufb_set_edc_mmbuf_clk(dpufd, g_vote_cmd);
		if (ret < 0)
			DPU_FB_INFO("set edc mmbuf clk failed\n");
	}

	memset(&g_vote_cmd, 0, sizeof(g_vote_cmd));

	up(&g_dpufb_dss_clk_vote_sem);
}

#endif

