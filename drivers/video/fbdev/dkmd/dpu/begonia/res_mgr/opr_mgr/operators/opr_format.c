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

#include "opr_format.h"
#include "dkmd_log.h"

static struct dpu_to_soc_pixel_format g_dpu_fmt_map[] = {
	/* dpu_fmt------------------sdma_format------------------wdma_format---------------static_dfc_format------------dynamic_dfc_format------------------- */
	{DPU_FMT_RGB_565,           SDMA_FMT_BGR_565,            WDMA_FMT_RGB_565,         DFC_STATIC_FMT_BGR_565,      DFC_DYNAMIC_FMT_BGR_565},
	{DPU_FMT_BGR_565,           SDMA_FMT_RGB_565,            WDMA_FMT_RGB_565,         DFC_STATIC_FMT_RGB_565,      DFC_DYNAMIC_FMT_RGB_565},
	{DPU_FMT_RGBX_4444,         SDMA_FMT_XBGR_4444,          WDMA_FMT_XRGB_4444,       DFC_STATIC_FMT_XBGR_4444,    DFC_DYNAMIC_FMT_XBGR_4444},
	{DPU_FMT_BGRX_4444,         SDMA_FMT_XRGB_4444,          WDMA_FMT_XRGB_4444,       DFC_STATIC_FMT_XRGB_4444,    DFC_DYNAMIC_FMT_XRGB_4444},
	{DPU_FMT_RGBA_4444,         SDMA_FMT_ABGR_4444,          WDMA_FMT_ARGB_4444,       DFC_STATIC_FMT_ABGR_4444,    DFC_DYNAMIC_FMT_ABGR_4444},
	{DPU_FMT_BGRA_4444,         SDMA_FMT_ARGB_4444,          WDMA_FMT_ARGB_4444,       DFC_STATIC_FMT_ARGB_4444,    DFC_DYNAMIC_FMT_ARGB_4444},
	{DPU_FMT_RGBX_5551,         SDMA_FMT_XBGR_1555,          WDMA_FMT_XRGB_5551,       DFC_STATIC_FMT_XBGR_1555,    DFC_DYNAMIC_FMT_XBGR_1555},
	{DPU_FMT_BGRX_5551,         SDMA_FMT_XRGB_1555,          WDMA_FMT_XRGB_5551,       DFC_STATIC_FMT_XRGB_1555,    DFC_DYNAMIC_FMT_XRGB_1555},
	{DPU_FMT_RGBA_5551,         SDMA_FMT_ABGR_1555,          WDMA_FMT_ARGB_5551,       DFC_STATIC_FMT_ABGR_1555,    DFC_DYNAMIC_FMT_ABGR_1555},
	{DPU_FMT_BGRA_5551,         SDMA_FMT_ARGB_1555,          WDMA_FMT_ARGB_5551,       DFC_STATIC_FMT_ARGB_1555,    DFC_DYNAMIC_FMT_ARGB_1555},
	{DPU_FMT_RGBX_8888,         SDMA_FMT_XBGR_8888,          WDMA_FMT_XRGB_8888,       DFC_STATIC_FMT_XBGR_8888,    DFC_DYNAMIC_FMT_XBGR_8888},
	{DPU_FMT_BGRX_8888,         SDMA_FMT_XRGB_8888,          WDMA_FMT_XRGB_8888,       DFC_STATIC_FMT_XRGB_8888,    DFC_DYNAMIC_FMT_XRGB_8888},
	{DPU_FMT_RGBA_8888,         SDMA_FMT_ABGR_8888,          WDMA_FMT_ARGB_8888,       DFC_STATIC_FMT_ABGR_8888,    DFC_DYNAMIC_FMT_ABGR_8888},
	{DPU_FMT_BGRA_8888,         SDMA_FMT_ARGB_8888,          WDMA_FMT_ARGB_8888,       DFC_STATIC_FMT_ARGB_8888,    DFC_DYNAMIC_FMT_ARGB_8888},
	{DPU_FMT_YUV_422_I,         SDMA_FMT_YUYV_422_8BIT_PKG,  WDMA_FMT_YUYV_422_PKG,    DFC_STATIC_FMT_YUYV422,      DFC_DYNAMIC_FMT_YUYV422},
	{DPU_FMT_YUYV_422_PKG,      SDMA_FMT_YUYV_422_8BIT_PKG,  WDMA_FMT_YUYV_422_PKG,    DFC_STATIC_FMT_YUYV422,      DFC_DYNAMIC_FMT_YUYV422},
	{DPU_FMT_YVYU_422_PKG,      SDMA_FMT_YUYV_422_8BIT_PKG,  WDMA_FMT_YUYV_422_PKG,    DFC_STATIC_FMT_YVYU422,      DFC_DYNAMIC_FMT_YVYU422},
	{DPU_FMT_UYVY_422_PKG,      SDMA_FMT_YUYV_422_8BIT_PKG,  WDMA_FMT_YUYV_422_PKG,    DFC_STATIC_FMT_UYVY422,      DFC_DYNAMIC_FMT_UYVY422},
	{DPU_FMT_VYUY_422_PKG,      SDMA_FMT_YUYV_422_8BIT_PKG,  WDMA_FMT_YUYV_422_PKG,    DFC_STATIC_FMT_VYUY422,      DFC_DYNAMIC_FMT_VYUY422},
	{DPU_FMT_YCBCR_422_P,       SDMA_FMT_YUYV_422_8BIT_P,    WDMA_FMT_YUV_422_P_HP,    DFC_STATIC_FMT_YUYV422,      DFC_DYNAMIC_FMT_YUV422_P_8BIT},
	{DPU_FMT_YCRCB_422_P,       SDMA_FMT_YUYV_422_8BIT_P,    WDMA_FMT_YUV_422_P_HP,    DFC_STATIC_FMT_YVYU422,      DFC_DYNAMIC_FMT_YVYU422},
	{DPU_FMT_YCBCR_420_P,       SDMA_FMT_YUYV_420_8BIT_P,    WDMA_FMT_YUV_420_P_HP,    DFC_STATIC_FMT_YUYV422,      DFC_DYNAMIC_FMT_YUV420_P_8BIT},
	{DPU_FMT_YCRCB_420_P,       SDMA_FMT_YUYV_420_8BIT_P,    WDMA_FMT_YUV_420_P_HP,    DFC_STATIC_FMT_YVYU422,      DFC_DYNAMIC_FMT_YUV420_P_8BIT},
	{DPU_FMT_YCBCR_422_SP,      SDMA_FMT_YUYV_422_8BIT_SP,   WDMA_FMT_YUV_422_SP_HP,   DFC_STATIC_FMT_YUYV422,      DFC_DYNAMIC_FMT_YUV422_SP_8BIT},
	{DPU_FMT_YCRCB_422_SP,      SDMA_FMT_YUYV_422_8BIT_SP,   WDMA_FMT_YUV_422_SP_HP,   DFC_STATIC_FMT_YVYU422,      DFC_DYNAMIC_FMT_YUV422_SP_8BIT},
	{DPU_FMT_YCBCR_420_SP,      SDMA_FMT_YUYV_420_8BIT_SP,   WDMA_FMT_YUV_420_SP_HP,   DFC_STATIC_FMT_YUYV422,      DFC_DYNAMIC_FMT_YUV420_SP_8BIT},
	{DPU_FMT_YCRCB_420_SP,      SDMA_FMT_YUYV_420_8BIT_SP,   WDMA_FMT_YUV_420_SP_HP,   DFC_STATIC_FMT_YVYU422,      DFC_DYNAMIC_FMT_YUV420_SP_8BIT},
	{DPU_FMT_YUVA444,           -1,                          WDMA_FMT_AYUV_4444,       DFC_STATIC_FMT_YUV444,       -1},
	{DPU_FMT_RGBA_1010102,      SDMA_FMT_RGBA_1010102,       WDMA_FMT_RGBA_1010102,    DFC_STATIC_FMT_RGBA_1010102, DFC_DYNAMIC_FMT_RGBA_1010102},
	{DPU_FMT_BGRA_1010102,      SDMA_FMT_RGBA_1010102,       WDMA_FMT_RGBA_1010102,    DFC_STATIC_FMT_RGBA_1010102, DFC_DYNAMIC_FMT_BGRA_1010102},
	{DPU_FMT_ARGB_10101010,     SDMA_FMT_ARGB_10101010,      -1,                       -1,                          -1},
	{DPU_FMT_XRGB_10101010,     SDMA_FMT_XRGB_10101010,      -1,                       -1,                          -1},
	{DPU_FMT_AYUV_10101010,     SDMA_FMT_AYUV_10101010,      -1,                       -1,                          -1},
	{DPU_FMT_Y410_10BIT,        -1,                          WDMA_FMT_Y410_10BIT,      DFC_STATIC_FMT_UYVA_1010102, DFC_DYNAMIC_FMT_UYVA_1010102},
	{DPU_FMT_YUV422_10BIT,      SDMA_FMT_YUYV_422_10BIT_PKG, WDMA_FMT_YUV422_10BIT,    DFC_STATIC_FMT_YUYV_10,      DFC_DYNAMIC_FMT_YUYV_10},
	{DPU_FMT_YCRCB420_SP_10BIT, SDMA_FMT_YUYV_420_10BIT_SP,  WDMA_FMT_YUV420_SP_10BIT, DFC_STATIC_FMT_YUYV_10,      DFC_DYNAMIC_FMT_YUV420_SP_10BIT},
	{DPU_FMT_YCBCR420_SP_10BIT, SDMA_FMT_YUYV_420_10BIT_SP,  WDMA_FMT_YUV420_SP_10BIT, DFC_STATIC_FMT_YUYV_10,      DFC_DYNAMIC_FMT_YUV420_SP_10BIT},
	{DPU_FMT_YCBCR422_SP_10BIT, SDMA_FMT_YUYV_422_10BIT_SP,  WDMA_FMT_YUV422_SP_10BIT, DFC_STATIC_FMT_YUYV_10,      DFC_DYNAMIC_FMT_YUV422_SP_10BIT},
	{DPU_FMT_YCBCR420_P_10BIT,  SDMA_FMT_YUYV_420_10BIT_P,   WDMA_FMT_YUV420_P_10BIT,  DFC_STATIC_FMT_YUYV_10,      DFC_DYNAMIC_FMT_YUV420_P_10BIT},
	{DPU_FMT_YCBCR422_P_10BIT,  SDMA_FMT_YUYV_422_10BIT_P,   WDMA_FMT_YUV422_P_10BIT,  DFC_STATIC_FMT_YUYV_10,      DFC_DYNAMIC_FMT_YUV422_P_10BIT},
	{DPU_FMT_RGB_DELTA_10BIT,   -1,                          WDMA_FMT_RGB_DELTA_10BIT, -1,                          -1},
};

int dpu_fmt_to_sdma(int format)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(g_dpu_fmt_map); ++i) {
		if (format == g_dpu_fmt_map[i].dpu_format)
			return g_dpu_fmt_map[i].sdma_format;
	}

	dpu_pr_warn("format=%d cannot convert to Sdma pixel format!", format);
	return SDMA_FMT_ARGB_8888;
}

int dpu_fmt_to_wdma(int format)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(g_dpu_fmt_map); ++i) {
		if (format == g_dpu_fmt_map[i].dpu_format)
			return g_dpu_fmt_map[i].wdma_format;
	}

	dpu_pr_warn("format=%d cannot convert to Wdma pixel format!", format);
	return WDMA_FMT_ARGB_8888;
}

int dpu_fmt_to_static_dfc(int format)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(g_dpu_fmt_map); ++i) {
		if (format == g_dpu_fmt_map[i].dpu_format)
			return g_dpu_fmt_map[i].static_dfc_format;
	}

	dpu_pr_warn("format=%d cannot convert to StaticDfc pixel format!", format);
	return DFC_STATIC_FMT_ARGB_8888;
}

int dpu_fmt_to_dynamic_dfc(int format)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(g_dpu_fmt_map); ++i) {
		if (format == g_dpu_fmt_map[i].dpu_format)
			return g_dpu_fmt_map[i].dynamic_dfc_format;
	}

	dpu_pr_warn("format=%d cannot convert to DynamicDfc pixel format!", format);
	return DFC_DYNAMIC_FMT_ARGB_8888;
}