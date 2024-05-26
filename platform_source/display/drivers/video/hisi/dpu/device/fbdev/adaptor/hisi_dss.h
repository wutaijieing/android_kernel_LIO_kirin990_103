/* include/linux/hisi_dss.h
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

#ifndef _HISI_DSS_H_
#define _HISI_DSS_H_

#include <linux/types.h>
#include <linux/fb.h>

#define HISIFB_IOCTL_MAGIC 'M'

#define FB_ACCEL_DPUV2xx 0x1
#define FB_ACCEL_DPUV10x 0x2
#define FB_ACCEL_DPUV20x 0x4
#define FB_ACCEL_DPUV30x 0x8
#define FB_ACCEL_DPUV40x 0x10  // not support for all above platforms
#define FB_ACCEL_KIRIN970 0x40
#define FB_ACCEL_DSSV320 0x80
#define FB_ACCEL_DSSV501 0x100
#define FB_ACCEL_DSSV510 0x200
#define FB_ACCEL_DSSV330 0x400
#define FB_ACCEL_DSSV600 0x800
#define FB_ACCEL_DSSV350 0x1000
#define FB_ACCEL_DSSV345 0x2000
#define FB_ACCEL_DSSV700 0x8000
#define FB_ACCEL_DSSV720 0x8002
#define FB_ACCEL_PLATFORM_TYPE_FPGA 0x10000000   //FPGA
#define FB_ACCEL_PLATFORM_TYPE_ASIC 0x20000000   //ASIC

#define HISIFB_LCD_DIRTY_REGION_INFO_GET _IOW(HISIFB_IOCTL_MAGIC, 801, struct lcd_dirty_region_info)
#define HISIFB_PLATFORM_TYPE_GET _IOW(HISIFB_IOCTL_MAGIC, 802, int)
#define HISIFB_MDC_CHANNEL_INFO_REQUEST _IOW(HISIFB_IOCTL_MAGIC, 803, struct mdc_ch_info)
#define HISIFB_MDC_CHANNEL_INFO_RELEASE _IOW(HISIFB_IOCTL_MAGIC, 804, struct mdc_ch_info)
#define HISIFB_MDC_POWER_DOWNUP_CTRL _IOW(HISIFB_IOCTL_MAGIC, 805, int)
#define HISIFB_GRALLOC_GET_PHYS _IOW(HISIFB_IOCTL_MAGIC, 806, struct ion_phys_data)
#define HISIFB_GRALLOC_MAP_IOVA _IOW(HISIFB_IOCTL_MAGIC, 807, struct iova_info)
#define HISIFB_GRALLOC_UNMAP_IOVA _IOW(HISIFB_IOCTL_MAGIC, 808, struct iova_info)

#define HISIFB_VSYNC_CTRL _IOW(HISIFB_IOCTL_MAGIC, 0x02, unsigned int)
#define HISIFB_DSS_VOTE_CMD_SET _IOW(HISIFB_IOCTL_MAGIC, 0x04, struct dss_vote_cmd)
#define HISIFB_DIRTY_UPDT_SUPPORT_GET _IOW(HISIFB_IOCTL_MAGIC, 0x06, unsigned int)
#define HISIFB_VIDEO_IDLE_CTRL _IOW(HISIFB_IOCTL_MAGIC, 0x07, int)
#define HISIFB_DSS_MMBUF_ALLOC _IOW(HISIFB_IOCTL_MAGIC, 0x08, struct dss_mmbuf)
#define HISIFB_DSS_MMBUF_FREE _IOW(HISIFB_IOCTL_MAGIC, 0x09, struct dss_mmbuf)
#define HISIFB_DSS_VOLTAGE_GET _IOW(HISIFB_IOCTL_MAGIC, 0x10, struct dss_vote_cmd)
#define HISIFB_DSS_VOLTAGE_SET _IOW(HISIFB_IOCTL_MAGIC, 0x11, struct dss_vote_cmd)
#define HISIFB_MMBUF_SIZE_QUERY _IOW(HISIFB_IOCTL_MAGIC, 0x12, struct dss_mmbuf)
#define HISIFB_PLATFORM_PRODUCT_INFO_GET _IOW(HISIFB_IOCTL_MAGIC, 0x13, struct platform_product_info)
#define HISIFB_DSS_MMBUF_FREE_ALL _IOW(HISIFB_IOCTL_MAGIC, 0x14, int)

#define HISIFB_ONLINE_PLAY_BYPASS _IOW(HISIFB_IOCTL_MAGIC, 0x20, int)
#define HISIFB_OV_ONLINE_PLAY _IOW(HISIFB_IOCTL_MAGIC, 0x21, struct dss_overlay)
#define HISIFB_OV_OFFLINE_PLAY _IOW(HISIFB_IOCTL_MAGIC, 0x22, struct dss_overlay)
#define HISIFB_OV_COPYBIT_PLAY _IOW(HISIFB_IOCTL_MAGIC, 0x23, struct dss_overlay)
#define HISIFB_OV_MEDIA_COMMON_PLAY _IOW(HISIFB_IOCTL_MAGIC, 0x24, struct dss_overlay)
#define HISIFB_DEBUG_CHECK_FENCE_TIMELINE _IOW(HISIFB_IOCTL_MAGIC, 0x30, int)
#define HISIFB_IDLE_IS_ALLOWED _IOW(HISIFB_IOCTL_MAGIC, 0x42, int)

#define HISIFB_CE_ENABLE _IOW(HISIFB_IOCTL_MAGIC, 0x49, int)
#define HISIFB_CE_SUPPORT_GET _IOW(HISIFB_IOCTL_MAGIC, 0x50, unsigned int)
#define HISIFB_CE_SERVICE_LIMIT_GET _IOW(HISIFB_IOCTL_MAGIC, 0x51, int)
#define HISIFB_CE_HIST_GET _IOW(HISIFB_IOCTL_MAGIC, 0x52, int)
#define HISIFB_CE_LUT_SET _IOW(HISIFB_IOCTL_MAGIC, 0x53, int)
#define HISIFB_CE_PARAM_GET _IOW(HISIFB_IOCTL_MAGIC, 0x54, int)
#define HISIFB_CE_PARAM_SET _IOW(HISIFB_IOCTL_MAGIC, 0x55, int)
#define HISIFB_GET_REG_VAL _IOW(HISIFB_IOCTL_MAGIC, 0x56, struct dss_reg)
#define HISIFB_HIACE_PARAM_GET _IOW(HISIFB_IOCTL_MAGIC, 0x57, struct dss_effect_info)
#define HISIFB_HIACE_HDR10_LUT_SET _IOW(HISIFB_IOCTL_MAGIC, 0x58, struct int)

// for hiace single mode
#define HISIFB_HIACE_SINGLE_MODE_TRIGGER _IOW(HISIFB_IOCTL_MAGIC, 0x59, struct dss_hiace_single_mode_ctrl_info)
#define HISIFB_HIACE_BLOCK_ONCE_SET _IOW(HISIFB_IOCTL_MAGIC, 0x5A, unsigned int)
#define HISIFB_HIACE_HIST_GET _IOW(HISIFB_IOCTL_MAGIC, 0x5B, int)
#define HISIFB_HIACE_FNA_DATA_GET _IOW(HISIFB_IOCTL_MAGIC, 0x5C, int)

#define HISIFB_EFFECT_MODULE_INIT _IOW(HISIFB_IOCTL_MAGIC, 0x60, struct dss_effect)
#define HISIFB_EFFECT_MODULE_DEINIT _IOW(HISIFB_IOCTL_MAGIC, 0x61, struct dss_effect)
#define HISIFB_EFFECT_INFO_GET _IOW(HISIFB_IOCTL_MAGIC, 0x62, struct dss_effect_info)
#define HISIFB_EFFECT_INFO_SET _IOW(HISIFB_IOCTL_MAGIC, 0x63, struct dss_effect_info)

#define HISIFB_DISPLAY_ENGINE_INIT _IOW(HISIFB_IOCTL_MAGIC, 0x70, struct display_engine)
#define HISIFB_DISPLAY_ENGINE_DEINIT _IOW(HISIFB_IOCTL_MAGIC, 0x71, struct display_engine)
#define HISIFB_DISPLAY_ENGINE_PARAM_GET _IOW(HISIFB_IOCTL_MAGIC, 0x72, struct display_engine_param)
#define HISIFB_DISPLAY_ENGINE_PARAM_SET _IOW(HISIFB_IOCTL_MAGIC, 0x73, struct display_engine_param)

#define HISIFB_DPTX_GET_COLOR_BIT_MODE _IOW(HISIFB_IOCTL_MAGIC, 0x80, int)
#define HISIFB_DPTX_GET_SOURCE_MODE _IOW(HISIFB_IOCTL_MAGIC, 0x81, int)
#define HISIFB_DPTX_SEND_HDR_METADATA _IOW(HISIFB_IOCTL_MAGIC, 0x82, struct hdr_metadata)

#define HISIFB_PANEL_REGION_NOTIFY _IOW(HISIFB_IOCTL_MAGIC, 0x90, struct _panel_region_notify)
#define HISIFB_GET_HIACE_ENABLE _IOW(HISIFB_IOCTL_MAGIC, 0x91, int)
#define HISIFB_HIACE_ROI_GET _IOW(HISIFB_IOCTL_MAGIC, 0x92, struct hiace_roi_info)

#define HISIFB_GET_RELEASE_AND_RETIRE_FENCE _IOW(HISIFB_IOCTL_MAGIC, 0xA0, struct dss_fence)
#define HISIFB_OV_ASYNC_PLAY _IOW(HISIFB_IOCTL_MAGIC, 0xA1, struct dss_overlay)

#ifndef BIT
#define BIT(x)	(1<<(x))
#endif

/* lcd fps scence */
#define LCD_FPS_SCENCE_NORMAL	(0)
#define LCD_FPS_SCENCE_IDLE 	BIT(0)
#define LCD_FPS_SCENCE_VIDEO	BIT(1)
#define LCD_FPS_SCENCE_GAME 	BIT(2)
#define LCD_FPS_SCENCE_WEB		BIT(3)
#define LCD_FPS_SCENCE_EBOOK	BIT(4)
#define LCD_FPS_SCENCE_FORCE_30FPS	  BIT(5)
#define LCD_FPS_SCENCE_FUNC_DEFAULT_ENABLE	  BIT(6)
#define LCD_FPS_SCENCE_FUNC_DEFAULT_DISABLE    BIT(7)

/* lcd fps value */
#define LCD_FPS_30 (30)
#define LCD_FPS_60 (60)

#define DSS_WCH_MAX  (2)

#define GTM_PQ2SLF_SIZE 8
#define GTM_LUT_PQ2SLF_SIZE 64
#define GTM_LUT_LUMA_SIZE 256
#define GTM_LUT_CHROMA_SIZE 256
#define GTM_LUT_CHROMA0_SIZE 256
#define GTM_LUT_CHROMA1_SIZE 256
#define GTM_LUT_CHROMA2_SIZE 256
#define GTM_LUT_LUMALUT0_SIZE 64
#define GTM_LUT_CHROMALUT0_SIZE 16

#define ITM_LUT_LUMA_SIZE 256
#define ITM_LUT_CHROMA_SIZE 256
#define ITM_LUT_CHROMA0_SIZE 256
#define ITM_LUT_CHROMA1_SIZE 256
#define ITM_LUT_CHROMA2_SIZE 256
#define ITM_LUT_LUMALUT0_SIZE 64
#define ITM_LUT_CHROMALUT0_SIZE 16

enum MMBUF_USED_SERVICES {
	SERVICE_MIN = 0x0,
	SERVICE_MDC = SERVICE_MIN,
	RESERVED_SERVICE_MAX,	// above:define mmbuf reserved service id. below:define normal service id
	SERVICE_HWC = RESERVED_SERVICE_MAX,
	SERVICE_MAX,
};

enum DFR_NOTIFY_TYPE {
	TYPE_NOTIFY_PERF_ONLY = 0,
	TYPE_NOTIFY_EFFECT_ONLY,
	TYPE_NOTIFY_PERF_EFFECT,
};

typedef enum {
	DEBUG_DIRTY_UPDT_DISABLE = BIT(0),
	LCD_CTRL_DIRTY_UPDT_DISABLE = BIT(1),
	COLOR_TEMPERATURE_DIRTY_UPDT_DISABLE = BIT(2),
	DISPLAY_EFFECT_DIRTY_UPDT_DISABLE = BIT(3),
	ESD_HAPPENED_DIRTY_UPDT_DISABLE = BIT(4),
	SECURE_ENABLED_DIRTY_UPDT_DISABLE = BIT(5),
	AOD_ENABLED_DIRTY_UPDT_DISABLE = BIT(6),
	VR_ENABLED_DIRTY_UPDT_DISABLE = BIT(7),
	ACE_ENABLED_DIRTY_UPDT_DISABLE = BIT(8),
	PIPE_ENABLED_DIRTY_UPDT_DISABLE = BIT(9),
	EXTERNAL_DISPLAY_DISABLE_PARTIAL_UPDATE = BIT(10),
	IOCTL_CALLED_PARTIAL_UPDATE_FAILED = BIT(11),
} dirty_updt_disable_event_t;

/* for YUV */
#define MAX_PLANES	(3)

enum dss_wb_compose_type {
	DSS_WB_COMPOSE_PRIMARY = 0,
	DSS_WB_COMPOSE_COPYBIT,
	DSS_WB_COMPOSE_MEDIACOMMON,
	DSS_WB_COMPOSE_TYPE_MAX,
};

enum hisi_fb_blending {
	HISI_FB_BLENDING_NONE = 0,
	HISI_FB_BLENDING_PREMULT = 1,
	HISI_FB_BLENDING_COVERAGE = 2,
	HISI_FB_BLENDING_MAX = 3,
};

enum dss_afbc_scramble_mode {
	DSS_AFBC_SCRAMBLE_NONE = 0,
	DSS_AFBC_SCRAMBLE_MODE1,
	DSS_AFBC_SCRAMBLE_MODE2,
	DSS_AFBC_SCRAMBLE_MODE3,
	DSS_AFBC_SCRAMBLE_MODE_MAX,
};

enum dss_hfbc_scramble_mode {
	DSS_HFBC_SCRAMBLE_NONE = 0,
	DSS_HFBC_SCRAMBLE_MODE1,
	DSS_HFBC_SCRAMBLE_MODE2,
	DSS_HFBC_SCRAMBLE_MODE3,
	DSS_HFBC_SCRAMBLE_MODE_MAX,
};

enum dss_chn_idx {
	DSS_RCHN_NONE = -1,
	DSS_RCHN_D2 = 0,
	DSS_RCHN_D3,
	DSS_RCHN_V0,
	DSS_RCHN_G0,
	DSS_RCHN_V1,
	DSS_RCHN_G1,
	DSS_RCHN_D0,
	DSS_RCHN_D1,

	DSS_WCHN_W0,
	DSS_WCHN_W1,

	DSS_CHN_MAX,

	DSS_RCHN_V2 = DSS_CHN_MAX,	//for copybit, only supported in chicago
	DSS_WCHN_W2,

	DSS_CHN_MAX_DEFINE,
};

enum dss_ovl_idx {
	DSS_OVL0 = 0,
	DSS_OVL1,
	DSS_OVL2,
	DSS_OVL3,
	DSS_OVL_IDX_MAX,
};

//this head file to save the structs that both ade and dss will use
//note: if the left_align is 8,right_align is 8,and w_min is larger than 802,then w_min should be set to 808,
//make sure that it is 8 align,if w_min is set to 802,there will be an error.left_align,right_align,top_align
//bottom_align,w_align,h_align,w_min and h_min's valid value should be larger than 0,top_start and bottom_start
//maybe equal to 0. if it's not surpport partial update, these value should set to invalid value(-1).
typedef struct lcd_dirty_region_info {
	int left_align;
	int right_align;
	int top_align;
	int bottom_align;

	int w_align;
	int h_align;
	int w_min;
	int h_min;

	int top_start;
	int bottom_start;
	int spr_overlap;
} lcd_dirty_region_info_t;

typedef struct dss_rect {
	int32_t x;
	int32_t y;
	int32_t w;
	int32_t h;
} dss_rect_t;

typedef struct dss_rect_ltrb {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
} dss_rect_ltrb_t;

typedef struct dss_mmbuf {
	uint32_t addr;
	uint32_t size;
	int owner;
} dss_mmbuf_t;

// typedef struct iova_info {
// 	uint64_t iova;
// 	uint64_t size;
// 	int share_fd;
// 	int calling_pid;
// } iova_info_t;

typedef struct dss_img {
	uint32_t format;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;	/* bytes per pixel */
	uint32_t buf_size;
	uint32_t stride;
	uint32_t stride_plane1;
	uint32_t stride_plane2;
	uint64_t phy_addr;
	uint64_t vir_addr;
	uint32_t offset_plane1;
	uint32_t offset_plane2;

	uint64_t afbc_header_addr;
	uint64_t afbc_header_offset;
	uint64_t afbc_payload_addr;
	uint64_t afbc_payload_offset;
	uint32_t afbc_header_stride;
	uint32_t afbc_payload_stride;
	uint32_t afbc_scramble_mode;
	uint32_t hfbcd_block_type;
	uint64_t hfbc_header_addr0;
	uint64_t hfbc_header_offset0;
	uint64_t hfbc_payload_addr0;
	uint64_t hfbc_payload_offset0;
	uint32_t hfbc_header_stride0;
	uint32_t hfbc_payload_stride0;
	uint64_t hfbc_header_addr1;
	uint64_t hfbc_header_offset1;
	uint64_t hfbc_payload_addr1;
	uint64_t hfbc_payload_offset1;
	uint32_t hfbc_header_stride1;
	uint32_t hfbc_payload_stride1;
	uint32_t hfbc_scramble_mode;
	uint32_t hfbc_mmbuf_base0_y8;
	uint32_t hfbc_mmbuf_size0_y8;
	uint32_t hfbc_mmbuf_base1_c8;
	uint32_t hfbc_mmbuf_size1_c8;
	uint32_t hfbc_mmbuf_base2_y2;
	uint32_t hfbc_mmbuf_size2_y2;
	uint32_t hfbc_mmbuf_base3_c2;
	uint32_t hfbc_mmbuf_size3_c2;

	uint32_t hebcd_block_type;

	uint64_t hebc_header_addr0;
	uint64_t hebc_header_offset0;
	uint64_t hebc_payload_addr0;
	uint64_t hebc_payload_offset0;
	uint32_t hebc_header_stride0;
	uint32_t hebc_payload_stride0;

	uint64_t hebc_header_addr1;
	uint64_t hebc_header_offset1;
	uint64_t hebc_payload_addr1;
	uint64_t hebc_payload_offset1;
	uint32_t hebc_header_stride1;
	uint32_t hebc_payload_stride1;

	uint32_t hebc_scramble_mode;
	uint32_t hebc_mmbuf_base0_y8;
	uint32_t hebc_mmbuf_size0_y8;
	uint32_t hebc_mmbuf_base1_c8;
	uint32_t hebc_mmbuf_size1_c8;

	uint32_t mmbuf_base;
	uint32_t mmbuf_size;

	uint32_t mmu_enable;
	uint32_t csc_mode;
	uint32_t secure_mode;
	int32_t shared_fd;
	uint32_t display_id;
} dss_img_t;

typedef struct dss_block_info {
	int32_t first_tile;
	int32_t last_tile;
	uint32_t acc_hscl;
	uint32_t h_ratio;
	uint32_t v_ratio;
	uint32_t h_ratio_arsr2p;
	uint32_t arsr2p_left_clip;
	uint32_t both_vscfh_arsr2p_used;
	dss_rect_t arsr2p_in_rect;
	uint32_t arsr2p_src_x; //new added
	uint32_t arsr2p_src_y; //new added
	uint32_t arsr2p_dst_x; //new added
	uint32_t arsr2p_dst_y; //new added
	uint32_t arsr2p_dst_w; //new added
	int32_t h_v_order;
} dss_block_info_t;

typedef struct dss_layer {
	dss_img_t img;
	dss_rect_t src_rect;
	dss_rect_t src_rect_mask;
	dss_rect_t dst_rect;
	uint32_t transform;
	int32_t blending;
	uint32_t glb_alpha;
	uint32_t color; /* background color or dim color */
	int32_t layer_idx;
	int32_t chn_idx;
	uint32_t need_cap;
	int32_t acquire_fence;

	dss_block_info_t block_info;

	uint32_t is_cld_layer;
	uint32_t cld_width;
	uint32_t cld_height;
	uint32_t cld_data_offset;
	int32_t axi_idx;
	int32_t mmaxi_idx;
} dss_layer_t;

typedef struct dss_wb_block_info {
	uint32_t acc_hscl;
	uint32_t h_ratio;
	int32_t h_v_order;
	dss_rect_t src_rect;
	dss_rect_t dst_rect;
} dss_wb_block_info_t;

typedef struct dss_wb_layer {
	dss_img_t dst;
	dss_rect_t src_rect;
	dss_rect_t dst_rect;
	uint32_t transform;
	int32_t chn_idx;
	uint32_t need_cap;

	int32_t acquire_fence;
	int32_t release_fence;
	int32_t retire_fence;

	dss_wb_block_info_t wb_block_info;
} dss_wb_layer_t;


/*
** dss error status
*/
#define DSS_PDP_LDI_UNDERFLOW		BIT(0)
#define DSS_SDP_LDI_UNDERFLOW		BIT(1)
#define DSS_PDP_SMMU_ERR			BIT(2)
#define DSS_SDP_SMMU_ERR			BIT(3)

/*
** crc enable status
*/
enum dss_crc_enable_status {
	DSS_CRC_NONE = 0,
	DSS_CRC_OV_EN = 1,
	DSS_CRC_LDI_EN,
	DSS_CRC_SUM_EN,
};

/*
** sec enable status
*/
enum dss_sec_enable_status {
	DSS_SEC_STOP = 0,
	DSS_SEC_RUN = 1,
};

typedef struct dss_crc_info {
	uint32_t crc_ov_result;
	uint32_t crc_ldi_result;
	uint32_t crc_sum_result;
	uint32_t crc_ov_frm;
	uint32_t crc_ldi_frm;
	uint32_t crc_sum_frm;

	uint32_t err_status;
	uint32_t reserved0;
} dss_crc_info_t;

enum dss_to_be_continued_type {
	DSS_LAYER_SERIAL_COMPOSE = 0,
	DSS_LAYER_PARALLEL_COMPOSE = 1,
};

/* Max multi-src channel number of the DSS. */
#define MAX_DSS_SRC_NUM (9)

#define MAX_DSS_DST_NUM (2)

#define HISI_DSS_OV_BLOCK_NUMS	(23)

/*********************** MDC START *****************************/
enum {
	MDC_POWER_INVALD_CMD = 0x0,
	MDC_POWER_UP_REQUEST = 0x1,
	MDC_POWER_DOWN_REQUEST = 0x2,
};

enum {
	HWC_REQUEST = 0x0,
	MDC_REQUEST = 0x1,
};

enum {
	SPR_OVERLAP_NONE = 0x0,
	SPR_OVERLAP_TOP = 0x1,
	SPR_OVERLAP_BOTTOM = 0x2,
	SPR_OVERLAP_TOP_BOTTOM = 0x3,
};

typedef struct mdc_ch_info {
	uint32_t hold_flag;
	uint32_t is_drm;
	uint32_t rch_need_cap;
	uint32_t wch_need_cap;
	int32_t rch_idx;
	int32_t wch_idx;
	int32_t ovl_idx;
	uint32_t wb_composer_type;

	uint32_t mmbuf_addr;
	uint32_t mmbuf_size;
	uint32_t need_request_again;
} mdc_ch_info_t;
/*********************** MDC END *****************************/

typedef struct dss_overlay_block {
	dss_layer_t layer_infos[MAX_DSS_SRC_NUM];
	dss_rect_t ov_block_rect;
	uint32_t layer_nums;
	uint32_t reserved0;
} dss_overlay_block_t;

typedef struct dss_overlay {
	dss_wb_layer_t wb_layer_infos[MAX_DSS_DST_NUM];
	dss_rect_t wb_ov_rect;
	uint32_t wb_layer_nums;
	uint32_t wb_compose_type;

	/* hihdr */
	uint64_t ov_hihdr_infos_ptr;

	// struct dss_overlay_block
	uint64_t ov_block_infos_ptr;
	uint32_t ov_block_nums;
	int32_t ovl_idx;
	uint32_t wb_enable;
	uint32_t frame_no;

	// dirty region
	dss_rect_t dirty_rect;
	int spr_overlap_type;
	uint32_t frameFps;

	// resolution change
	struct dss_rect res_updt_rect;

	// crc
	dss_crc_info_t crc_info;
	int32_t crc_enable_status;
	uint32_t sec_enable_status;

	uint32_t to_be_continued;
	int32_t release_fence;
	int32_t retire_fence;
	uint8_t video_idle_status;
	uint8_t mask_layer_exist;
	uint8_t reserved_0;
	uint8_t reserved_1;

	uint32_t online_wait_timediff;

	bool hiace_roi_support;
	bool hiace_roi_enable;
	dss_rect_t hiace_roi_rect;

	// rog scale size, for scale ratio calculating
	uint32_t rog_width;
	uint32_t rog_height;

	uint32_t is_evs_request;
} dss_overlay_t;

typedef struct dpu_source_layers {
	struct dss_overlay ov_frame;
	struct dss_overlay_block *ov_blocks;
} dpu_source_layers_t;

struct dss_fence {
	int32_t release_fence;
	int32_t retire_fence;
};

typedef struct dss_vote_cmd {
	uint64_t dss_pri_clk_rate;
	uint64_t dss_axi_clk_rate;
	uint64_t dss_pclk_dss_rate;
	uint64_t dss_pclk_pctrl_rate;
	uint64_t dss_mmbuf_rate;
	uint32_t dss_voltage_level;
	uint32_t reserved;
} dss_vote_cmd_t;

typedef struct ce_algorithm_parameter {
	int iDiffMaxTH;
	int iDiffMinTH;
	int iAlphaMinTH;
	int iFlatDiffTH;
	int iBinDiffMaxTH;

	int iDarkPixelMinTH;
	int iDarkPixelMaxTH;
	int iDarkAvePixelMinTH;
	int iDarkAvePixelMaxTH;
	int iWhitePixelTH;
	int fweight;
	int fDarkRatio;
	int fWhiteRatio;

	int iDarkPixelTH;
	int fDarkSlopeMinTH;
	int fDarkSlopeMaxTH;
	int fDarkRatioMinTH;
	int fDarkRatioMaxTH;

	int iBrightPixelTH;
	int fBrightSlopeMinTH;
	int fBrightSlopeMaxTH;
	int fBrightRatioMinTH;
	int fBrightRatioMaxTH;

	int iZeroPos0MaxTH;
	int iZeroPos1MaxTH;

	int iDarkFMaxTH;
	int iDarkFMinTH;
	int iPos0MaxTH;
	int iPos0MinTH;

	int fKeepRatio;
} ce_algorithm_parameter_t;

typedef struct ce_parameter {
	int width;
	int height;
	int hist_mode;
	int mode;
	int result;
	uint32_t reserved0;
	uint32_t *histogram;
	uint8_t *lut_table;
	void *service;
	ce_algorithm_parameter_t ce_alg_param;
} ce_parameter_t;

#define META_DATA_SIZE 1024
#define LCD_PANEL_VERSION_SIZE 32
#define SN_CODE_LENGTH_MAX 54

//HIACE struct
typedef struct hiace_alg_parameter {
	//paramters to avoid interference of black/white edge
	int iGlobalHistBlackPos;
	int iGlobalHistWhitePos;
	int iGlobalHistBlackWeight;
	int iGlobalHistWhiteWeight;
	int iGlobalHistZeroCutRatio;
	int iGlobalHistSlopeCutRatio;

	//Photo metadata
	char Classifieresult[META_DATA_SIZE];
	int iResultLen;

	//function enable/disable switch
	int iDoLCE;
	int iDoSRE;
	int iDoAPLC;

	// minimum ambient light to enable SRE
	int iLaSensorSREOnTH;

	int iWidth;
	int iHeight;
	int bitWidth;
	int iMode; // Image(1) or Video(0) mode
	int iLevel; // Video(0), gallery(1) ...
	int ilhist_sft;

	int iMaxLcdLuminance;
	int iMinLcdLuminance;
	int iMaxBackLight;
	int iMinBackLight;
	int iAmbientLight;
	int iBackLight;
	long lTimestamp; // Timestamp of frame in millisecond

	// path of xml file
	char chCfgName[512];
} hiace_alg_parameter_t;

typedef struct hiace_interface_set {
	int thminv;
	unsigned int *lut;
} hiace_interface_set_t;

typedef struct hiace_HDR10_lut_set {
	unsigned int *detail_weight;
	unsigned int *LogLumEOTFLUT;
	unsigned int *LumEOTFGammaLUT;
} hiace_HDR10_lut_set_t;


enum display_engine_module_id {
	DISPLAY_ENGINE_BLC = BIT(0),
	DISPLAY_ENGINE_DDIC_CABC = BIT(1),
	DISPLAY_ENGINE_DDIC_COLOR = BIT(2),
	DISPLAY_ENGINE_PANEL_INFO = BIT(3),
	DISPLAY_ENGINE_DDIC_RGBW = BIT(4),
	DISPLAY_ENGINE_HBM = BIT(5),
	DISPLAY_ENGINE_COLOR_RECTIFY = BIT(6),
	DISPLAY_ENGINE_AMOLED_ALGO = BIT(7),
	DISPLAY_ENGINE_FLICKER_DETECTOR = BIT(8),
	DISPLAY_ENGINE_SHAREMEM = BIT(9),
	DISPLAY_ENGINE_MANUFACTURE_BRIGHTNESS = BIT(10),
	DISPLAY_ENGINE_FOLDABLE_INFO = BIT(11),
	DISPLAY_ENGINE_UD_FINGERPRINT_BACKLIGHT = BIT(12),
	DISPLAY_ENGINE_DEMURA = BIT(13),
	DISPLAY_ENGINE_IRDROP = BIT(14),
	DISPLAY_ENGINE_AMOLED_SYNC = BIT(15),
};

typedef struct display_engine_hbm_param {
	uint8_t dimming;
	uint32_t level;
	uint8_t mmi_flag;
} display_engine_hbm_param_t;

typedef struct display_engine_amoled_param {
	int HBMEnable;
	int amoled_diming_enable;
	int HBM_Threshold_BackLight;
	int HBM_Threshold_Dimming;
	int HBM_Dimming_Frames;
	int HBM_Min_BackLight;
	int HBM_Max_BackLight;
	int HBM_MinLum_Regvalue;
	int HBM_MaxLum_Regvalue;
	int Hiac_DBVThres;
	int Hiac_DBV_XCCThres;
	int Hiac_DBV_XCC_MinThres;
	int Lowac_DBVThres;
	int Lowac_DBV_XCCThres;
	int Lowac_DBV_XCC_MinThres;
	int Lowac_Fixed_DBVThres;
	int dc_brightness_dimming_enable;
	int dc_brightness_dimming_enable_real;
	int dc_lowac_dbv_thre;
	int dc_lowac_fixed_dbv_thres;
	int dc_backlight_delay_us;
	int amoled_enable_from_hal;
	int dc_lowac_dbv_thres_low;
} display_engine_amoled_param_t;

typedef struct display_engine_amoled_param_sync {
	// point backlight map from level(0~10000) to dbv level
	uint16_t *high_dbv_map;
	uint32_t map_size;
} display_engine_amoled_param_sync_t;

typedef struct display_engine_blc_param {
	uint32_t enable;
	int32_t delta;
} display_engine_blc_param_t;

typedef struct display_engine_ddic_cabc_param {
	uint32_t ddic_cabc_mode;
} display_engine_ddic_cabc_param_t;

typedef struct display_engine_ddic_color_param {
	uint32_t ddic_color_mode;
} display_engine_ddic_color_param_t;

typedef struct display_engine_ddic_rgbw_param {
	int ddic_panel_id;
	int ddic_rgbw_mode;
	int ddic_rgbw_backlight;
	int rgbw_saturation_control;
	int frame_gain_limit;
	int frame_gain_speed;
	int color_distortion_allowance;
	int pixel_gain_limit;
	int pixel_gain_speed;
	int pwm_duty_gain;
} display_engine_ddic_rgbw_param_t;

typedef struct display_engine_panel_info_param {
	int width;
	int height;
	int maxluminance;
	int minluminance;
	int maxbacklight;
	int minbacklight;
	int factory_gamma_enable;
	uint16_t factory_gamma[800];//800 > 257 * 3
	char lcd_panel_version[LCD_PANEL_VERSION_SIZE];
	uint32_t sn_code_length;
	uint8_t sn_code[SN_CODE_LENGTH_MAX];
	int factory_runmode;
	int reserve0;
	int reserve1;
	int reserve2;
	int reserve3;
	int reserve4;
	int reserve5;
	int reserve6;
	int reserve7;
	int reserve8;
	int reserve9;
} display_engine_panel_info_param_t;

enum display_engine_foldable_panel_id {
	DISPLAY_ENGINE_FOLDABLE_PANEL_PRIMARY = 0,
	DISPLAY_ENGINE_FOLDABLE_PANEL_SLAVE,
	DISPLAY_ENGINE_FOLDABLE_PANEL_FOLDING,
	DISPLAY_ENGINE_FOLDABLE_PANEL_NUM,
};

typedef struct display_engine_foldable_info {
	uint32_t dbv_acc[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
	uint32_t screen_on_duration[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
	uint32_t screen_on_duration_with_hiace_enable[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
	uint32_t fold_num_acc;
	uint32_t hbm_acc[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
	uint32_t hbm_duration[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
} display_engine_foldable_info_t;

typedef struct display_engine_demura {
	uint8_t *lut;
	uint32_t size;
	bool flash;
} display_engine_demura_t;

typedef struct display_engine_irdrop {
	uint32_t *params;
	uint32_t count;
} display_engine_irdrop_t;

struct disp_panelid
{
    uint32_t modulesn;
    uint32_t equipid;
    uint32_t modulemanufactdate;
    uint32_t vendorid;
};

struct disp_coloruniformparams
{
    uint32_t c_lmt[3];
    uint32_t mxcc_matrix[3][3];
    uint32_t white_decay_luminace;
};

struct disp_colormeasuredata
{
    uint32_t chroma_coordinates[4][2];
    uint32_t white_luminance;
};

struct disp_lcdbrightnesscoloroeminfo
{
    uint32_t id_flag;
    uint32_t tc_flag;
    struct disp_panelid  panel_id;
    struct disp_coloruniformparams color_params;
    struct disp_colormeasuredata color_mdata;
};

struct hdr_metadata {
	uint32_t electro_optical_transfer_function;
	uint32_t static_metadata_descriptor_id;
	uint32_t red_primary_x;
	uint32_t red_primary_y;
	uint32_t green_primary_x;
	uint32_t green_primary_y;
	uint32_t blue_primary_x;
	uint32_t blue_primary_y;
	uint32_t white_point_x;
	uint32_t white_point_y;
	uint32_t max_display_mastering_luminance;
	uint32_t min_display_mastering_luminance;
	uint32_t max_content_light_level;
	uint32_t max_frame_average_light_level;
};

typedef struct display_engine_color_rectify_param {
	struct disp_lcdbrightnesscoloroeminfo lcd_color_oeminfo;
}display_engine_color_rectify_param_t;

typedef struct display_engine {
	uint8_t blc_support;
	uint8_t ddic_cabc_support;
	uint8_t ddic_color_support;
	uint8_t ddic_rgbw_support;
	uint8_t ddic_hbm_support;
	uint8_t ddic_color_rectify_support;
} display_engine_t;

typedef struct display_engine_flicker_detector_config {
	uint8_t detect_enable;
	uint8_t dump_enable;
	int32_t time_window_length_ms;
	int32_t weber_threshold;
	int32_t low_level_upper_bl_value_threshold;
	int32_t low_level_device_bl_delta_threshold;
} display_engine_flicker_detector_config_t;


#define SHARE_MEMORY_SIZE (128 * 128 * 4 * 2)  //w * h * bpp * buf_num

typedef struct display_engine_share_memory {
	uint64_t addr_virt;
	uint64_t addr_phy;
} display_engine_share_memory_t;

typedef struct display_engine_manufacture_brightness {
	uint32_t engine_mode;
} display_engine_manufacture_brightness_t;

typedef struct display_engine_sync_ud_fingerprint_backlight {
	uint32_t scene_info;
	uint32_t hbm_level;
	uint32_t current_level;
} display_engine_sync_ud_fingerprint_backlight_t;

typedef struct display_engine_param {
	uint32_t modules;
	display_engine_blc_param_t blc;
	display_engine_ddic_cabc_param_t ddic_cabc;
	display_engine_ddic_color_param_t ddic_color;
	display_engine_ddic_rgbw_param_t ddic_rgbw;
	display_engine_panel_info_param_t panel_info;
	display_engine_hbm_param_t hbm;
	display_engine_color_rectify_param_t color_param;
	display_engine_amoled_param_t amoled_param;
	display_engine_flicker_detector_config_t flicker_detector_config;
	display_engine_share_memory_t share_mem;
	display_engine_manufacture_brightness_t manufacture_brightness;
	display_engine_foldable_info_t foldable_info;
	display_engine_sync_ud_fingerprint_backlight_t ud_fingerprint_backlight;
	display_engine_demura_t demura;
	display_engine_irdrop_t irdrop;
	display_engine_amoled_param_sync_t amoled_param_sync;
} display_engine_param_t;

typedef enum dss_module_id {
	DSS_EFFECT_MODULE_ARSR2P   = BIT(0),
	DSS_EFFECT_MODULE_ARSR1P   = BIT(1),
	DSS_EFFECT_MODULE_ACM      = BIT(2),
	DSS_EFFECT_MODULE_HIACE    = BIT(4),
	DSS_EFFECT_MODULE_LCP_GMP  = BIT(5),
	DSS_EFFECT_MODULE_LCP_IGM  = BIT(6),
	DSS_EFFECT_MODULE_LCP_XCC  = BIT(7),
	DSS_EFFECT_MODULE_GAMMA    = BIT(8),
	DSS_EFFECT_MODULE_DITHER   = BIT(9),
	DSS_EFFECT_MODULE_ACE      = BIT(10),
	DSS_EFFECT_MODULE_DPPROI   = BIT(11),
	DSS_EFFECT_MODULE_POST_XCC = BIT(12),
	DSS_EFFECT_MODULE_MAX      = BIT(13),
} dss_module_id;


typedef enum {
	EN_DISPLAY_REGION_NONE = 0,
	EN_DISPLAY_REGION_A = 0x1,
	EN_DISPLAY_REGION_B = 0x2,
	EN_DISPLAY_REGION_AB = 0x3,
	EN_DISPLAY_REGION_AB_FOLDED = 0x7,
} ENUM_EN_DISPLAY_REGION;

typedef enum {
	EN_MODE_PRE_NOTIFY = 0x1,                   //the notify before state change
	EN_MODE_REAL_SWITCH_NOTIFY = 0x2,           //the notify when state already change
	EN_MODE_POWER_OFF_SWITCH_NOTIFY = 0x3,      //the notify when LCD power off
} ENUM_EN_NOTIFY_MODE;

typedef struct _panel_region_notify {
	ENUM_EN_NOTIFY_MODE notify_mode;
	ENUM_EN_DISPLAY_REGION panel_display_region;
} panel_region_notify_t;

/* for hiace single mode */
enum {
	EN_HIACE_INFO_TYPE_GLOBAL_HIST = 0x1,    // bit0
	EN_HIACE_INFO_TYPE_LOCAL_HIST = 0x2,     // bit1
	EN_HIACE_INFO_TYPE_HIST = 0x3,           // bit0 + bit1
	EN_HIACE_INFO_TYPE_FNA = 0x4,            // bit2
};

struct dss_hiace_single_mode_ctrl_info {
	uint32_t info_type;      // global hist, local hist, or fna
	uint32_t blocking_mode;  // 0:asynchronous, 1:synchronize;
	uint32_t isr_handle;     // if get by isr routine directly
};

/* IFBC compress mode */
enum ifbc_mode {
	IFBC_MODE_NONE = 0,
	IFBC_MODE_ORISE2X,
	IFBC_MODE_ORISE3X,
	IFBC_MODE_HIMAX2X,
	IFBC_MODE_RSP2X,
	IFBC_MODE_RSP3X,
	IFBC_MODE_VESA2X_SINGLE,
	IFBC_MODE_VESA3X_SINGLE,
	IFBC_MODE_VESA2X_DUAL,
	IFBC_MODE_VESA3X_DUAL,
	IFBC_MODE_VESA3_75X_DUAL,
	IFBC_MODE_MAX,
};

struct platform_product_info {
	uint32_t max_hwc_mmbuf_size;
	uint32_t max_mdc_mmbuf_size;
	uint32_t fold_display_support;
	uint32_t dfr_support;
	uint32_t dummy_pixel_num;
	uint32_t ifbc_type;
};

struct hiace_roi_info {
	uint32_t roi_top;
	uint32_t roi_left;
	uint32_t roi_bot;
	uint32_t roi_right;
	uint32_t roi_enable;
};

inline static uint16_t get_adaptive_framerate(uint8_t low, uint8_t high)
{
	return (high << 8) | low;
}
#endif /*_HISI_DSS_H_*/
