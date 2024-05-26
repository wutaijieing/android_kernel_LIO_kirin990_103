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

#include "rx_sdp.h"
#include "dprx.h"
#include "rx_reg.h"
#include "../composer/operators/dsc/dsc_algorithm.h"
#include <securec.h>

#undef pr_fmt
#define pr_fmt(fmt)  "[DPRX] " fmt

void rx_sdp_parse_pps(struct pps_sdp *pps_table)
{
	struct dsc_info dsc;
	uint8_t i;

	dpu_check_and_no_retval((pps_table == NULL), err, "pps_table is NULL!\n");

	dsc.dsc_version_major = pps_table->pps[0] >> 4;
	dsc.dsc_version_minor = pps_table->pps[0] & GENMASK(3, 0);

	dsc.pps_identifier = pps_table->pps[1];
	dsc.dsc_bpc = (pps_table->pps[3]) >> 4;
	dsc.linebuf_depth = pps_table->pps[3] & GENMASK(3, 0);
	dsc.block_pred_enable = (pps_table->pps[4] & BIT(5)) >> 5;
	dsc.convert_rgb = (pps_table->pps[4] & BIT(4)) >> 4;
	dsc.simple_422 = (pps_table->pps[4] & BIT(3)) >> 3;
	dsc.vbr_enable = (pps_table->pps[4] & BIT(2)) >> 2;
	dsc.dsc_bpp = ((pps_table->pps[4] & GENMASK(1, 0)) << 8) | pps_table->pps[5];

	dsc.pic_height = (pps_table->pps[6] << 8) | pps_table->pps[7];
	dsc.pic_width = (pps_table->pps[8] << 8) | pps_table->pps[9];
	dsc.slice_height = (pps_table->pps[10] << 8) | pps_table->pps[11];
	dsc.slice_width = (pps_table->pps[12] << 8) | pps_table->pps[13];
	dsc.chunk_size = (pps_table->pps[14] << 8) | pps_table->pps[15];

	dsc.initial_xmit_delay = ((pps_table->pps[16] & GENMASK(1, 0)) << 8) | pps_table->pps[17];
	dsc.initial_dec_delay = (pps_table->pps[18] << 8) | pps_table->pps[19];
	dsc.initial_scale_value = pps_table->pps[21] & GENMASK(5, 0);
	dsc.scale_increment_interval = (pps_table->pps[22] << 8) | pps_table->pps[23];
	dsc.scale_decrement_interval = ((pps_table->pps[24] & GENMASK(3, 0)) << 8) | pps_table->pps[25];

	dsc.first_line_bpg_offset = pps_table->pps[27] & GENMASK(4, 0);
	dsc.nfl_bpg_offset = (pps_table->pps[28] << 8) | pps_table->pps[29];
	dsc.slice_bpg_offset = (pps_table->pps[30] << 8) | pps_table->pps[31];
	dsc.initial_offset = (pps_table->pps[32] << 8) | pps_table->pps[33];
	dsc.final_offset = (pps_table->pps[34] << 8) | pps_table->pps[35];

	dsc.flatness_min_qp = pps_table->pps[36] & GENMASK(4, 0);
	dsc.flatness_max_qp = pps_table->pps[37] & GENMASK(4, 0);

	dsc.rc_model_size = (pps_table->pps[38] << 8) | pps_table->pps[39];
	dsc.rc_edge_factor = pps_table->pps[40] & GENMASK(3, 0);
	dsc.rc_quant_incr_limit0 = pps_table->pps[41] & GENMASK(4, 0);
	dsc.rc_quant_incr_limit1 = pps_table->pps[42] & GENMASK(4, 0);
	dsc.rc_tgt_offset_hi = pps_table->pps[43] >> 4;
	dsc.rc_tgt_offset_lo = pps_table->pps[43] & GENMASK(3, 0);

	for (i = 0; i < DSC_THRESH_LENGTH; i++)
		dsc.rc_buf_thresh[i] = pps_table->pps[i + 44];

	for (i = 0; i < DSC_RC_RANGE_LENGTH; i++) {
		dsc.rc_range[i].min_qp = pps_table->pps[i * 2 + 58] >> 3; /* first: 7:3 */
		/* first: 2:0 | second: 7:6 */
		dsc.rc_range[i].max_qp = 
			((pps_table->pps[i * 2 + 58] & GENMASK(2, 0)) << 2) | (pps_table->pps[i * 2 + 1 + 58] >> 6);
		dsc.rc_range[i].offset = pps_table->pps[i * 2 + 1 + 58] & GENMASK(5, 0); /* second: 5:0 */
	}

	dsc.native_420 = (pps_table->pps[88] & BIT(1)) >> 1;
	dsc.native_422 = (pps_table->pps[88] & BIT(0));

	dsc.second_line_bpg_offset = pps_table->pps[89] & GENMASK(4, 0);
	dsc.nsl_bpg_offset = (pps_table->pps[90] << 8) | pps_table->pps[91];
	dsc.second_line_offset_adj = (pps_table->pps[92] << 8) | pps_table->pps[93];

	pr_info("dsc vesion: %u.%u\n", dsc.dsc_version_major, dsc.dsc_version_minor);
	pr_info("dsc.pps_identifier: %u\n", dsc.pps_identifier);
	pr_info("dsc bpc: %u\n", dsc.dsc_bpp);

	pr_info("dsc linebuf_depth: %u\n", dsc.linebuf_depth);
	pr_info("dsc block_pred_enable: %u\n", dsc.block_pred_enable);
	pr_info("dsc convert_rgb: %u\n", dsc.convert_rgb);
	pr_info("dsc simple_422: %u\n", dsc.simple_422);
	pr_info("dsc vbr_enable: %u\n", dsc.vbr_enable);
	pr_info("dsc dsc_bpp: %u\n", dsc.dsc_bpp);

	pr_info("dsc pic_height: %u\n", dsc.pic_height);
	pr_info("dsc pic_width: %u\n", dsc.pic_width);
	pr_info("dsc slice_height: %u\n", dsc.slice_height);
	pr_info("dsc slice_width: %u\n", dsc.slice_width);
	pr_info("dsc chunk_size: %u\n", dsc.chunk_size);

	pr_info("dsc initial_xmit_delay: %u\n", dsc.initial_xmit_delay);
	pr_info("dsc initial_dec_delay: %u\n", dsc.initial_dec_delay);
	pr_info("dsc initial_scale_value: %u\n", dsc.initial_scale_value);
	pr_info("dsc scale_increment_interval: %u\n", dsc.scale_increment_interval);
	pr_info("dsc scale_decrement_interval: %u\n", dsc.scale_decrement_interval);

	pr_info("dsc first_line_bpg_offset: %u\n", dsc.first_line_bpg_offset);
	pr_info("dsc nfl_bpg_offset: %u\n", dsc.nfl_bpg_offset);
	pr_info("dsc slice_bpg_offset: %u\n", dsc.slice_bpg_offset);
	pr_info("dsc initial_offset: %u\n", dsc.initial_offset);
	pr_info("dsc final_offset: %u\n", dsc.final_offset);

	pr_info("dsc flatness_min_qp: %u\n", dsc.flatness_min_qp);
	pr_info("dsc flatness_max_qp: %u\n", dsc.flatness_max_qp);

	pr_info("dsc rc_model_size: %u\n", dsc.rc_model_size);
	pr_info("dsc rc_edge_factor: %u\n", dsc.rc_edge_factor);
	pr_info("dsc rc_quant_incr_limit0: %u\n", dsc.rc_quant_incr_limit0);
	pr_info("dsc rc_quant_incr_limit1: %u\n", dsc.rc_quant_incr_limit1);
	pr_info("dsc rc_tgt_offset_hi: %u\n", dsc.rc_tgt_offset_hi);
	pr_info("dsc rc_tgt_offset_lo: %u\n", dsc.rc_tgt_offset_lo);

	for (i = 0; i < DSC_THRESH_LENGTH; i++)
		pr_info("dsc rc_buf_thresh[%u]: %u\n", i, dsc.rc_buf_thresh[i]);

	for (i = 0; i < DSC_RC_RANGE_LENGTH; i++)
		pr_info("dsc rc_range[%u]: min_qp = %u, max_qp = %u, offset = %u\n",
			i, dsc.rc_range[i].min_qp, dsc.rc_range[i].max_qp, dsc.rc_range[i].offset);

	pr_info("dsc native_420: %u\n", dsc.native_420);
	pr_info("dsc native_422: %u\n", dsc.native_422);
	pr_info("dsc second_line_bpg_offset: %u\n", dsc.second_line_bpg_offset);
	pr_info("dsc nsl_bpg_offset: %u\n", dsc.nsl_bpg_offset);
	pr_info("dsc second_line_offset_adj: %u\n", dsc.second_line_offset_adj);
}

uint8_t test_pps_table[PPS_SIZE] = {
	0x12, 0x00, 0x00, 0x89, 0x10, 0x80, 0x04, 0x38, 0x07, 0x80, 0x04, 0x38, 0x01, 0xE0, 0x01, 0xE0,
	0x02, 0x00, 0x02, 0x2C, 0x00, 0x20, 0x56, 0xEF, 0x00, 0x06, 0x00, 0x0F, 0x00, 0x1D, 0x00, 0x1C,
	0x18, 0x00, 0x10, 0xF0, 0x03, 0x0C, 0x20, 0x00, 0x06, 0x0B, 0x0B, 0x33, 0x0E, 0x1C, 0x2A, 0x38,
	0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7B, 0x7D, 0x7E, 0x01, 0x02, 0x01, 0x00, 0x09, 0x40,
	0x09, 0xBE, 0x19, 0xFC, 0x19, 0xFA, 0x19, 0xF8, 0x1A, 0x38, 0x1A, 0x78, 0x1A, 0xB6, 0x2A, 0xB6,
	0x2A, 0xF4, 0x2A, 0xF4, 0x4B, 0x34, 0x63, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

struct drm_dsc_config *dsc_cfg;

void rx_sdp_parse_pps2(struct drm_dsc_picture_parameter_set *pps_payload)
{
	uint8_t i;

	dpu_check_and_no_retval((pps_payload == NULL), err, "pps_payload is NULL!\n");

	dsc_cfg = kzalloc(sizeof(*dsc_cfg), GFP_KERNEL);

	/* PPS 0 */
	dsc_cfg->dsc_version_major = 0;
	dsc_cfg->dsc_version_minor = pps_payload->dsc_version & GENMASK(3, 0);
	dsc_cfg->dsc_version_major = pps_payload->dsc_version >> DSC_PPS_VERSION_MAJOR_SHIFT;

	/* PPS 1, 2 is 0 */

	/* PPS 3 */
	dsc_cfg->bits_per_component = pps_payload->pps_3 >> DSC_PPS_BPC_SHIFT;
	dsc_cfg->line_buf_depth = pps_payload->pps_3 & GENMASK(3, 0);

	/* PPS 4, 5 */
	dsc_cfg->block_pred_enable = (pps_payload->pps_4 & BIT(5)) >> 5;
	dsc_cfg->convert_rgb = (pps_payload->pps_4 & BIT(4)) >> 4;
	dsc_cfg->simple_422 = (pps_payload->pps_4 & BIT(3)) >> 3;
	dsc_cfg->vbr_enable = (pps_payload->pps_4 & BIT(2)) >> 2;
	dsc_cfg->bits_per_pixel = ((pps_payload->pps_4 & GENMASK(1, 0)) << 8) | pps_payload->bits_per_pixel_low;

	/*
	 * The DSC panel expects the PPS packet to have big endian format
	 * for data spanning 2 bytes. Use a macro cpu_to_be16() to convert
	 * to big endian format. If format is little endian, it will swap
	 * bytes to convert to Big endian else keep it unchanged.
	 */

	/* PPS 6, 7 */
	dsc_cfg->pic_height = be16_to_cpu(pps_payload->pic_height);

	/* PPS 8, 9 */
	dsc_cfg->pic_width = be16_to_cpu(pps_payload->pic_width);

	/* PPS 10, 11 */
	dsc_cfg->slice_height = be16_to_cpu(pps_payload->slice_height);

	/* PPS 12, 13 */
	dsc_cfg->slice_width = be16_to_cpu(pps_payload->slice_width);

	/* PPS 14, 15 */
	dsc_cfg->slice_chunk_size = be16_to_cpu(pps_payload->chunk_size);

	/* PPS 16, 17 */
	dsc_cfg->initial_xmit_delay = 
		((pps_payload->initial_xmit_delay_high & GENMASK(1, 0)) << 8) | pps_payload->initial_xmit_delay_low;

	/* PPS 18, 19 */
	dsc_cfg->initial_dec_delay = be16_to_cpu(pps_payload->initial_dec_delay);

	/* PPS 20 is 0 */

	/* PPS 21 */
	dsc_cfg->initial_scale_value = pps_payload->initial_scale_value;

	/* PPS 22, 23 */
	dsc_cfg->scale_increment_interval = be16_to_cpu(pps_payload->scale_increment_interval);

	/* PPS 24, 25 */
	dsc_cfg->scale_decrement_interval = 
		((pps_payload->scale_decrement_interval_high & GENMASK(3, 0)) << 8) | pps_payload->scale_decrement_interval_low;

	/* PPS 26[7:0], PPS 27[7:5] RESERVED */

	/* PPS 27 */
	dsc_cfg->first_line_bpg_offset = pps_payload->first_line_bpg_offset;

	/* PPS 28, 29 */
	dsc_cfg->nfl_bpg_offset = be16_to_cpu(pps_payload->nfl_bpg_offset);

	/* PPS 30, 31 */
	dsc_cfg->slice_bpg_offset = be16_to_cpu(pps_payload->slice_bpg_offset);

	/* PPS 32, 33 */
	dsc_cfg->initial_offset = be16_to_cpu(pps_payload->initial_offset);

	/* PPS 34, 35 */
	dsc_cfg->final_offset = be16_to_cpu(pps_payload->final_offset);

	/* PPS 36 */
	dsc_cfg->flatness_min_qp = pps_payload->flatness_min_qp;

	/* PPS 37 */
	dsc_cfg->flatness_max_qp = pps_payload->flatness_max_qp;

	/* PPS 38, 39 */
	dsc_cfg->rc_model_size = be16_to_cpu(pps_payload->rc_model_size);

	/* PPS 40 */
	dsc_cfg->rc_edge_factor = pps_payload->rc_edge_factor;

	/* PPS 41 */
	dsc_cfg->rc_quant_incr_limit0 = pps_payload->rc_quant_incr_limit0;

	/* PPS 42 */
	dsc_cfg->rc_quant_incr_limit1 = pps_payload->rc_quant_incr_limit1;

	/* PPS 43 */
	dsc_cfg->rc_tgt_offset_low = pps_payload->rc_tgt_offset & GENMASK(3, 0);
	dsc_cfg->rc_tgt_offset_high = pps_payload->rc_tgt_offset >> 4;

	/* PPS 44 - 57 */
	for (i = 0; i < DSC_NUM_BUF_RANGES - 1; i++)
		dsc_cfg->rc_buf_thresh[i] = pps_payload->rc_buf_thresh[i];

	/* PPS 58 - 87 */
	/*
	 * For DSC sink programming the RC Range parameter fields
	 * are as follows: Min_qp[15:11], max_qp[10:6], offset[5:0]
	 */
	for (i = 0; i < DSC_NUM_BUF_RANGES; i++) {
		uint16_t tmp = be16_to_cpu(pps_payload->rc_range_parameters[i]);
		dsc_cfg->rc_range_params[i].range_bpg_offset = tmp & GENMASK(5, 0);
		dsc_cfg->rc_range_params[i].range_max_qp = (tmp & GENMASK(10, 6)) >> DSC_PPS_RC_RANGE_MAXQP_SHIFT;
		dsc_cfg->rc_range_params[i].range_min_qp = (tmp & GENMASK(15, 11)) >> DSC_PPS_RC_RANGE_MINQP_SHIFT;
	}

	/* PPS 88 */
	dsc_cfg->native_422 = pps_payload->native_422_420 & BIT(0);
	dsc_cfg->native_420 = (pps_payload->native_422_420 & BIT(1)) >> 1;

	/* PPS 89 */
	dsc_cfg->second_line_bpg_offset = pps_payload->second_line_bpg_offset;

	/* PPS 90, 91 */
	dsc_cfg->nsl_bpg_offset = be16_to_cpu(pps_payload->nsl_bpg_offset);

	/* PPS 92, 93 */
	dsc_cfg->second_line_offset_adj = be16_to_cpu(pps_payload->second_line_offset_adj);

	/* PPS 94 - 127 are O */

	pr_info("dsc vesion: %u.%u\n", dsc_cfg->dsc_version_major, dsc_cfg->dsc_version_minor);

	pr_info("dsc bpc: %u\n", dsc_cfg->bits_per_component);

	pr_info("dsc linebuf_depth: %u\n", dsc_cfg->line_buf_depth);
	pr_info("dsc block_pred_enable: %u\n", dsc_cfg->block_pred_enable);
	pr_info("dsc convert_rgb: %u\n", dsc_cfg->convert_rgb);
	pr_info("dsc simple_422: %u\n", dsc_cfg->simple_422);
	pr_info("dsc vbr_enable: %u\n", dsc_cfg->vbr_enable);
	pr_info("dsc dsc_bpp: %u\n", dsc_cfg->bits_per_pixel);

	pr_info("dsc pic_height: %u\n", dsc_cfg->pic_height);
	pr_info("dsc pic_width: %u\n", dsc_cfg->pic_width);
	pr_info("dsc slice_height: %u\n", dsc_cfg->slice_height);
	pr_info("dsc slice_width: %u\n", dsc_cfg->slice_width);
	pr_info("dsc chunk_size: %u\n", dsc_cfg->slice_chunk_size);

	pr_info("dsc initial_xmit_delay: %u\n", dsc_cfg->initial_xmit_delay);
	pr_info("dsc initial_dec_delay: %u\n", dsc_cfg->initial_dec_delay);
	pr_info("dsc initial_scale_value: %u\n", dsc_cfg->initial_scale_value);
	pr_info("dsc scale_increment_interval: %u\n", dsc_cfg->scale_increment_interval);
	pr_info("dsc scale_decrement_interval: %u\n", dsc_cfg->scale_decrement_interval);

	pr_info("dsc first_line_bpg_offset: %u\n", dsc_cfg->first_line_bpg_offset);
	pr_info("dsc nfl_bpg_offset: %u\n", dsc_cfg->nfl_bpg_offset);
	pr_info("dsc slice_bpg_offset: %u\n", dsc_cfg->slice_bpg_offset);
	pr_info("dsc initial_offset: %u\n", dsc_cfg->initial_offset);
	pr_info("dsc final_offset: %u\n", dsc_cfg->final_offset);

	pr_info("dsc flatness_min_qp: %u\n", dsc_cfg->flatness_min_qp);
	pr_info("dsc flatness_max_qp: %u\n", dsc_cfg->flatness_max_qp);

	pr_info("dsc rc_model_size: %u\n", dsc_cfg->rc_model_size);
	pr_info("dsc rc_edge_factor: %u\n", dsc_cfg->rc_edge_factor);
	pr_info("dsc rc_quant_incr_limit0: %u\n", dsc_cfg->rc_quant_incr_limit0);
	pr_info("dsc rc_quant_incr_limit1: %u\n", dsc_cfg->rc_quant_incr_limit1);
	pr_info("dsc rc_tgt_offset_hi: %u\n", dsc_cfg->rc_tgt_offset_high);
	pr_info("dsc rc_tgt_offset_lo: %u\n", dsc_cfg->rc_tgt_offset_low);

	for (i = 0; i < DSC_THRESH_LENGTH; i++)
		pr_info("dsc rc_buf_thresh[%u]: %u\n", i, dsc_cfg->rc_buf_thresh[i]);

	for (i = 0; i < DSC_RC_RANGE_LENGTH; i++) {
		pr_info("dsc rc_range[%u]: min_qp = %u, max_qp = %u\n",
			i, dsc_cfg->rc_range_params[i].range_min_qp, dsc_cfg->rc_range_params[i].range_max_qp);
		pr_info("offset = %u\n", dsc_cfg->rc_range_params[i].range_bpg_offset);
	}

	pr_info("dsc native_420: %u\n", dsc_cfg->native_420);
	pr_info("dsc native_422: %u\n", dsc_cfg->native_422);
	pr_info("dsc second_line_bpg_offset: %u\n", dsc_cfg->second_line_bpg_offset);
	pr_info("dsc nsl_bpg_offset: %u\n", dsc_cfg->nsl_bpg_offset);
	pr_info("dsc second_line_offset_adj: %u\n", dsc_cfg->second_line_offset_adj);
}

void rx_sdp_init(struct pps_sdp *pps_table)
{
	struct drm_dsc_picture_parameter_set pps_payload;
	errno_t err_ret;

	dpu_check_and_no_retval((pps_table == NULL), err, "pps_table is NULL!\n");

	err_ret = memcpy_s(pps_table, sizeof(*pps_table), &test_pps_table, PPS_SIZE);
	if (err_ret != EOK) {
		disp_pr_err("memcpy pps_table error\n");
		return;
	}

	rx_sdp_parse_pps(pps_table);

	memset_s(&pps_payload, sizeof(struct drm_dsc_picture_parameter_set), 0, sizeof(struct drm_dsc_picture_parameter_set));
	err_ret = memcpy_s(&pps_payload, sizeof(struct drm_dsc_picture_parameter_set), &test_pps_table, PPS_SIZE);
	if (err_ret != EOK) {
		disp_pr_err("memcpy pps_payload error\n");
		return;
	}
	rx_sdp_parse_pps2(&pps_payload);
}

static void dprx_sdp_parse_colorimetry_format(uint8_t colorimetry_format)
{
	switch (colorimetry_format) {
	case 0:
		dpu_pr_info("[DPRX] sRGB (IEC 61966-2-1)\n");
		break;
	case 1:
		dpu_pr_info("[DPRX] RGB wide gamut fixed point\n");
		break;
	case 2:
		dpu_pr_info("[DPRX] RGB wide gamut floating point\n");
		break;
	case 3:
		dpu_pr_info("[DPRX] Adobe RGB\n");
		break;
	case 4:
		dpu_pr_info("[DPRX] DCI-P3\n");
		break;
	case 5:
		dpu_pr_info("[DPRX] Custom Color Profile\n");
		break;
	case 6:
		dpu_pr_info("[DPRX] ITU-R BT.2020 R'G'B'\n");
		break;
	default:
		break;
	}
}

static void dprx_sdp_parse_color_depth(uint8_t color_depth_indication)
{
	switch (color_depth_indication) {
	case 0:
		dpu_pr_info("[DPRX] bpc = 6\n");
		break;
	case 1:
		dpu_pr_info("[DPRX] bpc = 8\n");
		break;
	case 2:
		dpu_pr_info("[DPRX] bpc = 10\n");
		break;
	case 3:
		dpu_pr_info("[DPRX] bpc = 12\n");
		break;
	case 4:
		dpu_pr_info("[DPRX] bpc = 16\n");
		break;
	default:
		break;
	}
}

/* for test, todo */
static int dprx_sdp_parse_vsc_sdp(uint8_t *db_data)
{
	uint8_t pixel_encoding_indication;
	uint8_t colorimetry_format;
	uint8_t color_depth_indication;

	pr_info("[DPRX] db16: 0x%u, db17: 0x%u\n", db_data[16], db_data[17]);
	pixel_encoding_indication = (db_data[16] >> 4) & GENMASK(3, 0);
	colorimetry_format = db_data[16] & GENMASK(3, 0);
	color_depth_indication = db_data[17] & GENMASK(2, 0);

	switch (pixel_encoding_indication) {
	case 0:
		dpu_pr_info("[DPRX] pixel_encoding: RGB\n");
		dprx_sdp_parse_colorimetry_format(colorimetry_format);
		dprx_sdp_parse_color_depth(color_depth_indication);
		break;
	case 1:
	case 2:
	case 3:
		if (pixel_encoding_indication == 1)
			dpu_pr_info("[DPRX] pixel_encoding: YCbCr 444\n");
		else if (pixel_encoding_indication == 2)
			dpu_pr_info("[DPRX] pixel_encoding: YCbCr 422\n");
		else
			dpu_pr_info("[DPRX] pixel_encoding: YCbCr 420\n");

		switch (colorimetry_format) {
		case 0:
			dpu_pr_info("[DPRX] ITU-R BT.601\n");
			break;
		case 1:
			dpu_pr_info("[DPRX] ITU-R BT.709\n");
			break;
		case 2:
			dpu_pr_info("[DPRX] xvYCC601\n");
			break;
		case 3:
			dpu_pr_info("[DPRX] xvYCC709\n");
			break;
		case 4:
			dpu_pr_info("[DPRX] sYCC601\n");
			break;
		case 5:
			dpu_pr_info("[DPRX] opYCC601\n");
			break;
		case 6:
			dpu_pr_info("[DPRX] ITU-R BT.2020 Y'C_C'BC_C'RC\n");
			break;
		case 7:
			dpu_pr_info("[DPRX] ITU-R BT.2020 Y'C'BC'R\n");
			break;
		default:
			break;
	}
	dprx_sdp_parse_color_depth(color_depth_indication);
	break;
	case 4:
		dpu_pr_info("[DPRX] pixel_encoding: YONLY\n");
		break;
	case 5:
		dpu_pr_info("[DPRX] pixel_encoding: RAW\n");
		break;
	default:
		break;
	}

	return 0;
}

static void dprx_parse_pps_sdp(uint32_t db_data_len, uint8_t *db_data, struct dprx_ctrl *dprx)
{
	uint32_t reg;
	if (db_data_len != PPS_SIZE) {
		dpu_pr_err("[DPRX] PPS_SDP len error\n");
		return;
	}
	rx_sdp_parse_pps2((struct drm_dsc_picture_parameter_set *)db_data);
	reg = dprx_readl(dprx, 0x160);
	if (reg & BIT(0)) {
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + 0x814);
		reg &= ~BIT(5);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0x814, reg);
		reg = 0x0;
		reg |= (0xFFFF & dsc_cfg->slice_chunk_size);
		reg |= ((0xFFFF & dsc_cfg->slice_width) << 16);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0x810, reg);
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + 0x814);
		reg = 0x1F & (dsc_cfg->pic_width / dsc_cfg->slice_width);
		reg |= BIT(5);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0x814, reg);
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + 0x830);
		reg |= BIT(5);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0x830, reg);
	}
}

static int dprx_parse_vsc_sdp(struct sdp_hb_fifo_data *hb_data, uint8_t *db_data, uint32_t db_data_len)
{
	int ret = 0;
	if (hb_data->header.headers.hb2 != 0x5 || hb_data->header.headers.hb3 != 0x13) {
		dpu_pr_info("[DPRX] VSC_SDP dont contain Pixel Encoding/Colorimetry Format indication\n");
		return ret;
	}
	if (db_data_len != SDP_COMMON_PAYLOAD_SIZE) {
		dpu_pr_err("[DPRX] VSC_SDP len error\n");
		return ret;
	}
	ret = dprx_sdp_parse_vsc_sdp(db_data);
	return ret;
}

static int dprx_parse_single_sdp(struct dprx_ctrl *dprx, struct sdp_hb_fifo_data *hb_data, uint8_t *db_data,
									uint32_t db_data_len)
{
	int ret = 0;

	switch (hb_data->header.headers.hb1) {
	case DISPLAYPORT_RESERVED1_SDP:
	case DISPLAYPORT_RESERVED2_SDP:
		dpu_pr_info("[DPRX] SDP_Type: DisplayPort Reserved\n");
		break;
	case AUDIO_TIMESTAMP_SDP:
		dpu_pr_info("[DPRX] SDP_Type: AUDIO_TIMESTAMP_SDP\n");
		break;
	case AUDIO_STREAM_SDP:
		dpu_pr_info("[DPRX] SDP_Type: AUDIO_STREAM_SDP\n");
		break;
	case EXTENSION_SDP:
		dpu_pr_info("[DPRX] SDP_Type: EXTENSION_SDP\n");
		break;
	case AUDIO_COPY_MANAGEMENT_SDP:
		dpu_pr_info("[DPRX] SDP_Type: AUDIO_COPY_MANAGEMENT_SDP\n");
		break;
	case ISRC_SDP:
		dpu_pr_info("[DPRX] SDP_Type: ISRC_SDP\n");
		break;
	case VSC_SDP:
		dpu_pr_info("[DPRX] SDP_Type: VSC_SDP\n");
		/* for test, todo */
		ret = dprx_parse_vsc_sdp(hb_data, db_data, db_data_len);
		break;
	case PPS_SDP:
		dpu_pr_info("[DPRX] SDP_Type: PPS_SDP\n");
		/* for test, todo */
		dprx_parse_pps_sdp(db_data_len, db_data, dprx);
		break;
	case VSC_EXT_VESA_SDP:
		dpu_pr_info("[DPRX] SDP_Type: VSC_EXT_VESA_SDP\n");
		break;
	case VSC_EXT_CTA_SDP:
		dpu_pr_info("[DPRX] SDP_Type: VSC_EXT_CTA_SDP\n");
		break;
	case ADAPTIVE_SYNC_SDP:
		dpu_pr_info("[DPRX] SDP_Type: ADAPTIVE_SYNC_SDP\n");
		break;
	case VENDOR_SPECIFIC_INFOFRAME_SDP:
		dpu_pr_info("[DPRX] SDP_Type: VENDOR_SPECIFIC_INFOFRAME_SDP\n");
		break;
	case AVI_INFOFRAME_SDP:
		dpu_pr_info("[DPRX] SDP_Type: AVI_INFOFRAME_SDP\n");
		break;
	case SPD_INFOFRAME_SDP:
		dpu_pr_info("[DPRX] SDP_Type: SPD_INFOFRAME_SDP\n");
		break;
	case AUDIO_INFOFRAME_SDP:
		dpu_pr_info("[DPRX] SDP_Type: AUDIO_INFOFRAME_SDP\n");
		break;
	case MPEG_SOURCE_INFOFRAME_SDP:
		dpu_pr_info("[DPRX] SDP_Type: MPEG_SOURCE_INFOFRAME_SDP\n");
		break;
	case NTSC_VBI_INFOFRAME_SDP:
		dpu_pr_info("[DPRX] SDP_Type: NTSC_VBI_INFOFRAME_SDP\n");
		break;
	case DYNAMIC_RANGE_AND_MASTERING_INFOFRAME_SDP:
		dpu_pr_info("[DPRX] SDP_Type: DYNAMIC_RANGE_AND_MASTERING_INFOFRAME_SDP\n");
		/* todo */
		break;
	default:
		break;
	}

	return ret;
}

static int dprx_sdp_parse(struct dprx_ctrl *dprx)
{
	struct rx_sdp_raw_data *sdp_raw_data = &dprx->sdp_raw_data;
	int ret = 0;
	uint32_t i;
	uint32_t data_len = 0;
	uint32_t data_len_sum = 0;
	uint8_t *block = NULL;

	block = sdp_raw_data->db_fifo_data;

	for (i = 0; i < sdp_raw_data->hb_data_len; i++) {
		data_len = sdp_raw_data->hb_fifo_data[i].data_len;
		data_len_sum += data_len;
		if (data_len_sum > sdp_raw_data->db_data_len) {
			dpu_pr_err("[DPRX] SDP parse error\n");
			return -1;
		}
		dpu_pr_info("[DPRX] SDP %u len: %u\n", i, data_len);

		ret = dprx_parse_single_sdp(dprx, &sdp_raw_data->hb_fifo_data[i], block, data_len * SDP_ONE_DB_SIZE);
		block += data_len * SDP_ONE_DB_SIZE;
	}

	return ret;
}

/* for test, todo */
static void dprx_dump_hw_parse_sdp_type(uint32_t reg)
{
	if (reg & EXT_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: EXTENSION_SDP\n");
	if (reg & COPY_MANAG_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: AUDIO_COPY_MANAGEMENT_SDP\n");
	if (reg & ISRC_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: ISRC_SDP\n");
	if (reg & VSC_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: VSC_SDP\n");
	if (reg & PPS_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: PPS_SDP\n");
	if (reg & VEN_SPEC_INFOFRAME_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: VENDOR_SPECIFIC_INFOFRAME_SDP\n");
	if (reg & AVI_INFOFRAME_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: AVI_INFOFRAME_SDP\n");
	if (reg & SPD_INFOFRAME_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: SPD_INFOFRAME_SDP\n");
	if (reg & AUDIO_INFOFRAME_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: AUDIO_INFOFRAME_SDP\n");
	if (reg & MPEG_INFOFRAME_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: MPEG_SOURCE_INFOFRAME_SDP\n");
	if (reg & VBI_INFOFRAME_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: NTSC_VBI_INFOFRAME_SDP\n");
	if (reg & DRM_INFOFRAME_SDP_TYPE)
		dpu_pr_info("[DPRX] HW_PARSE_SDP_Type: DYNAMIC_RANGE_AND_MASTERING_INFOFRAME_SDP\n");
}

static void dprx_handle_sdp(struct dprx_ctrl *dprx)
{
	int ret;
	uint32_t reg;
	uint32_t sdp_db_len;
	uint32_t sdp_hb_len;
	uint32_t i;

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_SDP_TYPE_STATUS);
	dprx_dump_hw_parse_sdp_type(reg);

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_SDP_DB_HB_FIFO_CNT);
	sdp_db_len = (reg & DPRX_SDP_DB_LEN) >> DPRX_SDP_DB_LEN_OFFSET;
	sdp_hb_len = (reg & DPRX_SDP_HB_LEN) >> DPRX_SDP_HB_LEN_OFFSET;
	dpu_pr_info("[DPRX] SDP_DB_LEN: %u, SDP_HB_LEN: %u\n", sdp_db_len, sdp_hb_len);

	if (sdp_hb_len > SDP_HB_FIFO_SIZE || sdp_db_len > SDP_DB_FIFO_SIZE / SDP_ONE_DB_SIZE) {
		dpu_pr_err("[DPRX] SDP len error");
		/* todo: clear fifo */
		return;
	}

	memset_s(&dprx->sdp_raw_data, sizeof(struct rx_sdp_raw_data), 0, sizeof(struct rx_sdp_raw_data));
	dprx->sdp_raw_data.hb_data_len = sdp_hb_len;
	dprx->sdp_raw_data.db_data_len = sdp_db_len;

	/* read hb data */
	dpu_pr_info("[DPRX] SDP_HB_INFO:\n");
	for (i = 0; i < sdp_hb_len; i++) {
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_SDP_HB_DATA);
		dprx->sdp_raw_data.hb_fifo_data[i].data_len = reg & GENMASK(15, 0);
		dpu_pr_info("[DPRX] 0x%x\n", reg);
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_SDP_HB_DATA);
		dprx->sdp_raw_data.hb_fifo_data[i].header.value = reg;
		dpu_pr_info("[DPRX] 0x%x\n", reg);
	}

	/* read db data */
	dpu_pr_info("[DPRX] SDP_DB_INFO:\n");
	for (i = 0; i < sdp_db_len * 4; i++) {
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_SDP_DB_DATA);
		dpu_pr_info("[DPRX] 0x%x\n", reg);
		dprx->sdp_raw_data.db_fifo_data[i * 4] = reg & GENMASK(7, 0);
		dprx->sdp_raw_data.db_fifo_data[i * 4 + 1] = (reg >> 8) & GENMASK(7, 0);
		dprx->sdp_raw_data.db_fifo_data[i * 4 + 2] = (reg >> 16) & GENMASK(7, 0);
		dprx->sdp_raw_data.db_fifo_data[i * 4 + 3] = (reg >> 24) & GENMASK(7, 0);
	}
	ret = dprx_sdp_parse(dprx);

	return;
}

void dprx_sdp_wq_handler(struct work_struct *work)
{
	struct dprx_ctrl *dprx = NULL;

	dprx = container_of(work, struct dprx_ctrl, dprx_sdp_work);

	dpu_check_and_no_retval((dprx == NULL), err, "[DPRX] dprx is NULL!\n");

	if (!dprx->power_on) /* for test, todo: add video_on */
		return;

	dprx_handle_sdp(dprx);
}