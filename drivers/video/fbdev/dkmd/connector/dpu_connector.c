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

#include "dpu_conn_mgr.h"
#include "hdmitx_ctrl_dev.h"

static uint32_t g_connector_offset[PIPE_SW_POST_CH_MAX] = {
	DPU_MIPI_DSI0_OFFSET,
	DPU_MIPI_DSI1_OFFSET,
	DPU_MIPI_DSI2_OFFSET,
	0, /* dp0 offset */
	0, /* dp1 offset */
	0, /* offline offset */
	0,
};

void dpu_connector_setup(struct dpu_connector *connector)
{
	if (!connector) {
		dpu_pr_err("connector is nullptr!");
		return;
	}

	switch (connector->pipe_sw_post_chn_idx) {
	case PIPE_SW_POST_CH_DSI0:
	case PIPE_SW_POST_CH_DSI1:
	case PIPE_SW_POST_CH_DSI2:
		mipi_dsi_default_setup(connector);
		break;
	case PIPE_SW_POST_CH_EDP:
	case PIPE_SW_POST_CH_DP:
		dp_ctrl_default_setup(connector);
		break;
	case PIPE_SW_POST_CH_OFFLINE:
		dpu_offline_default_setup(connector);
		break;
	case PIPE_SW_POST_CH_HDMI:
		hdmitx_ctrl_default_setup(connector);
		break;
	default:
		dpu_pr_err("invalid pipe_sw post channel!");
		return;
	}
	connector->connector_base = connector->dpu_base + g_connector_offset[connector->pipe_sw_post_chn_idx];
	connector->dpp_base = connector->dpu_base + DPU_DPP0_OFFSET;
	connector->dsc_base = connector->dpu_base + DPU_DSC0_OFFSET;

	/* for dual-mipi or cphy 1+1 */
	if (connector->bind_connector)
		connector->bind_connector->connector_base = connector->dpu_base +
			g_connector_offset[connector->bind_connector->pipe_sw_post_chn_idx];
}