/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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


#ifndef DPUFE_COMMON_DEF_H
#define DPUFE_COMMON_DEF_H

#include <linux/types.h>

#define MAX_DSS_SRC_NUM 9
#define DPU_OV_BLOCK_NUMS 1
#define SND_INFO_MAGIC_NUM 0x12345678

enum DISP_SOURCE {
	RDA_DISP = 0,
	NON_RDA_DISP,
	MAX_SRC_NUM,
};

typedef struct dss_rect {
	int32_t x;
	int32_t y;
	int32_t w;
	int32_t h;
} dss_rect_t;

struct layer_info {
	uint32_t format;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint32_t buf_size;
	uint32_t stride;
	uint64_t vir_addr;
	int32_t shared_fd;
	struct dma_buf *buf;
	dss_rect_t src_rect;
	dss_rect_t dst_rect;
	uint32_t transform;
	int32_t blending;
	uint32_t glb_alpha;
	uint32_t color;
	int32_t layer_idx;
	int32_t chn_idx;
	uint32_t need_cap;

	uint64_t afbc_header_addr;
	uint64_t afbc_header_offset;
	uint64_t afbc_payload_addr;
	uint64_t afbc_payload_offset;
	uint32_t afbc_header_stride;
	uint32_t afbc_payload_stride;
	uint32_t afbc_scramble_mode;
	uint32_t stride_plane1;
	uint32_t stride_plane2;
	uint32_t offset_plane1;
	uint32_t offset_plane2;
	uint32_t csc_mode;
	uint32_t mmbuf_base;
	uint32_t mmbuf_size;
	uint32_t mmu_enable;
	uint32_t disp_source;
};

struct block_info {
	struct layer_info layer_infos[MAX_DSS_SRC_NUM];
	dss_rect_t ov_block_rect;
	uint32_t layer_nums;
};

struct dpu_core_disp_data {
	struct block_info ov_block_infos[DPU_OV_BLOCK_NUMS];
	uint32_t ov_block_nums;
	uint32_t disp_id;
	int32_t ovl_idx;
	uint32_t frame_no;
	uint32_t magic_num;
};

#endif
