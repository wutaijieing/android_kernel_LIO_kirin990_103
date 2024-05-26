/**@file
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

#include "hisi_fb.h"
#include "hisi_fb_pm.h"
#include "hisi_disp_config.h"
#include "hisi_disp_smmu.h"
#include "hisi_composer_core.h"
#include "hisi_fb_sysfs.h"
#include "hisi_fb_vsync.h"

static int hisi_fb_base_turn_on(uint32_t id, struct platform_device *pdev)
{
	struct hisi_fb_info *info = NULL;
	struct hisi_pipeline_ops *next_ops = NULL;

	disp_pr_info("enter ++++");

	info = platform_get_drvdata(pdev);
	disp_assert_if_cond(info == NULL);
	next_ops = info->next_ops;

	// TODO: other action

	// turn on next device
	if (next_ops->turn_on)
		next_ops->turn_on(info->id, info->next_pdev);

	return 0;
}

static int hisi_fb_base_turn_off(uint32_t id, struct platform_device *pdev)
{
	struct hisi_fb_info *info = NULL;
	struct hisi_pipeline_ops *next_ops = NULL;

	disp_pr_info("enter ++++");

	info = platform_get_drvdata(pdev);
	disp_assert_if_cond(info == NULL);

	next_ops = info->next_ops;

	// turn off next device
	if (next_ops->turn_off)
		next_ops->turn_off(info->id, info->next_pdev);

	// TODO: other action

	return 0;
}

static int hisi_fb_base_present(uint32_t id, struct platform_device *pdev, void *data)
{
	struct hisi_fb_info *info = NULL;
	struct hisi_composer_ops *composer_ops = NULL;
	struct timespec64 prepare_tv;
	struct timespec64 present_tv;
	int ret;

	disp_pr_debug(" ++++ ");

	info = platform_get_drvdata(pdev);
	disp_assert_if_cond(info == NULL);

	composer_ops = (struct hisi_composer_ops*)info->next_ops;
	if (!composer_ops) {
		disp_pr_info(" ----1 ");
		return 0;
	}

	disp_trace_ts_begin(&present_tv);

	down(&info->blank_sem);
	if (!hisi_pm_is_unblank(info)) {
		// up(&info->blank_sem); ybr
		disp_pr_debug(" ----2 ");
		// return 0;
	}

	if (composer_ops->base.present)
		composer_ops->base.present(info->id, info->next_pdev, data);

	// TODO: other action

	up(&info->blank_sem);

	disp_trace_ts_end(&present_tv, "fb present");

	disp_pr_debug(" ----3 ");
	return 0;
}

static bool hisi_fb_fastboot_display (struct fb_info *info) {
	struct hisi_fb_info *hfb_info = NULL;
	struct hisi_composer_device *ov_device = NULL;
	struct hisi_composer_data *ov_data = NULL;
	struct hisi_disp_isr_ops *isr_ops = NULL;
	int ret;

	disp_pr_info(" enter ++++");

	hfb_info = (struct hisi_fb_info *)info->par;
	ov_device = platform_get_drvdata(hfb_info->next_pdev);
	ov_data = &ov_device->ov_data;

	/* 1, enable regulator */
	hisi_disp_pm_enable_regulators(ov_data->regulator_bits);

	hisi_online_init(ov_data);

	// hisi_drv_connector_set_fastboot();
	hfb_info->pwr_on = true;

	return true;
}

void hisi_fb_base_register_private_ops(void *info)
{
	struct hisi_fb_info *fb_info = (struct hisi_fb_info *)info;
	struct hisi_fb_priv_ops *fb_priv_ops = &(fb_info->fb_priv_ops);
	uint32_t  enable_fastboot_display = 0;

	fb_priv_ops->base.turn_on = hisi_fb_base_turn_on;
	fb_priv_ops->base.turn_off = hisi_fb_base_turn_off;
	fb_priv_ops->base.present = hisi_fb_base_present;
	enable_fastboot_display = hisi_config_get_enable_fastboot_display_flag();
	disp_pr_info("enable_fastboot_display %d",enable_fastboot_display);

	if (enable_fastboot_display) {
		fb_priv_ops->open.set_fastboot_cb = hisi_fb_fastboot_display;
	} else {
		fb_priv_ops->open.set_fastboot_cb = NULL;
	}

	fb_priv_ops->vsync_ops.vsync_register = hisifb_vsync_register;
	fb_priv_ops->vsync_ops.vsync_unregister = hisifb_vsync_unregister;
	fb_priv_ops->vsync_ops.vsync_ctrl_fnc = hisifb_vsync_ctrl;
	fb_priv_ops->vsync_ops.vsync_isr_handler = hisifb_vsync_isr_handler;

	fb_priv_ops->sysfs_ops.sysfs_attrs_append_fnc = hisifb_sysfs_attrs_append;
	fb_priv_ops->sysfs_ops.sysfs_create_fnc = hisifb_sysfs_create;
	fb_priv_ops->sysfs_ops.sysfs_remove_fnc = hisifb_sysfs_remove;
}

void hisi_external_fb_base_register_private_ops(void *info)
{
	struct hisi_fb_info *fb_info = (struct hisi_fb_info *)info;
	struct hisi_fb_priv_ops *fb_priv_ops = &(fb_info->fb_priv_ops);
	struct hisi_composer_device *ov_dev = NULL;
	struct hisi_connector_device *connector_dev = NULL;

	disp_pr_info(" ++++ ");

	fb_priv_ops->base.turn_on = hisi_fb_base_turn_on;
	fb_priv_ops->base.turn_off = hisi_fb_base_turn_off;
	fb_priv_ops->base.present = hisi_fb_base_present;

	ov_dev = platform_get_drvdata(fb_info->next_pdev);
	if (!ov_dev) {
		disp_pr_info(" ----1 ");
		return;
	}

	connector_dev = platform_get_drvdata(ov_dev->next_device[0]->next_pdev);
	if (!connector_dev) {
		disp_pr_info(" ----2 ");
		return;
	}

	//connector_dev->master.connector_ops.dpu_channel_on = fb_priv_ops->base.turn_on;
	//connector_dev->master.connector_ops.dpu_channel_off= fb_priv_ops->base.turn_off;

	fb_priv_ops->vsync_ops.vsync_register = hisifb_vsync_register;
	fb_priv_ops->vsync_ops.vsync_unregister = hisifb_vsync_unregister;
	fb_priv_ops->vsync_ops.vsync_ctrl_fnc = hisifb_vsync_ctrl;
	fb_priv_ops->vsync_ops.vsync_isr_handler = hisifb_vsync_isr_handler;

	fb_priv_ops->sysfs_ops.sysfs_attrs_append_fnc = hisifb_sysfs_attrs_append;
	fb_priv_ops->sysfs_ops.sysfs_create_fnc = hisifb_sysfs_create;
	fb_priv_ops->sysfs_ops.sysfs_remove_fnc = hisifb_sysfs_remove;

	disp_pr_info(" ---- ");
}


