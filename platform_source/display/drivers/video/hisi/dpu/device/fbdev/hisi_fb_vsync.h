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

#ifndef HISI_FB_VSYNC_H
#define HISI_FB_VSYNC_H

void hisifb_vsync_register(struct platform_device *pdev);
void hisifb_vsync_unregister(struct platform_device *pdev);
int hisifb_vsync_ctrl(struct fb_info *info, const void __user *argp);
void hisifb_vsync_isr_handler(struct hisi_fb_info *hfb_info);

#endif /* HISI_FB_VSYNC_H */
