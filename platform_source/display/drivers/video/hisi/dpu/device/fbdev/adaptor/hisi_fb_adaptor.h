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
#ifndef HISI_FB_ADAPTOR_H
#define HISI_FB_ADAPTOR_H

int hisi_fb_adaptor_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);

struct hisi_fb_info;
struct dpu_source_layers;

int hisi_fb_open(struct fb_info *info, int user);
int hisi_fb_adaptor_online_play(struct hisi_fb_info *hfb_info, struct dpu_source_layers *sourcelayers);
int hisi_fb_release(struct fb_info *info, int user);
#endif /* HISI_FB_ADAPTOR_H */
