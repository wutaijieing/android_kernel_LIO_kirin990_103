/* Copyright (c) 2022, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HDMIRX_VIDEO_DSC_H
#define HDMIRX_VIDEO_DSC_H

#include "hdmirx_struct.h"

typedef enum {
	DSC_STATUS_UNSTABLE,
	DSC_STATUS_STABLE,
	DSC_STATUS_MAX
} hdmirx_dsc_status;

typedef struct {
	bool dsc_support;
	bool dsc_en;
	uint32_t  wait_stable_time;
	uint32_t  max_wait_stable_time;
	hdmirx_dsc_status status;
} hdmirx_dsc_ctx;

#define S_YES "yes"
#define S_NO "no"

typedef struct {
	uint8_t  dsc_version_major;  /* pps0[7:4] */
	uint8_t  dsc_version_minor;  /* pps0[3:0] */
	uint8_t  pps_identifier;     /* pps1[7:0] */
	uint8_t  bits_per_component; /* pps3[7:4] */
	uint8_t  linebuf_depth;      /* pps3[3:0] */
	uint8_t  block_pred_enable;  /* pps4[5] */
	uint8_t  convert_rgb;        /* pps4[4] */
	uint8_t  simple_422;         /* pps4[3] */
	uint8_t  vbr_enable;         /* pps4[2] */
	uint16_t bits_per_pixel;     /* pps4[1:0] pps5 */
	uint16_t pic_height;         /* pps6-7 */
	uint16_t pic_width;          /* pps8-9 */
	uint16_t slice_heigh;        /* pps10-11 */
	uint16_t slice_width;        /* pps12-13 */
	uint16_t chunk_size;         /* pps14-15 */
	uint16_t initial_xmit_delay; /* pps16[1:0] pps17 */
	uint16_t initial_dec_delay;  /* pps18-19 */
	uint16_t initial_scale_value; /* pps21[5:0] */
	uint16_t scale_increment_interval; /* pps22-23 */
	uint16_t scale_decrement_interval; /* pps24-25 */
	uint16_t first_line_bpg_offset; /* pps27[4:0] */
	uint16_t nfl_bpg_offset;     /* pps28-29 */
	uint16_t slice_bpg_offset;   /* pps30-31 */
	uint16_t initial_offset;     /* pps32-33 */
	uint16_t final_offset;       /* pps34-35 */
	uint8_t  flatness_min_qp;    /* pps36[4:0] */
	uint8_t  flatness_max_qp;    /* pps37[4:0] */
	uint8_t  rc_parameter_set[50]; /* 400 bit pps38-87 */
	uint8_t  native_420;         /* pps88[1] */
	uint8_t  native_422;         /* pps88[0] */
	uint8_t  second_line_bpg_offset; /* pps89[4:0] */
	uint16_t nsl_bpg_offset;     /* pps90-91 */
	uint16_t second_line_offset_adj; /* pps92-93 */
} hdmirx_dsc_pps_info;

int hdmirx_video_dsc_mask(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_video_dsc_irq_proc(struct hdmirx_ctrl_st *hdmirx);

#endif