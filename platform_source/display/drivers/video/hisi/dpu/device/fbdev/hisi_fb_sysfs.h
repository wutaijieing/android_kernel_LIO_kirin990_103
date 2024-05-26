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

#ifndef HISI_FB_SYSFS_H
#define HISI_FB_SYSFS_H

/*******************************************************************************
 * fb sys fs
 */

void hisifb_sysfs_init(struct hisi_fb_info *hfb_info);

void hisifb_sysfs_attrs_append(struct hisi_fb_info *hfb_info, struct attribute *attr);

int hisifb_sysfs_create(struct platform_device *pdev);

void hisifb_sysfs_remove(struct platform_device *pdev);

#endif /* HISI_FB_SYSFS_H */
