/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/device.h>

#include "dpufe_dbg.h"
#include "dpufe.h"

static bool dpufe_vsync_is_enabled(struct dpufe_vsync *vsync_ctrl)
{
	bool enable = false;
	unsigned long flags = 0;

	spin_lock_irqsave(&vsync_ctrl->vsync_lock, flags);
	enable = !!vsync_ctrl->enabled;
	spin_unlock_irqrestore(&vsync_ctrl->vsync_lock, flags);

	return enable;
}

static int vsync_timestamp_changed(struct dpufe_vsync *vsync_ctrl, ktime_t prev_timestamp)
{
	return !(prev_timestamp == vsync_ctrl->timestamp);
}

static ssize_t vsync_show_event(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	int vsync_flag;
	struct fb_info *fbi = NULL;
	struct dpufe_data_type *dfd = NULL;
	ktime_t prev_timestamp;
	struct dpufe_vsync *vsync_ctrl = NULL;

	dpufe_check_and_return(!dev || !buf, -1, "dev or buf is NULL.\n");

	fbi = dev_get_drvdata(dev);
	dpufe_check_and_return((!fbi), -1, "fbi is NULL.\n");

	dfd = (struct dpufe_data_type *)fbi->par;
	dpufe_check_and_return((!dfd), -1, "dfd is NULL.\n");

	vsync_ctrl = &dfd->vsync_ctrl;
	prev_timestamp = vsync_ctrl->timestamp;

	ret = wait_event_interruptible(vsync_ctrl->vsync_wait,
		(vsync_timestamp_changed(vsync_ctrl, prev_timestamp) && vsync_ctrl->enabled));

	vsync_flag = (vsync_timestamp_changed(vsync_ctrl, prev_timestamp) && vsync_ctrl->enabled);
	if (vsync_flag) {
		ret = snprintf(buf, PAGE_SIZE, "VSYNC=%llu, xxxxxxEvent=x\n",
			ktime_to_ns(vsync_ctrl->timestamp));
		buf[strlen(buf) + 1] = '\0';
	}

	return ret;
}

static ssize_t vsync_timestamp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct fb_info *fbi = NULL;
	struct dpufe_data_type *dfd = NULL;

	if (!dev) {
		DPUFE_ERR("NULL Pointer.\n");
		return -1;
	}

	fbi = dev_get_drvdata(dev);
	if (!fbi) {
		DPUFE_ERR("NULL Pointer.\n");
		return -1;
	}

	dfd = (struct dpufe_data_type *)fbi->par;
	if (!dfd) {
		DPUFE_ERR("NULL Pointer.\n");
		return -1;
	}

	if (!buf) {
		DPUFE_ERR("NULL Pointer.\n");
		return -1;
	}

	ret = snprintf(buf, PAGE_SIZE, "%llu\n", ktime_to_ns(dfd->vsync_ctrl.timestamp));
	buf[strlen(buf) + 1] = '\0';

	return ret;
}

static DEVICE_ATTR(vsync_event, 0444, vsync_show_event, NULL);
static DEVICE_ATTR(vsync_timestamp, 0444, vsync_timestamp_show, NULL);

void dpufe_vsync_unregister(struct dpufe_vsync *vsync_ctrl, struct dpufe_attr *vattrs)
{
	DPUFE_DEBUG("+\n");

	dpufe_check_and_no_retval(!vsync_ctrl, "vsync_ctrl is null\n");
	dpufe_check_and_no_retval(!vattrs, "vattrs is null\n");

	if (!vsync_ctrl->vsync_created)
		return;

	vsync_ctrl->vsync_created = 0;
	DPUFE_DEBUG("-\n");
}

void dpufe_vsync_register(struct dpufe_vsync *vsync_ctrl, struct dpufe_attr *vattrs)
{
	DPUFE_DEBUG("+\n");

	dpufe_check_and_no_retval(!vsync_ctrl, "vsync_ctrl is null\n");
	dpufe_check_and_no_retval(!vattrs, "vattrs is null\n");

	if (vsync_ctrl->vsync_created)
		return;

	init_waitqueue_head(&vsync_ctrl->vsync_wait);

	spin_lock_init(&(vsync_ctrl->vsync_lock));
	vsync_ctrl->enabled = 0;
	vsync_ctrl->timestamp = 0;

	dpufe_sysfs_attrs_append(vattrs, &dev_attr_vsync_event.attr);
	dpufe_sysfs_attrs_append(vattrs, &dev_attr_vsync_timestamp.attr);

	vsync_ctrl->vsync_created = 1;

	DPUFE_DEBUG("-\n");
}

void dpufe_vsync_isr_handler(struct dpufe_vsync *vsync_ctrl, uint64_t timestamp, uint32_t fb_index)
{
	ktime_t pre_timestamp;

	dpufe_check_and_no_retval(!vsync_ctrl, "vsync_ctrl is null ptr\n");

	pre_timestamp = vsync_ctrl->timestamp;
	vsync_ctrl->timestamp = timestamp;

	if (g_dpufe_debug_fence_timeline != 0)
		DPUFE_DEBUG("fb%u VSYNC pre:%lu cur:%lu\n", fb_index, pre_timestamp, vsync_ctrl->timestamp);

	if (dpufe_vsync_is_enabled(vsync_ctrl))
		wake_up_interruptible_all(&(vsync_ctrl->vsync_wait));
}

int dpufe_vsync_ctrl(struct dpufe_vsync *vsync_ctrl, const void __user *argp)
{
	int enable;
	int ret;
	unsigned long flags = 0;

	dpufe_check_and_return(!vsync_ctrl, -1, "vsync_ctrl is null ptr\n");
	dpufe_check_and_return(!argp, -1, "argp is null ptr\n");

	ret = copy_from_user(&enable, argp, sizeof(enable));
	dpufe_check_and_return(ret != 0, -1, "copy from user failed\n");
	DPUFE_DEBUG("%d\n", enable);

	enable = (enable > 0) ? 1 : 0;
	spin_lock_irqsave(&vsync_ctrl->vsync_lock, flags);
	if (vsync_ctrl->enabled == enable) {
		spin_unlock_irqrestore(&vsync_ctrl->vsync_lock, flags);
		return 0;
	}
	vsync_ctrl->enabled = enable;
	spin_unlock_irqrestore(&vsync_ctrl->vsync_lock, flags);

	return 0;
}
