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
#include "hisi_drv_dp.h"
#include "hisi_dp_core.h"
#include "hisi_disp_pm.h"
#include "hisi_disp_gadgets.h"
#include "hisi_connector_utils.h"

static int hisi_dp_master_base_on(uint32_t connector_id, struct platform_device *pdev)
{
#if 0
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_connector_info *connector_info = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;
	int ret;

	disp_pr_info("[DP] enter ++++");

	connector_dev = platform_get_drvdata(pdev);
	connector_info = connector_dev->master.connector_info;
	dp_info = (struct hisi_dp_info *)connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");
	dptx = &(dp_info->dp);

	ret = dp_on(dptx);
	if (ret) {
		disp_pr_err("[DP] dp on fail\n");
		return ret;
	}

	if (dptx->handle_hotplug) {
		ret = dptx->handle_hotplug(dptx);
		if (ret) {
			disp_pr_err("[DP] hotplug error\n");
			return ret;
		}
	}
#endif
	return 0;
}

static int hisi_dp_master_base_off(uint32_t connector_id, struct platform_device *pdev)
{
#if 0
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_connector_info *connector_info = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;
	int ret;

	connector_dev = platform_get_drvdata(pdev);
	connector_info = connector_dev->master.connector_info;
	dp_info = (struct hisi_dp_info *)connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");
	dptx = &(dp_info->dp);

	if (dptx->handle_hotunplug) {
		ret = dptx->handle_hotunplug(dptx);
		if (ret) {
			disp_pr_err("[DP] hotplug error\n");
			return ret;
		}
	}

	ret = dp_off(dptx);
	if (ret) {
		disp_pr_err("[DP] dp off fail\n");
		return ret;
	}
#endif
	return 0;
}

static int hisi_dp_master_base_present(uint32_t connector_id, struct platform_device *pdev, void *data)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_connector_info *connector_info = NULL;
	struct hisi_dp_info *dp_info = NULL;
	struct dp_ctrl *dptx = NULL;
	int ret;

	disp_pr_info(" +++++++ \n");

	connector_dev = platform_get_drvdata(pdev);
	connector_info = connector_dev->master.connector_info;
	dp_info = (struct hisi_dp_info *)connector_info;
	disp_check_and_return((dp_info == NULL), -EINVAL, err, "[DP] dp_info is NULL!\n");
	dptx = &(dp_info->dp);

	dptx->dptx_enable_dpu_pipeline(dptx);

	disp_pr_info(" -------- \n");

	return 0;
}

static void hisi_dp_init_master(struct hisi_connector_device *dp_dev, struct hisi_connector_component *component)
{
	component->parent = dp_dev;
	component->connector_id = hisi_get_connector_id();

	component->connector_ops.connector_on = hisi_dp_master_base_on;
	component->connector_ops.connector_off = hisi_dp_master_base_off;
	component->connector_ops.connector_present = hisi_dp_master_base_present;
}

void hisi_dp_init_component(struct hisi_connector_device *dp_dev)
{
	hisi_dp_init_master(dp_dev, &dp_dev->master);
}
