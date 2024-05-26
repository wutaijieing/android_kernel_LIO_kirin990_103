/** @file
 *
 * Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef _HISI_DISP_H_
#define _HISI_DISP_H_

#include <linux/types.h>
#include <linux/fb.h>

#define HISI_DISP_IOCTL_MAGIC 'M'

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, al)  ((val) & ~((al)-1))
#endif
#ifndef ALIGN_UP
#define ALIGN_UP(val, al)    (((val) + ((al)-1)) & ~((al)-1))
#endif

enum {
	DISP_IOCTL_CMD_ONLINE_PLAY = 0x21,
	DISP_IOCTL_CMD_OFFLINE_PLAY,
	DISP_IOCTL_CMD_COPYBIT_PLAY,
	DISP_IOCTL_CMD_MAX,
};

#define HISI_DISP_ONLINE_PLAY  _IOW(HISI_DISP_IOCTL_MAGIC, DISP_IOCTL_CMD_ONLINE_PLAY,  struct hisi_display_frame)
#define HISI_DISP_OFFLINE_PLAY _IOW(HISI_DISP_IOCTL_MAGIC, DISP_IOCTL_CMD_OFFLINE_PLAY, struct hisi_display_frame)
#define HISI_DISP_COPYBIT_PLAY _IOW(HISI_DISP_IOCTL_MAGIC, DISP_IOCTL_CMD_COPYBIT_PLAY, struct hisi_display_frame)

#ifndef BIT
#define BIT(x)    (1 << (x))
#endif

/* 0, pipeline is pre
 * 1, pipeline is post
 */
enum PIPELINE_TYPE {
	PIPELINE_TYPE_PRE = 0,
	PIPELINE_TYPE_POST_ONLINE = BIT(0),
	PIPELINE_TYPE_POST_OFFLINE = BIT(1),
	PIPELINE_TYPE_POST = PIPELINE_TYPE_POST_ONLINE | PIPELINE_TYPE_POST_OFFLINE,

};

enum LAYER_SOURCE_TYPE_EN {
	LAYER_SRC_TYPE_LOCAL = 0,
	LAYER_SRC_TYPE_HDMIRX = 1,
	LAYER_SRC_TYPE_DPRX = 2,
	LAYER_SRC_TYPE_EDPRX = 3
};

#define MAX_CLIP_CNT     63
#define WLT_SLICE_MAX_COUNT 4

/* for fb0 fb1 fb2 and so on */
#define PRIMARY_PANEL_IDX     0
#define EXTERNAL_PANEL_IDX    1
#define AUXILIARY_PANEL_IDX   2
#define MEDIACOMMON_PANEL_IDX 3

#define PRE_PIPELINE_OPERATORS_MAX_COUNT 32
#define SRC_LAYERS_MAX_COUNT             32
#define POST_OV_ONLINE_PIPELINE_MAX_COUNT        2 // 3 offline pipeline + 2 online pipeline
#define POST_OV_OFFLINE_PIPELINE_MAX_COUNT       4
#define OPERATOR_BIT_FIELD_LEN        16

#define build_operator_id(type, id)  (((type) << OPERATOR_BIT_FIELD_LEN) | (id))
#define DPU_WIDTH(width)    ((width) - 1)
#define DPU_HEIGHT(height)  ((height) - 1)

/* align */
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, al)  ((val) & ~((typeof(val))(al)-1))
#endif
#ifndef ALIGN_UP
#define ALIGN_UP(val, al)    (((val) + ((al)-1)) & ~((typeof(val))(al)-1))
#endif

enum OV_BLENDING_MODE {
	BLENDING_NONE = 0,
	BLENDING_PREMULT = 1,
	BLENDING_COVERAGE = 2,
	BLENDING_MAX = 3,
};

enum dss_blend_mode {
	DSS_BLEND_CLEAR = 0,
	DSS_BLEND_SRC, // 1
	DSS_BLEND_DST, // 2
	DSS_BLEND_SRC_OVER_DST, // 3
	DSS_BLEND_DST_OVER_SRC, // 4
	DSS_BLEND_SRC_IN_DST, // 5
	DSS_BLEND_DST_IN_SRC, // 6
	DSS_BLEND_SRC_OUT_DST, // 7
	DSS_BLEND_DST_OUT_SRC, // 8
	DSS_BLEND_SRC_ATOP_DST, // 9
	DSS_BLEND_DST_ATOP_SRC, // 10
	DSS_BLEND_SRC_XOR_DST, // 11
	DSS_BLEND_SRC_ADD_DST, // 12
	DSS_BLEND_FIX_OVER, // 13
	DSS_BLEND_FIX_PER0, // 14
	DSS_BLEND_FIX_PER1, // 15
	DSS_BLEND_FIX_PER2, // 16
	DSS_BLEND_FIX_PER3, // 17
	DSS_BLEND_FIX_PER4, // 18
	DSS_BLEND_FIX_PER5, // 19
	DSS_BLEND_FIX_PER6, // 20
	DSS_BLEND_FIX_PER7, // 21
	DSS_BLEND_FIX_PER8, // 22
	DSS_BLEND_FIX_PER9, // 23
	DSS_BLEND_FIX_PER10, // 24
	DSS_BLEND_FIX_PER11, // 25
	DSS_BLEND_FIX_PER12, // 26
	DSS_BLEND_FIX_PER13, // 27
	DSS_BLEND_FIX_PER14, // 28
	DSS_BLEND_FIX_PER15, // 29
	DSS_BLEND_FIX_PER16, // 30
	DSS_BLEND_FIX_PER17, // 31

	DSS_BLEND_MAX,
};

typedef struct dss_ovl_alpha {
	uint32_t src_amode;
	uint32_t src_gmode;
	uint32_t alpha_offsrc;
	uint32_t src_lmode;
	uint32_t src_pmode;

	uint32_t alpha_smode;

	uint32_t dst_amode;
	uint32_t dst_gmode;
	uint32_t alpha_offdst;
	uint32_t dst_pmode;

	uint32_t fix_mode;
} dss_ovl_alpha_t;


/* dss capability priority description */
enum {
	CAP_BASE = BIT(0),
	CAP_DIM = BIT(1),
	CAP_PURE_COLOR = BIT(2),
	CAP_ROT = BIT(3),
	CAP_SCL = BIT(4),
	CAP_YUV_PACKAGE = BIT(5),
	CAP_YUV_SEMI_PLANAR = BIT(6),
	CAP_YUV_PLANAR = BIT(7),
	CAP_YUV_DEINTERLACE = BIT(8),
	CAP_AFBCE = BIT(9),
	CAP_AFBCD = BIT(10),
	CAP_TILE = BIT(11),
	CAP_2D_SHARPNESS = BIT(12),
	CAP_1D_SHARPNESS = BIT(13),
	CAP_HFBCE = BIT(14),
	CAP_HFBCD = BIT(15),
	CAP_HEBCE = BIT(16),
	CAP_HEBCD = BIT(17),
	CAP_HIGH_SCL = BIT(18),
	CAP_WLT = BIT(19),
	CAP_UVUP = BIT(20),
	CAP_HDR = BIT(21),
	CAP_CLD = BIT(22),
	CAP_DPP = BIT(23),
	CAP_MITM = BIT(24),
	CAP_ARSR0_POST = BIT(25),
	CAP_ARSR1_PRE = BIT(26),
	CAP_DEMA = BIT(27),
};

enum wb_compose_type {
	WB_COMPOSE_PRIMARY = 0,
	WB_COMPOSE_COPYBIT,
	WB_COMPOSE_TYPE_MAX,
};

enum {
	COMPRESS_TYPE_NONE = 0,
	COMPRESS_TYPE_HEBC,
	COMPRESS_TYPE_HEMC,
	COMPRESS_TYPE_AFBC
};

enum dpu_csc_mode {
	DPU_CSC_601_WIDE = 0,
	DPU_CSC_601_NARROW,
	DPU_CSC_709_WIDE,
	DPU_CSC_709_NARROW,
	DPU_CSC_2020,
	DPU_CSC_MOD_MAX,
};

enum hisi_fb_pixel_format {
	HISI_FB_PIXEL_FORMAT_RGB_565 = 0,
	HISI_FB_PIXEL_FORMAT_RGBX_4444,
	HISI_FB_PIXEL_FORMAT_RGBA_4444,
	HISI_FB_PIXEL_FORMAT_RGBX_5551,
	HISI_FB_PIXEL_FORMAT_RGBA_5551,
	HISI_FB_PIXEL_FORMAT_RGBX_8888,
	HISI_FB_PIXEL_FORMAT_RGBA_8888,

	HISI_FB_PIXEL_FORMAT_BGR_565,
	HISI_FB_PIXEL_FORMAT_BGRX_4444,
	HISI_FB_PIXEL_FORMAT_BGRA_4444,
	HISI_FB_PIXEL_FORMAT_BGRX_5551,
	HISI_FB_PIXEL_FORMAT_BGRA_5551,
	HISI_FB_PIXEL_FORMAT_BGRX_8888,
	HISI_FB_PIXEL_FORMAT_BGRA_8888,

	HISI_FB_PIXEL_FORMAT_YUV_422_I,

	/* YUV Semi-planar */
	HISI_FB_PIXEL_FORMAT_YCBCR_422_SP, /* NV16 */
	HISI_FB_PIXEL_FORMAT_YCRCB_422_SP,
	HISI_FB_PIXEL_FORMAT_YCBCR_420_SP,
	HISI_FB_PIXEL_FORMAT_YCRCB_420_SP, /* NV21*/

	/* YUV Planar */
	HISI_FB_PIXEL_FORMAT_YCBCR_422_P,
	HISI_FB_PIXEL_FORMAT_YCRCB_422_P,
	HISI_FB_PIXEL_FORMAT_YCBCR_420_P,
	HISI_FB_PIXEL_FORMAT_YCRCB_420_P, /* HISI_FB_PIXEL_FORMAT_YV12 */

	/* YUV Package */
	HISI_FB_PIXEL_FORMAT_YUYV_422_PKG,
	HISI_FB_PIXEL_FORMAT_UYVY_422_PKG,
	HISI_FB_PIXEL_FORMAT_YVYU_422_PKG,
	HISI_FB_PIXEL_FORMAT_VYUY_422_PKG,

	/* 10bit */
	HISI_FB_PIXEL_FORMAT_RGBA_1010102,
	HISI_FB_PIXEL_FORMAT_BGRA_1010102,
	HISI_FB_PIXEL_FORMAT_Y410_10BIT,
	HISI_FB_PIXEL_FORMAT_YUV422_10BIT,

	HISI_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT,
	HISI_FB_PIXEL_FORMAT_YCRCB422_SP_10BIT,
	HISI_FB_PIXEL_FORMAT_YCRCB420_P_10BIT,
	HISI_FB_PIXEL_FORMAT_YCRCB422_P_10BIT,

	HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT,
	HISI_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT,
	HISI_FB_PIXEL_FORMAT_YCBCR420_P_10BIT,
	HISI_FB_PIXEL_FORMAT_YCBCR422_P_10BIT,

	HISI_FB_PIXEL_FORMAT_YUVA444,

	HISI_FB_PIXEL_FORMAT_YUV444,
	HISI_FB_PIXEL_FORMAT_YUYV_420_PKG,
	HISI_FB_PIXEL_FORMAT_YUYV_420_PKG_10BIT,

	HISI_FB_PIXEL_FORMAT_D3_128,
	HISI_FB_PIXEL_FORMAT_MAX,
};

enum hisi_fb_transform {
	HISI_FB_TRANSFORM_NOP = 0x0,
	/* flip source image horizontally (around the vertical axis) */
	HISI_FB_TRANSFORM_FLIP_H = 0x01,
	/* flip source image vertically (around the horizontal axis)*/
	HISI_FB_TRANSFORM_FLIP_V = 0x02,
	/* rotate source image 90 degrees clockwise */
	HISI_FB_TRANSFORM_ROT_90 = 0x04,
	/* rotate source image 180 degrees */
	HISI_FB_TRANSFORM_ROT_180 = 0x03,
	/* rotate source image 270 degrees clockwise */
	HISI_FB_TRANSFORM_ROT_270 = 0x07,
};

enum {
	HEBC_PANEL_Y = 0,
	HEBC_PANEL_UV,
	HEBC_PANEL_DEMA_LUT,
	HEBC_PANEL_MAX_COUNT
};

enum {
	YUV_PANEL_Y = 0,
	YUV_PANEL_UV,
	YUV_PANEL_MAX_COUNT
};

enum CSC_MODE {
	CSC_MODE_NONE = 0,
	CSC_601_WIDE = 1,
	CSC_601_NARROW,
	CSC_709_WIDE,
	CSC_709_NARROW,
	CSC_2020,
	CSC_MODE_MAX,
};

enum WCG_MODE {
	WCG_MODE_NONE = 0,
	WCG_SRGB_2_P3,
	WCG_YUV_2_P3,
};

struct disp_rect {
	int32_t x;
	int32_t y;
	uint32_t w;
	uint32_t h;
	uint32_t line_offset;
	bool last_block_per_slice;
};

/***********************************static inline *********************/
static inline bool has_same_width(const struct disp_rect *src_rect, const struct disp_rect *dst_rect)
{
	return src_rect->w == dst_rect->w;
}

static inline bool has_same_height(const struct disp_rect *src_rect, const struct disp_rect *dst_rect)
{
	return src_rect->h == dst_rect->h;
}

#define has_same_dim(src, dst) (has_same_width(src, dst) && has_same_height(src, dst))

struct disp_rect_ltrb {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

/*********************************** layer data struct ***********************/
struct hebc_plane_info {
	uint64_t header_addr;
	uint64_t header_offset;
	uint32_t header_stride;

	uint64_t payload_addr;
	uint64_t payload_offset;
	uint32_t payload_stride;
};

struct slice_plane_info {
	uint64_t header_addr;
	uint64_t payload_addr;
};

struct layer_img {
	uint64_t phy_addr;
	uint64_t vir_addr;
	int32_t shared_fd;

	uint32_t buff_size;
	uint32_t format;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint8_t bpp;
	uint8_t reserved[3];

	uint8_t csc_mode; // enum csc_mode
	uint8_t wcg_mode;
	uint8_t mmu_enable;
	uint8_t secure_mode;

	uint32_t compress_type;
	uint32_t wlt_dma_addr;

	uint32_t stride_plane1;
	uint32_t stride_plane2;
	uint32_t offset_plane1;
	uint32_t offset_plane2;

	/* hebc */
	uint32_t hebcd_block_type;
	uint32_t hebc_scramble_mode;
	struct hebc_plane_info hebc_planes[HEBC_PANEL_MAX_COUNT];
};

struct block_info {
	int32_t first_tile;
	int32_t last_tile;
	uint32_t acc_hscl;
	uint32_t h_ratio;
	uint32_t v_ratio;
	uint32_t h_ratio_arsr2p;
	uint32_t arsr2p_left_clip;
	uint32_t both_vscfh_arsr2p_used;
	struct disp_rect arsr2p_in_rect;
	uint32_t arsr2p_src_x; /* new added */
	uint32_t arsr2p_src_y; /* new added */
	uint32_t arsr2p_dst_x; /* new added */
	uint32_t arsr2p_dst_y; /* new added */
	uint32_t arsr2p_dst_w; /* new added */
	int32_t h_v_order;

	uint32_t slice_addr[WLT_SLICE_MAX_COUNT];

	struct slice_plane_info slice_planes[WLT_SLICE_MAX_COUNT][YUV_PANEL_MAX_COUNT];
	uint32_t wlt_slice_row;
} ;

struct pipeline_src_layer {
	struct layer_img img;
	struct disp_rect src_rect;
	struct disp_rect src_rect_mask;
	struct disp_rect dst_rect;
	struct block_info block_info;
	int32_t blending_mode;
	uint16_t transform;
	uint16_t glb_alpha;
	uint32_t glb_color;

	uint32_t need_caps;
	uint32_t slice_num;
	int32_t acquire_fence;
	int32_t layer_id;
	int32_t dma_sel;

	uint32_t is_cld_layer;
	uint32_t cld_width;
	uint32_t cld_height;
	uint32_t cld_data_offset;
	bool cld_reuse;

	struct disp_rect wb_block_rect;

	uint32_t src_type; /* LAYER_SOURCE_TYPE_EN ; default LAYER_SRC_TYPE_LOCAL */
	uint32_t dsd_index;
};

/*********************************** online data struct ***********************/
struct post_online_info {
	struct disp_rect dirty_rect;
	struct disp_rect rog_src_rect;
	struct disp_rect rog_dst_rect;
	int32_t release_fence;
	int32_t retire_fence;
	uint32_t frame_no;

	/* other data */
};

/*********************************** offline data struct ***********************/
struct wb_block {
	uint32_t acc_hscl;
	uint32_t h_ratio;
	int32_t h_v_order;
	struct disp_rect src_rect;
	struct disp_rect dst_rect;
};

struct disp_wb_layer {
	struct layer_img dst;
	struct disp_rect src_rect;
	struct disp_rect dst_rect;
	uint32_t transform;
	int32_t wchn_idx;
	uint32_t need_caps;
	uint32_t slice_num;
	int32_t layer_id;

	int32_t acquire_fence;
	int32_t release_fence;
	int32_t retire_fence;

	struct wb_block wb_block;
};

struct disp_overlay_block {
	struct pipeline_src_layer layer_infos[9]; // MAX_DPU_SRC_NUM
	struct disp_rect ov_block_rect;
	uint32_t layer_nums;
	uint32_t reserved0;
};

struct post_offline_info {
	uint8_t wb_type;
	uint8_t enable_offline;
	uint8_t to_be_continued;
	uint8_t reserved;

	struct disp_wb_layer wb_layer;

	/* block calculate result */
	struct disp_rect wb_ov_block_rect;

	/* for special treatment */
	bool last_block;
	int h_block_index;
	uint32_t wb_layer_nums;
	struct disp_rect src_layer_size;
};

/*********************************** pipeline data struct ***********************/
union operator_id {
	uint32_t id;
	struct {
		uint32_t type : OPERATOR_BIT_FIELD_LEN;
		uint32_t idx : OPERATOR_BIT_FIELD_LEN;
	} info;
};

struct pipeline {
	uint32_t pipeline_id;

	/* @brief the pipeline include operators, the operators must be in order at operats arry.
	 *        such as: rdma1->rot1->scl1->ov0.
	 *        now, the max operators count is 32, it's a estimated number. ^_^
	 */
	uint32_t operator_count;
	union operator_id operators[PRE_PIPELINE_OPERATORS_MAX_COUNT];
};

struct pre_pipeline {
	struct pipeline pipeline;
	struct pipeline_src_layer src_layer;
};

struct post_online_pipeline {
	struct pipeline pipeline;
	struct post_online_info online_info;

};

struct post_offline_pipeline {
	struct pipeline pipeline;
	struct post_offline_info offline_info;
};

/*********************************** display frame data struct *********************/
struct ov_req_infos {
	/* struct wb info */
	struct disp_wb_layer wb_layers[2]; // MAX_DSS_DST_NUM
	struct disp_rect wb_ov_rect;
	uint32_t wb_layer_nums;
	uint32_t wb_type;

	/* struct disp_overlay_block */
	uint64_t ov_block_infos_ptr;
	uint32_t ov_block_nums;
	int32_t ovl_idx;
	uint32_t wb_enable;
	uint32_t frame_no;
};

struct hisi_display_frame {
	uint32_t pre_pipeline_count;
	struct pre_pipeline pre_pipelines[SRC_LAYERS_MAX_COUNT];

	uint32_t post_online_count;
	struct post_online_pipeline post_online_pipelines[POST_OV_ONLINE_PIPELINE_MAX_COUNT];

	uint32_t post_offline_count;
	struct post_offline_pipeline post_offline_pipelines[POST_OV_OFFLINE_PIPELINE_MAX_COUNT];

	struct ov_req_infos ov_req; // ori info from user
};

/*********************************** operator data struct *********************/
struct disp_csc_info {
	int chn_idx;
	uint32_t format;
	uint32_t csc_mode;
	bool need_yuv2p3;
};

#endif /* _HISI_DISP_H_ */
