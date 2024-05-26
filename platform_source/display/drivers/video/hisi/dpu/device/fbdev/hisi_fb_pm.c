 /* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include <linux/fb.h>

#include "hisi_fb_pm.h"

int hisi_fb_suspend_sub(struct platform_device *pdev, pm_message_t state)
{
	disp_pr_info("enter ++++\n");

	int ret = 0;
	struct hisi_fb_info *hfb_info = NULL;
	disp_check_and_return(!pdev, -EINVAL, err, "pdev is NULL\n");

	hfb_info = platform_get_drvdata(pdev);
	disp_check_and_return(!hfb_info, -EINVAL, err, "fb_info is NULL\n");

	disp_pr_info("enter ++++, %d\n", hfb_info->index);

	if (hfb_info->index != PRIMARY_PANEL_IDX)
		return 0;

	ret = hisi_fb_pm_blank(FB_BLANK_POWERDOWN, hfb_info);
	if (ret) {
		disp_pr_err("fb%d can't turn off display, error=%d!\n", hfb_info->index, ret);
		return ret;
	}

	return ret;
}

int hisi_fb_resume_sub(struct platform_device *pdev)
{
	disp_pr_info("enter ++++\n");

	int ret = 0;
	struct hisi_fb_info *hfb_info = NULL;
	disp_check_and_return(!pdev, -EINVAL, err, "pdev is NULL\n");

	hfb_info = platform_get_drvdata(pdev);
	disp_check_and_return(!hfb_info, -EINVAL, err, "fb_info is NULL\n");

	if (hfb_info->index != PRIMARY_PANEL_IDX)
		return 0;

	ret = hisi_fb_pm_blank(FB_BLANK_UNBLANK, hfb_info);
	if (ret) {
		disp_pr_err("fb%d can't turn on display, error=%d!\n", hfb_info->index, ret);
		return ret;
	}

	return ret;
}

int hisi_fb_runtime_suspend_sub(struct device *dev)
{
	return 0;
}

int hisi_fb_runtime_resume_sub(struct device *dev)
{
	return 0;
}

int hisi_fb_runtime_idle_sub(struct device *dev)
{
	return 0;
}

int hisi_fb_pm_resume_on_fpga(struct device *dev)
{
	return 0;
}

int hisi_fb_pm_suspend_sub(struct device *dev)
{
	return 0;
}

int hisi_fb_pm_force_blank(int blank_mode, struct hisi_fb_info *hfb_info, bool force)
{
	struct hisi_fb_priv_ops *fb_ops = NULL;

	fb_ops = &hfb_info->fb_priv_ops;

	disp_pr_info("enter ++++, hfb_info->id=%d, pwr_on=%d\n", hfb_info->id, hfb_info->pwr_on);

	/* TODO: other modules listen to the blank event, FB_EARLY_EVENT_BLANK or FB_EVENT_BLANK
	 * such as, AOD. those event had been sended at fbmen.c
	 */
	down(&hfb_info->blank_sem);
	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		if (!hfb_info->pwr_on || force) {
			if (fb_ops->base.turn_on)
				fb_ops->base.turn_on(hfb_info->id, hfb_info->pdev);

			hfb_info->pwr_on = true;
		}
		break;
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
	case FB_BLANK_POWERDOWN:
	default:
		if (hfb_info->pwr_on || force) {
			if (fb_ops->base.turn_off)
				fb_ops->base.turn_off(hfb_info->id, hfb_info->pdev);

			hfb_info->pwr_on = false;
		}
		break;
	}

	disp_pr_info("exit ----, pwr_on=%d\n", hfb_info->pwr_on);

	up(&hfb_info->blank_sem);
	return 0;
}

int hisi_fb_pm_blank(int blank_mode, struct hisi_fb_info *hfb_info)
{
	return hisi_fb_pm_force_blank(blank_mode, hfb_info, false);
}
