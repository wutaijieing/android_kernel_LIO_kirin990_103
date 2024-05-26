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

#ifndef OPR_FORMAT_H
#define OPR_FORMAT_H

#include <linux/types.h>
#include <dpu_format.h>
#include <dpu/soc_dpu_format.h>

struct dpu_to_soc_pixel_format {
	int32_t dpu_format;
	int32_t sdma_format;
	int32_t wdma_format;
	int32_t static_dfc_format;
	int32_t dynamic_dfc_format;
};

static inline bool dpu_fmt_has_alpha(int32_t format)
{
	switch (format) {
	case DPU_FMT_RGBA_4444:
	case DPU_FMT_RGBA_5551:
	case DPU_FMT_RGBA_8888:

	case DPU_FMT_BGRA_4444:
	case DPU_FMT_BGRA_5551:
	case DPU_FMT_BGRA_8888:

	case DPU_FMT_RGBA_1010102:
	case DPU_FMT_BGRA_1010102:
		return true;

	default:
		return false;
	}
}

static inline bool is_8bit(uint32_t format)
{
	switch (format) {
	case DPU_FMT_RGBA_8888:
	case DPU_FMT_RGBX_8888:
	case DPU_FMT_BGRA_8888:
	case DPU_FMT_BGRX_8888:
	/* YUV Semi Planar */
	case DPU_FMT_YCBCR_422_SP: /* NV16 */
	case DPU_FMT_YCRCB_422_SP:
	case DPU_FMT_YCBCR_420_SP:
	case DPU_FMT_YCRCB_420_SP: /* NV21 */

	/* YUV Planar */
	case DPU_FMT_YCBCR_422_P:
	case DPU_FMT_YCRCB_422_P:
	case DPU_FMT_YCBCR_420_P:
	case DPU_FMT_YCRCB_420_P: /* DPU_FMT_YV12 */

	/* YUV Package */
	case DPU_FMT_YUYV_422_PKG:
	case DPU_FMT_UYVY_422_PKG:
	case DPU_FMT_YVYU_422_PKG:
	case DPU_FMT_VYUY_422_PKG:
		return true;
	default:
		return false;
	}
}

static inline bool is_40bit(int32_t format)
{
	switch (format) {
	case DPU_FMT_ARGB_10101010:
	case DPU_FMT_XRGB_10101010:
	case DPU_FMT_AYUV_10101010:
		return true;
	default:
		return false;
	}
}

static inline bool is_10bit(int32_t format)
{
	if (is_40bit(format))
		return true;

	switch (format) {
	case DPU_FMT_RGBA_1010102:
	case DPU_FMT_BGRA_1010102:
		return true;
	default:
		return false;
	}
}

static inline bool is_yuv_package(int32_t format)
{
	switch (format) {
	case DPU_FMT_YUV_422_I:
	case DPU_FMT_YUV422_10BIT:
	case DPU_FMT_YUYV_422_PKG:
	case DPU_FMT_UYVY_422_PKG:
	case DPU_FMT_YVYU_422_PKG:
	case DPU_FMT_VYUY_422_PKG:
		return true;
	default:
		return false;
	}
}

static inline bool is_yuv420_semi_planar(uint32_t format)
{
	switch (format) {
	case DPU_FMT_YCBCR_420_SP:
	case DPU_FMT_YCRCB_420_SP:
	case DPU_FMT_YCRCB420_SP_10BIT:
	case DPU_FMT_YCBCR420_SP_10BIT:
		return true;
	default:
		return false;
	}
}

static inline bool is_yuv422_semi_planar(uint32_t format)
{
	switch (format) {
	case DPU_FMT_YCBCR_422_SP:
	case DPU_FMT_YCRCB_422_SP:
	case DPU_FMT_YCBCR422_SP_10BIT:
	case DPU_FMT_YCRCB422_SP_10BIT:
		return true;
	default:
		return false;
	}
}

static inline bool is_yuv_semi_planar(int32_t format)
{
	if (is_yuv420_semi_planar(format))
		return true;

	if (is_yuv422_semi_planar(format))
		return true;

	return false;
}

static inline bool is_yuv_plane(int32_t format)
{
	switch (format) {
	case DPU_FMT_YCBCR_422_P:
	case DPU_FMT_YCRCB_422_P:
	case DPU_FMT_YCBCR_420_P:
	case DPU_FMT_YCRCB_420_P:
	case DPU_FMT_YCBCR420_P_10BIT:
	case DPU_FMT_YCBCR422_P_10BIT:
		return true;
	default:
		return false;
	}
}

static inline bool is_yuv_fmt(int32_t format)
{
	return is_yuv_package(format) || is_yuv_semi_planar(format) || is_yuv_plane(format);
}

static inline bool is_rgb_fmt(int32_t format)
{
	switch (format) {
	case DPU_FMT_RGB_565:
	case DPU_FMT_RGBX_4444:
	case DPU_FMT_RGBA_4444:
	case DPU_FMT_RGBX_5551:
	case DPU_FMT_RGBA_5551:
	case DPU_FMT_RGBX_8888:
	case DPU_FMT_RGBA_8888:

	case DPU_FMT_BGR_565:
	case DPU_FMT_BGRX_4444:
	case DPU_FMT_BGRA_4444:
	case DPU_FMT_BGRX_5551:
	case DPU_FMT_BGRA_5551:
	case DPU_FMT_BGRX_8888:
	case DPU_FMT_BGRA_8888:

	case DPU_FMT_RGBA_1010102:
	case DPU_FMT_BGRA_1010102:
		return true;
	default:
		return false;
	}
}

int32_t dpu_fmt_to_sdma(int32_t format);
int32_t dpu_fmt_to_wdma(int32_t format);
int32_t dpu_fmt_to_static_dfc(int32_t format);
int32_t dpu_fmt_to_dynamic_dfc(int32_t format);

#endif