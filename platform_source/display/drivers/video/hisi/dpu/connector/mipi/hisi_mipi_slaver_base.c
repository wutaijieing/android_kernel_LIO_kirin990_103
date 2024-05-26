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
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <soc_dte_define.h>

#include "hisi_mipi_core.h"
#include "hisi_connector_utils.h"
#include "hisi_disp_pm.h"

static int hisi_mipi_slaver_base_on(uint32_t slaver_id, struct platform_device *pdev)
{
	struct hisi_connector_device *mipi_device = NULL;
	struct hisi_connector_component *slaver = NULL;

	disp_pr_info(" ++++ \n");
	mipi_device = platform_get_drvdata(pdev);
	slaver = &(mipi_device->slavers[slaver_id]);

	/* TODO deleted by FPGA test */
	// hisi_disp_pm_enable_clks(slaver->connector_info->clk_bits);

	return 0;
}

static int hisi_mipi_slaver_base_off(uint32_t slaver_id, struct platform_device *pdev)
{

	return 0;
}

static int hisi_mipi_slaver_base_present(uint32_t slaver_id, struct platform_device *pdev, void *data)
{
	return 0;
}

void hisi_mipi_init_slaver(struct hisi_connector_device *mipi_dev, struct hisi_connector_component *out_slaver)
{
	struct hisi_mipi_info *mipi_info = NULL;

	mipi_info = kzalloc(sizeof(struct hisi_mipi_info), GFP_KERNEL);
	if (!mipi_info)
		return;

	disp_pr_info(" ++++ \n");
	/* TODO:init slaver mipi info */
	mipi_info->base.clk_bits = BIT(PM_CLK_TX_DPHY1_REF) | BIT(PM_CLK_TX_DPHY1_CFG) | BIT(PM_CLK_DSI1);
	mipi_info->base.power_step_cnt = 3;
	mipi_info->base.current_step = 0;
	mipi_info->base.connector_base = mipi_dev->base_addr.dpu_base_addr + DPU_MIPI_DSI1_OFFSET;
	mipi_info->base.connector_type = HISI_CONNECTOR_TYPE_MIPI;

	out_slaver->connector_info = (struct hisi_connector_info*)mipi_info;
	out_slaver->connector_ops.connector_on = hisi_mipi_slaver_base_on;
	out_slaver->connector_ops.connector_off = hisi_mipi_slaver_base_off;
	out_slaver->connector_ops.connector_present = hisi_mipi_slaver_base_present;
}


