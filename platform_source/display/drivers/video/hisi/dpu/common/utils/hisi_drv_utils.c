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

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/device.h>

#include "hisi_drv_utils.h"


void* hisi_drv_create_device(const char *name, int data_len,
	init_device_data_cb init_data_func, void *input, void *input_ops)
{
	struct platform_device *pdev = NULL;
	void *dev_data = NULL;
	int ret;

	disp_pr_info("+++++++, name=%s", name);

	pdev = platform_device_alloc(name, PLATFORM_DEVID_AUTO);
	if (!pdev) {
		disp_pr_err("create %s device fail", name);
		return NULL;
	}

	/* the best function is devm_kzalloc, but it will result to WARNING_ON at platform_device_add */
	dev_data = kzalloc(data_len, GFP_KERNEL);
	if (!dev_data) {
		disp_pr_err("kzalloc factory device fail");
		goto kzalloc_fail;
	}

	if (init_data_func)
		init_data_func(pdev, dev_data, input, input_ops);

	platform_set_drvdata(pdev, dev_data);
	ret = platform_device_add(pdev);
	if (ret) {
		disp_pr_err("add factory device fail");
		goto device_add_fail;
	}

	disp_pr_info("--------");
	return dev_data;

device_add_fail:
	kfree(dev_data);
kzalloc_fail:
	platform_device_put(pdev);

	return NULL;
}

uint8_t dpu_get_bpp(enum DPU_FORMAT format)
{
	switch (format) {
	case RGB565:
	case BGR565:
	case XRGB4444:
	case ARGB4444:
	case XRGB1555:
	case ARGB1555:
	case XBGR4444:
	case ABGR4444:
	case XBGR1555:
	case ABGR1555:
		return FORMAT_BPP_2;
	case XRGB8888:
	case ARGB8888:
	case XBGR8888:
	case ABGR8888:
	case BGRA1010102:
	case RGBA1010102:
		return FORMAT_BPP_4;
	case YUV444:
	case YVU444:
		return FORMAT_BPP_3;
	case YUVA1010102:
	case UYVA1010102:
	case VUYA1010102:
		return FORMAT_BPP_4;
	case YUYV422_8B_PACKET:
	case YUYV422_8B_SP:
	case YUYV422_8B_PLANER:
	case YUYV422_10B_PACKET:
	case YUYV422_10B_SP:
	case YUYV422_10B_PLANER:
	case YVYU422_8B_PACKET:
	case YVYU422_10B_PACKET:
	case VYUY422_8B_PACKET:
	case VYUY422_10B_PACKET:
	case UYVY422_8B_PACKET:
	case UYVY422_10B_PACKET:
	case YUYV420_8B_SP:
	case YUYV420_8B_PLANER:
	case YUYV420_10B_SP:
	case YUYV420_10B_PLANER:
		return FORMAT_BPP_2;
	case D1_128:
	case D3_128:
	case RGB_10BIT:
	case BGR_10BIT:
	case ARGB10101010:
	case XRGB10101010:
	case AYUV10101010:
		disp_pr_warn("unkonw format BPP %u", format);
		return FORMAT_BPP_1;
	/* those formats is SPR format, no bpp
	 * RGBG, RGBG_HIDIC, RGBG_IDLEPACK, RGBG_DEBURNIN, RGB_DELTA,
	 * RGB_DELTA_HIDIC RGB_DELTA_IDLEPACK,RGB_DELTA_DEBURNIN:
	 */
	default:
		disp_pr_err("unkonw format BPP %u", format);
		disp_assert_if_cond(true);
		return FORMAT_BPP_1;
	}
}



