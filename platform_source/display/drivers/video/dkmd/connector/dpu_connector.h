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

#ifndef DPU_CONNECTOR_H
#define DPU_CONNECTOR_H

#include "peri/dkmd_peri.h"
#include "dkmd_isr.h"
#include "mipi_cdphy_utils.h"
#include "mipi_dsi_dev.h"
#include "dsc/dsc_config.h"
#include "spr/spr_config.h"
#include "dp_ctrl_dev.h"
#include "dpu_offline_dev.h"

/* mipi cdphy and dpctrl clk nums_max is same */
enum {
	CLK_GATE_TXDPHY_REF = 0,
	CLK_GATE_TXDPHY_CFG = 1,
	CLK_DPCTRL_16M      = 0,
	CLK_DPCTRL_PCLK     = 1,
	CLK_GATE_MAX_IDX    = 2,
};

struct dpu_connector {
	uint32_t pipe_sw_post_chn_idx;

	/* Base address register, power supply, clock is public */
	char __iomem *dpu_base;
	char __iomem *peri_crg_base;
	char __iomem *connector_base;
	char __iomem *dpp_base;
	char __iomem *dsc_base;
	char __iomem *pctrl_base;

	int32_t connector_irq;
	struct clk *connector_clk[CLK_GATE_MAX_IDX];

	/* used for dsi or composer */
	struct mipi_panel_info mipi;
	struct mipi_dsi_phy_ctrl dsi_phy_ctrl;

	struct dptx_combophy_ctrl combophy_ctrl;

	/* TODO: add other struct define such as DSC or SPR etc. */
	struct dsc_calc_info dsc;
	struct spr_info spr;
	struct ldi_panel_info ldi;

	/* cphy 1+1 or dual-mipi  */
	struct dpu_connector *bind_connector;

	/* pointer for connector which will be used for composer */
	struct dkmd_connector_info *conn_info;

	/* save composer manager pointer */
	struct dpu_conn_manager *conn_mgr;

	/* there would be null if no next device */
	int32_t (*on_func)(struct dkmd_connector_info *pinfo);
	int32_t (*off_func)(struct dkmd_connector_info *pinfo);
	int32_t (*ops_handle_func)(struct dkmd_connector_info *pinfo, uint32_t ops_cmd_id, void *value);
};

struct connector_ops_handle_data {
	uint32_t ops_cmd_id;
	int32_t (*handle_func)(struct dpu_connector *connector, const void *desc);
};

static inline int32_t dkdm_connector_hanlde_func(struct connector_ops_handle_data ops_table[], uint32_t len,
	uint32_t ops_cmd_id, struct dpu_connector *connector, void *value)
{
	uint32_t i;
	struct connector_ops_handle_data *handler = NULL;

	for (i = 0; i < len; i++) {
		handler = &(ops_table[i]);
		if ((ops_cmd_id == handler->ops_cmd_id) && handler->handle_func)
			return handler->handle_func(connector, value);
	}

	return -1;
}

void dpu_connector_setup(struct dpu_connector *connector);

#endif