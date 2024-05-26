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

#ifndef DPU_FORMAT_H
#define DPU_FORMAT_H

// All enum values are not used as concrete code, just a name
enum DPU_PIXEL_FORMAT {
	DPU_FMT_RGB_565 = 0,
	DPU_FMT_RGBX_4444,
	DPU_FMT_RGBA_4444,
	DPU_FMT_RGBX_5551,
	DPU_FMT_RGBA_5551,
	DPU_FMT_RGBX_8888 = 5,
	DPU_FMT_RGBA_8888,

	DPU_FMT_BGR_565,
	DPU_FMT_BGRX_4444,
	DPU_FMT_BGRA_4444,
	DPU_FMT_BGRX_5551 = 10,
	DPU_FMT_BGRA_5551,
	DPU_FMT_BGRX_8888,
	DPU_FMT_BGRA_8888,

	DPU_FMT_YUV_422_I,

	/* YUV Semi-planar */
	DPU_FMT_YCBCR_422_SP = 15, /* NV16 */
	DPU_FMT_YCRCB_422_SP,
	DPU_FMT_YCBCR_420_SP, /* NV12 */
	DPU_FMT_YCRCB_420_SP, /* NV21 */

	/* YUV Planar */
	DPU_FMT_YCBCR_422_P,
	DPU_FMT_YCRCB_422_P = 20,
	DPU_FMT_YCBCR_420_P,
	DPU_FMT_YCRCB_420_P, /* DPU_FMT_YV12 */

	/* YUV Package */
	DPU_FMT_YUYV_422_PKG,
	DPU_FMT_UYVY_422_PKG,
	DPU_FMT_YVYU_422_PKG = 25,
	DPU_FMT_VYUY_422_PKG,

	/* 10bit */
	DPU_FMT_RGBA_1010102,
	DPU_FMT_BGRA_1010102,
	DPU_FMT_ARGB_10101010,
	DPU_FMT_XRGB_10101010 = 30,
	DPU_FMT_AYUV_10101010,
	DPU_FMT_Y410_10BIT,
	DPU_FMT_YUV422_10BIT,

	DPU_FMT_YCBCR420_SP_10BIT,
	DPU_FMT_YCRCB420_SP_10BIT = 35,
	DPU_FMT_YCBCR420_P_10BIT,
	DPU_FMT_YCRCB420_P_10BIT,
	DPU_FMT_YCBCR422_P_10BIT,
	DPU_FMT_YCRCB422_P_10BIT,
	DPU_FMT_YCBCR422_SP_10BIT = 40,
	DPU_FMT_YCRCB422_SP_10BIT,

	DPU_FMT_YUVA444,

	DPU_FMT_RGB_DELTA_8BIT,
	DPU_FMT_RGB_DELTA_10BIT, // HEMC
	DPU_FMT_RGBG_8BIT = 45,
	DPU_FMT_RGBG_10BIT,

	DPU_FMT_MAX,
};

enum BYTES_PER_PIXEL {
	BPP_RGB_2PXL = 2,
	BPP_RGB_4PXL = 4,
	BPP_YUV_2PXL = 2,
	BPP_YUV_1PXL = 1,
};

static inline int get_bpp_by_dpu_format(int format)
{
	switch (format) {
	case DPU_FMT_RGB_565:
	case DPU_FMT_BGR_565:
		return BPP_RGB_2PXL;
	case DPU_FMT_RGBX_8888:
	case DPU_FMT_BGRX_8888:
	case DPU_FMT_RGBA_8888:
	case DPU_FMT_BGRA_8888:
	case DPU_FMT_RGBA_1010102:
	case DPU_FMT_BGRA_1010102:
	case DPU_FMT_ARGB_10101010:
	case DPU_FMT_AYUV_10101010:
	case DPU_FMT_YUVA444:
		return BPP_RGB_4PXL;
	case DPU_FMT_YCBCR_422_SP:
	case DPU_FMT_YUV_422_I:
	case DPU_FMT_YCBCR420_SP_10BIT:
	case DPU_FMT_YCRCB420_SP_10BIT:
		return BPP_YUV_2PXL;
	case DPU_FMT_YCBCR_420_P:
	case DPU_FMT_YCBCR_420_SP:
	case DPU_FMT_YCRCB_420_SP:
	case DPU_FMT_YCRCB_420_P:
		return BPP_YUV_1PXL;
	default:
		return -1;
	}
}

#endif