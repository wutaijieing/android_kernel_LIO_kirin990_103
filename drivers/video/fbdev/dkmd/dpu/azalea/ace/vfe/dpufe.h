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

#ifndef DPUFE_H
#define DPUFE_H

#include <linux/types.h>
#include <linux/fb.h>
#include <linux/wait.h>

#include "dpufe_sysfs.h"
#include "dpufe_vsync.h"
#include "dpufe_vactive.h"
#include "../include/dpu_communi_def.h"
#include "dpufe_disp_recorder.h"

#define MAX_DSS_DST_NUM 2

struct dss_fence {
	int32_t release_fence;
	int32_t retire_fence;
};

struct alsc_reg_info {
	uint32_t alsc_en;
	uint32_t alsc_addr;
	uint32_t alsc_size;
	uint32_t pic_size;
};

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
	int reserved;

	struct alsc_reg_info alsc;
} lcd_dirty_region_info_t;

typedef struct dss_img {
	uint32_t format;
	uint32_t width;
	uint32_t height;
	uint32_t bpp; /* bytes per pixel */
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
	uint32_t arsr2p_src_x;
	uint32_t arsr2p_src_y;
	uint32_t arsr2p_dst_x;
	uint32_t arsr2p_dst_y;
	uint32_t arsr2p_dst_w;
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

typedef struct dss_overlay_block {
	dss_layer_t layer_infos[MAX_DSS_SRC_NUM];
	dss_rect_t ov_block_rect;
	uint32_t layer_nums;
	uint32_t reserved0;
} dss_overlay_block_t;

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

typedef struct dss_mmbuf {
	uint32_t addr;
	uint32_t size;
	int owner;
} dss_mmbuf_t;

typedef struct dss_overlay {
	dss_wb_layer_t wb_layer_infos[MAX_DSS_DST_NUM];
	dss_rect_t wb_ov_rect;
	uint32_t wb_layer_nums;
	uint32_t wb_compose_type;
	uint64_t ov_hihdr_infos_ptr;
	uint64_t ov_block_infos_ptr;
	uint32_t ov_block_nums;
	int32_t ovl_idx;
	uint32_t wb_enable;
	uint32_t frame_no;
	dss_rect_t dirty_rect;
	int spr_overlap_type;
	uint32_t frame_fps;
	struct dss_rect res_updt_rect;
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
	uint32_t rog_width;
	uint32_t rog_height;
	uint32_t is_evs_request;
} dss_overlay_t;

struct dpufe_buf_sync {
	char *fence_name;
	struct dpufe_timeline *timeline;
	struct dpufe_timeline *timeline_retire;

	int timeline_max;
	int threshold;
	int retire_threshold;
	int refresh;
	spinlock_t refresh_lock;

	struct workqueue_struct *free_layerbuf_queue;
	struct work_struct free_layerbuf_work;
	struct list_head layerbuf_list;
	bool layerbuf_flushed;
	spinlock_t layerbuf_spinlock;
	struct semaphore layerbuf_sem;
};

struct dpufe_data_type {
	uint32_t index;
	uint32_t frame_count;
	uint32_t fb_buf_num;
	uint32_t fb_img_type;
	uint32_t ref_cnt;
	uint32_t online_play_count;
	dss_overlay_t ov_req;
	dss_overlay_block_t ov_block_infos[DPU_OV_BLOCK_NUMS];
	dss_overlay_t ov_req_prev;
	dss_overlay_t ov_req_prev_prev;

	struct gen_pool *mmbuf_gen_pool;
	struct list_head *mmbuf_list;

	struct fb_info *fbi;
	struct platform_device *pdev;
	struct semaphore blank_sem; /* for blank */

	struct dpufe_vstart_mgr vstart_ctl;
	struct dpufe_vsync vsync_ctrl;
	struct dpufe_attr attrs;
	struct dpufe_buf_sync buf_sync_ctrl;

	struct sg_table *fb_sg_table;
	bool fb_mem_free_flag;

	wait_queue_head_t asynchronous_play_wq;
	bool evs_enable;
	bool panel_power_on;
	uint32_t asynchronous_play_flag;

	struct dpu_core_disp_data *disp_data;

	struct disp_state_recorder disp_recorder;

	void (*buf_sync_suspend)(struct dpufe_data_type *dfd);
	int (*sysfs_create_fnc)(struct device *dev, struct dpufe_attr *attrs);
	void (*sysfs_remove_fnc)(struct device *dev, struct dpufe_attr *attrs);
	void (*vsync_register)(struct dpufe_vsync *vsync_ctl, struct dpufe_attr *attrs);
	void (*vsync_unregister)(struct dpufe_vsync *vsync_ctl, struct dpufe_attr *attrs);
	void (*vstart_isr_register)(struct dpufe_vstart_mgr *vstart_ctl);
	void (*vstart_isr_unregister)(struct dpufe_vstart_mgr *vstart_ctl);
	void (*buf_sync_register)(struct dpufe_data_type *dfd);
	void (*buf_sync_unregister)(struct dpufe_data_type *dfd);
};

#endif
