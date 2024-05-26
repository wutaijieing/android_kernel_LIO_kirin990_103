/* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"

#include <linux/uaccess.h>
#include "hisi_fb.h"

int g_debug_online_vsync;

#ifdef CONFIG_DKMD_DPU_V720
extern void mali_kbase_pm_report_vsync(int);
#endif

static int vsync_timestamp_changed(struct hisi_fb_info *fb_info,
	ktime_t prev_timestamp)
{
	if (!fb_info) {
		disp_pr_err("fb_info is NULL\n");
		return -EINVAL;
	}
	return !(prev_timestamp == fb_info->vsync_ctrl.vsync_timestamp);
}

static ssize_t vsync_show_event(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	int vsync_flag;
	struct fb_info *fbi = NULL;
	struct hisi_fb_info *fb_info = NULL;
	ktime_t prev_timestamp;

	disp_check_and_return((!dev), -1, err, "dev is NULL.\n");

	fbi = dev_get_drvdata(dev);
	disp_check_and_return((!fbi), -1, err, "fbi is NULL.\n");

	fb_info = (struct hisi_fb_info *)fbi->par;
	disp_check_and_return((!fb_info), -1, err, "fb_info is NULL.\n");

	disp_check_and_return((!buf), -1, err, "buf is NULL.\n");

	//disp_pr_info(" ++++ ");
	prev_timestamp = fb_info->vsync_ctrl.vsync_timestamp;

	/*lint -e666*/
	ret = wait_event_interruptible(fb_info->vsync_ctrl.vsync_wait, /*lint !e578*/
		(vsync_timestamp_changed(fb_info, prev_timestamp) && fb_info->vsync_ctrl.vsync_enabled));
	/*lint +e666*/

	vsync_flag = (vsync_timestamp_changed(fb_info, prev_timestamp) && fb_info->vsync_ctrl.vsync_enabled);

	if (vsync_flag) {
		ret = snprintf(buf, PAGE_SIZE, "VSYNC=%llu, xxxxxxEvent=x\n",
			ktime_to_ns(fb_info->vsync_ctrl.vsync_timestamp));
		buf[strlen(buf) + 1] = '\0';
	} else {
		return -1;
	}
	//disp_pr_info(" ---- ");
	return ret;
}

static ssize_t vsync_timestamp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct fb_info *fbi = NULL;
	struct hisi_fb_info *fb_info = NULL;

	if (!dev) {
		disp_pr_err("NULL Pointer.\n");
		return -1;
	}

	fbi = dev_get_drvdata(dev);
	if (!fbi) {
		disp_pr_err("NULL Pointer.\n");
		return -1;
	}

	fb_info = (struct hisi_fb_info *)fbi->par;
	if (!fb_info) {
		disp_pr_err("NULL Pointer.\n");
		return -1;
	}

	if (!buf) {
		disp_pr_err("NULL Pointer.\n");
		return -1;
	}
	disp_pr_info(" ++++ ");
	ret = snprintf(buf, PAGE_SIZE, "%llu\n", ktime_to_ns(fb_info->vsync_ctrl.vsync_timestamp));
	buf[strlen(buf) + 1] = '\0';

	disp_pr_info(" ---- ");
	return ret;
}

static DEVICE_ATTR(vsync_event, 0444, vsync_show_event, NULL);
static DEVICE_ATTR(vsync_timestamp, 0444, vsync_timestamp_show, NULL);

void hisifb_vsync_register(struct platform_device *pdev)
{
	struct hisi_fb_info *hfb_info = NULL;
	struct hisifb_vsync *vsync_ctrl = NULL;
	disp_pr_info(" ++++ ");
	if (!pdev) {
		disp_pr_err("pdev is NULL\n");
		return;
	}
	hfb_info = platform_get_drvdata(pdev);
	if (!hfb_info) {
		dev_err(&pdev->dev, "fb_info is NULL\n");
		return;
	}
	disp_pr_info(" ++2++ ");

	vsync_ctrl = &(hfb_info->vsync_ctrl);
	if (!vsync_ctrl) {
		dev_err(&pdev->dev, "vsync_ctrl is NULL\n");
		return;
	}

	disp_pr_info(" ++3++ ");

	if (vsync_ctrl->vsync_created)
		return;

	mutex_init(&(vsync_ctrl->vsync_lock));
	mutex_lock(&(vsync_ctrl->vsync_lock));
	vsync_ctrl->hfb_info = hfb_info;
	vsync_ctrl->vsync_infinite = 0;
	vsync_ctrl->vsync_enabled = 0;
	vsync_ctrl->vsync_ctrl_offline_enabled = 0;
	mutex_unlock(&(vsync_ctrl->vsync_lock));

	vsync_ctrl->vsync_timestamp = ktime_get();
	vsync_ctrl->vactive_timestamp = ktime_get();

	init_waitqueue_head(&(vsync_ctrl->vsync_wait));
	spin_lock_init(&(vsync_ctrl->spin_lock));
	//INIT_WORK(&vsync_ctrl->vsync_ctrl_work, hisifb_vsync_ctrl_workqueue_handler);

	atomic_set(&(vsync_ctrl->buffer_updated), 1);
#ifdef CONFIG_REPORT_VSYNC
	vsync_ctrl->vsync_report_fnc = mali_kbase_pm_report_vsync;
#else
	vsync_ctrl->vsync_report_fnc = NULL;
#endif

	if (hfb_info->fb_priv_ops.sysfs_ops.sysfs_attrs_append_fnc) {
		hfb_info->fb_priv_ops.sysfs_ops.sysfs_attrs_append_fnc(hfb_info, &dev_attr_vsync_event.attr);
		hfb_info->fb_priv_ops.sysfs_ops.sysfs_attrs_append_fnc(hfb_info, &dev_attr_vsync_timestamp.attr);
	}

	vsync_ctrl->vsync_created = 1;
	disp_pr_info(" ---- ");
}

void hisifb_vsync_unregister(struct platform_device *pdev)
{
	struct hisi_fb_info *hfb_info = NULL;
	struct hisifb_vsync *vsync_ctrl = NULL;

	if (!pdev) {
		disp_pr_err("pdev is NULL\n");
		return;
	}
	hfb_info = platform_get_drvdata(pdev);
	if (!hfb_info) {
		dev_err(&pdev->dev, "fb_info is NULL\n");
		return;
	}
	vsync_ctrl = &(hfb_info->vsync_ctrl);
	if (!vsync_ctrl) {
		dev_err(&pdev->dev, "vsync_ctrl is NULL\n");
		return;
	}

	if (!vsync_ctrl->vsync_created)
		return;

	vsync_ctrl->vsync_created = 0;
}

int hisifb_vsync_ctrl(struct fb_info *info, const void __user *argp)
{
	int ret;
	struct hisi_fb_info *hfb_info = NULL;
	struct hisifb_vsync *vsync_ctrl = NULL;
	int enable = 0;
	disp_pr_debug(" ++++ ");

	disp_check_and_return((!info), -EINVAL, err, "vsync ctrl info NULL Pointer!\n");

	hfb_info = (struct hisi_fb_info *)info->par;
	disp_check_and_return((!hfb_info), -EINVAL, err, "vsync ctrl hisifd NULL Pointer!\n");

	disp_check_and_return(((hfb_info->index != PRIMARY_PANEL_IDX) && (hfb_info->index!= EXTERNAL_PANEL_IDX)),
		-EINVAL, err, "fb%d, vsync ctrl not supported!\n", hfb_info->index);

	vsync_ctrl = &(hfb_info->vsync_ctrl);
	disp_check_and_return((!vsync_ctrl), -EINVAL, err, "vsync_ctrl NULL Pointer!\n");

	disp_check_and_return((!argp), -EINVAL, err, "argp NULL Pointer!\n");

	ret = copy_from_user(&enable, argp, sizeof(enable));
	disp_check_and_return(ret, ret, err, "vsync ctrl ioctl failed!\n");

	enable = (enable) ? 1 : 0;

	mutex_lock(&(vsync_ctrl->vsync_lock));

	if (vsync_ctrl->vsync_enabled == enable) {
		mutex_unlock(&(vsync_ctrl->vsync_lock));
		return 0;
	}

	disp_pr_debug("fb%d, enable=%d!\n", hfb_info->index, enable);

	vsync_ctrl->vsync_enabled = 0; // enable; // tmp

	mutex_unlock(&(vsync_ctrl->vsync_lock));

	disp_pr_debug(" ---- ");

	return 0;
}

void hisifb_vsync_isr_handler(struct hisi_fb_info *hfb_info)
{
	struct hisifb_vsync *vsync_ctrl = NULL;
	int buffer_updated = 0;
	ktime_t pre_vsync_timestamp;

	disp_check_and_no_retval(!hfb_info, err, "hisifd is NULL\n");

	vsync_ctrl = &(hfb_info->vsync_ctrl);

	pre_vsync_timestamp = vsync_ctrl->vsync_timestamp;
	vsync_ctrl->vsync_timestamp = ktime_get();

	if (hfb_info->vsync_ctrl.vsync_enabled)
		wake_up_interruptible_all(&(vsync_ctrl->vsync_wait));

	if (g_debug_online_vsync)
		disp_pr_debug("fb%d, VSYNC=%llu, time_diff=%llu.\n", hfb_info->index,
			ktime_to_ns(hfb_info->vsync_ctrl.vsync_timestamp),
			(ktime_to_ns(hfb_info->vsync_ctrl.vsync_timestamp) - ktime_to_ns(pre_vsync_timestamp)));
}

#pragma GCC diagnostic pop

