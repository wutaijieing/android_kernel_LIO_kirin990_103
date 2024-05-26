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

#ifndef HISI_TESTCASE_FB_H
#define HISI_TESTCASE_FB_H

#include <linux/types.h>
#include <linux/fb.h>

#include "hisi_drv_test.h"

extern struct hisi_drv_test_module g_hisi_disp_test_module;

extern int hisi_fb_blank(int blank_mode, struct fb_info *info);
extern int hisi_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);

#endif /* HISI_TESTCASE_FB_H */
