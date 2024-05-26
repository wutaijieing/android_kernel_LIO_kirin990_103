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

#ifndef HISI_DRV_UTILS_H
#define HISI_DRV_UTILS_H

#include <linux/platform_device.h>
#include <linux/types.h>
#include <soc_dte_define.h>

#include "hisi_disp_debug.h"

typedef int (*power_cb)(uint32_t id, struct platform_device *pdev);
typedef int (*present_cb)(uint32_t id, struct platform_device *pdev, void *data);
typedef int (*set_backlight_cb)(struct platform_device *pdev, uint32_t level);
typedef void (*hisi_register_cb)(void *info);
typedef void (*init_device_data_cb)(struct platform_device *pdev, void *dev_data, void *input_data, void *input_ops);

#define dpu_check_and_call_func(ret, func, ...) \
	do { \
		if (func) \
			ret = func(__VA_ARGS__); \
	} while (0)

/* device probe order:
 * panel-->mipi-->overlay-->factory-->display0-->fb0
 *                                |-->display1-->fb1
 * or maybe:
 * panel-->mipi-->overlay0-->factory-->display0-->fb0
 * panel-->mipi-->overlay1--|
 */
#define HISI_FAKE_PANEL_DEV_NAME            "hisi_disp_primary_mipi_fake_panel"
#define HISI_CONNECTOR_PRIMARY_MIPI_NAME    "hisi_disp_primary_mipi"
#define HISI_PRIMARY_OVERLAY_DEV_NAME       "hisi_disp_primary_overlay"
#define HISI_PRIMARY_FB_DEV_NAME            "dpu_disp_primary_fb"
#define HISI_OFFLINE_OVERLAY_DEV_NAME      "hisi_disp_ofline_overlay"

#define HISI_FAKE_DP_PANEL_DEV_NAME            "hisi_disp_external_dp_fake_panel"
#define HISI_CONNECTOR_EXTERNAL_DP_NAME    "hisi_disp_external_dp"
#define HISI_EXTERNAL_OVERLAY_DEV_NAME       "hisi_disp_external_overlay"
#define HISI_TMP_FB2_DEV_NAME       "hisi_disp_tmp_fb2"
#define HISI_CONNECTOR_HDMIRX_NAME    "hisi_disp_hdmi_rx"
#define DTS_NAME_HDMIRX0 "dkmd,dpu_hdmi_receiver0"
#define HISI_HDMIRX_CHR_NAME "hdmi_rx0"

#define DTS_COMP_PRIMARY_OVERLAY      "hisilicon,hisidpu"

#define HISI_OFFLINE0_CHR_NAME "disp_offline0"
#define HISI_OFFLINE1_CHR_NAME "disp_offline1"

#define HISI_HDMIRX_CHR_NAME    "hdmi_rx0"

#define HISI_DISP_DEVICE_NAME_MAX 40
#define GET_ONLINE_DEV_ID(id)  ((COMP_DEV_TYPE_ONLINE) | BIT(id))
#define GET_OFFLINE_DEV_ID(id) ((COMP_DEV_TYPE_OFFLINE) | BIT(id))

#define DPU_IS_MULTIPLE_WITH(value, align)  (!!((value) % (align)))

enum {
	COMP_DEV_TYPE_ONLINE = BIT(16),
	COMP_DEV_TYPE_OFFLINE = BIT(17),
};

enum FORMAT_BPP {
	FORMAT_BPP_1 = 1,
	FORMAT_BPP_2 = 2,
	FORMAT_BPP_3 = 3,
	FORMAT_BPP_4 = 4,
};

enum {
	ONLINE_OVERLAY_ID_PRIMARY,
	ONLINE_OVERLAY_ID_AUXILIARY,
	ONLINE_OVERLAY_ID_EXTERNAL1,
	ONLINE_OVERLAY_ID_EXTERNAL2,
	ONLINE_OVERLAY_ID_MAX,
};

enum {
	OFFLINE_OVERLAY_ID_0 = 0,
	OFFLINE_OVERLAY_ID_1,
	OFFLINE_OVERLAY_ID_MAX,
};

struct hisi_pipeline_ops {
	int (*turn_on)(uint32_t id, struct platform_device *dev);
	int (*turn_off)(uint32_t id, struct platform_device *dev);
	int (*present)(uint32_t id, struct platform_device *dev, void *data);

	/* other ops */
};

void* hisi_drv_create_device(const char *name, int data_len, init_device_data_cb init_data_func, void *input, void *input_ops);
uint8_t dpu_get_bpp(enum DPU_FORMAT format);

static inline uint8_t dpu_get_pixel_num(uint8_t bpp)
{
	if (bpp == FORMAT_BPP_2)
		return 4;

	if (bpp == FORMAT_BPP_4)
		return 2;

	disp_pr_warn("unsupport bpp %u", bpp);
	return 1;
}

#endif /* HISI_DRV_UTILS_H */
