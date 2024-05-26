/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "hdmirx_ctrl.h"
#include "hdmirx_common.h"
#include "video/hdmirx_video_hdr.h"
#include "hisi_disp.h"

static struct hdmirx_to_layer_format g_hdmi2layer_fmt[] = {
	{HDMIRX_COLOR_SPACE_RGB, HDMIRX_INPUT_WIDTH_24, HISI_FB_PIXEL_FORMAT_RGBA_8888},
	{HDMIRX_COLOR_SPACE_YCBCR422, HDMIRX_INPUT_WIDTH_24, HISI_FB_PIXEL_FORMAT_YUYV_422_PKG},
	{HDMIRX_COLOR_SPACE_YCBCR420, HDMIRX_INPUT_WIDTH_24, HISI_FB_PIXEL_FORMAT_YUYV_420_PKG},
	{HDMIRX_COLOR_SPACE_RGB, HDMIRX_INPUT_WIDTH_30, HISI_FB_PIXEL_FORMAT_RGBA_1010102},
	{HDMIRX_COLOR_SPACE_YCBCR420, HDMIRX_INPUT_WIDTH_30, HISI_FB_PIXEL_FORMAT_YUYV_420_PKG_10BIT},
	{HDMIRX_COLOR_SPACE_YCBCR444, HDMIRX_INPUT_WIDTH_24, HISI_FB_PIXEL_FORMAT_YUV444},
	{HDMIRX_COLOR_SPACE_YCBCR444, HDMIRX_INPUT_WIDTH_30, HISI_FB_PIXEL_FORMAT_YUVA444},
	{HDMIRX_COLOR_SPACE_YCBCR422, HDMIRX_INPUT_WIDTH_30, HISI_FB_PIXEL_FORMAT_YUV422_10BIT},
};

static uint32_t hdmirx_ctrl_hdmi2layer_fmt(hdmirx_color_space color_space, hdmirx_input_width bit_width)
{
	int i;
	int size;
	struct hdmirx_to_layer_format *p_hdmi2layer_fmt;

	size = sizeof(g_hdmi2layer_fmt) / sizeof(g_hdmi2layer_fmt[0]);
	p_hdmi2layer_fmt = g_hdmi2layer_fmt;

	for (i = 0; i < size; i++) {
		if ((p_hdmi2layer_fmt[i].color_space == color_space) &&
			(p_hdmi2layer_fmt[i].bit_width == bit_width))
			return p_hdmi2layer_fmt[i].layer_format;
	}

	disp_pr_err("input color_space %d, bit_width %d not support\n", color_space, bit_width);
	return HISI_FB_PIXEL_FORMAT_RGBA_8888;
}

uint32_t hdmirx_ctrl_get_layer_fmt(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t bit_width;
	uint32_t color_space;
	uint32_t format;

	if (hdmirx == NULL) {
		disp_pr_info("hdmirx NULL\n");
		return HISI_FB_PIXEL_FORMAT_RGBA_8888;
	}

	bit_width = hdmirx_packet_get_color_depth(hdmirx);
	color_space = hdmirx_packet_avi_get_color_space(hdmirx);

	format = hdmirx_ctrl_hdmi2layer_fmt(color_space, bit_width);
	disp_pr_info("color_space 0x%x,bit_width 0x%x, format is 0x%x\n", color_space, bit_width, format);

	return format;
}

void hdmirx_ctrl_yuv422_set(struct hdmirx_ctrl_st *hdmirx, uint32_t enable)
{
	if (hdmirx == NULL) {
		disp_pr_err("hdmirx NULL\n");
		return;
	}

	set_reg(hdmirx->hdmirx_pwd_base + 0x526C, enable, 1, 0);
}

static int hdmirx_ctrl_burrs_filter_set(struct hdmirx_ctrl_st *hdmirx)
{
	// video data period filter enable
	set_reg(hdmirx->hdmirx_pwd_base + 0xA100, 0, 1, 19);

	// data island period filter enable
	set_reg(hdmirx->hdmirx_pwd_base + 0xA100, 0, 1, 15);

	// control period filter enable
	set_reg(hdmirx->hdmirx_pwd_base + 0xA100, 0, 1, 11);

	// hsync filter enable
	set_reg(hdmirx->hdmirx_pwd_base + 0xA100, 0, 1, 7);

	// vsync filter enable
	set_reg(hdmirx->hdmirx_pwd_base + 0xA100, 0, 1, 3);

	return 0;
}

int hdmirx_ctrl_init(struct hdmirx_ctrl_st *hdmirx)
{
	disp_pr_info("[hdmirx] ctrl init\n");

	hdmirx_ctrl_burrs_filter_set(hdmirx);

	// TEST
	set_reg(hdmirx->hdmirx_aon_base + 0x10B0, 0, 32, 0);

	return 0;
}

void hdmirx_ctrl_source_select_hdmi(struct hdmirx_ctrl_st *hdmirx)
{
	disp_pr_info("[hdmirx] select hdmi source HDMI-->DPTX\n");
	set_reg(hdmirx->hsdt1_sysctrl_base + 0x30, 0x4, 3, 3);
	set_reg(hdmirx->hsdt1_sysctrl_base + 0x30, 0x4, 3, 9);
}

void hdmirx_ctrl_set_sys_mute(struct hdmirx_ctrl_st *hdmirx, bool enable)
{
	if (enable == true)
		set_reg(hdmirx->hdmirx_pwd_base + 0x2c, 0x1, 1, 1);
	else
		set_reg(hdmirx->hdmirx_pwd_base + 0x2c, 0x1, 1, 0);
}

bool hdmirx_ctrl_get_sys_mute(struct hdmirx_ctrl_st *hdmirx)
{
	return (inp32(hdmirx->hdmirx_pwd_base + 0x1c) & 0x1);
}

void hdmirx_ctrl_mute_irq_mask(struct hdmirx_ctrl_st *hdmirx)
{
	set_reg(hdmirx->hdmirx_sysctrl_base + 0x40, 0x1, 1, 7);
}

void hdmirx_ctrl_mute_proc(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t temp;

	temp = inp32(hdmirx->hdmirx_sysctrl_base + 0x44);
	if (!(temp & 0xC0))
		return;

	// sys mute
	if (temp & 0x80) {
		disp_pr_info("have sys mute. mute status 0x%x\n", (temp & 0xC0));
		set_reg(hdmirx->hdmirx_sysctrl_base + 0x44, 0x1, 1, 7);
	}

	// audio mute (temp & 0x40)
}

bool hdmirx_ctrl_is_hdr10(void)
{
	return hdmirx_hdr_type_is_hdr10();
}