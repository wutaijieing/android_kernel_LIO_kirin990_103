/* Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#if defined(CONFIG_PERI_DVFS)
#include "platform_include/basicplatform/linux/peri_volt_poll.h"
#endif


DEFINE_SEMAPHORE(dpu_fb_dss_inner_clk_sem);

static int g_dss_inner_clk_refcount;

#define OFFSET_FRACTIONAL_BITS 11

#define PERI_VOLTAGE_LEVEL0_060V 0 /* 0.60v */
#define PERI_VOLTAGE_LEVEL1_065V 1 /* 0.65v */
#define PERI_VOLTAGE_LEVEL2_070V 2 /* 0.70v */
#define PERI_VOLTAGE_LEVEL3_080V 3 /* 0.80v */

#define FPGA_DEFAULT_DSS_CORE_CLK_RATE_L1 40000000UL
#define FRAME_RATE_1440P 110

//### from dpu_dpe_utils_dssv330.c
static int get_lcd_frame_rate(struct dpu_panel_info *pinfo)
{
	return pinfo->pxl_clk_rate / (pinfo->xres + pinfo->pxl_clk_rate_div *
		(pinfo->ldi.h_back_porch + pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width)) / (pinfo->yres +
		pinfo->ldi.v_back_porch + pinfo->ldi.v_front_porch + pinfo->ldi.v_pulse_width);
}

bool is_vote_needed_for_low_temp(bool is_lowtemp, int volt_to_set)
{
	DPU_FB_DEBUG("is_lowtemp=%d, volt=%d\n", is_lowtemp, volt_to_set);
	return !is_lowtemp;
}

static void init_dss_fpga_vote_cmd(struct dss_vote_cmd *pdss_vote_cmd)
{
	if (pdss_vote_cmd->dss_pclk_dss_rate == 0) {
		pdss_vote_cmd->dss_pri_clk_rate = 40 * 1000000UL;
		pdss_vote_cmd->dss_pclk_dss_rate = 20 * 1000000UL;
		pdss_vote_cmd->dss_pclk_pctrl_rate = 20 * 1000000UL;
	}
}

struct dss_vote_cmd *get_dss_vote_cmd(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	struct dss_vote_cmd *pdss_vote_cmd = NULL;
	int frame_rate;

	dpu_check_and_return(!dpufd, NULL, ERR, "dpufd is null.\n");

	pinfo = &(dpufd->panel_info);
	pdss_vote_cmd = &(dpufd->dss_vote_cmd);

	frame_rate = get_lcd_frame_rate(pinfo);

	if (g_fpga_flag == 1) {
		init_dss_fpga_vote_cmd(pdss_vote_cmd);
		return pdss_vote_cmd;
	}
	if (pdss_vote_cmd->dss_pclk_dss_rate != 0)
		return pdss_vote_cmd;
	if ((pinfo->xres * pinfo->yres) >= RES_4K_PHONE) {
		pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L3;
		pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L3;
		pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
		pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
		dpufd->core_clk_upt_support = 0;
	} else if ((pinfo->xres * pinfo->yres) >= RES_1440P) {
		if (frame_rate >= 110) { /* 110Hz is inherited param */
			pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L3;
			pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L3;
			pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
			pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
			dpufd->core_clk_upt_support = 0;
		} else {
			pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE;
			pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L1;
			pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
			pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
			dpufd->core_clk_upt_support = 1;
		}
	} else if ((pinfo->xres * pinfo->yres) >= RES_1080P) {
		pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L1;
		pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L1;
		pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
		pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
		dpufd->core_clk_upt_support = 1;
	} else {
		pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L1;
		pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L1;
		pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
		pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
		dpufd->core_clk_upt_support = 1;
	}

	return pdss_vote_cmd;
}

static int get_mdc_clk_rate(dss_vote_cmd_t vote_cmd, uint64_t *clk_rate)
{
	switch (vote_cmd.dss_voltage_level) {
	case PERI_VOLTAGE_LEVEL0:
		*clk_rate = DEFAULT_MDC_CORE_CLK_RATE_L1;
		break;
	case PERI_VOLTAGE_LEVEL1:
		*clk_rate = DEFAULT_MDC_CORE_CLK_RATE_L2;
		break;
	case PERI_VOLTAGE_LEVEL2:
		*clk_rate = DEFAULT_MDC_CORE_CLK_RATE_L3;
		break;
	case PERI_VOLTAGE_LEVEL3:
		*clk_rate = DEFAULT_MDC_CORE_CLK_RATE_L4;
		break;
	default:
		DPU_FB_ERR("no support dss_voltage_level %u!\n", vote_cmd.dss_voltage_level);
		return -1;
	}

	DPU_FB_DEBUG("get mdc clk rate: %llu\n", *clk_rate);
	return 0;
}

static int set_mdc_core_clk(struct dpu_fb_data_type *dpufd, dss_vote_cmd_t vote_cmd)
{
	int ret;
	uint64_t clk_rate = 0;

	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is NULL\n");

	if (vote_cmd.dss_voltage_level == dpufd->dss_vote_cmd.dss_voltage_level) {
		DPU_FB_INFO("vote dss_voltage_level is equal to current dss_voltage_level\n");
		return 0;
	}

	if (get_mdc_clk_rate(vote_cmd, &clk_rate)) {
		DPU_FB_ERR("get mdc clk rate failed!\n");
		return -1;
	}

	ret = clk_set_rate(dpufd->dss_clk_media_common_clk, clk_rate);
	if (ret < 0) {
		DPU_FB_ERR("set dss_clk_media_common_clk(%llu) failed, error= %d\n",
			clk_rate, ret);
		return -1;
	}
	dpufd->dss_vote_cmd.dss_voltage_level = vote_cmd.dss_voltage_level;

	DPU_FB_DEBUG("set dss_clk_media_common_clk = %llu, dss_voltage_level = %u\n",
		clk_rate, vote_cmd.dss_voltage_level);

	return ret;
}

static int dss_core_clk_enable(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->dss_pri_clk)
		(void)clk_prepare_enable(dpufd->dss_pri_clk);

	if (dpufd->dss_axi_clk)
		(void)clk_prepare_enable(dpufd->dss_axi_clk);

	if (dpufd->dss_mmbuf_clk)
		(void)clk_prepare_enable(dpufd->dss_mmbuf_clk);

	return 0;
}

static int dss_core_clk_disable(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->dss_pri_clk)
		clk_disable_unprepare(dpufd->dss_pri_clk);

	if (dpufd->dss_axi_clk)
		clk_disable_unprepare(dpufd->dss_axi_clk);

	if (dpufd->dss_mmbuf_clk)
		clk_disable_unprepare(dpufd->dss_mmbuf_clk);

	return 0;
}

static int set_primary_core_clk(struct dpu_fb_data_type *dpufd, dss_vote_cmd_t dss_vote_cmd)
{
	int ret = 0;
	struct dpu_fb_data_type *targetfd = NULL;
	bool set_rate_succ = true;

	targetfd = dpufd_list[AUXILIARY_PANEL_IDX];
	if (!targetfd) {
		DPU_FB_ERR("fb2 is null\n");
		return -1;
	}

	if (dss_vote_cmd.dss_pri_clk_rate >= targetfd->dss_vote_cmd.dss_pri_clk_rate) {
		if (dpufd->panel_info.vsync_ctrl_type & VSYNC_CTRL_CLK_OFF) {
			ret = dss_core_clk_enable(dpufd);
			if (ret < 0) {
				DPU_FB_ERR("dss_core_clk_enable failed, error=%d!\n", ret);
				return -1;
			}
		}
		ret = clk_set_rate(dpufd->dss_pri_clk, dss_vote_cmd.dss_pri_clk_rate);
		if (ret < 0) {
			set_rate_succ = false;
			DPU_FB_ERR("set dss_pri_clk_rate(%llu) failed, error=%d!\n",
				dss_vote_cmd.dss_pri_clk_rate, ret);
		}

		if (dpufd->panel_info.vsync_ctrl_type & VSYNC_CTRL_CLK_OFF) {
			ret = dss_core_clk_disable(dpufd);
			if (ret < 0) {
				DPU_FB_ERR("dss_core_clk_disable, error=%d!\n", ret);
				return -1;
			}
		}

		DPU_FB_DEBUG("fb%d get dss_pri_clk = %llu.\n",
			dpufd->index, (uint64_t)clk_get_rate(dpufd->dss_pri_clk));
		DPU_FB_DEBUG("fb%d get dss_mmbuf_rate = %llu.\n",
			dpufd->index, (uint64_t)clk_get_rate(dpufd->dss_mmbuf_clk));
		if (set_rate_succ) {
			dpufd->dss_vote_cmd.dss_pri_clk_rate = dss_vote_cmd.dss_pri_clk_rate;
			dpufd->dss_vote_cmd.dss_axi_clk_rate = dss_vote_cmd.dss_axi_clk_rate;
		} else {
			return -1;
		}
	}

	DPU_FB_DEBUG("set fb0 coreClk=%llu, fb2->coreClk=%llu.\n",
		dss_vote_cmd.dss_pri_clk_rate, targetfd->dss_vote_cmd.dss_pri_clk_rate);
	return ret;
}

int set_dss_vote_cmd(struct dpu_fb_data_type *dpufd, dss_vote_cmd_t vote_cmd)
{
	int ret = 0;
	struct dpu_fb_data_type *targetfd = NULL;
	uint64_t pri_clk_rate = vote_cmd.dss_pri_clk_rate; /* recording the vote clk rate */

	dpu_check_and_return(!dpufd, -1, ERR, "NULL Pointer!\n");

	if ((vote_cmd.dss_pri_clk_rate != DEFAULT_DSS_CORE_CLK_RATE_L1) &&
		(vote_cmd.dss_pri_clk_rate != DEFAULT_DSS_CORE_CLK_RATE_L2) &&
		(vote_cmd.dss_pri_clk_rate != DEFAULT_DSS_CORE_CLK_RATE_L3) &&
		(vote_cmd.dss_pri_clk_rate != DEFAULT_DSS_CORE_CLK_RATE_L4)) {
		DPU_FB_ERR("fb%d no support set dss_pri_clk_rate(%llu)!\n", dpufd->index, vote_cmd.dss_pri_clk_rate);
		return -1;
	}

	dpu_check_and_return((vote_cmd.dss_pri_clk_rate == dpufd->dss_vote_cmd.dss_pri_clk_rate), ret, DEBUG,
		"vote dss_pri_clk_rate is equal to current dss_pri_clk_rate! the rate is %llu\n",
		vote_cmd.dss_pri_clk_rate);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		/*
		 * if dss pri clk rate set from 400M to one of the value 232M/320M/384M,
		 * we need first adjust to a temp clk rate in order to avoid high freq for an instance
		 */
		if (((vote_cmd.dss_pri_clk_rate == DEFAULT_DSS_CORE_CLK_RATE_L1) ||
			(vote_cmd.dss_pri_clk_rate == DEFAULT_DSS_CORE_CLK_RATE_L2) ||
			(vote_cmd.dss_pri_clk_rate == DEFAULT_DSS_CORE_CLK_RATE_L3)) &&
			(dpufd->dss_vote_cmd.dss_pri_clk_rate == DEFAULT_DSS_CORE_CLK_RATE_L4)) {
			vote_cmd.dss_pri_clk_rate = TEMP_DSS_CORE_CLK_RATE;
			ret = set_primary_core_clk(dpufd, vote_cmd);
			if (ret < 0)
				DPU_FB_ERR("set dss_pri_clk_rate(TEMP_DSS_CORE_CLK_RATE) failed, error = %d!\n", ret);
			else
				DPU_FB_DEBUG("set dss_pri_clk_rate(TEMP_DSS_CORE_CLK_RATE) succ!\n");
		}

		vote_cmd.dss_pri_clk_rate = pri_clk_rate;
		ret = set_primary_core_clk(dpufd, vote_cmd);
		if (ret < 0)
			DPU_FB_ERR("set dss_pri_clk_rate(%llu) failed, error = %d!\n", vote_cmd.dss_pri_clk_rate, ret);
		else
			DPU_FB_DEBUG("set dss_pri_clk_rate(%llu) succ!\n", vote_cmd.dss_pri_clk_rate);

		return ret;
	}

	if (dpufd->index == AUXILIARY_PANEL_IDX) {
		targetfd = dpufd_list[PRIMARY_PANEL_IDX];
		if (targetfd && (vote_cmd.dss_pri_clk_rate >= targetfd->dss_vote_cmd.dss_pri_clk_rate)) {
			dpufd->need_tuning_clk = true;
			DPU_FB_DEBUG("need tuning fb%d pri_clk_rate to %llu.\n",
				dpufd->index, vote_cmd.dss_pri_clk_rate);
		}

		dpufd->dss_vote_cmd.dss_pri_clk_rate = vote_cmd.dss_pri_clk_rate;
	}

	DPU_FB_DEBUG("set dss_pri_clk_rate = %llu.\n", vote_cmd.dss_pri_clk_rate);

	return ret;
}

int dpe_get_voltage_value(struct dpu_fb_data_type *dpufd, uint32_t dss_voltage_level)
{
	void_unused(dpufd);

	switch (dss_voltage_level) {
	case PERI_VOLTAGE_LEVEL0:
		return PERI_VOLTAGE_LEVEL0_060V;
	case PERI_VOLTAGE_LEVEL1:
		return PERI_VOLTAGE_LEVEL1_065V;
	case PERI_VOLTAGE_LEVEL2:
		return PERI_VOLTAGE_LEVEL2_070V;
	case PERI_VOLTAGE_LEVEL3:
		return PERI_VOLTAGE_LEVEL3_080V;
	default:
		DPU_FB_ERR("not support dss_voltage_level is %u\n", dss_voltage_level);
		return -1;
	}
}

int dpe_get_voltage_level(int votage_value)
{
	switch (votage_value) {
	case PERI_VOLTAGE_LEVEL0_060V:
		return PERI_VOLTAGE_LEVEL0;
	case PERI_VOLTAGE_LEVEL1_065V:
		return PERI_VOLTAGE_LEVEL1;
	case PERI_VOLTAGE_LEVEL2_070V:
		return PERI_VOLTAGE_LEVEL2;
	case PERI_VOLTAGE_LEVEL3_080V:
		return PERI_VOLTAGE_LEVEL3;
	default:
		DPU_FB_ERR("not support votage_value is %d\n", votage_value);
		return PERI_VOLTAGE_LEVEL0;
	}
}

int dpe_set_clk_rate(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	struct dss_vote_cmd *pdss_vote_cmd = NULL;
	uint64_t dss_pri_clk_rate;
	int ret;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is nullptr!\n");

	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is nullptr!\n");

	pinfo = &(dpufd->panel_info);
	pdss_vote_cmd = get_dss_vote_cmd(dpufd);
	dpu_check_and_return(!pdss_vote_cmd, -1, ERR, "pdss_vote_cmd is nullptr!\n");

	/* mmbuf_clk */
	ret = clk_set_rate(dpufd->dss_mmbuf_clk, pdss_vote_cmd->dss_mmbuf_rate);
	dpu_check_and_return((ret < 0), -EINVAL, ERR, "fb%d dss_mmbuf clk_set_rate(%llu) failed, error=%d!\n",
		dpufd->index, pdss_vote_cmd->dss_mmbuf_rate, ret);


	/* edc0_clk (pri_clk) (core_clk) */
	dss_pri_clk_rate = pdss_vote_cmd->dss_pri_clk_rate;

	if (dpufd->index != PRIMARY_PANEL_IDX && dpufd_list[PRIMARY_PANEL_IDX]) {
		if (dpufd_list[PRIMARY_PANEL_IDX]->dss_vote_cmd.dss_pri_clk_rate > dss_pri_clk_rate)
			dss_pri_clk_rate = dpufd_list[PRIMARY_PANEL_IDX]->dss_vote_cmd.dss_pri_clk_rate;
	}

	ret = clk_set_rate(dpufd->dss_pri_clk, dss_pri_clk_rate);
	dpu_check_and_return((ret < 0), -EINVAL, ERR, "fb%d dss_pri_clk clk_set_rate(%llu) failed, error=%d!\n",
		dpufd->index, dss_pri_clk_rate, ret);

	/* ldi0 clk (pxl0_clk) */
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ret = clk_set_rate(dpufd->dss_pxl0_clk, pinfo->pxl_clk_rate);
		if (ret < 0) {
			DPU_FB_ERR("fb%d dss_pxl0_clk clk_set_rate(%llu) failed, error=%d!\n",
				dpufd->index, pinfo->pxl_clk_rate, ret);
			if (g_fpga_flag == 0)
				return -EINVAL;
		}
	} else if ((dpufd->index == EXTERNAL_PANEL_IDX) && !dpufd->panel_info.fake_external) {
		DPU_FB_ERR("not support.\n");
	} else {
		;
	}

	/* print clk info */
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		DPU_FB_INFO("dss_mmbuf_clk:[%llu]->[%llu].\n",
			pdss_vote_cmd->dss_mmbuf_rate, (uint64_t)clk_get_rate(dpufd->dss_mmbuf_clk));
		DPU_FB_INFO("dss_pri_clk:[%llu]->[%llu].\n",
			pdss_vote_cmd->dss_pri_clk_rate, (uint64_t)clk_get_rate(dpufd->dss_pri_clk));
		DPU_FB_INFO("dss_pxl0_clk:[%llu]->[%llu].\n",
			pinfo->pxl_clk_rate, (uint64_t)clk_get_rate(dpufd->dss_pxl0_clk));
	}

	return ret;
}

int dpe_set_pixel_clk_rate_on_pll0(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
	return 0;
}

static void dpufb_set_default_pri_clk_rate(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	struct dss_vote_cmd *pdss_vote_cmd = NULL;
	int frame_rate;

	pinfo = &(dpufd->panel_info);
	pdss_vote_cmd = &(dpufd->dss_vote_cmd);
	frame_rate = get_lcd_frame_rate(pinfo);

	if (g_fpga_flag == 1) {
		pdss_vote_cmd->dss_pri_clk_rate = FPGA_DEFAULT_DSS_CORE_CLK_RATE_L1;
		return;
	}
	if ((pinfo->xres * pinfo->yres) >= (RES_4K_PHONE)) {
		pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L3;
	} else if ((pinfo->xres * pinfo->yres) >= (RES_1440P)) {
		if (frame_rate >= FRAME_RATE_1440P)
			pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L3;
		else
			pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L1;
	} else if ((pinfo->xres * pinfo->yres) >= (RES_1080P)) {
		pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L1;
	} else {
		pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L1;
	}
}

#if defined(CONFIG_PERI_DVFS)
static int dpe_set_pvp(void)
{
	int ret;
	struct peri_volt_poll *pvp = NULL;

	pvp = peri_volt_poll_get(DEV_DSS_VOLTAGE_ID, NULL);
	if (!pvp) {
		DPU_FB_ERR("get pvp failed!\n");
		return -EINVAL;
	}

	ret = peri_set_volt(pvp, PERI_VOLTAGE_LEVEL0_060V); /* 0.6v */
	if (ret != 0) {
		DPU_FB_ERR("set voltage_value=0 failed!\n");
		return -EINVAL;
	}
	dpufd_list[PRIMARY_PANEL_IDX]->dss_vote_cmd.dss_voltage_level = PERI_VOLTAGE_LEVEL0;
	dpufd_list[AUXILIARY_PANEL_IDX]->dss_vote_cmd.dss_voltage_level = PERI_VOLTAGE_LEVEL0;

	DPU_FB_INFO("set dss_voltage_level=0!\n");
	return ret;
}
#endif

int dpe_set_common_clk_rate_on_pll0(struct dpu_fb_data_type *dpufd)
{
	int ret;
	uint64_t clk_rate;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL Pointer!\n");
		return -EINVAL;
	}

	if (g_fpga_flag == 1)
		return 0;

	clk_rate = DEFAULT_DSS_MMBUF_CLK_RATE_POWER_OFF;
	ret = clk_set_rate(dpufd->dss_mmbuf_clk, clk_rate);
	if (ret < 0) {
		DPU_FB_ERR("fb%d dss_mmbuf clk_set_rate(%llu) failed, error=%d!\n", dpufd->index, clk_rate, ret);
		return -EINVAL;
	}
	DPU_FB_INFO("dss_mmbuf_clk:[%llu]->[%llu].\n", clk_rate, (uint64_t)clk_get_rate(dpufd->dss_mmbuf_clk));

	clk_rate = DEFAULT_DSS_CORE_CLK_RATE_POWER_OFF;
	ret = clk_set_rate(dpufd->dss_pri_clk, clk_rate);
	if (ret < 0) {
		DPU_FB_ERR("fb%d dss_pri_clk clk_set_rate(%llu) failed, error=%d!\n", dpufd->index, clk_rate, ret);
		return -EINVAL;
	}
	DPU_FB_INFO("dss_pri_clk:[%llu]->[%llu].\n", clk_rate, (uint64_t)clk_get_rate(dpufd->dss_pri_clk));

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		clk_rate = DEFAULT_DSS_PXL0_CLK_RATE_POWER_OFF;
		ret = clk_set_rate(dpufd->dss_pxl0_clk, clk_rate);
		if (ret < 0) {
			DPU_FB_ERR("fb%d dss_pxl0_clk clk_set_rate(%llu) failed, error=%d!\n",
				dpufd->index, clk_rate, ret);
			return -EINVAL;
		}
		DPU_FB_INFO("dss_pxl0_clk:[%llu]->[%llu].\n", clk_rate, (uint64_t)clk_get_rate(dpufd->dss_pxl0_clk));

	} else if ((dpufd->index == EXTERNAL_PANEL_IDX) && !dpufd->panel_info.fake_external) {
		DPU_FB_ERR("not support.\n");
	} else {
		;
	}

	dpufb_set_default_pri_clk_rate(dpufd_list[PRIMARY_PANEL_IDX]);
	dpufd_list[AUXILIARY_PANEL_IDX]->dss_vote_cmd.dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L1;
#if defined(CONFIG_PERI_DVFS)
	ret = dpe_set_pvp();
	dpu_check_and_return((ret != 0), ret, ERR, "dpe_set_pvp failed!\n");
#endif

	return ret;
}

#ifdef CONFIG_DSS_LP_USED
static void dss_lp_set_reg(char __iomem *dss_base)
{
	outp32(dss_base + GLB_MODULE_CLK_SEL, 0x00000000); /* core clk, pclk */
	outp32(dss_base + DSS_VBIF0_AIF + AIF_MODULE_CLK_SEL, 0x00000000); /* axi clk */
	outp32(dss_base + DSS_VBIF1_AIF + AIF_MODULE_CLK_SEL, 0x00000000); /* mmbuf clk */

	/*
	 * second DSS LP  axi
	 * cmd
	 */
	outp32(dss_base + DSS_CMDLIST_OFFSET + CMD_CLK_SEL, 0x00000000);
	/* aif0 */
	outp32(dss_base + DSS_VBIF0_AIF + AIF_CLK_SEL0, 0x00000000);
	/* aif0 */
	outp32(dss_base + DSS_VBIF0_AIF + AIF_CLK_SEL1, 0x00000000);
	/* mmu */
	outp32(dss_base + DSS_SMMU_OFFSET + SMMU_LP_CTRL, 0x00000001);

	/*
	 * DSS LP mmbuf
	 * aif1
	 */
	outp32(dss_base + DSS_VBIF1_AIF + AIF_CLK_SEL0, 0x00000000);
	/* aif1 */
	outp32(dss_base + DSS_VBIF1_AIF + AIF_CLK_SEL1, 0x00000000);

	/* third DSS LP core
	 * mif
	 */
	outp32(dss_base + DSS_MIF_OFFSET + MIF_CLK_CTL,  0x00000001);
	/* mctl mutex0 */
	outp32(dss_base + DSS_MCTRL_CTL0_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
	/* mctl mutex1 */
	outp32(dss_base + DSS_MCTRL_CTL1_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
	/* mctl mutex2 */
	outp32(dss_base + DSS_MCTRL_CTL2_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
	/* mctl mutex3 */
	outp32(dss_base + DSS_MCTRL_CTL3_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
	/* mctl mutex4 */
	outp32(dss_base + DSS_MCTRL_CTL4_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
	/* mctl mutex5 */
	outp32(dss_base + DSS_MCTRL_CTL5_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
	/* mctl sys */
	outp32(dss_base + DSS_MCTRL_SYS_OFFSET + MCTL_MCTL_CLK_SEL, 0x00000000);
	/* mctl sys */
	outp32(dss_base + DSS_MCTRL_SYS_OFFSET + MCTL_MOD_CLK_SEL, 0x00000000);
	/* rch_v1 */
	outp32(dss_base + DSS_RCH_VG1_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
	/* rch_g1 */
	outp32(dss_base + DSS_RCH_G1_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
	/* rch_d0 */
	outp32(dss_base + DSS_RCH_D0_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
	/* rch_d1 */
	outp32(dss_base + DSS_RCH_D1_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
	/* rch_d2 */
	outp32(dss_base + DSS_RCH_D2_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
	/* rch_d3 */
	outp32(dss_base + DSS_RCH_D3_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
	/* wch0 */
	outp32(dss_base + DSS_WCH0_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
	/* ov0 */
	outp32(dss_base + DSS_OVL0_OFFSET + OVL6_OV_CLK_SEL, 0x00000000);
	/* ov2 */
	outp32(dss_base + DSS_OVL2_OFFSET + OVL6_OV_CLK_SEL, 0x00000000);
}
#else
static void dss_normal_set_reg(char __iomem *dss_base)
{
	/* core/axi/mmbuf */
	outp32(dss_base + DSS_CMDLIST_OFFSET + CMD_MEM_CTRL, 0x00000008); /* cmd mem */

	if (g_fpga_flag == 1)
		outp32(dss_base + DSS_RCH_VG1_ARSR_OFFSET + ARSR2P_LB_MEM_CTRL, 0x00000000); /* rch_v1, arsr2p mem */
	else
		outp32(dss_base + DSS_RCH_VG1_ARSR_OFFSET + ARSR2P_LB_MEM_CTRL, 0x00000008); /* rch_v1, arsr2p mem */

	outp32(dss_base + DSS_RCH_VG1_SCL_OFFSET + SCF_COEF_MEM_CTRL, 0x00000088); /* rch_v1 ,scf mem */
	outp32(dss_base + DSS_RCH_VG1_SCL_OFFSET + SCF_LB_MEM_CTRL, 0x00000008); /* rch_v1 ,scf mem */
	outp32(dss_base + DSS_RCH_VG1_DMA_OFFSET + VPP_MEM_CTRL, 0x00000008); /* rch_v1 ,vpp mem */
	outp32(dss_base + DSS_RCH_VG1_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008); /* rch_v1 ,dma_buf mem */
	outp32(dss_base + DSS_RCH_VG1_DMA_OFFSET + AFBCD_MEM_CTRL, 0x00008888); /* rch_v1 ,afbcd mem */

	outp32(dss_base + DSS_RCH_G1_SCL_OFFSET + SCF_COEF_MEM_CTRL, 0x00000088); /* rch_g1 ,scf mem */
	outp32(dss_base + DSS_RCH_G1_SCL_OFFSET + SCF_LB_MEM_CTRL, 0x0000008); /* rch_g1 ,scf mem */
	outp32(dss_base + DSS_RCH_G1_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008); /* rch_g1 ,dma_buf mem */
	outp32(dss_base + DSS_RCH_G1_DMA_OFFSET + AFBCD_MEM_CTRL, 0x00008888); /* rch_g1 ,afbcd mem */

	outp32(dss_base + DSS_RCH_D0_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008); /* rch_d0 ,dma_buf mem */
	outp32(dss_base + DSS_RCH_D0_DMA_OFFSET + AFBCD_MEM_CTRL, 0x00008888); /* rch_d0 ,afbcd mem */
	outp32(dss_base + DSS_RCH_D1_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008); /* rch_d1 ,dma_buf mem */
	outp32(dss_base + DSS_RCH_D1_DMA_OFFSET + AFBCD_MEM_CTRL, 0x00008888); /* rch_d1 ,afbcd mem */
	outp32(dss_base + DSS_RCH_D2_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008); /* rch_d2 ,dma_buf mem */
	outp32(dss_base + DSS_RCH_D3_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008); /* rch_d3 ,dma_buf mem */

	outp32(dss_base + DSS_WCH0_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008); /* wch0 DMA/AFBCE mem */
	outp32(dss_base + DSS_WCH0_DMA_OFFSET + AFBCE_MEM_CTRL, 0x00000888); /* wch0 DMA/AFBCE mem */
	outp32(dss_base + DSS_WCH0_DMA_OFFSET + ROT_MEM_CTRL, 0x00000008); /* wch0 rot mem */
}
#endif

void dss_inner_clk_common_enable(struct dpu_fb_data_type *dpufd, bool fastboot_enable)
{
	char __iomem *dss_base = NULL;
	int prev_refcount;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	dss_base = dpufd->dss_base;

	down(&dpu_fb_dss_inner_clk_sem);

	prev_refcount = g_dss_inner_clk_refcount++;
	if (!prev_refcount && !fastboot_enable) {
#ifdef CONFIG_DSS_LP_USED
		dss_lp_set_reg(dss_base);
#else
		dss_normal_set_reg(dss_base);
#endif
	}

	DPU_FB_DEBUG("fb%d, g_dss_inner_clk_refcount=%d\n",
		dpufd->index, g_dss_inner_clk_refcount);

	up(&dpu_fb_dss_inner_clk_sem);
}

void dss_inner_clk_common_disable(struct dpu_fb_data_type *dpufd)
{
	int new_refcount;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	down(&dpu_fb_dss_inner_clk_sem);
	new_refcount = --g_dss_inner_clk_refcount;
	if (new_refcount < 0)
		DPU_FB_ERR("dss new_refcount err.\n");

	DPU_FB_DEBUG("fb%d, g_dss_inner_clk_refcount=%d\n",
		dpufd->index, g_dss_inner_clk_refcount);
	up(&dpu_fb_dss_inner_clk_sem);
}

void dss_inner_clk_pdp_enable(struct dpu_fb_data_type *dpufd, bool fastboot_enable)
{
	char __iomem *dss_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	dss_base = dpufd->dss_base;

	if (fastboot_enable)
		return;

#ifdef CONFIG_DSS_LP_USED
	/* itf0 clk */
	outp32(dss_base + DSS_LDI0_OFFSET + LDI_MODULE_CLK_SEL, 0x00000006);

	outp32(dss_base + DSS_DPP_OFFSET + DPP_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_LDI0_OFFSET + LDI_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_DBUF0_OFFSET + DBUF_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_POST_SCF_OFFSET + SCF_CLK_SEL, 0x00000000);
#else
	outp32(dss_base + DSS_LDI0_OFFSET + LDI_MEM_CTRL, 0x00000008);
	outp32(dss_base + DSS_POST_SCF_OFFSET + SCF_COEF_MEM_CTRL, 0x00000088);
	outp32(dss_base + DSS_POST_SCF_OFFSET + SCF_LB_MEM_CTRL, 0x00000008);
	outp32(dss_base + DSS_DBUF0_OFFSET + DBUF_MEM_CTRL, 0x00000008);
	outp32(dss_base + DSS_DPP_DITHER_OFFSET + DITHER_MEM_CTRL, 0x00000008);
#endif
}

void dss_inner_clk_pdp_disable(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
}

void dss_inner_clk_sdp_enable(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dss_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	dss_base = dpufd->dss_base;

#ifdef CONFIG_DSS_LP_USED
	/* itf1 clk */
	outp32(dss_base + DSS_LDI1_OFFSET + LDI_MODULE_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_LDI1_OFFSET + LDI_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_DBUF1_OFFSET + DBUF_CLK_SEL, 0x00000000);
#else
	outp32(dss_base + DSS_LDI1_OFFSET + LDI_MEM_CTRL, 0x00000008);
	outp32(dss_base + DSS_DBUF1_OFFSET + DBUF_MEM_CTRL, 0x00000008);
#endif
}

void dss_inner_clk_sdp_disable(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
}

void init_dpp(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dpp_base = NULL;
	struct dpu_panel_info *pinfo = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dpp_base = dpufd->dss_base + DSS_DPP_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
		return;
	}

	outp32(dpp_base + DPP_IMG_SIZE_BEF_SR, (DSS_HEIGHT(pinfo->yres) << 16) | DSS_WIDTH(pinfo->xres));
	outp32(dpp_base + DPP_IMG_SIZE_AFT_SR, (DSS_HEIGHT(pinfo->yres) << 16) | DSS_WIDTH(pinfo->xres));

#ifdef CONFIG_DPU_FB_DPP_COLORBAR_USED
	outp32(dpp_base + DPP_CLRBAR_CTRL, (0x30 << 24) | (0 << 1) | 0x1);
	set_reg(dpp_base + DPP_CLRBAR_1ST_CLR, 0xFF, 8, 16); /* Red */
	set_reg(dpp_base + DPP_CLRBAR_2ND_CLR, 0xFF, 8, 8); /* Green */
	set_reg(dpp_base + DPP_CLRBAR_3RD_CLR, 0xFF, 8, 0); /* Blue */
#endif
}

void init_ifbc(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
}

void init_post_scf(struct dpu_fb_data_type *dpufd)
{
	char __iomem *scf_lut_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}
	scf_lut_base = dpufd->dss_base + DSS_POST_SCF_LUT_OFFSET;

	if (!DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_POST_SCF))
		return;

	dpu_post_scl_load_filter_coef(dpufd, false, scf_lut_base, SCL_COEF_RGB_IDX);
}

static void init_dfs_ram_data(struct dpu_fb_data_type *dpufd, struct dfs_ram_info *dfs_ram_data)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (pinfo->xres * pinfo->yres >= RES_4K_PHONE)
			dfs_ram_data->dfs_time_min = DFS_TIME_MIN_4K;
		else
			dfs_ram_data->dfs_time_min = DFS_TIME_MIN;
		dfs_ram_data->dfs_time = DFS_TIME;
		dfs_ram_data->depth = DBUF0_DEPTH;
	} else {
		dfs_ram_data->dfs_time = DFS_TIME;
		dfs_ram_data->dfs_time_min = DFS_TIME_MIN;
		dfs_ram_data->depth = DBUF1_DEPTH;
	}
}

static void init_dbuf_calculate_thd(struct dpu_fb_data_type *dpufd,
	struct dfs_ram_info *dfs_ram_data, struct thd *thd_datas)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);

	if (pinfo->pxl_clk_rate_div <= 0)
		pinfo->pxl_clk_rate_div = 1;
	/* The follow code from chip protocol, It contains lots of fixed numbers */
	thd_datas->cg_out = (dfs_ram_data->dfs_time * pinfo->pxl_clk_rate * pinfo->xres) /
		(((pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch + pinfo->ldi.h_front_porch) *
		pinfo->pxl_clk_rate_div + pinfo->xres) * 6 * 1000000UL);
	dfs_ram_data->sram_valid_num = thd_datas->cg_out / dfs_ram_data->depth;
	thd_datas->cg_in = (dfs_ram_data->sram_valid_num + 1) * dfs_ram_data->depth - 1;

	dfs_ram_data->sram_max_mem_depth = (dfs_ram_data->sram_valid_num + 1) * dfs_ram_data->depth;

	thd_datas->rqos_in = thd_datas->cg_out * 85 / 100;
	thd_datas->rqos_out = thd_datas->cg_out;
	thd_datas->flux_req_befdfs_in = GET_FLUX_REQ_IN(dfs_ram_data->sram_max_mem_depth);
	thd_datas->flux_req_befdfs_out = GET_FLUX_REQ_OUT(dfs_ram_data->sram_max_mem_depth);

	dfs_ram_data->sram_min_support_depth = dfs_ram_data->dfs_time_min * pinfo->xres *
		pinfo->pxl_clk_rate_div / (1000000 / 60 / (pinfo->yres + pinfo->ldi.v_back_porch +
		pinfo->ldi.v_front_porch + pinfo->ldi.v_pulse_width) * (DBUF_WIDTH_BIT / 3 / BITS_PER_BYTE));

	thd_datas->flux_req_aftdfs_in = (dfs_ram_data->sram_max_mem_depth - dfs_ram_data->sram_min_support_depth) / 3;
	thd_datas->flux_req_aftdfs_out = 2 * (dfs_ram_data->sram_max_mem_depth -
		dfs_ram_data->sram_min_support_depth) / 3;

	thd_datas->dfs_ok = thd_datas->flux_req_befdfs_in;

	DPU_FB_DEBUG("sram_valid_num=%d,\n"
		"thd_rqos_in=0x%x\n"
		"thd_rqos_out=0x%x\n"
		"thd_cg_in=0x%x\n"
		"thd_cg_out=0x%x\n"
		"thd_flux_req_befdfs_in=0x%x\n"
		"thd_flux_req_befdfs_out=0x%x\n"
		"thd_flux_req_aftdfs_in=0x%x\n"
		"thd_flux_req_aftdfs_out=0x%x\n"
		"thd_dfs_ok=0x%x\n",
		dfs_ram_data->sram_valid_num,
		thd_datas->rqos_in,
		thd_datas->rqos_out,
		thd_datas->cg_in,
		thd_datas->cg_out,
		thd_datas->flux_req_befdfs_in,
		thd_datas->flux_req_befdfs_out,
		thd_datas->flux_req_aftdfs_in,
		thd_datas->flux_req_aftdfs_out,
		thd_datas->dfs_ok);
}

static void init_dbuf_reset_thd(struct dpu_fb_data_type *dpufd,
	struct dfs_ram_info *dfs_ram_data, struct thd *thd_datas)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);

	if (g_fpga_flag == 1) {
		thd_datas->flux_req_befdfs_out = 0xacf;
		thd_datas->flux_req_befdfs_in = 0x734;
		thd_datas->flux_req_aftdfs_out = 0x4dc;
		thd_datas->flux_req_aftdfs_in = 0x26e;
		thd_datas->dfs_ok = 0x960;
		dfs_ram_data->dfs_ok_mask = 0;
		thd_datas->rqos_out = 0x9c0;
		thd_datas->rqos_in = 0x898;
		thd_datas->cg_out = 0x9c0;
		thd_datas->cg_in = 0x1780;
	} else {
		dfs_ram_data->sram_valid_num = 0;
		thd_datas->cg_out = 0x9d8;
		thd_datas->cg_in = 0xaef;
		thd_datas->rqos_in = 0x8c0;
		thd_datas->rqos_out = 0x9d8;
		thd_datas->flux_req_befdfs_in = 0x7a8;
		thd_datas->flux_req_befdfs_out = 0x8c0;
		thd_datas->flux_req_aftdfs_in = 0x7a8;
		thd_datas->flux_req_aftdfs_out = 0x8c0;
		thd_datas->dfs_ok = 0x7a8;
	}
}

static void set_dbuf_reg(struct dpu_fb_data_type *dpufd, struct dfs_ram_info dfs_ram_data,
	struct thd thd_datas, char __iomem *dbuf_base)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);

	outp32(dbuf_base + DBUF_FRM_SIZE, pinfo->xres * pinfo->yres);
	outp32(dbuf_base + DBUF_FRM_HSIZE, DSS_WIDTH(pinfo->xres));
	outp32(dbuf_base + DBUF_SRAM_VALID_NUM, dfs_ram_data.sram_valid_num); /* 0x0 */
	outp32(dbuf_base + DBUF_THD_RQOS, (thd_datas.rqos_out << 16) | thd_datas.rqos_in); /* 0x9d8 0x8c0 */
	outp32(dbuf_base + DBUF_THD_WQOS, (thd_datas.wqos_out << 16) | thd_datas.wqos_in); /* 0,0 */
	outp32(dbuf_base + DBUF_THD_CG, (thd_datas.cg_out << 16) | thd_datas.cg_in); /* 0x9d8 0xaef */
	outp32(dbuf_base + DBUF_THD_OTHER, (thd_datas.cg_hold << 16) | thd_datas.wr_wait); /* 0 0 */
	outp32(dbuf_base + DBUF_THD_FLUX_REQ_BEF, (thd_datas.flux_req_befdfs_out << 16) |
		thd_datas.flux_req_befdfs_in); /* 0x8c0 0x7a8 */
	outp32(dbuf_base + DBUF_THD_FLUX_REQ_AFT, (thd_datas.flux_req_aftdfs_out << 16) |
		thd_datas.flux_req_aftdfs_in); /* 0x8c0 0x7a8 */
	outp32(dbuf_base + DBUF_THD_DFS_OK, thd_datas.dfs_ok); /* 0x7a8 */
	outp32(dbuf_base + DBUF_FLUX_REQ_CTRL, (dfs_ram_data.dfs_ok_mask << 1) | thd_datas.flux_req_sw_en); /* 0 0 */

	outp32(dbuf_base + DBUF_THD_RQOS_IDLE, 0); /* 0x0 */

	outp32(dbuf_base + DBUF_DFS_LP_CTRL, 0x1);

	DPU_FB_DEBUG("-.!\n");
}

void init_dbuf(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dbuf_base = NULL;
	struct dpu_panel_info *pinfo = NULL;
	struct thd thd_datas = {0};
	struct dfs_ram_info dfs_ram_data = {0};

	dpu_check_and_no_retval((!dpufd), ERR, "dpufd is NULL.\n");
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dbuf_base = dpufd->dss_base + DSS_DBUF0_OFFSET;
		if (!DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_DBUF))
			return;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		dbuf_base = dpufd->dss_base + DSS_DBUF1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
		return;
	}

	init_dfs_ram_data(dpufd, &dfs_ram_data);
	init_dbuf_calculate_thd(dpufd, &dfs_ram_data, &thd_datas);
	init_dbuf_reset_thd(dpufd, &dfs_ram_data, &thd_datas);
	set_dbuf_reg(dpufd, dfs_ram_data, thd_datas, dbuf_base);
}

static void init_ldi_pxl_div(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *ldi_base = NULL;
	uint32_t ifbc_type;
	uint32_t mipi_idx;
	uint32_t pxl0_div2_gt_en;
	uint32_t pxl0_div4_gt_en;
	uint32_t pxl0_divxcfg;
	uint32_t pxl0_dsi_gt_en;

	pinfo = &(dpufd->panel_info);

	if (dpufd->index == EXTERNAL_PANEL_IDX)
		return;

	ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;

	ifbc_type = pinfo->ifbc_type;
	if (ifbc_type >= IFBC_TYPE_MAX) {
		DPU_FB_ERR("ifbc_type is invalid.\n");
		return;
	}

	mipi_idx = is_dual_mipi_panel(dpufd) ? 1 : 0;

	pxl0_div2_gt_en = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_div2_gt_en;
	pxl0_div4_gt_en = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_div4_gt_en;
	pxl0_divxcfg = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_divxcfg;
	pxl0_dsi_gt_en = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_dsi_gt_en;

	set_reg(ldi_base + LDI_PXL0_DIV2_GT_EN, pxl0_div2_gt_en, 1, 0);
	set_reg(ldi_base + LDI_PXL0_DIV4_GT_EN, pxl0_div4_gt_en, 1, 0);
	set_reg(ldi_base + LDI_PXL0_GT_EN, 0x1, 1, 0);
	set_reg(ldi_base + LDI_PXL0_DSI_GT_EN, pxl0_dsi_gt_en, 2, 0);
	set_reg(ldi_base + LDI_PXL0_DIVXCFG, pxl0_divxcfg, 3, 0);
}

static void ldi_init_rect(struct dpu_fb_data_type *dpufd, dss_rect_t *rect)
{
	rect->x = 0;
	rect->y = 0;
	rect->w = dpufd->panel_info.xres;
	rect->h = dpufd->panel_info.yres;
}

static void dual_mipi_panel_init_ldi(struct dpu_fb_data_type *dpufd, char __iomem *ldi_base, dss_rect_t rect)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);
	if (is_dual_mipi_panel(dpufd)) {
		set_reg(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_DSI1_RST_SEL, 0x0, 1, 0);
		set_reg(dpufd->dss_base + DSS_MCTRL_SYS_OFFSET + MCTL_DSI_MUX_SEL, 0x0, 1, 0);

		if (is_mipi_video_panel(dpufd)) {
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL0, (pinfo->ldi.h_back_porch +
				DSS_WIDTH(pinfo->ldi.h_pulse_width)) << 16);
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL1, 0);
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL2, DSS_WIDTH(rect.w));
		} else {
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL0, pinfo->ldi.h_back_porch << 16);
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL1, DSS_WIDTH(pinfo->ldi.h_pulse_width));
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL2, DSS_WIDTH(rect.w));
		}

		outp32(ldi_base + LDI_OVERLAP_SIZE,
			pinfo->ldi.dpi0_overlap_size | (pinfo->ldi.dpi1_overlap_size << 16));

		/* dual_mode_en */
		set_reg(ldi_base + LDI_CTRL, 1, 1, 5);

		/* split mode */
		set_reg(ldi_base + LDI_CTRL, 0, 1, 16);

		/* dual lcd: 0x1, dual mipi: 0x0 */
		set_reg(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_DSI1_CLK_SEL, 0x0, 1, 0);
	}
}

static void init_ldi_timing(struct dpu_fb_data_type *dpufd, char __iomem *ldi_base, dss_rect_t rect)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);
	if (is_mipi_video_panel(dpufd)) {
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL0,
			pinfo->ldi.h_front_porch | ((pinfo->ldi.h_back_porch +
			DSS_WIDTH(pinfo->ldi.h_pulse_width)) << 16));
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL1, 0);
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL2, DSS_WIDTH(rect.w));
	} else {
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL0,
			pinfo->ldi.h_front_porch | (pinfo->ldi.h_back_porch << 16));
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL1, DSS_WIDTH(pinfo->ldi.h_pulse_width));
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL2, DSS_WIDTH(rect.w));
	}
	outp32(ldi_base + LDI_VRT_CTRL0,
		pinfo->ldi.v_front_porch | (pinfo->ldi.v_back_porch << 16));
	outp32(ldi_base + LDI_VRT_CTRL1, DSS_HEIGHT(pinfo->ldi.v_pulse_width));
	outp32(ldi_base + LDI_VRT_CTRL2, DSS_HEIGHT(rect.h));

	outp32(ldi_base + LDI_PLR_CTRL,
		pinfo->ldi.vsync_plr | (pinfo->ldi.hsync_plr << 1) |
		(pinfo->ldi.pixelclk_plr << 2) | (pinfo->ldi.data_en_plr << 3));

	/* bpp */
	set_reg(ldi_base + LDI_CTRL, pinfo->bpp, 2, 3);
	/* bgr */
	set_reg(ldi_base + LDI_CTRL, pinfo->bgr_fmt, 1, 13);
}

static void init_ldi_te(struct dpu_fb_data_type *dpufd, char __iomem *ldi_base)
{
	uint32_t te_source;
	struct dpu_panel_info *pinfo = NULL;

	pinfo = &(dpufd->panel_info);

	if (is_mipi_cmd_panel(dpufd)) {
		set_reg(ldi_base + LDI_DSI_CMD_MOD_CTRL, 0x1, 2, 0);
		/*
		 * DSI_TE_CTRL
		 * te_source = 0, select te_pin
		 * te_source = 1, select te_triger
		 */
		te_source = 0;
		/* dsi_te_hard_en */
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x1, 1, 0);
		/* dsi_te0_pin_p , dsi_te1_pin_p */
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 2, 1);
		/* dsi_te_hard_sel */
		set_reg(ldi_base + LDI_DSI_TE_CTRL, te_source, 1, 3);
		/* select TE0 PIN */
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x01, 2, 6);
		/* dsi_te_mask_en */
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 1, 8);
		/* dsi_te_mask_dis */
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 4, 9);
		/* dsi_te_mask_und */
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 4, 13);
		/* dsi_te_pin_en */
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x1, 1, 17);
		set_reg(ldi_base + LDI_DSI_TE_HS_NUM, 0x0, 32, 0);
		set_reg(ldi_base + LDI_DSI_TE_HS_WD, 0x24024, 32, 0);

		if (pinfo->pxl_clk_rate_div == 0) {
			DPU_FB_ERR("pxl_clk_rate_div is NULL, not support !\n");
			pinfo->pxl_clk_rate_div = 1;
		}
		set_reg(ldi_base + LDI_DSI_TE_VS_WD,
			(0x3FC << 12) | (2 * pinfo->pxl_clk_rate / pinfo->pxl_clk_rate_div / 1000000), 32, 0);
	} else {
		/* dsi_te_hard_en */
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 1, 0);
		set_reg(ldi_base + LDI_DSI_CMD_MOD_CTRL, 0x2, 2, 0);
	}
}

static void init_ldi_colorbar_used(char __iomem *ldi_base)
{
#ifdef CONFIG_DPU_FB_LDI_COLORBAR_USED
	/* colorbar width */
	set_reg(ldi_base + LDI_CTRL, DSS_WIDTH(0x3c), 7, 6);
	/* colorbar ort */
	set_reg(ldi_base + LDI_WORK_MODE, 0x0, 1, 1);
	/* colorbar enable */
	set_reg(ldi_base + LDI_WORK_MODE, 0x0, 1, 0);
#else
	/* normal */
	set_reg(ldi_base + LDI_WORK_MODE, 0x1, 1, 0);
#endif
}

void init_ldi(struct dpu_fb_data_type *dpufd, bool fastboot_enable)
{
	char __iomem *ldi_base = NULL;
	struct dpu_panel_info *pinfo = NULL;
	dss_rect_t rect = {0};

	dpu_check_and_no_retval((!dpufd), ERR, "dpufd is NULL.\n");
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
		if (g_fpga_flag == 1)
			set_reg(dpufd->dss_base + GLB_TP_SEL, 0x2, 2, 0);
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
		return;
	}

	ldi_init_rect(dpufd, &rect);

	mipi_ifbc_get_rect(dpufd, &rect);

	init_ldi_pxl_div(dpufd);

	dual_mipi_panel_init_ldi(dpufd, ldi_base, rect);
	init_ldi_timing(dpufd, ldi_base, rect);

	/* for ddr pmqos */
	outp32(ldi_base + LDI_VINACT_MSK_LEN,
		pinfo->ldi.v_front_porch);

	/* cmd event sel */
	outp32(ldi_base + LDI_CMD_EVENT_SEL, 0x1);

	/* for 1Hz LCD and mipi command LCD */
	init_ldi_te(dpufd, ldi_base);

	init_ldi_colorbar_used(ldi_base);

	if (is_mipi_cmd_panel(dpufd))
		set_reg(ldi_base + LDI_FRM_MSK, (dpufd->frame_update_flag == 1) ? 0x0 : 0x1, 1, 0);

	if (dpufd->index == EXTERNAL_PANEL_IDX && (is_mipi_panel(dpufd)))
		set_reg(ldi_base + LDI_DP_DSI_SEL, 0x1, 1, 0);

	/* ldi disable */
	if (!fastboot_enable)
		set_reg(ldi_base + LDI_CTRL, 0x0, 1, 0);

	DPU_FB_DEBUG("-.!\n");
}

void deinit_ldi(struct dpu_fb_data_type *dpufd)
{
	char __iomem *ldi_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
		return;
	}

	set_reg(ldi_base + LDI_CTRL, 0, 1, 0);
}

void enable_ldi(struct dpu_fb_data_type *dpufd)
{
	char __iomem *ldi_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
		return;
	}

	/* ldi enable */
	set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
}

void disable_ldi(struct dpu_fb_data_type *dpufd)
{
	char __iomem *ldi_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return;
	}

	/* ldi disable */
	set_reg(ldi_base + LDI_CTRL, 0x0, 1, 0);
}

void ldi_frame_update(struct dpu_fb_data_type *dpufd, bool update)
{
	char __iomem *ldi_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;

		if (is_mipi_cmd_panel(dpufd)) {
			set_reg(ldi_base + LDI_FRM_MSK, (update ? 0x0 : 0x1), 1, 0);
			if (update)
				set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		}
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
	}
}

void single_frame_update(struct dpu_fb_data_type *dpufd)
{
	char __iomem *ldi_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
		if (is_mipi_cmd_panel(dpufd)) {
			set_reg(ldi_base + LDI_FRM_MSK_UP, 0x1, 1, 0);
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		} else {
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		}

	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;

		if (is_mipi_cmd_panel(dpufd)) {
			set_reg(ldi_base + LDI_FRM_MSK_UP, 0x1, 1, 0);
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		} else {
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		}
	} else {
		;
	}
}

void dpe_interrupt_clear(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dss_base = NULL;
	uint32_t clear;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL.\n");
		return;
	}

	dss_base = dpufd->dss_base;

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		clear = ~0;
		outp32(dss_base + GLB_CPU_PDP_INTS, clear);
		outp32(dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS, clear);
		outp32(dss_base + DSS_DPP_OFFSET + DPP_INTS, clear);

		outp32(dss_base + DSS_DBG_OFFSET + DBG_MCTL_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_WCH0_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH0_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH1_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH4_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH5_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH6_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH7_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_DSS_GLB_INTS, clear);
	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		clear = ~0;
		outp32(dss_base + GLB_CPU_OFF_INTS, clear);
	} else {
		DPU_FB_ERR("fb%d, not support this device!\n", dpufd->index);
	}

}

static void primary_panel_dpe_interrupt_unmask(struct dpu_fb_data_type *dpufd,
		struct dpu_panel_info *pinfo, char __iomem *dss_base)
{
	uint32_t unmask;

	unmask = ~0;
	unmask &= ~(BIT_DPP_INTS | BIT_ITF0_INTS | BIT_MMU_IRPT_NS);
	outp32(dss_base + GLB_CPU_PDP_INT_MSK, unmask);

	unmask = ~0;
	if (is_mipi_cmd_panel(dpufd))
		unmask &= ~(BIT_LCD_TE0_PIN | BIT_VACTIVE0_START | BIT_VACTIVE0_END | BIT_FRM_END);
	else
		unmask &= ~(BIT_VSYNC | BIT_VACTIVE0_START | BIT_VACTIVE0_END | BIT_FRM_END);
	outp32(dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, unmask);

	unmask = ~0;
	if ((pinfo->acm_ce_support == 1) && (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACE) != 0))
		unmask &= ~(BIT_CE_END_IND);

	if (pinfo->hiace_support == 1)
		unmask &= ~(BIT_HIACE_IND);

	outp32(dss_base + DSS_DPP_OFFSET + DPP_INT_MSK, unmask);
}

void dpe_interrupt_unmask(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dss_base = NULL;
	uint32_t unmask;
	struct dpu_panel_info *pinfo = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	dss_base = dpufd->dss_base;

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		primary_panel_dpe_interrupt_unmask(dpufd, pinfo, dss_base);
	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		unmask = ~0;
		unmask &= ~(BIT_OFF_WCH0_INTS | BIT_OFF_WCH1_INTS |
			BIT_OFF_WCH0_WCH1_FRM_END_INT | BIT_OFF_MMU_IRPT_NS);
		outp32(dss_base + GLB_CPU_OFF_INT_MSK, unmask);
		unmask = ~0;
		unmask &= ~(BIT_OFF_CAM_WCH2_FRMEND_INTS);
		outp32(dss_base + GLB_CPU_OFF_CAM_INT_MSK, unmask);
	} else {
		DPU_FB_ERR("fb%d, not support this device!\n", dpufd->index);
	}

}

void dpe_interrupt_mask(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dss_base = NULL;
	uint32_t mask;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return;
	}

	dss_base = dpufd->dss_base;

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		mask = ~0;
		outp32(dss_base + GLB_CPU_PDP_INT_MSK, mask);
		outp32(dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, mask);
		outp32(dss_base + DSS_DPP_OFFSET + DPP_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_DSS_GLB_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_MCTL_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_WCH0_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH0_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH1_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH4_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH5_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH6_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH7_INT_MSK, mask);
	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		mask = ~0;
		outp32(dss_base + GLB_CPU_OFF_INT_MSK, mask);
		outp32(dss_base + GLB_CPU_OFF_CAM_INT_MSK, mask);
	} else {
		DPU_FB_ERR("fb%d, not support this device!\n", dpufd->index);
	}

}
/*lint +e838 */
void ldi_data_gate(struct dpu_fb_data_type *dpufd, bool enble)
{
	char __iomem *ldi_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	if (!is_mipi_cmd_panel(dpufd)) {
		dpufd->ldi_data_gate_en = (enble ? 1 : 0);
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
		return;
	}

	if (g_ldi_data_gate_en == 1) {
		dpufd->ldi_data_gate_en = (enble ? 1 : 0);
		set_reg(ldi_base + LDI_CTRL, (enble ? 0x1 : 0x0), 1, 2);
	} else {
		dpufd->ldi_data_gate_en = 0;
		set_reg(ldi_base + LDI_CTRL, 0x0, 1, 2);
	}

	DPU_FB_DEBUG("ldi_data_gate_en=%d!\n", dpufd->ldi_data_gate_en);
}

/* dpp csc not surport */
void init_dpp_csc(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
}

void init_acm(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
}

static void init_dither_hifreq(char __iomem *dither_base)
{
	set_reg(dither_base + DITHER_HIFREQ_REG_INI_CFG_EN, 0x00000001, 1, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI0_0, 0x6495FC13, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI0_1, 0x27E5DB75, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI0_2, 0x69036280, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI0_3, 0x7478D47C, 31, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI1_0, 0x36F5325D, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI1_1, 0x90757906, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI1_2, 0xBBA85F01, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI1_3, 0x74B34461, 31, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI2_0, 0x76435C64, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI2_1, 0x4989003F, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI2_2, 0xA2EA34C6, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_REG_INI2_3, 0x4CAD42CB, 31, 0);
	set_reg(dither_base + DITHER_HIFREQ_POWER_CTRL, 0x00000009, 4, 0);
	set_reg(dither_base + DITHER_HIFREQ_FILT_0, 0x00000421, 15, 0);
	set_reg(dither_base + DITHER_HIFREQ_FILT_1, 0x00000701, 15, 0);
	set_reg(dither_base + DITHER_HIFREQ_FILT_2, 0x00000421, 15, 0);
	set_reg(dither_base + DITHER_HIFREQ_THD_R0, 0x6D92B7DB, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_THD_R1, 0x00002448, 16, 0);
	set_reg(dither_base + DITHER_HIFREQ_THD_G0, 0x6D92B7DB, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_THD_G1, 0x00002448, 16, 0);
	set_reg(dither_base + DITHER_HIFREQ_THD_B0, 0x6D92B7DB, 32, 0);
	set_reg(dither_base + DITHER_HIFREQ_THD_B1, 0x00002448, 16, 0);
}

void init_dither(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *dither_base = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return;
	}

	pinfo = &(dpufd->panel_info);

	if (pinfo->dither_support != 1)
		return;

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dither_base = dpufd->dss_base + DSS_DPP_DITHER_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
		return;
	}

	set_reg(dither_base + DITHER_CTL1, 0x00000005, 6, 0);
	set_reg(dither_base + DITHER_CTL0, 0x0000000B, 5, 0);
	set_reg(dither_base + DITHER_TRI_THD12_0, 0x00080080, 24, 0);
	set_reg(dither_base + DITHER_TRI_THD12_1, 0x00000080, 12, 0);
	set_reg(dither_base + DITHER_TRI_THD10, 0x02008020, 30, 0);
	set_reg(dither_base + DITHER_TRI_THD12_UNI_0, 0x00100100, 24, 0);
	set_reg(dither_base + DITHER_TRI_THD12_UNI_1, 0x00000000, 12, 0);
	set_reg(dither_base + DITHER_TRI_THD10_UNI, 0x00010040, 30, 0);
	set_reg(dither_base + DITHER_BAYER_CTL, 0x00000000, 28, 0);
	set_reg(dither_base + DITHER_BAYER_ALPHA_THD, 0x00000000, 30, 0);
	set_reg(dither_base + DITHER_MATRIX_PART1, 0x5D7F91B3, 32, 0);
	set_reg(dither_base + DITHER_MATRIX_PART0, 0x6E4CA280, 32, 0);

	init_dither_hifreq(dither_base);
	set_reg(dither_base + DITHER_ERRDIFF_CTL, 0x00000000, 3, 0);
	set_reg(dither_base + DITHER_ERRDIFF_WEIGHT, 0x01232134, 28, 0);

	set_reg(dither_base + DITHER_FRC_CTL, 0x00000001, 4, 0);
	set_reg(dither_base + DITHER_FRC_01_PART1, 0xFFFF0000, 32, 0);
	set_reg(dither_base + DITHER_FRC_01_PART0, 0x00000000, 32, 0);
	set_reg(dither_base + DITHER_FRC_10_PART1, 0xFFFFFFFF, 32, 0);
	set_reg(dither_base + DITHER_FRC_10_PART0, 0x00000000, 32, 0);
	set_reg(dither_base + DITHER_FRC_11_PART1, 0xFFFFFFFF, 32, 0);
	set_reg(dither_base + DITHER_FRC_11_PART0, 0xFFFF0000, 32, 0);
}
void deinit_dbuf(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
}

