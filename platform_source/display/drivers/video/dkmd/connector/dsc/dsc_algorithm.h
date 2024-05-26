/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef DSC_ALGORITHM_H
#define DSC_ALGORITHM_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>

#define range_clamp(x, min, max) ((x) > (max) ? (max) : ((x) < (min) ? (min) : (x)))
#define range_check(s, x, low_bound, upper_bound) \
	do { \
		if (((x) < (low_bound)) || ((x) > (upper_bound))) { \
			dpu_pr_err(s" out of range, needs to be between %d and %d\n", low_bound, upper_bound); \
		} \
	} while (0)
#ifndef round1
#define round1(x, y)  ((x) / (y) + ((x) % (y)  ? 1 : 0))
#endif
#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif
#define PLATFORM_COUNT 5
#define DSC_0BPP (0)
#define DPTX_DSC_NUM_ENC_MSK GENMASK(3, 0)
#define DPTX_DSC_CTL 0x0234
#define DPTX_DSC_HWCFG 0x0230
#define DSC_RC_RANGE_LENGTH 15
#define DSC_THRESH_LENGTH 14
#define DSC_ENCODERS 2
#define DSC_6BPP 6
#define DSC_8BPP 8
#define DSC_10BPP 10
#define DSC_12BPP 12
#define DSC_14BPP 14
#define DSC_16BPP 16
#define DSC_8BPC 8
#define DSC_10BPC 10
#define DSC_12BPC 12
#define DSC_14BPC 14
#define DSC_16BPC 16

#define OFFSET_FRACTIONAL_BIT 11
#define LINEBUF_DEPTH_9 9
#define LINEBUF_DEPTH_11 11
#define MAX_LINEBUF_DEPTH 11
#define DPTX_CONFIG_REG2 0x104
#define DSC_MAX_NUM_LINES_SHIFT 16
#define DSC_MAX_NUM_LINES_MASK GENMASK(31, 16)

#define INITIAL_OFFSET_8BPP 6144
#define INITIAL_DELAY_8BPP 512

#define INITIAL_OFFSET_10BPP 5632
#define INITIAL_DELAY_10BPP 512
#define INITIAL_DELAY_10BPP_RGB_DEFAULT 410

#define INITIAL_OFFSET_12BPP 2048
#define INITIAL_DELAY_12BPP 341
#define INITIAL_OFFSET_12BPP_RGB_DEFAULT 2048
#define INITIAL_DELAY_12BPP_RGB_DEFAULT 341

#define INITIAL_OFFSET_12BPP_YUV422_DEFAULT 6144
#define INITIAL_DELAY_12BPP_YUV422_DEFAULT 512
#define INITIAL_OFFSET_12BPP_YUV422 5632

#define INITIAL_OFFSET_14BPP_YUV422_DEFAULT 5632
#define INITIAL_DELAY_14BPP_YUV422_DEFAULT 410

#define INITIAL_OFFSET_16BPP_YUV422 2048
#define INITIAL_DELAY_16BPP_8BPC_YUV422 256
#define INITIAL_OFFSET_16BPP_YUV422_DEFAULT 2048
#define INITIAL_DELAY_16BPP_YUV422_DEFAULT 341

#define FLATNESS_MIN_QP_8BPC 3
#define FLATNESS_MAX_QP_8BPC 12
#define FLATNESS_MIN_QP_10BPC 7
#define FLATNESS_MAX_QP_10BPC 16
#define FLATNESS_MIN_QP_12BPC 11
#define FLATNESS_MAX_QP_12BPC 20
#define RC_QUANT_INCR_LIMIT0_8BPC 11
#define RC_QUANT_INCR_LIMIT1_8BPC 11
#define RC_QUANT_INCR_LIMIT0_10BPC 15
#define RC_QUANT_INCR_LIMIT1_10BPC 15
#define RC_QUANT_INCR_LIMIT0_12BPC 19
#define RC_QUANT_INCR_LIMIT1_12BPC 19

#define RC_MODEL_SIZE 8192
#define RC_EDGE_FACTOR 6
#define RC_TGT_OFFSET_HIGH 3
#define RC_TGT_OFFSET_LOW 3
#define DP_DSC_MAJOR_SHIFT 0
#define PIXELS_PER_GROUP 3
#define BITS_PER_CHUNK 8
#define RC_RANGE_COUNT 13
#define RC_MODEL_GEN_RC_COUNT 4
#define RC_MODEL_YUV422_DEFAULT 3
#define RC_MODEL_RGB_DEFAULT 4
#define DSC_OUTPUT_MODE 24

enum DSC_VER {
	DSC_VERSION_V_1_1 = 0,
	DSC_VERSION_V_1_2
};

enum pixel_format {
	DSC_RGB = 0,
	DSC_YUV,
	DSC_YUV420,
	DSC_YUV422,
	DSC_YUV444,
};

enum gen_rc_params {
	DSC_NOT_GENERATE_RC_PARAMETERS = 0,
	DSC_GENERATE_RC_PARAMETERS,
};

struct rc_table_param {
	uint32_t min_qp : 5;
	uint32_t max_qp : 5;
	uint32_t offset : 6;
};

struct input_dsc_info {
	enum DSC_VER dsc_version;
	enum pixel_format format;
	uint32_t pic_width;
	uint32_t pic_height;
	uint32_t slice_width;
	uint32_t slice_height;
	uint32_t dsc_bpp;
	uint32_t dsc_bpc;
	uint32_t linebuf_depth;
	uint32_t block_pred_enable;
	uint32_t gen_rc_params;
};

struct dsc_info {
	uint32_t dsc_version_major;
	uint32_t dsc_version_minor;
	enum pixel_format format;
	uint32_t dsc_bpc;
	uint32_t linebuf_depth;
	uint16_t block_pred_enable;
	uint16_t gen_rc_params;
	uint16_t convert_rgb;
	uint16_t simple_422;
	uint16_t native_420;
	uint16_t native_422;
	uint32_t dsc_bpp;
	uint32_t pic_height;
	uint32_t pic_width;
	uint32_t slice_height;
	uint32_t slice_width;
	uint32_t output_height;
	uint32_t output_width;

	uint32_t chunk_size;
	uint32_t initial_xmit_delay;
	uint32_t initial_dec_delay;
	uint32_t initial_scale_value;
	uint32_t scale_increment_interval;
	uint32_t scale_decrement_interval;
	uint32_t first_line_bpg_offset;
	uint32_t second_line_bpg_offset;
	uint32_t nfl_bpg_offset;
	uint32_t slice_bpg_offset;
	uint32_t initial_offset;
	uint32_t final_offset;
	uint32_t flatness_min_qp;
	uint32_t flatness_max_qp;
	uint32_t groups_per_line;

	uint32_t rc_model_size;
	uint32_t rc_edge_factor;
	uint32_t rc_quant_incr_limit0;
	uint32_t rc_quant_incr_limit1;
	uint32_t rc_tgt_offset_hi;
	uint32_t rc_tgt_offset_lo;

	uint32_t rc_buf_thresh[DSC_THRESH_LENGTH];
	struct rc_table_param rc_range[DSC_RC_RANGE_LENGTH];
};

struct dpu_dsc_algorithm {
	void (*vesa_dsc_info_calc)(const struct input_dsc_info *input_para,
		struct dsc_info *output_para, struct dsc_info *exist_para);
	void (*dsc_config_rc_table)(struct dsc_info *dsc_info, struct rc_table_param *user_rc_table, uint32_t rc_table_len);
};

void init_dsc_algorithm(struct dpu_dsc_algorithm *p);

#endif
