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

#include <platform_include/display/linux/dpu_dss_dp.h>
#include "dp_drv.h"
#include "dp_ctrl.h"
#include "dpu_conn_mgr.h"

static void dp_on_params_init(struct dp_ctrl *dptx)
{
	dptx->dptx_enable = true;
	dptx->detect_times = 0;
	dptx->current_link_rate = dptx->max_rate;
	dptx->current_link_lanes = dptx->max_lanes;
}

static int dp_on(struct dp_ctrl *dptx)
{
	if (dptx->video_transfer_enable) {
		dpu_pr_info("[DP] dptx has already on\n");
		return 0;
	}

	if (dptx->dp_dis_reset && (dptx->dp_dis_reset(dptx, true) != 0)) {
		dpu_pr_err("[DP] dptx dis reset failed!\n");
		return -1;
	}

	/* enable clock */
	if (clk_prepare_enable(dptx->connector->connector_clk[CLK_DPCTRL_16M]) != 0) {
		dpu_pr_err("[DP] enable dpctrl 16m clk failed!\n");
		return -1;
	}

	if (clk_prepare_enable(dptx->connector->connector_clk[CLK_DPCTRL_PCLK]) != 0) {
		dpu_pr_err("[DP] enable dpctrl pclk failed!\n");
		return -1;
	}

	if (dptx->dptx_core_on && (dptx->dptx_core_on(dptx) != 0)) {
		dpu_pr_err("[DP] dptx core on failed!\n");
		return -1;
	}
	dp_on_params_init(dptx);

	return 0;
}

static void dp_hpd_status_init(struct dp_ctrl *dptx, TYPEC_PLUG_ORIEN_E typec_orien)
{
	/* default is same source */
	dptx->user_mode = 0;
	dptx->dptx_plug_type = typec_orien;
	dptx->user_mode_format = VCEA;

	/* DP HPD event must be delayed when system is booting */
	if (!g_dpu_complete_start)
		wait_event_interruptible_timeout(dptx->dptxq, g_dpu_complete_start, msecs_to_jiffies(20000));
}

static int dp_get_lanes_mode(TCPC_MUX_CTRL_TYPE mode, uint8_t *dp_lanes)
{
	switch (mode) {
	case TCPC_DP:
		*dp_lanes = 4;
		break;
	case TCPC_USB31_AND_DP_2LINE:
		*dp_lanes = 2;
		break;
	default:
		*dp_lanes = 4;
		dpu_pr_err("[DP] not supported tcpc_mux_ctrl_type=%d\n", mode);
		return -EINVAL;
	}

	return 0;
}

static void dp_off_params_reset(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] dptx is NULL!\n");

	dptx->detect_times = 0;
	dptx->dptx_vr = false;
	dptx->dptx_enable = false;
	dptx->video_transfer_enable = false;
	dptx->dptx_plug_type = DP_PLUG_TYPE_NORMAL;
}

static int dp_off(struct dp_ctrl *dptx)
{
	int ret = 0;

	if (dptx->dptx_core_off)
		dptx->dptx_core_off(dptx);

	/* disable clk */
	clk_disable_unprepare(dptx->connector->connector_clk[CLK_DPCTRL_16M]);
	clk_disable_unprepare(dptx->connector->connector_clk[CLK_DPCTRL_PCLK]);

	if (dptx->dp_dis_reset && (dptx->dp_dis_reset(dptx, false) != 0)) {
		dpu_pr_err("[DP] DPTX dis reset failed !!!\n");
		ret = -1;
	}
	dp_off_params_reset(dptx);

	return ret;
}

static int dpu_dptx_handle_plugin(struct dp_ctrl *dptx, int mode, int typec_orien)
{
	if (dptx->dptx_enable) {
		dpu_pr_info("[DP] dptx has already connected");
		return 0;
	}

	dp_hpd_status_init(dptx, typec_orien);
	dp_get_lanes_mode(mode, &dptx->max_lanes);
	if (dp_on(dptx) != 0)
		return -EINVAL;

	mdelay(1);
	if (dptx->dptx_hpd_handler)
		dptx->dptx_hpd_handler(dptx, true, dptx->max_lanes);

	return 0;
}

static int dpu_dptx_check_power(struct dp_ctrl *dptx, int mode, int typec_orien)
{
	if (!dptx->dptx_enable) {
		dpu_pr_info("[DP] dptx has already off");
		return -EINVAL;
	}
	/* already power off, need power on first */
	if (dptx->power_saving_mode && dptx->dptx_power_handler) {
		dpu_pr_info("[DP] dptx has already off, power on first");
		dptx->dptx_power_handler(dptx, true);
		dptx->power_saving_mode = false;
	}

	dp_hpd_status_init(dptx, typec_orien);
	dp_get_lanes_mode(mode, &dptx->max_lanes);

	return 0;
}

static int dpu_dptx_handle_plugout(struct dp_ctrl *dptx, int mode, int typec_orien)
{
	if (dpu_dptx_check_power(dptx, mode, typec_orien) != 0) {
		dpu_pr_info("[DP] dptx check power error!");
		return -1;
	}

	if (dptx->dptx_hpd_handler)
		dptx->dptx_hpd_handler(dptx, false, dptx->max_lanes);

	mdelay(1);
	dp_off(dptx);

	return 0;
}

static int dpu_dptx_handle_shot_plug(struct dp_ctrl *dptx, int mode, int typec_orien)
{
	if (dpu_dptx_check_power(dptx, mode, typec_orien) != 0)
		return -1;

	if (dptx->dptx_hpd_irq_handler)
		dptx->dptx_hpd_irq_handler(dptx);

	return 0;
}

int dpu_dptx_hpd_trigger(TCA_IRQ_TYPE_E irq_type, TCPC_MUX_CTRL_TYPE mode, TYPEC_PLUG_ORIEN_E typec_orien)
{
	int ret = 0;
	struct dp_ctrl *dptx = NULL;
	struct dp_private *dp_priv = NULL;
	struct dkmd_connector_info *pinfo = NULL;

	dpu_check_and_return(!g_dkmd_dp_devive, -EINVAL, err, "[DP] dp device is null pointer!");
	pinfo = platform_get_drvdata(g_dkmd_dp_devive);

	dpu_check_and_return(!pinfo, -EINVAL, err, "[DP] get connector info failed!");
	dp_priv = to_dp_private(pinfo);

	dptx = &dp_priv->dp;
	dpu_pr_info("[DP] DP HPD Type: %d, Mode: %d, Gate: %d", irq_type, mode, g_dpu_complete_start);

	mutex_lock(&dptx->dptx_mutex);
	switch (irq_type) {
	case TCA_IRQ_HPD_IN:
		ret = dpu_dptx_handle_plugin(dptx, mode, typec_orien);
		break;
	case TCA_IRQ_HPD_OUT:
		ret = dpu_dptx_handle_plugout(dptx, mode, typec_orien);
		break;
	case TCA_IRQ_SHORT:
		ret = dpu_dptx_handle_shot_plug(dptx, mode, typec_orien);
		break;
	default:
		break;
	}
	mutex_unlock(&dptx->dptx_mutex);

	return ret;
}
EXPORT_SYMBOL(dpu_dptx_hpd_trigger);

int dpu_dptx_notify_switch(void)
{
	return 0;
}
EXPORT_SYMBOL(dpu_dptx_notify_switch);

bool dpu_dptx_ready(void)
{
	return true;
}
EXPORT_SYMBOL(dpu_dptx_ready);

int dpu_dptx_triger(bool benable)
{
	return 0;
}
EXPORT_SYMBOL(dpu_dptx_triger);