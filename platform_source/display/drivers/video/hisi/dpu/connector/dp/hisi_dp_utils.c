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
#include <linux/types.h>
#include <linux/platform_device.h>

#include "hisi_connector_utils.h"
#include "hisi_disp_debug.h"
#include "hisi_drv_utils.h"

static void hisi_dp_init_data(struct platform_device *dp_dev, void *dev_data, void *input, void *panel_ops)
{
	struct hisi_connector_device *dp_data = NULL;
	struct hisi_panel_device *panel_data = NULL;

	disp_assert_if_cond(dp_dev == NULL);
	disp_assert_if_cond(dev_data == NULL);

	dp_data = (struct hisi_connector_device*)dev_data;
	dp_data->pdev = dp_dev;

	if (!input)
		return;

	panel_data = (struct hisi_panel_device *)input;
	dp_data->next_dev = panel_data->pdev;
	dp_data->next_dev_ops = panel_ops;
	dp_data->fix_panel_info = &panel_data->panel_info;

	dp_data->master.connector_info = panel_data->connector_info;
	dp_data->master.parent = dp_data;
	dp_data->slaver_count = 0;
}

struct hisi_connector_device *hisi_dp_create_platform_device(struct hisi_panel_device *panel_data,
		struct hisi_panel_ops *panel_ops)
{
	struct hisi_connector_device *connector_dev = NULL;

	/* create parent device, such as dptx device */
	connector_dev = hisi_drv_create_device(HISI_CONNECTOR_EXTERNAL_DP_NAME, sizeof(*connector_dev),
				hisi_dp_init_data, panel_data, panel_ops);
	if (!connector_dev) {
		disp_pr_err("create %s device fail", HISI_CONNECTOR_EXTERNAL_DP_NAME);
		return NULL;
	}

	return connector_dev;
}

