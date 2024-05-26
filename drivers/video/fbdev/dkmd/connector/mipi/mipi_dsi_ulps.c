/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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

#include <linux/module.h>
#include <dpu/soc_dpu_define.h>
#include "dkmd_log.h"
#include "dpu_connector.h"
#include "mipi_dsi_dev.h"

static void mipi_dsi_get_ulps_stopstate(struct dpu_connector *connector, uint32_t *cmp_ulpsactivenot_val,
	uint32_t *cmp_stopstate_val, bool is_ulps)
{
	if (is_ulps) {
		if (connector->mipi.lane_nums >= DSI_4_LANES) {
			*cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10) | BIT(12));
			*cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9) | BIT(11));
		} else if (connector->mipi.lane_nums >= DSI_3_LANES) {
			*cmp_ulpsactivenot_val = (BIT(5) | BIT(8) | BIT(10));
			*cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9));
		} else if (connector->mipi.lane_nums >= DSI_2_LANES) {
			*cmp_ulpsactivenot_val = (BIT(5) | BIT(8));
			*cmp_stopstate_val = (BIT(4) | BIT(7));
		} else {
			*cmp_ulpsactivenot_val = (BIT(5));
			*cmp_stopstate_val = (BIT(4));
		}
	} else { /* ulps exit */
		if (connector->mipi.lane_nums >= DSI_4_LANES) {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8) | BIT(10) | BIT(12));
			*cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7) | BIT(9) | BIT(11));
		} else if (connector->mipi.lane_nums >= DSI_3_LANES) {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8) | BIT(10));
			*cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7) | BIT(9));
		} else if (connector->mipi.lane_nums >= DSI_2_LANES) {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5) | BIT(8));
			*cmp_stopstate_val = (BIT(2) | BIT(4) | BIT(7));
		} else {
			*cmp_ulpsactivenot_val = (BIT(3) | BIT(5));
			*cmp_stopstate_val = (BIT(2) | BIT(4));
		}
		if (connector->mipi.phy_mode == CPHY_MODE) {
			*cmp_ulpsactivenot_val &= ~(BIT(3));
			*cmp_stopstate_val &= ~(BIT(2));
		}
	}
}

static int mipi_dsi_check_ulps_stopstate(struct dpu_connector *connector, char __iomem *mipi_dsi_base,
	uint32_t cmp_stopstate_val, bool is_ulps)
{
	uint32_t try_times = 0;
	uint32_t temp;

	/* check DPHY data and clock lane stopstate */
	temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));
	while ((temp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(10);
		if (++try_times > 100) {  /* try 1ms */
			dpu_pr_info("dsi%d, check phy data and clk lane stop state failed! "
				"PHY_STATUS=0x%x", connector->conn_info->sw_post_chn_idx[0], temp);
			return -1;
		}
		if (((temp & cmp_stopstate_val) == (cmp_stopstate_val & ~BIT(2))) && is_ulps) {
			dpu_pr_info("dsi%d, datalanes are in stop state, pull down phy_txrequestclkhs",
				connector->conn_info->sw_post_chn_idx[0]);
			set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 1);
			set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);
		}
		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));
	}

	return 0;
}

static void mipi_dsi_data_clock_lane_enter_ulps(struct dpu_connector *connector, char __iomem *mipi_dsi_base,
	uint32_t cmp_ulpsactivenot_val)
{
	uint32_t try_times = 0;
	uint32_t temp;

	/* request data lane enter ULPS */
	set_reg(DPU_DSI_CDPHY_ULPS_CTRL_ADDR(mipi_dsi_base), 0x4, 4, 0);

	/* check DPHY data lane ulpsactivenot_status */
	temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));
	while ((temp & cmp_ulpsactivenot_val) != 0) {
		udelay(10);  /* 10us */
		if (++try_times > 100) {  /* try 1ms */
			dpu_pr_info("dsi%d, request phy data lane enter ulps failed! "
				"PHY_STATUS=0x%x.", connector->conn_info->sw_post_chn_idx[0], temp);
			break;
		}
		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));
	}

	if (connector->mipi.phy_mode != DPHY_MODE)
		return;

	/* request clock lane enter ULPS */
	set_reg(DPU_DSI_CDPHY_ULPS_CTRL_ADDR(mipi_dsi_base), 0x5, 4, 0);

	/* check DPHY clock lane ulpsactivenot_status */
	try_times = 0;
	temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));
	while ((temp & BIT(3)) != 0) {
		udelay(10); /* 10us */
		if (++try_times > 100) { /* try 1ms */
			dpu_pr_info("dsi%d, request phy clk lane enter ulps failed! "
				"PHY_STATUS=0x%x", connector->conn_info->sw_post_chn_idx[0], temp);
			break;
		}
		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));
	}
}

static int mipi_dsi_ulps_enter(struct dpu_connector *connector)
{
	char __iomem *mipi_dsi_base = connector->connector_base;
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	bool is_ready = false;
	uint32_t temp;

	dpu_pr_info("dsi%d, %s +!", connector->conn_info->sw_post_chn_idx[0], __func__);

	mipi_dsi_get_ulps_stopstate(connector, &cmp_ulpsactivenot_val, &cmp_stopstate_val, true);

	temp = (uint32_t)inp32(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base)) & BIT(1);
	if (temp && (connector->mipi.phy_mode == DPHY_MODE))
		cmp_stopstate_val |= BIT(2);

	if (mipi_dsi_check_ulps_stopstate(connector, mipi_dsi_base, cmp_stopstate_val, true) != 0)
		return 0;

	/* disable DPHY clock lane's Hight Speed Clock */
	set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);

	/* force_pll = 0 */
	set_reg(DPU_DSI_CDPHY_RST_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 3);

	mipi_dsi_data_clock_lane_enter_ulps(connector, mipi_dsi_base, cmp_ulpsactivenot_val);

	/* check phy lock == 0? */
	is_ready = mipi_phy_status_check(mipi_dsi_base, 0x0);
	if (!is_ready)
		dpu_pr_info("dsi%d, phylock == 1!", connector->conn_info->sw_post_chn_idx[0]);

	/* bit13 lock sel enable (dual_mipi_panel mipi_dsi1_base+bit13 set 1), close clock gate */
	set_reg(DPU_DSI_DPHYTX_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 13);

	dpu_pr_info("dsi%d, %s -!", connector->conn_info->sw_post_chn_idx[0], __func__);

	return 0;
}

static void mipi_dsi_data_clock_lane_exit_ulps(struct dpu_connector *connector, char __iomem *mipi_dsi_base,
	uint32_t cmp_ulpsactivenot_val)
{
	uint32_t try_times = 0;
	uint32_t temp;

	/* request that data lane and clock lane exit ULPS */
	outp32(DPU_DSI_CDPHY_ULPS_CTRL_ADDR(mipi_dsi_base), 0xF);

	temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));
	while ((temp & cmp_ulpsactivenot_val) != cmp_ulpsactivenot_val) {
		udelay(10);  /* delay 10us */
		if (++try_times > 100) {  /* try 1ms */
			dpu_pr_info("dsi%d, request data clock lane exit ulps fail! "
				"PHY_STATUS=0x%x", connector->conn_info->sw_post_chn_idx[0], temp);
			break;
		}
		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(mipi_dsi_base));
	}
}

static int mipi_dsi_ulps_exit(struct dpu_connector *connector)
{
	char __iomem *mipi_dsi_base = connector->connector_base;
	uint32_t cmp_ulpsactivenot_val = 0;
	uint32_t cmp_stopstate_val = 0;
	bool is_ready = false;

	dpu_pr_info("dsi%d, %s +!", connector->conn_info->sw_post_chn_idx[0], __func__);

	mipi_dsi_get_ulps_stopstate(connector, &cmp_ulpsactivenot_val, &cmp_stopstate_val, false);

	/* force pll = 1 */
	set_reg(DPU_DSI_CDPHY_RST_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 3);

	udelay(100); /* wait pll clk */

	/* check phy lock == 1? */
	is_ready = mipi_phy_status_check(mipi_dsi_base, 0x1);
	if (!is_ready)
		dpu_pr_info("dsi%d, phylock == 0, phylock is not ready!", connector->conn_info->sw_post_chn_idx[0]);

	/* bit13 lock sel enable (dual_mipi_panel mipi_dsi1_base+bit13 set 1), colse clock gate */
	set_reg(DPU_DSI_DPHYTX_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 13);

	mipi_dsi_data_clock_lane_exit_ulps(connector, mipi_dsi_base, cmp_ulpsactivenot_val);

	/* mipi spec */
	mdelay(1);

	/* clear PHY_ULPS_CTRL */
	outp32(DPU_DSI_CDPHY_ULPS_CTRL_ADDR(mipi_dsi_base), 0x0);

	mipi_dsi_check_ulps_stopstate(connector, mipi_dsi_base, cmp_stopstate_val, false);

	/* enable DPHY clock lane's Hight Speed Clock */
	set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
	if (connector->mipi.non_continue_en)
		set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 1);

	/* reset dsi */
	outp32(DPU_DSI_POR_CTRL_ADDR(mipi_dsi_base), 0x0);
	udelay(5);
	/* power up dsi */
	outp32(DPU_DSI_POR_CTRL_ADDR(mipi_dsi_base), 0x1);

	dpu_pr_info("dsi%d, %s -!", connector->conn_info->sw_post_chn_idx[0], __func__);

	return 0;
}

void mipi_dsi_ulps_cfg(struct dpu_connector *connector, bool is_ulps)
{
	if (!is_mipi_cmd_panel(&connector->conn_info->base))
		return;

	if (is_ulps) {
		mipi_dsi_ulps_enter(connector);
		if (connector->bind_connector)
			mipi_dsi_ulps_enter(connector->bind_connector);
	} else {
		mipi_dsi_ulps_exit(connector);
		if (connector->bind_connector)
			mipi_dsi_ulps_exit(connector->bind_connector);
	}
}

void mipi_dsi_auto_ulps_config(struct mipi_dsi_timing *timing, struct dpu_connector *connector,
	char __iomem *mipi_dsi_base)
{
	uint32_t t_lanebyteclk;
	uint32_t twakeup_cnt;
	uint32_t auto_ulps_enter_delay;

	/* ref chip cfg: auto_ulps_enter_delay = timing->hline_time * 3 / 2; */
	auto_ulps_enter_delay = timing->hline_time * (timing->vsa + timing->vbp + timing->vfp + 2);

	/* twakeup_cnt * twakeup_clk_div * t_lanebyteclk > 1ms */
	if (connector->mipi.phy_mode == CPHY_MODE)
		t_lanebyteclk = connector->dsi_phy_ctrl.lane_word_clk;
	else
		t_lanebyteclk = connector->dsi_phy_ctrl.lane_byte_clk;
	/* twakeup_clk_div = 8, frequency division is 8 */
	twakeup_cnt = (t_lanebyteclk / 1000) * 11 / 10 / 8;

	set_reg(DPU_DSI_AUTO_ULPS_ENTRY_DELAY_ADDR(mipi_dsi_base), auto_ulps_enter_delay, 32, 0);
	/* frequency division is 8 */
	set_reg(DPU_DSI_AUTO_ULPS_WAKEUP_TIME_ADDR(mipi_dsi_base), (twakeup_cnt << 16) | 0x8, 32, 0);
}

void mipi_auto_ulps_ctrl(struct dpu_connector *connector, bool is_auto_ulps)
{
	char __iomem *mipi_dsi_base = connector->connector_base;
	struct dkmd_connector_info *pinfo = connector->conn_info;

	dpu_pr_info("dsi%u, config auto-ulps!", connector->pipe_sw_post_chn_idx);

	if (!is_mipi_cmd_panel(&pinfo->base))
		return;

	if (is_auto_ulps) {
		dpu_pr_info("dsi%u, enter auto-ulps!", connector->pipe_sw_post_chn_idx);
		set_reg(DPU_DSI_AUTO_ULPS_MODE_ADDR(mipi_dsi_base), 0x10001, 32, 0);
	} else {
		dpu_pr_info("dsi%u, exit auto-ulps!", connector->pipe_sw_post_chn_idx);
		set_reg(DPU_DSI_APB_WR_LP_HDR_ADDR(mipi_dsi_base), 0xA06, 32, 0);
		udelay(1);
		set_reg(DPU_DSI_AUTO_ULPS_MODE_ADDR(mipi_dsi_base), 0x0, 32, 0);
	}
}
