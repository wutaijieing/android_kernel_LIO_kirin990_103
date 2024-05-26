/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/delay.h>
#include <securec.h>

#include "dkmd_log.h"
#include "hdmitx_ctrl.h"
#include "hdmitx_connector.h"
#include "hdmitx_ddc.h"
#include "hdmitx_frl_config.h"
#include "hdmitx_frl.h"
#include "hdmitx_core_config.h"
#include "hdmitx_infoframe.h"
#include "hdmitx_avgen.h"

#define REG_TIMING_CTRL0 0x4C
#define REG_TIMING_CTRL0_M (0x1 << 0)

#define REG_H_FRONT 0x50
#define REG_H_SYNC 0x54
#define REG_H_BACK 0x58
#define REG_H_ACTIVE 0x5C

#define REG_V_FRONT 0x60
#define REG_V_SYNC 0x64
#define REG_V_BACK 0x68
#define REG_V_ACTIVE 0x6C

s32 hdmitx_timing_select(struct hdmitx_ctrl *hdmitx,
	struct hdmitx_display_mode *select_mode)
{
	struct list_head *n = NULL;
	struct list_head *pos = NULL;
	struct hdmitx_display_mode *dis_mode = NULL;
	struct hdmitx_connector *connector = NULL;

	if (hdmitx == NULL)
		return -1;

	connector = hdmitx->hdmitx_connector;
	if (connector == NULL)
		return -1;

	if (list_empty(&connector->probed_modes))
		return -1;

	list_for_each_safe(pos, n, &connector->probed_modes) {
		dis_mode = list_entry(pos, struct hdmitx_display_mode, head);
		if (dis_mode == NULL)
			continue;

		if ((dis_mode->type & HDMITX_MODE_TYPE_PREFERRED) != 0) {
			memcpy_s(&hdmitx->select_mode, sizeof(struct hdmitx_display_mode),
				dis_mode, sizeof(struct hdmitx_display_mode));
			return 0;
		}
	}
	return -1;
}

void hdmitx_timing_gen_enable(struct hdmitx_ctrl *hdmitx)
{
	hdmi_clrset(hdmitx->sysctrl_base, REG_TIMING_CTRL0, REG_TIMING_CTRL0_M, 0x1);
}

void hdmitx_timing_gen_disable(struct hdmitx_ctrl *hdmitx)
{
	hdmi_clrset(hdmitx->sysctrl_base, REG_TIMING_CTRL0, REG_TIMING_CTRL0_M, 0x0);
}

void hdmitx_set_video_path(struct hdmitx_ctrl *hdmitx)
{
	// dpu video data transfer to HDMI must be RGB
	// vdp packet = metadata control

	// to fix : according dpu color depth
	core_set_color_depth(hdmitx->base, HDMITX_BPC_24);
}

void hdmitx_set_mode(struct hdmitx_ctrl *hdmitx)
{
	core_set_hdmi_mode(hdmitx->base, true);
}

void hdmitx_frl_video_config(struct hdmitx_ctrl *hdmitx)
{
	hdmitx_set_infoframe(hdmitx);
	hdmitx_set_video_path(hdmitx);
	hdmitx_set_mode(hdmitx);
}

void hdmitx_set_avmute(struct hdmitx_ctrl *hdmitx, bool enable)
{
	if (enable) {
		core_send_av_mute(hdmitx->base);
		msleep(50);
	} else {
		core_send_av_unmute(hdmitx->base);
	}
}

void hdmitx_timing_gen_config(struct hdmitx_ctrl *hdmitx,
	struct hdmitx_display_mode *mode)
{
	struct hdmi_detail *detail = &mode->detail;

	hdmi_writel(hdmitx->sysctrl_base, REG_H_FRONT, detail->h_front);
	hdmi_writel(hdmitx->sysctrl_base, REG_H_SYNC, detail->h_sync);
	hdmi_writel(hdmitx->sysctrl_base, REG_H_BACK, detail->h_back);
	hdmi_writel(hdmitx->sysctrl_base, REG_H_ACTIVE, detail->h_active);

	hdmi_writel(hdmitx->sysctrl_base, REG_V_FRONT, detail->v_front);
	hdmi_writel(hdmitx->sysctrl_base, REG_V_SYNC, detail->h_sync);
	hdmi_writel(hdmitx->sysctrl_base, REG_V_BACK, detail->h_back);
	hdmi_writel(hdmitx->sysctrl_base, REG_V_ACTIVE, detail->h_active);
}

s32 hdmitx_timing_config_safe_mode(struct hdmitx_ctrl *hdmitx)
{
	struct hdmitx_display_mode *safe_mode = NULL;

	if (hdmitx == NULL)
		return -1;

	safe_mode = drv_hdmitx_modes_create_mode_by_vic(VIC_640X480P60_4_3);
	hdmitx_timing_gen_config(hdmitx, safe_mode);
	hdmitx_timing_gen_enable(hdmitx);
	return 0;
}

s32 hdmitx_timing_config(struct hdmitx_ctrl *hdmitx)
{
	struct hdmitx_display_mode select_mode = {0};

	if (hdmitx == NULL)
		return -1;

	hdmitx_timing_select(hdmitx, &select_mode);
	hdmitx->select_mode = select_mode;
	hdmitx_timing_gen_disable(hdmitx);
	hdmitx_timing_gen_config(hdmitx, &select_mode);
	hdmitx_timing_gen_enable(hdmitx);
	return 0;
}
