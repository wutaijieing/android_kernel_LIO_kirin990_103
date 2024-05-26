/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "hisi_drv_utils.h"
#include "hisi_drv_mipi.h"
#include "hisi_mipi_core.h"
#include "hisi_mipi_dsi.h"
#include "hisi_disp_pm.h"
#include "hisi_disp_gadgets.h"
#include "hisi_connector_utils.h"

static int hisi_mipi_slaver_turn_on(struct hisi_connector_device *connector_dev)
{
	struct hisi_connector_component *slaver = NULL;
	int i;
	int ret;
	disp_pr_info(" ++++ ");
	for (i = 0; i < connector_dev->slaver_count; i++) {
		slaver = &(connector_dev->slavers[i]);
		if (!slaver)
			continue;

		dpu_check_and_call_func(ret, slaver->connector_ops.connector_on, slaver->connector_id, connector_dev->pdev);
		if (ret) {
			disp_pr_err("call slaver power func fail", slaver->connector_id);
			continue;
		}
	}

	return 0;
}

static int hisi_mipi_slaver_turn_off(struct hisi_connector_device *connector_dev)
{
	struct hisi_connector_component *slaver = NULL;
	int i;
	int ret;

	for (i = 0; i < connector_dev->slaver_count; i++) {
		slaver = &(connector_dev->slavers[i]);
		if (!slaver)
			continue;

		dpu_check_and_call_func(ret, slaver->connector_ops.connector_off, slaver->connector_id, connector_dev->pdev);
		if (ret) {
			disp_pr_err("call slaver power func fail", slaver->connector_id);
			continue;
		}
	}

	return 0;
}

static int hisi_mipi_slaver_present(struct hisi_connector_device *connector_dev, void *data)
{
	struct hisi_connector_component *slaver = NULL;
	int i;
	int ret;

	for (i = 0; i < connector_dev->slaver_count; i++) {
		slaver = &(connector_dev->slavers[i]);
		if (!slaver)
			continue;

		dpu_check_and_call_func(ret, slaver->connector_ops.connector_present, slaver->connector_id, connector_dev->pdev, data);
		if (ret) {
			disp_pr_err("call slaver power func fail", slaver->connector_id);
			continue;
		}
	}

	return 0;
}

static int hisi_mipi_master_base_on(uint32_t connector_id, struct platform_device *pdev)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_connector_info *connector_info = NULL;
#ifdef CONFIG_DKMD_DPU_V720
	char __iomem *dss_base_addr = NULL;
#endif
	int ret;

	disp_pr_info(" ++++, %s",pdev->name);

	connector_dev = platform_get_drvdata(pdev);
	connector_info = connector_dev->master.connector_info;

	/* step0: set fpga */
	if (connector_info->current_step == 0) {
		disp_pr_info(" 1 ");
		dpu_mipi_dsi_dphy_on_fpga(connector_dev);
		return 0;
	}

	/* step1: set dphy at low speed mode */
	if (connector_info->current_step == 1) {
		/* 1, dis reset peri, enable clks */
		disp_pr_info(" 2 ");
		mipi_dsi_peri_dis_reset(connector_dev);

		/* TODO deleted by FPGA test */
		// hisi_disp_pm_enable_clks(connector_info->clk_bits);

#ifdef CONFIG_DKMD_DPU_V720
		// fix DSI 0xdeadbeef
		dss_base_addr = connector_dev->base_addr.dpu_base_addr;
		dpu_set_reg(DPU_GLB_DBG_CFG_CTRL_ADDR(dss_base_addr + DSS_GLB0_OFFSET), 0x3, 2, 0);
#endif

		/* 2, master power on, lp mode */
		mipi_dsi_set_lp_mode(connector_dev, connector_info);

		/* 3, power on slaver connector, lp mode */
		ret = hisi_mipi_slaver_turn_on(connector_dev);
		if (ret) {
			return -1;
		}

		// TODO:
		/* Here need to exit ulps when panel off bypass ddic power down
		if (panel_next_bypass_powerdown_ulps_support(pdev) ||
			hisifd->panel_info.skip_power_on_off)
			mipi_dsi_ulps_cfg(hisifd, 1);  // 0--enable 1--not enabled */

		return 0;
	}

	/* step2: set dphy at high speed mode */
	if (connector_info->current_step == 2) {
		mipi_dsi_set_hs_mode(connector_dev, connector_info);
		ret = hisi_mipi_slaver_turn_on(connector_dev);
		if (ret) {
			return -1;
		}
		return 0;
	}
	disp_pr_info(" ---- ");
	return 0;
}

static int hisi_mipi_master_base_off(uint32_t connector_id, struct platform_device *pdev)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_connector_info *connector_info = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret;

	connector_dev = platform_get_drvdata(pdev);
	connector_info = connector_dev->master.connector_info;
	pinfo = connector_dev->fix_panel_info;

	if (pinfo->lcd_uninit_step_support)
		/* add MIPI LP mode here if necessary MIPI LP mode end */
		dpu_check_and_call_func(ret, connector_dev->next_dev_ops->turn_off, connector_id, connector_dev->next_dev);

	/* 1, master power off itself */
	mipi_dsi_off_sub1(connector_dev, connector_info);
	mipi_dsi_off_sub2(connector_dev, connector_info);

	/* 2, power off slaver connector */
	ret = hisi_mipi_slaver_turn_off(connector_dev);
	if (ret) {
		return -1;
	}

	/* Todo delete for FPGA power on off test */
	// hisi_disp_pm_disable_clks(connector_info->clk_bits);
	dpu_mipi_dsi_dphy_off_fpga(connector_dev);
	mipi_dsi_peri_reset(connector_dev);

	if (pinfo->lcd_uninit_step_support)
		dpu_check_and_call_func(ret, connector_dev->next_dev_ops->turn_off, connector_id, connector_dev->next_dev);

	return 0;
}

static int hisi_mipi_master_base_present(uint32_t connector_id, struct platform_device *pdev, void *data)
{
	struct hisi_connector_device *connector_dev = NULL;
	int ret;

	connector_dev = platform_get_drvdata(pdev);

	/* 1, master present itself */

	/* 2, present slaver connector */
	ret = hisi_mipi_slaver_present(connector_dev, data);
	if (ret) {
		return -1;
	}

	return 0;
}

static void hisi_mipi_init_master(struct hisi_connector_device *mipi_dev, struct hisi_connector_component *component)
{
	disp_pr_info(" ++++ ");
	component->parent = mipi_dev;
	component->connector_id = hisi_get_connector_id();

	component->connector_ops.connector_on = hisi_mipi_master_base_on;
	component->connector_ops.connector_off = hisi_mipi_master_base_off;
	component->connector_ops.connector_present = hisi_mipi_master_base_present;

	component->connector_info->connector_type = HISI_CONNECTOR_TYPE_MIPI;
	component->connector_info->connector_base = mipi_dev->base_addr.dpu_base_addr + DPU_MIPI_DSI0_OFFSET;

	component->connector_info->clk_bits = BIT(PM_CLK_TX_DPHY0_REF) | BIT(PM_CLK_TX_DPHY0_CFG) | BIT(PM_CLK_DSI0);
#ifdef CONFIG_PCLK_PCTRL_USED
	component->connector_info->clk_bits |= BIT(PM_CLK_PCLK_PCTRL);
#endif
}

void hisi_mipi_init_component(struct hisi_connector_device *mipi_dev)
{
	uint32_t i;
	disp_pr_info(" ++++ ");
	hisi_mipi_init_master(mipi_dev, &mipi_dev->master);

	for (i = 0; i < mipi_dev->slaver_count; i++) {
		mipi_dev->slavers[i].connector_id = i;
		hisi_mipi_init_slaver(mipi_dev, &mipi_dev->slavers[i]);
	}
}
