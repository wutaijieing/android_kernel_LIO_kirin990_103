/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef HISI_FB_H
#define HISI_FB_H

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/atomic.h>
#include <linux/delay.h>

#include "hisi_drv_utils.h"
#include "hisi_disp_composer.h"
#include "hisi_disp.h"
#include "hisi_drv_kthread.h"

#include "adaptor/hisi_dss.h"

#define disp_get_frame_idx(idx) ((idx) % DISP_FRAME_MAX)

#define HISI_FB_SYSFS_ATTRS_NUM 64
#define HISI_FB_MAX_FBI_LIST 32

enum {
	DISP_FRAME_USING = 0,
	DISP_FRAME_DISPLAYING,
	DISP_FRAME_DISPLAYED,
	DISP_FRAME_MAX
};

struct hisi_fb_info;

struct hisi_fb_open_ops {
	int (*init_cmdlist_cb)(struct hisi_fb_info *hfb_info);
	int (*deinit_cmdlist_cb)(struct hisi_fb_info *hfb_info);
	bool (*set_fastboot_cb)(struct fb_info *info);
};

struct hisi_fb_vsync_ops {
	void (*vsync_register)(struct platform_device *pdev);
	void (*vsync_unregister)(struct platform_device *pdev);
	int (*vsync_ctrl_fnc)(struct fb_info *info, const void __user *argp);
	void (*vsync_isr_handler)(struct hisi_fb_info *hfb_info);
};

struct hisi_fb_sysfs_ops {
	void (*sysfs_attrs_append_fnc)(struct hisi_fb_info *hfb_info, struct attribute *attr);
	int (*sysfs_create_fnc)(struct platform_device *pdev);
	void (*sysfs_remove_fnc)(struct platform_device *pdev);
	void (*sysfs_attrs_add_fnc)(struct hisi_fb_info *hfb_info);
};

struct hisi_fb_priv_ops {
	struct hisi_pipeline_ops base;

	/* other fb private ops, such as primary and external have different ops */
	struct hisi_fb_open_ops open;

	struct hisi_fb_vsync_ops vsync_ops;
	struct hisi_fb_sysfs_ops sysfs_ops;
};

/* TODO: need loop the frame_id */
struct hisi_present_data {
	struct semaphore frames_sem;

	struct hisi_fb_info *hfb_info;
	struct hisi_display_frame frames[DISP_FRAME_MAX];
	uint32_t using_idx;
	uint32_t displaying_idx;
	uint32_t displayed_idx;
};

typedef struct hisi_fb_fix_var_screeninfo {
	uint32_t fix_type;
	uint32_t fix_xpanstep;
	uint32_t fix_ypanstep;
	uint32_t var_vmode;

	uint32_t var_blue_offset;
	uint32_t var_green_offset;
	uint32_t var_red_offset;
	uint32_t var_transp_offset;

	uint32_t var_blue_length;
	uint32_t var_green_length;
	uint32_t var_red_length;
	uint32_t var_transp_length;

	uint32_t var_blue_msb_right;
	uint32_t var_green_msb_right;
	uint32_t var_red_msb_right;
	uint32_t var_transp_msb_right;
	uint32_t bpp;
} hisi_fb_fix_var_screeninfo_t;

struct hisifb_vsync {
	wait_queue_head_t vsync_wait;
	ktime_t vsync_timestamp;
	ktime_t vsync_timestamp_prev;
	ktime_t vactive_timestamp;
	int vsync_created;
	/* flag for soft vsync signal synchronizing with TE */
	int vsync_enabled;
	int vsync_infinite;
	int vsync_infinite_count;

	int vsync_ctrl_expire_count;
	int vsync_ctrl_enabled;
	int vsync_ctrl_disabled_set;
	int vsync_ctrl_isr_enabled;
	int vsync_ctrl_offline_enabled;
	int vsync_disable_enter_idle;
	struct work_struct vsync_ctrl_work;
	spinlock_t spin_lock;

	struct mutex vsync_lock;

	atomic_t buffer_updated;
	void (*vsync_report_fnc)(int buffer_updated);

	struct hisi_fb_info *hfb_info;
};

struct hisi_fb_info {
	uint32_t index;
	struct platform_device *pdev;
	uint32_t id;

	atomic_t ref_cnt;
	bool be_connected;
	bool pwr_on;
	bool enable_present_async;

	struct semaphore blank_sem;
	struct hisi_fb_priv_ops fb_priv_ops;

	struct fb_info *fbi_info;
	struct hisi_display_info fix_display_info;

	struct hisi_disp_kthread present_kthread;
	struct hisi_present_data present_data;

	struct platform_device *next_pdev;
	struct hisi_pipeline_ops *next_ops;

	struct hisifb_vsync vsync_ctrl;

	int sysfs_index;
	struct attribute *sysfs_attrs[HISI_FB_SYSFS_ATTRS_NUM];
	struct attribute_group sysfs_attr_group;
};

void hisi_fb_create_platform_device(const char *name, void *ov_device, void *ov_ops);

static inline struct hisi_display_frame *hisi_fb_get_using_frame(struct hisi_present_data *present_data)
{
	return &present_data->frames[disp_get_frame_idx(present_data->using_idx)];
}

static inline void hisi_fb_switch_frame_index(struct hisi_present_data *present_data)
{
	present_data->displayed_idx = present_data->displaying_idx;
	present_data->displaying_idx = present_data->using_idx;
	present_data->using_idx = disp_get_frame_idx(present_data->using_idx + 1);
}


#endif /* HISI_FB_H */
