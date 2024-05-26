/* Copyright (c) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#ifndef DISPLAY_EFFECT_ALSC_H
#define DISPLAY_EFFECT_ALSC_H

#include "display_effect_alsc_interface.h"

#define ALSC_MAX_DATA_LEN 3

/* ALSC clock frequency 1.92M = 192M / 100 */
#define ALSC_CLK_FREQ 192
#define ALSC_CLK_FREQ_DIVISOR 100

#define ALSC_PICTURE_SIZE(xres, yres) ((((xres) & ALSC_PIC_HEIGHT_MASK) << ALSC_PIC_WIDTH_SHIFT) | ((yres) & ALSC_PIC_HEIGHT_MASK))

enum alsc_status {
	ALSC_UNINIT,
	ALSC_WORKING,
	ALSC_BLANK,
	ALSC_STATUS_MAX_NUM
};

struct dss_alsc_cb_func {
	void (*send_data_func)(struct alsc_noise*);
	void (*send_ddic_alpha)(uint32_t);
};

struct dss_alsc {
	uint32_t status;
	uint32_t action;
	uint32_t degamma_en;
	uint32_t degamma_lut_sel;
	uint32_t alsc_en;
	uint32_t alsc_en_by_dirty_region_limit;
	uint32_t sensor_channel;
	uint32_t pic_size;
	uint32_t addr;
	uint32_t addr_default;
	uint32_t size;
	uint32_t size_default;
	struct alsc_bl_param bl_param;
	uint32_t bl_update_to_hwc;
	uint32_t addr_block[ALSC_BLOCK_LEN];   // ADDR_BLOCK address range 0x1A00-0x1A74, which contains 30 blocks.
	uint32_t ave_block[ALSC_BLOCK_LEN];    // AVE_BLOCK address range 0x1A80-0x1AF4, which contains 30 blocks.
	uint32_t coef_block_r[ALSC_BLOCK_LEN]; // COEF_BLOCK_R address range 0x1B00-0x1B74, which contains 30 blocks.
	uint32_t coef_block_g[ALSC_BLOCK_LEN]; // COEF_BLOCK_G address range 0x1B80-0x1BF4, which contains 30 blocks.
	uint32_t coef_block_b[ALSC_BLOCK_LEN]; // COEF_BLOCK_B address range 0x1C00-0x1C74, which contains 30 blocks.
	uint32_t coef_block_c[ALSC_BLOCK_LEN]; // COEF_BLOCK_C address range 0x1C80-0x1CF4, which contains 30 blocks.
	uint32_t degamma_r_coef[ALSC_DEGAMMA_COEF_LEN]; // DEGAMMA_R_COEF address range 0x5000-0x51FC, which contains 129blocks.
	uint32_t degamma_g_coef[ALSC_DEGAMMA_COEF_LEN]; // DEGAMMA_G_COEF address range 0x5400-0x55FC, which contains 129blocks.
	uint32_t degamma_b_coef[ALSC_DEGAMMA_COEF_LEN]; // DEGAMMA_B_COEF address range 0x5800-0x59FC, which contains 129blocks.

	struct mutex alsc_lock;

	struct dss_alsc_data* data_head;
	struct dss_alsc_data* data_tail;

	struct dss_alsc_cb_func cb_func;
};

struct dss_alsc_data {
	uint32_t status;
	uint32_t noise1;
	uint32_t noise2;
	uint32_t noise3;
	uint32_t noise4;
	uint32_t timestamp_l;
	uint32_t timestamp_h;
	uint64_t ktimestamp;

	struct dss_alsc_data* prev;
	struct dss_alsc_data* next;
};

// Define a rectangle for convenience
struct rect {
	uint32_t x1;
	uint32_t y1;
	uint32_t x2;
	uint32_t y2;
};

/* dss internal interface */
void dpu_alsc_update_dirty_region_limit(struct dpu_fb_data_type *dpufd);
void dpu_alsc_store_data(struct dpu_fb_data_type *dpufd, uint32_t isr_flag);

void dpu_alsc_init(struct dpu_fb_data_type *dpufd);
void dpu_alsc_deinit(struct dpu_fb_data_type *dpufd);
void dpu_alsc_set_reg(struct dpu_fb_data_type *dpufd);
void dpu_alsc_send_data_work_func(struct work_struct *work);
void dpu_alsc_blank(struct dpu_fb_data_type *dpufd, int blank_mode);

#endif
