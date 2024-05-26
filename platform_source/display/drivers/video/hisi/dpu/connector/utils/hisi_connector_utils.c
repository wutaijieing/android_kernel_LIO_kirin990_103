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

#include "hisi_mipi_core.h"
#include "hisi_connector_utils.h"
#include "hisi_drv_utils.h"

uint32_t g_connector_id = 0;

static int hisi_drv_connector_on(uint32_t connector_id, struct platform_device *pdev)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_connector_priv_ops *master_ops = NULL;
	struct hisi_pipeline_ops *next_dev_ops = NULL;
	struct hisi_connector_info *info = NULL;
	int ret;
	disp_pr_info(" ++++ ");
	connector_dev = platform_get_drvdata(pdev);
	master_ops = &connector_dev->master.connector_ops;
	info = connector_dev->master.connector_info;

	/* TODO: refactor the function */
	for (info->current_step = 0; info->current_step < info->power_step_cnt; ++(info->current_step)) {
		/* step to power on master */
		disp_pr_info(" connector_on ");
		dpu_check_and_call_func(ret, master_ops->connector_on, connector_id, pdev);

		/* step to power on next device */
		next_dev_ops = connector_dev->next_dev_ops;
		disp_pr_info(" turn_on ");
		dpu_check_and_call_func(ret, next_dev_ops->turn_on, connector_id, connector_dev->next_dev);
	}

	disp_pr_info(" ---- ");
	return 0;
}

static int hisi_drv_connector_off(uint32_t connector_id, struct platform_device *pdev)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_connector_priv_ops *master_ops = NULL;
	struct hisi_pipeline_ops *next_dev_ops = NULL;
	int ret;

	connector_dev = platform_get_drvdata(pdev);
	master_ops = &connector_dev->master.connector_ops;

	connector_dev->fix_panel_info->lcd_uninit_step = LCD_UNINIT_MIPI_HS_SEND_SEQUENCE;
	next_dev_ops = connector_dev->next_dev_ops;
	dpu_check_and_call_func(ret, next_dev_ops->turn_off, connector_id, connector_dev->next_dev);

	dpu_check_and_call_func(ret, master_ops->connector_off, connector_id, pdev);

	hisi_backlight_cancel(connector_dev);

	return 0;
}

static int hisi_drv_connector_present(uint32_t connector_id, struct platform_device *pdev, void *data)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_connector_priv_ops *master_ops = NULL;
	struct hisi_pipeline_ops *next_dev_ops = NULL;
	struct hisi_panel_info *fix_panel_info = NULL;
	int ret;
	disp_pr_debug(" ++++ ");
	connector_dev = platform_get_drvdata(pdev);
	master_ops = &connector_dev->master.connector_ops;

	dpu_check_and_call_func(ret, master_ops->connector_present, connector_id, pdev, data);

	next_dev_ops = connector_dev->next_dev_ops;
	dpu_check_and_call_func(ret, next_dev_ops->present, connector_id, connector_dev->next_dev, data);

	fix_panel_info =  connector_dev->fix_panel_info;
	if(!fix_panel_info) {
		disp_pr_err("fix_panel_info  is NULL\n");
		return -EINVAL;
	}

	if (fix_panel_info->bl_info.bl_type == BL_SET_BY_NONE)
		return 0;
	hisi_backlight_update(connector_dev,fix_panel_info->bl_info.bl_default,false);
	disp_pr_debug(" ---- ");
	return 0;
}

static struct hisi_pipeline_ops g_hisi_connector_ops = {
	.turn_on = hisi_drv_connector_on,
	.turn_off = hisi_drv_connector_off,
	.present = hisi_drv_connector_present,
};

static int hisi_drv_connector_set_backlight ( struct platform_device *pdev, uint32_t level)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_panel_ops *panel_ops = NULL;
	struct hisi_bl_ops *bl_ops = NULL;
	int ret = 0;
	disp_pr_info("++++\n");
	if (!pdev) {
		disp_pr_err("pedv is NULL\n");
		return -EINVAL;
	}

	connector_dev = platform_get_drvdata(pdev);
	if(!connector_dev) {
		disp_pr_err("connector_dev is NULL\n");
		return -EINVAL;
      }

	if(connector_dev->next_dev_ops) {
		panel_ops = (struct hisi_panel_ops *)connector_dev->next_dev_ops;
		bl_ops =  &panel_ops->bl_ops;
		if(bl_ops->set_backlight) {
			ret =  bl_ops->set_backlight(connector_dev->next_dev, level);
		}
	}

       return ret;
}

static struct hisi_bl_ops g_hisi_connector_bl_ops = {
	.set_backlight = hisi_drv_connector_set_backlight ,
};

void hisi_drv_connector_register_bl_ops(struct hisi_bl_ops **connector_bl_ops)
{
	*connector_bl_ops = &g_hisi_connector_bl_ops;
}

void hisi_drv_connector_register_ops(struct hisi_pipeline_ops **connector_ops)
{
	*connector_ops = &g_hisi_connector_ops;
}

