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
#ifndef HISI_FB_PM_H
#define HISI_FB_PM_H

#include <linux/platform_device.h>
#include "hisi_fb.h"

int hisi_fb_suspend_sub(struct platform_device *pdev, pm_message_t state);
int hisi_fb_resume_sub(struct platform_device *pdev);
int hisi_fb_runtime_suspend_sub(struct device *dev);
int hisi_fb_runtime_resume_sub(struct device *dev);
int hisi_fb_runtime_idle_sub(struct device *dev);
int hisi_fb_pm_resume_on_fpga(struct device *dev);
int hisi_fb_pm_suspend_sub(struct device *dev);
void hisi_pm_register_early_suspend(void);
int hisi_fb_pm_blank(int blank_mode, struct hisi_fb_info *fb_info);
int hisi_fb_pm_force_blank(int blank_mode, struct hisi_fb_info *hfb_info, bool force);

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
#define hisi_fb_suspend hisi_fb_suspend_sub
#define hisi_fb_resume hisi_fb_resume_sub
#else
#define hisi_fb_suspend NULL
#define hisi_fb_resume NULL
#endif

#ifdef CONFIG_PM_RUNTIME
#define hisi_fb_runtime_suspend hisi_fb_runtime_suspend_sub
#define hisi_fb_runtime_resume hisi_fb_runtime_resume_sub
#define hisi_fb_runtime_idle hisi_fb_runtime_idle_sub
#else
#define hisi_fb_runtime_suspend NULL
#define hisi_fb_runtime_resume NULL
#define hisi_fb_runtime_idle NULL
#endif

#ifdef SUPPORT_FPGA_SUSPEND_RESUME
#define hisi_fb_pm_resume hisi_fb_pm_resume_on_fpga
#else
#define hisi_fb_pm_resume NULL
#endif

#ifdef CONFIG_PM_SLEEP
#define hisi_fb_pm_suspend hisi_fb_pm_suspend_sub
#else
#undef hisi_fb_pm_suspend
#define hisi_fb_pm_suspend NULL

#undef hisi_fb_pm_resume
#define hisi_fb_pm_resume NULL
#endif

static inline bool hisi_pm_is_unblank(struct hisi_fb_info *hfb_info)
{
	return hfb_info->be_connected && hfb_info->pwr_on;
}

#endif /* HISI_FB_PM_H */
