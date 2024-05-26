/* Copyright (c) Huawei Technologies Co., Ltd. 2013-2021. All rights reserved.
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

#ifndef DPU_OVERLAY_UTILS_STRUCT_H
#define DPU_OVERLAY_UTILS_STRUCT_H

#include "../dpu.h"

struct dpu_ov_compose_rect {
	dss_rect_t *wb_dst_rect;
	dss_rect_t *wb_ov_block_rect;
	dss_rect_ltrb_t *clip_rect;
	dss_rect_t *aligned_rect;
};

struct dpu_ov_compose_flag {
	bool rdma_stretch_enable;
	bool has_base;
	bool csc_needed;
	bool enable_cmdlist;
};

struct ov_module_set_regs_flag {
	bool enable_cmdlist;
	bool is_first_ov_block;
	int task_end;
	int last;
};

/* for overlay arsr begin */
enum arsr2p_effect {
	ARSR2P_EFFECT = 0,
	ARSR2P_EFFECT_SCALE_UP,
	ARSR2P_EFFECT_SCALE_DOWN,
};

struct dss_arsr2p_pos_size {
	int ih_inc;
	int iv_inc;
	int ih_left;  /* input left acc */
	int ih_right;  /* input end position */
	int iv_top;  /* input top position */
	int iv_bottom;  /* input bottom position */
	int uv_offset;
	int src_width;
	int src_height;
	int dst_whole_width;
	int outph_left;   /* output left acc */
	int outph_right;  /* output end position */
	int outpv_bottom;  /* output bottom position */
};

struct dss_arsr_mode {
	bool nointplen;  /* bit8 */
	bool prescaleren;  /* bit7 */
	bool nearest_en;  /* bit6 */
	bool diintpl_en;  /* bit5 */
	bool textureanalyhsisen_en;  /* bit4 */
	bool arsr2p_bypass;  /* bit0 */
};

struct dss_arsr2p_scl_flag {
	bool en_hscl;
	bool en_vscl;
	bool hscldown_flag;
	bool rdma_stretch_enable;
};

struct arsr_scl_para {
	const int **scl_down;
	const int **scl_up;
	int row;
	int col;
};
/* for overlay arsr end */

/* for overlay bce_bcd begin */
enum hebce_rect_align_type {
	ROT90_YUV = 0,
	ROT90_NO_YUV,
	ROT0_YUV,
	ROT0_NO_YUV,
	ALIGN_TYPE_MAX
};

struct hebce_rect_border {
	int32_t width_min;
	int32_t width_max;
	int32_t width_alion;
	int32_t heigth_mix;
	int32_t heigth_max;
	int32_t heigth_align;
};

struct dss_hfbcd_para {
	bool mmu_enable;
	bool is_yuv_semi_planar;
	bool is_yuv_planar;
	int rdma_format;
	int rdma_transform;
	uint32_t stretch_size_vrt;
	uint32_t stretched_line_num;
	uint32_t mm_base0_y8;
	uint32_t mm_base1_c8;
	uint32_t mm_base2_y2;
	uint32_t mm_base3_c2;
	uint32_t hfbcd_block_type;

	uint32_t hfbcd_block_width_align;
	uint32_t hfbcd_block_height_align;
	uint32_t hfbcd_crop_num_max;

	int rdma_oft_x0;
	int rdma_oft_x1;
	uint32_t hfbcd_header_addr0;
	uint32_t hfbcd_header_stride0;
	uint32_t hfbcd_header_addr1;
	uint32_t hfbcd_header_offset;
	uint32_t hfbcd_header_stride1;
	uint32_t hfbcd_top_crop_num;
	uint32_t hfbcd_bottom_crop_num;
	uint32_t hfbcd_payload_addr0;
	uint32_t hfbcd_payload_stride0;
	uint32_t hfbcd_payload0_align;
	uint32_t hfbcd_payload_addr1;
	uint32_t hfbcd_payload_stride1;
	uint32_t hfbcd_payload1_align;
	uint32_t hfbcd_hreg_pic_width;
	uint32_t hfbcd_hreg_pic_height;
};

struct dss_hebcd_para {
	bool mmu_enable;
	bool is_yuv_semi_planar;
	int rdma_format;
	int rdma_transform;
	uint32_t stretch_size_vrt;
	uint32_t stretched_line_num;
	uint32_t mm_base0_y8;
	uint32_t mm_base1_c8;
	uint32_t hebcd_block_type;
	uint32_t color_transform;

	uint32_t hebcd_block_width_align;
	uint32_t hebcd_block_height_align;
	uint32_t hebcd_crop_num_max;

	int rdma_oft_x0;
	int rdma_oft_x1;
	uint32_t hebcd_header_addr0;
	uint32_t hebcd_header_stride0;
	uint32_t hebcd_header_addr1;
	uint32_t hebcd_header_stride1;
	uint32_t hebcd_header_offset;
	uint32_t hebcd_top_crop_num;
	uint32_t hebcd_bottom_crop_num;
	uint32_t hebcd_payload_addr0;
	uint32_t hebcd_payload_stride0;
	uint32_t hebcd_payload0_align;
	uint32_t hebcd_payload_addr1;
	uint32_t hebcd_payload_stride1;
	uint32_t hebcd_payload1_align;
	uint32_t hebcd_hreg_pic_width;
	uint32_t hebcd_hreg_pic_height;
	uint32_t hebcd_height_bf_str;
};

struct dss_hfbce_para {
	int wdma_format;
	int wdma_transform;
	uint32_t wdma_addr;

	bool mmu_enable;
	uint32_t hfbce_hreg_pic_blks;
	uint32_t hfbce_header_addr0;
	uint32_t hfbce_header_stride0;
	uint32_t hfbce_header_offset;
	uint32_t hfbce_payload_addr0;
	uint32_t hfbce_payload_stride0;
	uint32_t hfbce_header_addr1;
	uint32_t hfbce_header_stride1;
	uint32_t hfbce_payload_addr1;
	uint32_t hfbce_payload_stride1;
	uint32_t hfbce_payload0_align;
	uint32_t hfbce_payload1_align;
	uint32_t hfbce_block_width_align;
	uint32_t hfbce_block_height_align;
};

struct dss_hebce_para {
	int wdma_format;
	int wdma_transform;
	uint32_t color_transform;

	uint32_t hebce_header_addr0;
	uint32_t hebce_header_stride0;
	uint32_t hebce_header_offset;
	uint32_t hebce_payload_addr0;
	uint32_t hebce_payload_stride0;
	uint32_t hebce_header_addr1;
	uint32_t hebce_header_stride1;
	uint32_t hebce_payload_addr1;
	uint32_t hebce_payload_stride1;
	uint32_t hebce_payload0_align;
	uint32_t hebce_payload1_align;
	uint32_t hebce_block_width_align;
	uint32_t hebce_block_height_align;
};
/* for overlay bce_bcd end */

/* for overlay csc begin */
struct dss_csc_coe {
	int (*csc_coe_yuv2rgb)[5];
	int (*csc_coe_rgb2yuv)[5];
};
/* for overlay csc end */

/* for overlay scf begin */
struct post_scf_config {
	bool en_hscl;
	bool en_vscl;
	uint32_t h_ratio;
	uint32_t v_ratio;
	uint32_t h_v_order;
	uint32_t scf_en_vscl;
};
/* for overlay scf end */

/* for overlay scl begin */
struct wb_scl_info {
	bool en_hscl;
	bool en_vscl;
	bool en_mmp;
	uint32_t h_ratio;
	uint32_t v_ratio;
	uint32_t h_v_order;
	uint32_t acc_hscl;
	uint32_t acc_vscl;
	uint32_t scf_en_vscl;
};
/* for overlay scl end */

#endif  /* DPU_OVERLAY_UTILS_STRUCT_H */

