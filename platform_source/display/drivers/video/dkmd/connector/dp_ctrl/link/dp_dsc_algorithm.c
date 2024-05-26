/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#include "dp_ctrl.h"
#include "dp_dsc_algorithm.h"
#include "dsc/dsc_algorithm.h"
#include "dsc/dsc_algorithm_manager.h"
#include "dsc/dsc_config.h"
#include "dp_aux.h"
#include "drm_dp_helper_additions.h"

/* (ppr_left_boundary, ppr_right_boundary) -> (slice_count) mapping */
static const struct slice_count_map g_slice_count_maps[] = {
	{ 0, 340, 1 },
	{ 340, 680, 2 },
	{ 680, 1360, 4 },
	{ 1360, 3200, 8 },
	{ 3200, 4800, 12 },
	{ 4800, 6400, 16 },
	{ 6400, 8000, 20 },
	{ 8000, 9600, 24 },
};

/* Line Buffer Bit Depth
 * Data Mapping
 * 0000 = 9 bits   0001 = 10 bits  0010 = 11 bits
 * 0011 = 12 bits  0100 = 13 bits  0101 = 14 bits
 * 0110 = 15 bits  0111 = 16 bits  1000 = 8 bits
 * All other values are RESERVED
 */
#define LINE_BUFFER_DEPTH_OFFSET 9

int dptx_get_slice_count(int ppr)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(g_slice_count_maps); i++) {
		/* Determine slice count by peak pixel throughput rate (ppr) (MP/s) */
		if (ppr > g_slice_count_maps[i].ppr_left_boundary && ppr <= g_slice_count_maps[i].ppr_right_boundary)
			return g_slice_count_maps[i].slice_count;
	}

	return -EINVAL;
}

struct slice_mask {
	uint8_t slice_count;
	uint32_t mask_bit;
};

struct slice_mask slice_cap_tbl[] = {
	{  1, DP_DSC_1_PER_DP_DSC_SINK },
	{  2, DP_DSC_2_PER_DP_DSC_SINK },
	{  4, DP_DSC_4_PER_DP_DSC_SINK },
	{  6, DP_DSC_6_PER_DP_DSC_SINK },
	{  8, DP_DSC_8_PER_DP_DSC_SINK },
	{ 10, DP_DSC_10_PER_DP_DSC_SINK },
	{ 12, DP_DSC_12_PER_DP_DSC_SINK },
	/* check slice_cap2 int below items */
	{ 16, DP_DSC_16_PER_DP_DSC_SINK },
	{ 20, DP_DSC_20_PER_DP_DSC_SINK },
	{ 24, DP_DSC_24_PER_DP_DSC_SINK },
};

bool dptx_slice_count_supported(struct dp_ctrl *dptx, uint8_t slice_count)
{
	uint16_t i;
	uint8_t slice_cap1 = 0;
	uint8_t slice_cap2 = 0;

	int retval = dptx_read_dpcd(dptx, DP_DSC_SLICE_CAP_1, &slice_cap1);
	if (retval != 0)
		dpu_pr_err("[DP] %s : DPCD read failed", __func__);

	retval = dptx_read_dpcd(dptx, DP_DSC_SLICE_CAP_2, &slice_cap2);
	if (retval != 0)
		dpu_pr_err("[DP] %s : DPCD read failed", __func__);

	for (i = 0; i < ARRAY_SIZE(slice_cap_tbl); i++) {
		if (slice_count <= 12) {
			if ((slice_cap_tbl[i].mask_bit & slice_cap1) &&
				(slice_cap_tbl[i].slice_count == slice_count))
				return true;
		} else {
			if ((slice_cap_tbl[i].mask_bit & slice_cap2) &&
				(slice_cap_tbl[i].slice_count == slice_count))
				return true;
		}
	}

	return false;
}

void dptx_sink_color_depth_capabilities(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t dsc_color_depth = 0;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	retval = dptx_read_dpcd(dptx, DP_DSC_DEC_COLOR_DEPTH_CAP, &dsc_color_depth);
	if (retval)
		dpu_pr_err("[DP] %s : DPCD read failed", __func__);

	if (dsc_color_depth & DP_DSC_8_BPC)
		dpu_pr_info("Sink supports 8 bpc");

	if (dsc_color_depth & DP_DSC_10_BPC)
		dpu_pr_info("Sink supports 10 bpc");

	if (dsc_color_depth & DP_DSC_12_BPC)
		dpu_pr_info("Sink supports 12 bpc");
}

void dptx_sink_color_format_capabilities(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t dsc_color_format = 0;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	retval = dptx_read_dpcd(dptx, DP_DSC_DEC_COLOR_FORMAT_CAP, &dsc_color_format);
	if (retval)
		dpu_pr_err("[DP] %s : DPCD read failed", __func__);

	if (dsc_color_format & DP_DSC_RGB)
		dpu_pr_info("Sink supports RGB color format");

	if (dsc_color_format & DP_DSC_YCBCR_444)
		dpu_pr_info("Sink supports YCbCr 4:4:4 color format");

	if (dsc_color_format & DP_DSC_YCBCR_422_SIMPLE)
		dpu_pr_info("Sink supports YCbCr 4:2:2 SIMPLE color format");

	if (dsc_color_format & DP_DSC_YCBCR_422_NATIVE)
		dpu_pr_info("Sink supports YCbCr 4:2:2 NATIVE color format");

	if (dsc_color_format & DP_DSC_YCBCR_420_NATIVE)
		dpu_pr_info("Sink supports YCbCr 4:2:0 NATIVE color format");
}

int dptx_get_rx_line_buffer_depth(struct dp_ctrl *dptx)
{
	int retval;
	uint8_t buf_bit_depth = 0;
	uint8_t line_buf_depth;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] null pointer");

	retval = dptx_read_dpcd(dptx, DP_DSC_LINE_BUF_BIT_DEPTH, &buf_bit_depth);
	if (retval)
		dpu_pr_err("[DP] %s : DPCD read failed", __func__);

	buf_bit_depth = (buf_bit_depth & DP_DSC_LINE_BUF_BIT_DEPTH_MASK);
	dpu_pr_info("[DP] %s PPS buf_bit_depth=%d", __func__, buf_bit_depth);

	if (buf_bit_depth < 8)
		line_buf_depth = buf_bit_depth + LINE_BUFFER_DEPTH_OFFSET;
	else
		line_buf_depth = 8; // RESERVED value set to 8 bits firstly

	if (dptx->dptx_line_buffer_depth_limit != NULL)
		line_buf_depth = (uint8_t)dptx->dptx_line_buffer_depth_limit(line_buf_depth);

	return line_buf_depth;
}

int dptx_dsc_initial(struct dp_ctrl *dptx)
{
	uint8_t byte = 0;
	int retval;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!");

	retval = dptx_read_dpcd(dptx, DP_DSC_SUPPORT, &byte);
	if (retval)
		return retval;

	if (dptx->dsc && (byte & DP_DSC_DECOMPRESSION_IS_SUPPORTED)) {
		dpu_pr_info("[DP] DSC enable! retval=%d", byte);
		dptx->dsc = true;
		dptx->fec = true;
	} else {
		dptx->dsc = false;
		dptx->fec = false;
	}

	return 0;
}

void dptx_dsc_check_rx_cap(struct dp_ctrl *dptx)
{
	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");
	dptx_sink_color_format_capabilities(dptx);
	dptx_sink_color_depth_capabilities(dptx);
}

void dptx_dsc_para_init(struct dp_ctrl *dptx)
{
	int slice_height_div;
	struct dsc_algorithm_manager *p_algorithm = NULL;
	struct input_dsc_info input_dsc_info;
	int linebuf_depth;

	dpu_check_and_no_retval(!dptx, err, "[DP] null pointer");

	if (dptx->dptx_slice_height_limit != NULL)
		slice_height_div = dptx->dptx_slice_height_limit(dptx, dptx->vparams.mdtd.v_active);
	else
		slice_height_div = 1;

	if (slice_height_div <= 0) {
		dpu_pr_err("[DP] slice_height_div calc error");
		return;
	}

	p_algorithm = get_dsc_algorithm_manager_instance();
	if (p_algorithm == NULL) {
		dpu_pr_err("[DP] get_dsc_check_instance null pointer");
		return;
	}

	input_dsc_info.dsc_version = DSC_VERSION_V_1_2;
	input_dsc_info.format = DSC_RGB;
	input_dsc_info.pic_width = dptx->vparams.mdtd.h_active;
	input_dsc_info.pic_height = dptx->vparams.mdtd.v_active;
	if (dptx->dsc_decoders == 0) {
		dpu_pr_err("[DP] dsc_decoders cannot be zero");
		return;
	}
	input_dsc_info.slice_width = input_dsc_info.pic_width / dptx->dsc_decoders;
	if (slice_height_div == 0) {
		dpu_pr_err("[DP] slice_height_div cannot be zero");
		return;
	}
	input_dsc_info.slice_height = input_dsc_info.pic_height / slice_height_div;
	input_dsc_info.dsc_bpc = 8;
	input_dsc_info.block_pred_enable = 1;
	linebuf_depth = dptx_get_rx_line_buffer_depth(dptx);
	if (linebuf_depth < 0) {
		dpu_pr_err("[DP] linebuf_depth calc error");
		return;
	}
	input_dsc_info.linebuf_depth = (uint32_t)linebuf_depth;

	switch (dptx->dsc_ifbc_type) {
	case IFBC_TYPE_VESA2X_DUAL:
		input_dsc_info.dsc_bpp = 12;
		break;
	case IFBC_TYPE_VESA3X_DUAL:
		input_dsc_info.dsc_bpp = 8;
		break;
	case IFBC_TYPE_NONE:
	default:
		dpu_pr_err("[DP] IFBC type error");
		return;
	}

	dpu_pr_info("[DP] %s PPS pic_width=%d", __func__, input_dsc_info.pic_width);
	dpu_pr_info("[DP] %s PPS slice_width=%d", __func__, input_dsc_info.slice_width);
	dpu_pr_info("[DP] %s PPS pic_height=%d", __func__, input_dsc_info.pic_height);

	p_algorithm->vesa_dsc_info_calc(&input_dsc_info, &(dptx->vparams.dp_dsc_info.dsc_info), NULL);
}
