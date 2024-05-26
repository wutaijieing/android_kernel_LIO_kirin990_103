
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

#include <soc_pctrl_interface.h>
#include <soc_crgperiph_interface.h>
#include "dpu_conn_mgr.h"
#include "dsc/dsc_config.h"
#include "spr/spr_config.h"

/*******************************************************************************
 * MIPI DPHY GPIO for FPGA
 */
#define GPIO_MIPI_DPHY_PG_SEL_A_NAME "pg_sel_a"
#define GPIO_MIPI_DPHY_PG_SEL_B_NAME "pg_sel_b"
#define GPIO_MIPI_DPHY_TX_RX_A_NAME "tx_rx_a"
#define GPIO_MIPI_DPHY_TX_RX_B_NAME "tx_rx_b"

#define GPIO_PG_SEL_A (56)
#define GPIO_TX_RX_A (58)
#define GPIO_PG_SEL_B (37)
#define GPIO_TX_RX_B (39)

static uint32_t gpio_pg_sel_a = GPIO_PG_SEL_A;
static uint32_t gpio_tx_rx_a = GPIO_TX_RX_A;
static uint32_t gpio_pg_sel_b = GPIO_PG_SEL_B;
static uint32_t gpio_tx_rx_b = GPIO_TX_RX_B;

static struct gpio_desc mipi_dphy_gpio_request_cmds[] = {
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 0 },
};

static struct gpio_desc mipi_dphy_gpio_free_cmds[] = {
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 0 },
};

static struct gpio_desc mipi_dphy_gpio_normal_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 1 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 1 },
};

static struct gpio_desc mipi_dphy_gpio_lowpower_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_A_NAME, &gpio_pg_sel_a, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_A_NAME, &gpio_tx_rx_a, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_PG_SEL_B_NAME, &gpio_pg_sel_b, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_MIPI_DPHY_TX_RX_B_NAME, &gpio_tx_rx_b, 0 },
};

static void mipi_dsi_dphy_fastboot_fpga(struct dkmd_connector_info *pinfo)
{
	if (pinfo->base.fpga_flag)
		(void)peri_gpio_cmds_tx(mipi_dphy_gpio_request_cmds, ARRAY_SIZE(mipi_dphy_gpio_request_cmds));
}

static void mipi_dsi_dphy_on_fpga(struct dkmd_connector_info *pinfo)
{
	if (pinfo->base.fpga_flag) {
		(void)peri_gpio_cmds_tx(mipi_dphy_gpio_request_cmds, ARRAY_SIZE(mipi_dphy_gpio_request_cmds));
		(void)peri_gpio_cmds_tx(mipi_dphy_gpio_normal_cmds, ARRAY_SIZE(mipi_dphy_gpio_normal_cmds));
	}
}

static int32_t mipi_dsi_clk_enable(struct dpu_connector *connector)
{
	int32_t i = 0;
	int32_t ret = 0;

	for (i = 0; i < CLK_GATE_MAX_IDX; i++) {
		if (!IS_ERR_OR_NULL(connector->connector_clk[i])) {
			ret = clk_prepare_enable(connector->connector_clk[i]);
			if (ret)
				dpu_pr_err("clk_id:%d enable failed, error=%d!\n", i, ret);
		}
	}

	if (connector->bind_connector) {
		dpu_pr_info("set bind_connector clk enable!\n");
		ret = mipi_dsi_clk_enable(connector->bind_connector);
		/* dsi mem shut down */
		if (connector->pctrl_base)
			set_reg(SOC_PCTRL_PERI_CTRL102_ADDR(connector->pctrl_base), 0xc0000, 32, 0);
	}

	return ret;
}

static void mipi_dsi_on_sub1(struct dpu_connector *connector)
{
	char __iomem *mipi_dsi_base = connector->connector_base;

	/* mipi init */
	mipi_init(connector);

	/* switch to cmd mode */
	set_reg(DPU_DSI_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
	/* cmd mode: low power mode */
	set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x7f, 7, 8);
	set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0xf, 4, 16);
	set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 24);
	/* disable generate High Speed clock */
	set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);

	if (connector->bind_connector) {
		dpu_pr_info("set bind_connector init and on sub1!\n");
		mipi_dsi_on_sub1(connector->bind_connector);
	}
}

static void mipi_dsi_on_sub2(struct dpu_connector *connector)
{
	char __iomem *mipi_dsi_base = connector->connector_base;
	struct dkmd_connector_info *pinfo = connector->conn_info;

	/* switch to video mode */
	if (is_mipi_video_panel(&pinfo->base))
		set_reg(DPU_DSI_MODE_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);

	/* cmd mode: high speed mode */
	if (is_mipi_cmd_panel(&pinfo->base)) {
		set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x0, 7, 8);
		set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x0, 4, 16);
		set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 24);
	}

	/* enable EOTP TX */
	if (connector->mipi.phy_mode == DPHY_MODE)
		set_reg(DPU_DSI_PERIP_CHAR_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);

	/* enable generate High Speed clock, non continue */
	if (connector->mipi.non_continue_en)
		set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x3, 2, 0);
	else
		set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x1, 2, 0);

	if (connector->bind_connector) {
		dpu_pr_info("set bind_connector on sub2!\n");
		mipi_dsi_on_sub2(connector->bind_connector);
	}
}

static int32_t mipi_dsi_on(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *connector = NULL;

	if (!pinfo) {
		dpu_pr_err("pinfo is NULL!\n");
		return -EINVAL;
	}

	connector = get_primary_connector(pinfo);
	if (!connector) {
		dpu_pr_err("primary connector is NULL!\n");
		return -EINVAL;
	}

	mipi_dsi_dphy_on_fpga(pinfo);

	/* pipeline next connector hardware power on */
	pipeline_next_on(pinfo->base.peri_device, pinfo);

	(void)mipi_dsi_clk_enable(connector);

	mipi_dsi_on_sub1(connector);

	/* panel lcd initialize code by mipi dsi */
	(void)pipeline_next_ops_handle(pinfo->base.peri_device, pinfo, LCD_SEND_INITIAL_LP_CMD, connector->connector_base);

	/* read lcd power status */
	(void)pipeline_next_ops_handle(pinfo->base.peri_device, pinfo, CHECK_LCD_STATUS, NULL);

	mipi_dsi_on_sub2(connector);

	/* mipi hs video/command mode */
	dpu_pr_info("primary dsi-%d -\n", connector->pipe_sw_post_chn_idx);

	return 0;
}

static void mipi_dsi_dphy_off_fpga(struct dkmd_connector_info *pinfo)
{
	if (pinfo->base.fpga_flag) {
		(void)peri_gpio_cmds_tx(mipi_dphy_gpio_lowpower_cmds, ARRAY_SIZE(mipi_dphy_gpio_lowpower_cmds));
		(void)peri_gpio_cmds_tx(mipi_dphy_gpio_free_cmds, ARRAY_SIZE(mipi_dphy_gpio_free_cmds));
	}
}

static void mipi_dsi_clk_disable(struct dpu_connector *connector)
{
	int32_t i = 0;

	for (i = 0; i < CLK_GATE_MAX_IDX; i++) {
		if (!IS_ERR_OR_NULL(connector->connector_clk[i]))
			clk_disable_unprepare(connector->connector_clk[i]);
	}

	if (connector->bind_connector) {
		dpu_pr_info("set bind_connector clk disable!\n");
		mipi_dsi_clk_disable(connector->bind_connector);
	}
}

static void mipi_dsi_off_sub(struct dpu_connector *connector)
{
	char __iomem *mipi_dsi_base = connector->connector_base;

	/* switch to cmd mode */
	set_reg(DPU_DSI_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 0);
	/* cmd mode: low power mode */
	set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x7f, 7, 8);
	set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0xf, 4, 16);
	set_reg(DPU_DSI_CMD_MODE_CTRL_ADDR(mipi_dsi_base), 0x1, 1, 24);

	/* disable generate High Speed clock */
	set_reg(DPU_DSI_LP_CLK_CTRL_ADDR(mipi_dsi_base), 0x0, 1, 0);
	udelay(10);  /* 10us */

	/* shutdown d_phy */
	set_reg(DPU_DSI_CDPHY_RST_CTRL_ADDR(mipi_dsi_base), 0x0, 3, 0);

	if (connector->bind_connector) {
		dpu_pr_info("set bind_connector off!\n");
		mipi_dsi_off_sub(connector->bind_connector);
	}
}

static int32_t mipi_dsi_off(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *connector = NULL;

	if (!pinfo) {
		dpu_pr_err("pinfo is NULL!\n");
		return -EINVAL;
	}

	connector = get_primary_connector(pinfo);
	if (!connector) {
		dpu_pr_err("primary connector is NULL!\n");
		return -EINVAL;
	}

	/* panel lcd uninitialize code by mipi dsi */
	pipeline_next_ops_handle(pinfo->base.peri_device, pinfo, LCD_SEND_UNINITIAL_LP_CMD, connector->connector_base);

	/* add MIPI LP mode here if necessary MIPI LP mode end */

	/* disable mipi ldi */
	pinfo->disable_ldi(pinfo);

	/* Here need to enter ulps when panel off bypass ddic power down */
	mipi_dsi_off_sub(connector);

	mipi_dsi_clk_disable(connector);

	mipi_dsi_dphy_off_fpga(pinfo);

	pipeline_next_off(pinfo->base.peri_device, pinfo);

	dpu_pr_info("dsi-%d, -\n", connector->pipe_sw_post_chn_idx);

	return 0;
}

static int32_t mipi_dsi_set_fastboot(struct dpu_connector *connector, const void *value)
{
	uint32_t fastboot_enable = *(uint32_t *)value;
	struct dkmd_connector_info *pinfo = connector->conn_info;

	dpu_pr_info("fastboot_enable: %d ++++++", fastboot_enable);

	if (!pinfo)
		return -EINVAL;

	mipi_dsi_dphy_fastboot_fpga(pinfo);
	mipi_dsi_clk_enable(connector);

	/* disable mipi ldi */
	if (is_mipi_cmd_panel(&pinfo->base))
		pinfo->disable_ldi(pinfo);

	(void)pipeline_next_ops_handle(pinfo->base.peri_device, pinfo, SET_FASTBOOT, connector->connector_base);

	return 0;
}

static uint32_t get_stopstate_mask_value(uint8_t lane_nums)
{
	uint32_t stopstate_msk;

	if (lane_nums == DSI_4_LANES)
		stopstate_msk = BIT(0);
	else if (lane_nums == DSI_3_LANES)
		stopstate_msk = BIT(0) | BIT(4);
	else if (lane_nums == DSI_2_LANES)
		stopstate_msk = BIT(0) | BIT(3) | BIT(4);
	else
		stopstate_msk = BIT(0) | BIT(2) | BIT(3) | BIT(4);

	return stopstate_msk;
}

static void mipi_dsi_set_reg(struct dkmd_connector_info *pinfo, uint32_t offset, uint32_t val, uint8_t bw, uint8_t bs)
{
	struct dpu_connector *primay_connector = get_primary_connector(pinfo);

	set_reg(primay_connector->connector_base + offset, val, bw, bs);
	if (primay_connector->bind_connector)
		set_reg(primay_connector->bind_connector->connector_base + offset, val, bw, bs);
}

static uint8_t get_mipi_dsi_lane_num(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *primay_connector = get_primary_connector(pinfo);
	return primay_connector->mipi.lane_nums;
}

static int32_t mipi_dsi_set_clear(struct dpu_connector *connector, const void *value)
{
	bool valid_status = false;
	uint32_t stopstate_msk = 0;
	struct dkmd_connector_info *pinfo = connector->conn_info;

	if (is_mipi_cmd_panel(&pinfo->base) && g_ldi_data_gate_en)
		mipi_dsi_set_reg(pinfo, DPU_DSI_FRM_VALID_DBG_ADDR(DSI_ADDR_TO_OFFSET), 1, 1, 29);

	/* 5. delay vfp+1, config stopstate_enable and stopstate_mask */
	stopstate_msk = get_stopstate_mask_value(get_mipi_dsi_lane_num(pinfo));
	mipi_dsi_set_reg(pinfo, DPU_DSI_DPHYTX_CTRL_ADDR(DSI_ADDR_TO_OFFSET), 1, 1, 0);
	mipi_dsi_set_reg(pinfo, DPU_DSI_DPHYTX_CTRL_ADDR(DSI_ADDR_TO_OFFSET), stopstate_msk, 5, 3);

	/* 6. wait and check dsi dphytx_trstop_flag */
	valid_status = check_addr_status_is_valid(
		DPU_DSI_DPHYTX_TRSTOP_FLAG_ADDR(connector->connector_base), 0b01, 5, 10); /* timeout: 50us */
	if (connector->bind_connector != NULL)
		valid_status = valid_status && check_addr_status_is_valid(
			DPU_DSI_DPHYTX_TRSTOP_FLAG_ADDR(connector->bind_connector->connector_base), 0b01, 5, 10); /* timeout: 50us */

	if (!valid_status)
		dpu_pr_warn("check dphytx trstop flag failed!");

	/* 7. config dsi core shutdownz */
	mipi_dsi_set_reg(pinfo, DPU_DSI_POR_CTRL_ADDR(DSI_ADDR_TO_OFFSET), 0x0, 1, 0);
	/* timing constraint */
	udelay(5);
	mipi_dsi_set_reg(pinfo, DPU_DSI_POR_CTRL_ADDR(DSI_ADDR_TO_OFFSET), 0x1, 1, 0);

	/* 8. clear dsi core shutdownz */
	mipi_dsi_set_reg(pinfo, DPU_DSI_DPHYTX_CTRL_ADDR(DSI_ADDR_TO_OFFSET), 0, 1, 0);
	mipi_dsi_set_reg(pinfo, DPU_DSI_DPHYTX_CTRL_ADDR(DSI_ADDR_TO_OFFSET), 0, 5, 3);

	/* issued instructions: single_frm_update */
	if (is_mipi_cmd_panel(&pinfo->base) && g_ldi_data_gate_en)
		mipi_dsi_set_reg(pinfo, DPU_DSI_LDI_FRM_MSK_UP_ADDR(DSI_ADDR_TO_OFFSET), 0x1, 1, 0);

	/* clear GLB DSS_RS_CLEAR */
	outp32(DPU_DSI_CPU_ITF_INTS_ADDR(connector->connector_base), ~0);
	pinfo->enable_ldi(pinfo);

	/* 9. wait vactive intr and enable frm_valid */
	if (is_mipi_cmd_panel(&pinfo->base) && g_ldi_data_gate_en)
		mipi_dsi_set_reg(pinfo, DPU_DSI_FRM_VALID_DBG_ADDR(DSI_ADDR_TO_OFFSET), 0, 1, 29);

	mdelay(16);
	if (inp32(DPU_DSI_CPU_ITF_INTS_ADDR(connector->connector_base)) & DSI_INT_UNDER_FLOW)
		return -1;

	return 0;
}

static int32_t mipi_dsi_isr_enable(struct dpu_connector *connector, const void *value)
{
	uint32_t mask = ~0;
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	/* 1. interrupt mask */
	outp32(DPU_DSI_CPU_ITF_INT_MSK_ADDR(connector->connector_base), mask);

	/* 2. enable irq */
	isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_ENABLE);

	/* 3. interrupt clear */
	outp32(DPU_DSI_CPU_ITF_INTS_ADDR(connector->connector_base), mask);

	/* 4. interrupt umask */
	outp32(DPU_DSI_CPU_ITF_INT_MSK_ADDR(connector->connector_base), isr_ctrl->unmask);

	if (is_mipi_cmd_panel(&connector->conn_info->base) && !g_ldi_data_gate_en)
		set_reg(DPU_DSI_FRM_VALID_DBG_ADDR(connector->connector_base), 1, 1, 29);

	return 0;
}

static int32_t mipi_dsi_isr_disable(struct dpu_connector *connector, const void *value)
{
	uint32_t mask = ~0;
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	/* 1. interrupt mask */
	outp32(DPU_DSI_CPU_ITF_INT_MSK_ADDR(connector->connector_base), mask);

	/* 2. disable irq */
	isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_DISABLE);

	return 0;
}

static irqreturn_t dpu_dsi_isr(int32_t irq, void *ptr)
{
	uint32_t isr_s2, mask, vsync_bit;
	struct dpu_connector *connector = NULL;
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)ptr;

	dpu_check_and_return(!isr_ctrl, IRQ_NONE, err, "isr_ctrl is null!");

	connector = (struct dpu_connector *)isr_ctrl->parent;
	dpu_check_and_return(!connector, IRQ_NONE, err, "connector is null!");

	isr_s2 = inp32(DPU_DSI_CPU_ITF_INTS_ADDR(connector->connector_base));
	outp32(DPU_DSI_CPU_ITF_INTS_ADDR(connector->connector_base), isr_s2);
	isr_s2 &= ~((uint32_t)inp32(DPU_DSI_CPU_ITF_INT_MSK_ADDR(connector->connector_base)));
	if ((isr_s2 & DSI_INT_VACT0_END) == DSI_INT_VACT0_END) {
		dpu_pr_debug("get vactive0 end itr notify, isr_s2: %#x", isr_s2);
		dkmd_isr_notify_listener(isr_ctrl, DSI_INT_VACT0_END);
	}

	if ((isr_s2 & DSI_INT_VACT0_START) == DSI_INT_VACT0_START) {
		dpu_pr_debug("get vactive0 start itr notify, isr_s2: %#x", isr_s2);
		dkmd_isr_notify_listener(isr_ctrl, DSI_INT_VACT0_START);
		outp32(DPU_DSI_CPU_ITF_INT_MSK_ADDR(connector->connector_base), isr_ctrl->unmask);
	}

	vsync_bit = (is_mipi_cmd_panel(&connector->conn_info->base) &&
		!g_debug_force_update) ? DSI_INT_LCD_TE0 : DSI_INT_VSYNC;
	if ((isr_s2 & vsync_bit) == vsync_bit)
		dkmd_isr_notify_listener(isr_ctrl, vsync_bit);

	if ((isr_s2 & DSI_INT_FRM_END) == DSI_INT_FRM_END) {
		dpu_pr_debug("get frame end itr notify, isr_s2: %#x", isr_s2);
		dkmd_isr_notify_listener(isr_ctrl, DSI_INT_FRM_END);
	}

	if ((isr_s2 & DSI_INT_UNDER_FLOW) == DSI_INT_UNDER_FLOW) {
		mask = (uint32_t)inp32(DPU_DSI_CPU_ITF_INT_MSK_ADDR(connector->connector_base));
		mask |= DSI_INT_UNDER_FLOW;
		outp32(DPU_DSI_CPU_ITF_INT_MSK_ADDR(connector->connector_base), mask);

		dpu_pr_warn("get underflow itr notify, isr_s2: %#x", isr_s2);
		dkmd_isr_notify_listener(isr_ctrl, DSI_INT_UNDER_FLOW);
	}

	return IRQ_HANDLED;
}

static int32_t mipi_dsi_isr_setup(struct dpu_connector *connector, const void *value)
{
	uint32_t vsync_bit = 0;
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	(void)snprintf(isr_ctrl->irq_name, sizeof(isr_ctrl->irq_name), "irq_dsi_%d", connector->pipe_sw_post_chn_idx);
	isr_ctrl->irq_no = connector->connector_irq;
	isr_ctrl->isr_fnc = dpu_dsi_isr;
	isr_ctrl->parent = connector;

	vsync_bit = (is_mipi_cmd_panel(&connector->conn_info->base) &&
		!g_debug_force_update) ? DSI_INT_LCD_TE0 : DSI_INT_VSYNC;
	isr_ctrl->unmask = ~(vsync_bit | DSI_INT_VACT0_START | DSI_INT_VACT0_END | DSI_INT_FRM_END | DSI_INT_UNDER_FLOW);

	return 0;
}

static int32_t mipi_dsc_init(struct dpu_connector *connector, const void *value)
{
	(void)value;
	dsc_init(&connector->dsc, connector->dsc_base);
	return 0;
}

static int32_t mipi_spr_init(struct dpu_connector *connector, const void *value)
{
	(void)value;
	spr_init(&connector->spr, connector->dpp_base, connector->dsc_base);
	return 0;
}

static int32_t mipi_dsi_ulps_handle(struct dpu_connector *connector, const void *value)
{
	mipi_dsi_ulps_cfg(connector, *(bool *)value);
	return 0;
}

static struct connector_ops_handle_data dsi_ops_table[] = {
	{ SET_FASTBOOT, mipi_dsi_set_fastboot },
	{ DO_CLEAR, mipi_dsi_set_clear },
	{ SETUP_ISR, mipi_dsi_isr_setup },
	{ ENABLE_ISR, mipi_dsi_isr_enable },
	{ DISABLE_ISR, mipi_dsi_isr_disable },
	{ INIT_SPR, mipi_spr_init },
	{ INIT_DSC, mipi_dsc_init },
	{ HANDLE_MIPI_ULPS, mipi_dsi_ulps_handle },
};

static int32_t mipi_dsi_ops_handle(struct dkmd_connector_info *pinfo, uint32_t ops_cmd_id, void *value)
{
	struct dpu_connector *connector = NULL;

	dpu_pr_debug("ops_cmd_id = %d!", ops_cmd_id);
	dpu_check_and_return(!pinfo, -EINVAL, err, "pinfo is NULL!");
	connector = get_primary_connector(pinfo);
	dpu_check_and_return(!connector, -EINVAL, err, "connector is NULL!");

	if (dkdm_connector_hanlde_func(dsi_ops_table, ARRAY_SIZE(dsi_ops_table), ops_cmd_id, connector, value) != 0)
		return pipeline_next_ops_handle(pinfo->base.peri_device, pinfo, ops_cmd_id, value);

	return 0;
}

static void enable_dsi_ldi(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *connector = get_primary_connector(pinfo);

	if (g_debug_vsync_delay_time != 0)
		set_reg(DPU_DSI_VSYNC_DELAY_TIME_ADDR(connector->connector_base), g_debug_vsync_delay_time, 32, 0);

	/* issued instructions: single_frm_update */
	set_reg(DPU_DSI_LDI_FRM_MSK_UP_ADDR(connector->connector_base), 0x1, 1, 0);
	if (connector->bind_connector)
		set_reg(DPU_DSI_LDI_FRM_MSK_UP_ADDR(connector->bind_connector->connector_base), 0x1, 1, 0);

	/* interrupt vsync delay when commit to hardware */
	dpu_pr_debug("commit and interrupt vsync delay!!");
	set_reg(DPU_DSI_VSYNC_DELAY_CTRL_ADDR(connector->connector_base), 0x3, 2, 0);

	if (connector->bind_connector)
		set_reg(DPU_DSI_LDI_CTRL_ADDR(connector->connector_base), 0x1, 1, 5);
	else
		set_reg(DPU_DSI_LDI_CTRL_ADDR(connector->connector_base), 0x1, 1, 0);
}

static void disable_dsi_ldi(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *connector = get_primary_connector(pinfo);

	if (connector->bind_connector)
		set_reg(DPU_DSI_LDI_CTRL_ADDR(connector->connector_base), 0x0, 1, 5);
	else
		set_reg(DPU_DSI_LDI_CTRL_ADDR(connector->connector_base), 0x0, 1, 0);

	if (g_debug_force_update) {
		/* force update need ldi enable */
		set_reg(DPU_DSI_LDI_CTRL_ADDR(connector->connector_base), 0x1, 1, 0);
		dpu_pr_info("[vsync] enable next frame delay!!");
	}

	/* enable next frame vsync delay when vactive start */
	set_reg(DPU_DSI_VSYNC_DELAY_CTRL_ADDR(connector->connector_base), 0x0, 2, 0);
}

void mipi_dsi_default_setup(struct dpu_connector *connector)
{
	connector->on_func = mipi_dsi_on;
	connector->off_func = mipi_dsi_off;
	connector->ops_handle_func = mipi_dsi_ops_handle;

	connector->conn_info->enable_ldi = enable_dsi_ldi;
	connector->conn_info->disable_ldi = disable_dsi_ldi;
}
