/**
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
 */

#ifndef DPU_EFFECT_ALSC_H
#define DPU_EFFECT_ALSC_H

#include <linux/mutex.h>
#include "dkmd_alsc_interface.h"

#define ALSC_MAX_DATA_LEN 3

#define ALSC_PIC_WIDTH_SHIFT 13
#define ALSC_PIC_HEIGHT_MASK ((1UL << ALSC_PIC_WIDTH_SHIFT) - 1)

#define ALSC_ADDR_Y_SHIFT 13
#define ALSC_ADDR_X_MASK ((1UL << ALSC_ADDR_Y_SHIFT) - 1)

#define ALSC_SIZE_Y_SHIFT 8
#define ALSC_SIZE_X_MASK ((1UL << ALSC_SIZE_Y_SHIFT) - 1)

#define ALSC_DEGAMMA_VAL_MASK ((1UL << 12) - 1)

#define ALSC_DEGAMMA_ODD_COEF_START_BIT 16

#define alsc_picture_size(xres, yres) \
	((((xres) & ALSC_PIC_HEIGHT_MASK) << ALSC_PIC_WIDTH_SHIFT) | ((yres) & ALSC_PIC_HEIGHT_MASK))

enum alsc_status {
	ALSC_UNINIT,
	ALSC_WORKING,
	ALSC_IDLE,
	ALSC_NO_DATA,
	ALSC_STATUS_MAX
};

struct dpu_alsc_cb_func {
	void (*send_data_func)(struct alsc_noise *);
	void (*send_ddic_alpha)(uint32_t);
};

struct dpu_alsc {
	struct dkmd_alsc dkmd_alsc;
	bool is_inited;
	uint32_t status;
	uint32_t action;
	uint32_t degamma_en;
	uint32_t degamma_lut_sel;
	uint32_t alsc_en_by_dirty_region_limit;
	uint32_t pic_size;
	uint32_t addr_default;
	uint32_t size_default;
	uint32_t bl_update_to_hwc;

	struct mutex alsc_lock;
	char __iomem *alsc_base;

	struct dpu_alsc_data *data_head;
	struct dpu_alsc_data *data_tail;

	struct dpu_alsc_cb_func cb_func;
};

struct dpu_alsc_data {
	struct alsc_noise noise;
	uint64_t ktimestamp;

	struct dpu_alsc_data *prev;
	struct dpu_alsc_data *next;
};

struct opr_cmd_data_base;

/* dpu internal interface */
void dpu_alsc_store_data(uint32_t isr_flag);

void dpu_alsc_init(struct dpu_alsc *alsc);
void dpu_alsc_deinit(struct dpu_alsc *alsc);
void dpu_alsc_set_reg(const struct opr_cmd_data_base *data);

#endif