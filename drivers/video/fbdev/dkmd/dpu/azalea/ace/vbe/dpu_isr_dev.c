/* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>

#include "dpu_fb_def.h"
#include "dpu_fb_struct.h"
#include "dpu_fb.h"
#include "chrdev/dpu_chrdev.h"
#include "dpu_fb_panel.h"

static struct cdev isr_cdev;

#define VSYNC0_QUERY _IOW(DPUFB_IOCTL_MAGIC, 0xC3, struct isr_info)
#define VSYNC1_QUERY _IOW(DPUFB_IOCTL_MAGIC, 0xC4, struct isr_info)
#define VACTIVE_START_QUERY _IOW(DPUFB_IOCTL_MAGIC, 0xC5, struct isr_info)

#define WAIT_ISR_TIMEOUT_THRESHOLD 50000
#define DEV_NUM 1

struct isr_info {
	int32_t isr_type;
	uint64_t timestamp;
};

typedef enum dpufb_isr_type {
	ISR_NONE = -1,
	ISR_VSYNC0,
	ISR_VSYNC1,
	ISR_VSTART1,
	ISR_MAX_TYPE_NUM
} dpufb_isr_type_t;

static bool dpufb_isr_timestamp_changed(ktime_t cur_timestamp, ktime_t prev_timestamp, int32_t isr_type)
{
	void_unused(isr_type);
	return !(prev_timestamp == cur_timestamp);
}

static int dpu_vsync0_query(struct isr_info *info)
{
	struct dpu_fb_data_type *dpufd = NULL;
	static ktime_t prev_timestamp = 0;
	struct timeval tv_start;

	dpu_trace_ts_begin(&tv_start);

	dpufd = dpufb_get_dpufd(PRIMARY_PANEL_IDX);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	(void)wait_event_interruptible_timeout(dpufd->vsync_ctrl.vsync_wait,
		dpufb_isr_timestamp_changed(dpufd->vsync_ctrl.vsync_timestamp, prev_timestamp, ISR_VSYNC0),
		msecs_to_jiffies(WAIT_ISR_TIMEOUT_THRESHOLD));

	if (g_debug_online_vsync)
		DPU_FB_INFO("fb%d, VSYNC=%llu, time_diff=%llu.\n", dpufd->index,
			ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp),
			(ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp) - ktime_to_ns(prev_timestamp)));

	info->isr_type = ISR_VSYNC0;
	info->timestamp = (uint64_t)(dpufd->vsync_ctrl.vsync_timestamp);
	prev_timestamp = (s64)info->timestamp;

	if (is_mipi_video_panel(dpufd))
		dpu_trace_ts_end(&tv_start, g_debug_isr_ioctl_timeout, "vsync0 isr ioctl cost:");
	return 0;
}

static int dpu_vactive_start_query(struct isr_info *info)
{
	struct dpu_fb_data_type *dpufd = NULL;
	static ktime_t prev_timestamp = 0;

	dpufd = dpufb_get_dpufd(EXTERNAL_PANEL_IDX);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	(void)wait_event_interruptible_timeout(dpufd->vsync_ctrl.vactive_wait,
		dpufb_isr_timestamp_changed(dpufd->vsync_ctrl.vactive_timestamp, prev_timestamp, ISR_VSTART1),
		msecs_to_jiffies(WAIT_ISR_TIMEOUT_THRESHOLD));

	if (g_debug_online_vactive)
		DPU_FB_INFO("fb%d, VSTART=%llu, time_diff=%llu.\n", dpufd->index,
			ktime_to_ns(dpufd->vsync_ctrl.vactive_timestamp),
			(ktime_to_ns(dpufd->vsync_ctrl.vactive_timestamp) - ktime_to_ns(prev_timestamp)));

	info->isr_type = ISR_VSTART1;
	info->timestamp = (uint64_t)(dpufd->vsync_ctrl.vactive_timestamp);
	prev_timestamp = (s64)info->timestamp;

	return 0;
}

static int dpu_vsync1_query(struct isr_info *info)
{
	struct dpu_fb_data_type *dpufd = NULL;
	static ktime_t prev_timestamp = 0;
	struct timeval tv_start;

	dpu_trace_ts_begin(&tv_start);

	dpufd = dpufb_get_dpufd(EXTERNAL_PANEL_IDX);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	(void)wait_event_interruptible_timeout(dpufd->vsync_ctrl.vsync_wait,
		dpufb_isr_timestamp_changed(dpufd->vsync_ctrl.vsync_timestamp, prev_timestamp, ISR_VSYNC1),
		msecs_to_jiffies(WAIT_ISR_TIMEOUT_THRESHOLD));

	if (g_debug_online_vsync)
		DPU_FB_INFO("fb%d, VSYNC=%llu, time_diff=%llu.\n", dpufd->index,
			ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp),
			(ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp) - ktime_to_ns(prev_timestamp)));

	info->isr_type = ISR_VSYNC1;
	info->timestamp = (uint64_t)(dpufd->vsync_ctrl.vsync_timestamp);
	prev_timestamp = (s64)info->timestamp;

	if (is_mipi_video_panel(dpufd))
		dpu_trace_ts_end(&tv_start, g_debug_isr_ioctl_timeout, "vsync1 isr ioctl cost:");
	return 0;
}

static long dpufb_isr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct isr_info info = {0};
	int ret = -1;
	void __user *argp = (void __user *)(uintptr_t)arg;
	void_unused(file);

	switch (cmd) {
	case VSYNC0_QUERY:
		ret = dpu_vsync0_query(&info);
		break;
	case VSYNC1_QUERY:
		ret = dpu_vsync1_query(&info);
		break;
	case VACTIVE_START_QUERY:
		ret = dpu_vactive_start_query(&info);
		break;
	default:
		DPU_FB_ERR("unsupported cmd:%u\n", cmd);
		return -1;
	}

	(void)copy_to_user((struct isr_info *)argp, &info, sizeof(struct isr_info));
	return ret;
}

static struct file_operations g_isr_ops = {
	.owner = THIS_MODULE,
	.read = NULL,
	.write = NULL,
	.open = NULL,
	.unlocked_ioctl = dpufb_isr_ioctl,
	.release = NULL,
};

int create_isr_query_dev(void)
{
	dev_t dev = 0;

	/* Allocating Major number */
	if(alloc_chrdev_region(&dev, 0, DEV_NUM, "dpu_isr_query") < 0) {
		DPU_FB_ERR("Cannot allocate major number\n");
		return -1;
	}

	/* Creating cdev structure */
	cdev_init(&isr_cdev, &g_isr_ops);

	/* Adding character device to the system */
	if(cdev_add(&isr_cdev, dev, DEV_NUM) < 0){
		DPU_FB_ERR("Cannot add the device to the system\n");
		return -1;
	}

	/* Creating device */
	if(!device_create(get_fb_class(), NULL, dev, NULL, "dpu_isr_query")){
		DPU_FB_ERR("create dpu_isr_query failed\n\n");
		return -1;
	}
	DPU_FB_INFO("dpu_isr_query dev is created\n");
	return 0;
}
