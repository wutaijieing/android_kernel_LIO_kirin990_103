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

#ifndef __DSC_OUTPUT_CALC_H__
#define __DSC_OUTPUT_CALC_H__

#include "dsc_algorithm.h"
#include <linux/types.h>

enum IFBC_TYPE {
	IFBC_TYPE_NONE = 0,
	IFBC_TYPE_VESA2X_SINGLE,
	IFBC_TYPE_VESA3X_SINGLE,
	IFBC_TYPE_VESA2X_DUAL,
	IFBC_TYPE_VESA3X_DUAL,
	IFBC_TYPE_VESA3_75X_DUAL,
	IFBC_TYPE_VESA4X_SINGLE_SPR,
	IFBC_TYPE_VESA4X_DUAL_SPR,
	IFBC_TYPE_VESA2X_SINGLE_SPR,
	IFBC_TYPE_VESA2X_DUAL_SPR,
	IFBC_TYPE_VESA3_75X_SINGLE,
	IFBC_TYPE_MAX
};

/* IFBC compress mode */
enum IFBC_COMP_MODE {
	IFBC_COMP_MODE_0 = 0,
	IFBC_COMP_MODE_1,
	IFBC_COMP_MODE_2,
	IFBC_COMP_MODE_3,
	IFBC_COMP_MODE_4,
	IFBC_COMP_MODE_5,
	IFBC_COMP_MODE_6,
};

/* xres_div */
enum XRES_DIV {
	XRES_DIV_1 = 1,
	XRES_DIV_2,
	XRES_DIV_3,
	XRES_DIV_4,
	XRES_DIV_5,
	XRES_DIV_6,
};

/* yres_div */
enum YRES_DIV {
	YRES_DIV_1 = 1,
	YRES_DIV_2,
	YRES_DIV_3,
	YRES_DIV_4,
	YRES_DIV_5,
	YRES_DIV_6,
};

/* pxl0_divxcfg */
enum PXL0_DIVCFG {
	PXL0_DIVCFG_0 = 0,
	PXL0_DIVCFG_1,
	PXL0_DIVCFG_2,
	PXL0_DIVCFG_3,
	PXL0_DIVCFG_4,
	PXL0_DIVCFG_5,
	PXL0_DIVCFG_6,
	PXL0_DIVCFG_7,
};

/* pxl0_div2_gt_en */
enum PXL0_DIV2_GT_EN {
	PXL0_DIV2_GT_EN_CLOSE = 0,
	PXL0_DIV2_GT_EN_OPEN,
};

/* pxl0_div4_gt_en */
enum PXL0_DIV4_GT_EN {
	PXL0_DIV4_GT_EN_CLOSE = 0,
	PXL0_DIV4_GT_EN_OPEN,
};

/* used for 8bit 10bit switch */
enum PANEL_MODE {
	MODE_8BIT = 0,
	MODE_10BIT_VIDEO_3X,
};

/* pxl0_dsi_gt_en */
enum PXL0_DSI_GT_EN {
	PXL0_DSI_GT_EN_0 = 0,
	PXL0_DSI_GT_EN_1,
	PXL0_DSI_GT_EN_2,
	PXL0_DSI_GT_EN_3,
};

struct dsc_calc_info {
	enum pixel_format format;
	uint16_t dsc_version;
	uint16_t native_422;
	uint32_t idata_422;
	uint32_t convert_rgb;
	uint32_t adjustment_bits;
	uint32_t adj_bits_per_grp;
	uint32_t bits_per_grp;
	uint32_t slices_per_line;
	uint32_t pic_line_grp_num;
	uint32_t dsc_insert_byte_num;
	uint32_t dual_dsc_en;
	uint32_t dsc_en;
	uint32_t spr_en;
	struct dsc_info dsc_info;
};

struct mipi_ifbc_division {
	uint32_t xres_div;
	uint32_t yres_div;
	uint32_t comp_mode;
	uint32_t pxl0_div2_gt_en;
	uint32_t pxl0_div4_gt_en;
	uint32_t pxl0_divxcfg;
	uint32_t pxl0_dsi_gt_en;
};

void dsc_calculation(struct dsc_calc_info *dsc, uint32_t ifbc_type);
uint32_t get_dsc_out_width(struct dsc_calc_info *dsc, uint32_t ifbc_type, uint32_t input_width, uint32_t panel_xres);
uint32_t get_dsc_out_height(struct dsc_calc_info *dsc, uint32_t ifbc_type, uint32_t input_height);
#endif
