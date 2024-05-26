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

#ifndef DPUFE_ENUM_H
#define DPUFE_ENUM_H

#define DSS_ONLINE_PLAY_PRINT_COUNT 3
#define DSS_PDP_LDI_UNDERFLOW BIT(0)
#define DSS_SDP_LDI_UNDERFLOW BIT(1)

/* dss capability priority description */
#define CAP_HEBCD BIT(17)
#define CAP_HEBCE BIT(16)
#define CAP_HFBCD BIT(15)
#define CAP_HFBCE BIT(14)
#define CAP_1D_SHARPNESS BIT(13)
#define CAP_2D_SHARPNESS BIT(12)
#define CAP_TILE BIT(11)
#define CAP_AFBCD BIT(10)
#define CAP_AFBCE BIT(9)
#define CAP_YUV_DEINTERLACE BIT(8)
#define CAP_YUV_PLANAR BIT(7)
#define CAP_YUV_SEMI_PLANAR BIT(6)
#define CAP_YUV_PACKAGE BIT(5)
#define CAP_SCL BIT(4)
#define CAP_ROT BIT(3)
#define CAP_PURE_COLOR BIT(2)
#define CAP_DIM BIT(1)
#define CAP_BASE BIT(0)

/* callback data type */
#define HOST_VSYNC_SIG 0x11
#define HOST_VACTIVE_START_SIG 0x12

#define DPU_FB0_BUF_NUM 3

enum dpu_chn_idx {
	DPU_RCHN_NONE = -1,
	DPU_RCHN_D2 = 0,
	DPU_RCHN_D3,
	DPU_RCHN_V0,
	DPU_RCHN_G0,
	DPU_RCHN_V1,
	DPU_RCHN_G1,
	DPU_RCHN_D0,
	DPU_RCHN_D1,

	DPU_WCHN_W0,
	DPU_WCHN_W1,

	DPU_CHN_MAX,

	DPU_RCHN_V2 = DPU_CHN_MAX,
	DPU_WCHN_W2,

	DPU_CHN_MAX_DEFINE,
};

enum dpu_ovl_idx {
	DPU_OVL0 = 0,
	DPU_OVL1,
	DPU_OVL2,
	DPU_OVL3,
	DPU_OVL_IDX_MAX,
};

enum dpu_fb_transform {
	DPU_FB_TRANSFORM_NOP = 0x0,
	/* flip source image horizontally (around the vertical axis) */
	DPU_FB_TRANSFORM_FLIP_H = 0x01,
	/* flip source image vertically (around the horizontal axis) */
	DPU_FB_TRANSFORM_FLIP_V = 0x02,
	/* rotate source image 90 degrees clockwise */
	DPU_FB_TRANSFORM_ROT_90 = 0x04,
	/* rotate source image 180 degrees */
	DPU_FB_TRANSFORM_ROT_180 = 0x03,
	/* rotate source image 270 degrees clockwise */
	DPU_FB_TRANSFORM_ROT_270 = 0x07,
};

enum dpu_fb_blending {
	DPU_FB_BLENDING_NONE = 0,
	DPU_FB_BLENDING_PREMULT = 1,
	DPU_FB_BLENDING_COVERAGE = 2,
	DPU_FB_BLENDING_MAX = 3,
};

#endif
