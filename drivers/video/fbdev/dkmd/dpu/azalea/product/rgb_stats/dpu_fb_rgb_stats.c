/* Copyright (c) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "dpu_fb_rgb_stats.h"

#include <chipset_common/dubai/dubai_plat.h>

#define DISP_GLB_REG_LENGTH		16

static DEFINE_MUTEX(rgb_lock);

static atomic_t rgb_read_status;
bool g_rgb_enable = false;
bool g_rgb_init = false;
struct display_hist_rgb g_rgb_data[DISP_GLB_REG_LENGTH];
struct display_hist_rgb g_rgb_data_last[DISP_GLB_REG_LENGTH];
struct display_hist_rgb g_rgb_data_sum[DISP_GLB_REG_LENGTH];

static void dpufb_rgb_sum_data(void)
{
	uint32_t i;

	mutex_lock(&rgb_lock);
	for (i = 0; i < DISP_GLB_REG_LENGTH; i++) {
		g_rgb_data_sum[i].red += g_rgb_data_last[i].red;
		g_rgb_data_sum[i].green += g_rgb_data_last[i].green;
		g_rgb_data_sum[i].blue += g_rgb_data_last[i].blue;
	}
	mutex_unlock(&rgb_lock);
}

static bool dpufb_rgb_check_enable(void)
{
	return (g_debug_rgb_stats_enable == 1) && g_rgb_enable;
}

void dpufb_rgb_read_register(const struct dpu_fb_data_type *dpufd)
{
	uint32_t i;
	const uint32_t reg_length = 4;

	if (dpufd == NULL || dpufd->dss_base == NULL) {
		DPU_FB_ERR("dpufd or dss base is NULL");
		return;
	}

	if (!dpufb_rgb_check_enable() || (!atomic_read(&rgb_read_status)))
		return;

	if ((inp32(dpufd->dss_base + DSS_MIPI_DSI0_OFFSET + MIPI_LDI_VRT_CTRL2) & 0x3fff) ==
		(dpufd->panel_info.yres - 1)) {
		for (i = 0; i < DISP_GLB_REG_LENGTH; i++) {
			g_rgb_data[i].red = inp32(dpufd->dss_base + DSS_DISP_GLB_OFFSET + R0_HIST + i * reg_length);
			g_rgb_data[i].green = inp32(dpufd->dss_base + DSS_DISP_GLB_OFFSET + G0_HIST + i * reg_length);
			g_rgb_data[i].blue = inp32(dpufd->dss_base + DSS_DISP_GLB_OFFSET + B0_HIST + i * reg_length);
		}
	}

	atomic_set(&rgb_read_status, 0);
}

static int dpufb_rgb_stats_update(void)
{
	if (!dpufb_rgb_check_enable()) {
		DPU_FB_ERR("rgb stats is disable");
		return -1;
	}

	if (!atomic_read(&rgb_read_status)) {
		memcpy(g_rgb_data_last, g_rgb_data, sizeof(g_rgb_data_last));
		atomic_set(&rgb_read_status, 1);
	}
	dpufb_rgb_sum_data();

	return 0;
}

static void dpufb_rgb_stats_init(void)
{
	if (g_rgb_init) {
		DPU_FB_ERR("dubai rgb stats has been initialized");
		return;
	}

	g_rgb_init = true;
	atomic_set(&rgb_read_status, 1);
	memset(g_rgb_data, 0, sizeof(g_rgb_data));
	memset(g_rgb_data_last, 0, sizeof(g_rgb_data_last));
	memset(g_rgb_data_sum, 0, sizeof(g_rgb_data_sum));
}

void dpufb_rgb_reg_enable(const struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL || dpufd->dss_base == NULL) {
		DPU_FB_ERR("dpufd or dss base is NULL");
		return;
	}

	if (dpufb_rgb_check_enable())
		outp32(dpufd->dss_base + DSS_DISP_GLB_OFFSET + RGB_HIST_EN, 0x1);
}

void dpufb_rgb_notify_panel_state(bool state)
{
	if (dpufb_rgb_check_enable())
		dubai_notify_display_state(state);
}

int dpufb_rgb_get_data(void *data, int len)
{
	if (data == NULL || len != sizeof(g_rgb_data_sum)) {
		DPU_FB_ERR("get rgb data parameter error");
		return -1;
	}

	if (dpufb_rgb_check_enable()) {
		mutex_lock(&rgb_lock);
		memcpy(data, g_rgb_data_sum, sizeof(g_rgb_data_sum));
		mutex_unlock(&rgb_lock);
		return 0;
	} else {
		DPU_FB_ERR("rgb stats is disable");
		return -1;
	}
}

int dpufb_rgb_set_enable(bool enable)
{
	g_rgb_enable = enable;
	if (dpufb_rgb_check_enable())
		dpufb_rgb_stats_init();
	if (enable == dpufb_rgb_check_enable())
		return 0;

	return -1;
}

size_t dpufb_rgb_get_len(void)
{
	return DISP_GLB_REG_LENGTH;
}

dubai_hist_policy_t dpufb_rgb_get_policy(void)
{
	return DISPLAY_HIST_POLICY_RGB;
}

static struct dubai_display_hist_ops hist_ops = {
	.get_policy = dpufb_rgb_get_policy,
	.get_len = dpufb_rgb_get_len,
	.enable = dpufb_rgb_set_enable,
	.update = dpufb_rgb_stats_update,
	.get = dpufb_rgb_get_data,
};

void dpufb_rgb_stats_register(const struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL || dpufd->dss_base == NULL) {
		DPU_FB_ERR("dpufd or dss base is NULL");
		return;
	}
	/* enable rgb stats register */
	outp32(dpufd->dss_base + DSS_DISP_GLB_OFFSET + RGB_HIST_EN, 0x1);
	dubai_register_module_ops(DUBAI_MODULE_DISPLAY, &hist_ops);
}
