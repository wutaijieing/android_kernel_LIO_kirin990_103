/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include <asm/uaccess.h>
#include <linux/module.h>
#include "hisi_disp_debug.h"
#include "hisi_disp_iommu.h"
#include "hisi_disp.h"
#include "hisi_operator_tool.h"
#include "hisi_disp_debug.h"

uint8_t g_scene_id;
static uint32_t g_dump_frame_cnt = 0;

int g_debug_block_scl_rot = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_block_scl_rot, g_debug_block_scl_rot, int, 0644);
MODULE_PARM_DESC(debug_block_scl_rot, "debug block scf rot");
#endif

int g_debug_wch_scf = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_wch_scf, g_debug_wch_scf, int, 0644);
MODULE_PARM_DESC(debug_wch_scf, "debug wch scf");
#endif

int g_debug_wch_scf_rot = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_wch_scf_rot, g_debug_wch_scf_rot, int, 0644);
MODULE_PARM_DESC(debug_wch_scf_rot, "debug wch scf rot");
#endif

int g_debug_wch_yuv_in = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_wch_yuv_in, g_debug_wch_yuv_in, int, 0644);
MODULE_PARM_DESC(debug_wch_yuv_in, "debug wch yuv in");
#endif

int g_debug_wch_clip_left = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_wch_clip_left, g_debug_wch_clip_left, int, 0644);
MODULE_PARM_DESC(debug_wch_clip_left, "debug clip l");
#endif

int g_debug_wch_clip_right = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_wch_clip_right, g_debug_wch_clip_right, int, 0644);
MODULE_PARM_DESC(debug_wch_clip_right, "debug clip r");
#endif

int g_debug_wch_clip_top = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_wch_clip_top, g_debug_wch_clip_top, int, 0644);
MODULE_PARM_DESC(debug_wch_clip_top, "debug clip t");
#endif

int g_debug_wch_clip_bot = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_wch_clip_bot, g_debug_wch_clip_bot, int, 0644);
MODULE_PARM_DESC(debug_wch_clip_bot, "debug clip b");
#endif

int g_debug_bbit_vdec = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_bbit_vdec, g_debug_bbit_vdec, int, 0644);
MODULE_PARM_DESC(debug_bbit_vdec, "vdec addr ");
#endif

int g_debug_bbit_venc = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_bbit_venc, g_debug_bbit_venc, int, 0644);
MODULE_PARM_DESC(debug_bbit_venc, "venc addr ");
#endif

long g_slice0_y_payload_addr = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(slice0_y_payload_addr, g_slice0_y_payload_addr, long, 0644);
MODULE_PARM_DESC(slice0_y_payload_addr, "slice0 y paload");
#endif

long g_slice0_c_payload_addr = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(slice0_c_payload_addr, g_slice0_c_payload_addr, long, 0644);
MODULE_PARM_DESC(slice0_c_payload_addr, "slice0 c paload");
#endif

long g_slice0_y_header_addr = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(slice0_y_header_addr, g_slice0_y_header_addr, long, 0644);
MODULE_PARM_DESC(slice0_y_header_addr, "slice0 y header");
#endif

long g_slice0_c_header_addr = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(slice0_c_header_addr, g_slice0_c_header_addr, long, 0644);
MODULE_PARM_DESC(slice0_c_header_addr, "slice0 c header");
#endif

long g_slice0_cmdlist_header_addr = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(slice0_cmdlist_header_addr, g_slice0_cmdlist_header_addr, long, 0644);
MODULE_PARM_DESC(slice0_cmdlist_header_addr, "slice0 cmd header addr");
#endif

long g_slice1_cmdlist_header_addr = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(slice1_cmdlist_header_addr, g_slice1_cmdlist_header_addr, long, 0644);
MODULE_PARM_DESC(slice1_cmdlist_header_addr, "slice1 cmd header addr");
#endif

long g_slice2_cmdlist_header_addr = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(slice2_cmdlist_header_addr, g_slice2_cmdlist_header_addr, long, 0644);
MODULE_PARM_DESC(slice2_cmdlist_header_addr, "slice2 cmd header addr");
#endif

long g_slice3_cmdlist_header_addr = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(slice3_cmdlist_header_addr, g_slice3_cmdlist_header_addr, long, 0644);
MODULE_PARM_DESC(slice3_cmdlist_header_addr, "slice3 cmd header addr");
#endif

int g_debug_en_uvup1 = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_en_uvup1, g_debug_en_uvup1, int, 0644);
MODULE_PARM_DESC(debug_en_uvup1, "debug en uvup1");
#endif

int g_debug_en_sdma1 = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_en_sdma1, g_debug_en_sdma1, int, 0644);
MODULE_PARM_DESC(g_debug_en_sdma1, "debug en sdma1");
#endif

int g_debug_en_demura = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_en_demura, g_debug_en_demura, int, 0644);
MODULE_PARM_DESC(debug_en_demura, "debug en demura");
#endif

int g_debug_en_ddic = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_en_ddic, g_debug_en_ddic, int, 0644);
MODULE_PARM_DESC(debug_en_ddic, "debug en ddic");
#endif

int g_debug_demura_type = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_demura_type, g_debug_demura_type, int, 0644);
MODULE_PARM_DESC(debug_demura_type, "debug demura type");
#endif
int g_debug_demura_id = 31;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(debug_demura_id, g_debug_demura_id, int, 0644);
MODULE_PARM_DESC(debug_demura_id, "debug demura id");
#endif

int g_demura_lut_fmt = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(demura_lut_fmt, g_demura_lut_fmt, int, 0644);
MODULE_PARM_DESC(demura_lut_fmt, "debug demura lut fmt");
#endif

int g_demura_idx = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(demura_idx, g_demura_idx, int, 0644);
MODULE_PARM_DESC(demura_idx, "debug demura idx");
#endif

int g_r_demura_w = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(r_demura_w, g_r_demura_w, int, 0644);
MODULE_PARM_DESC(r_demura_w, "r demura w");
#endif

int g_r_demura_h = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(r_demura_h, g_r_demura_h, int, 0644);
MODULE_PARM_DESC(r_demura_h, "r demura h");
#endif

int g_demura_plane_num = 0;
#ifdef CONFIG_OFFLINE_DBG
module_param_named(demura_plane_num, g_demura_plane_num, int, 0644);
MODULE_PARM_DESC(demura_plane_num, "demura plane num");
#endif

uint32_t g_local_dimming_en = 0;
module_param_named(local_dimming_en, g_local_dimming_en, int, 0644);
MODULE_PARM_DESC(local_dimming_en, "local dimming en");

int hisi_fb_display_effect_test = 0;
module_param_named(debug_display_effect_test, hisi_fb_display_effect_test, int, 0644);
MODULE_PARM_DESC(debug_display_effect_test, "hisi_fb_display_effect_test");

int hisi_fb_display_en_dpp_core = 0;
module_param_named(en_dpp_core, hisi_fb_display_en_dpp_core, int, 0644);
MODULE_PARM_DESC(en_dpp_core, "hisi_fb_display_en_dpp_core");

uint32_t g_gmp_bitext_mode = 1; // gmp_bitext_mode=1, copy; gmp_bitext_mode=2, zero
module_param_named(gmp_bitext_mode, g_gmp_bitext_mode, int, 0644);
MODULE_PARM_DESC(gmp_bitext_mode, "gmp bitext mode");

uint32_t DM_INPUTDATA_ST_ADDR[] = {
	DM_INPUTDATA_ST_ADDR0, DM_INPUTDATA_ST_ADDR1, DM_INPUTDATA_ST_ADDR2, DM_INPUTDATA_ST_ADDR3,
	DM_INPUTDATA_ST_ADDR4, DM_INPUTDATA_ST_ADDR5, DM_INPUTDATA_ST_ADDR6
};

struct hal_to_dma_pixel_fmt {
	uint32_t hal_pixel_format;
	uint32_t dma_pixel_format;
};

struct hal_to_dfc_pixel_fmt {
	uint32_t hal_pixel_format;
	uint32_t dfc_pixel_format;
};

bool hal_format_has_alpha(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_RGBA_4444:
	case HISI_FB_PIXEL_FORMAT_RGBA_5551:
	case HISI_FB_PIXEL_FORMAT_RGBA_8888:

	case HISI_FB_PIXEL_FORMAT_BGRA_4444:
	case HISI_FB_PIXEL_FORMAT_BGRA_5551:
	case HISI_FB_PIXEL_FORMAT_BGRA_8888:

	case HISI_FB_PIXEL_FORMAT_RGBA_1010102:
	case HISI_FB_PIXEL_FORMAT_BGRA_1010102:
		return true;

	default:
		return false;
	}
}

bool is_d3_128(uint32_t format)
{
	return format == HISI_FB_PIXEL_FORMAT_D3_128;
}

bool is_yuv_package(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YUV_422_I:
	case HISI_FB_PIXEL_FORMAT_YUV422_10BIT:
	case HISI_FB_PIXEL_FORMAT_YUYV_422_PKG:
	case HISI_FB_PIXEL_FORMAT_UYVY_422_PKG:
	case HISI_FB_PIXEL_FORMAT_YVYU_422_PKG:
	case HISI_FB_PIXEL_FORMAT_VYUY_422_PKG:
		return true;

	default:
		return false;
	}
}

bool is_10bit(uint32_t format)
{
    disp_pr_info("ov out always is 10bit");
    return true; /* ov out put is ARGB10bit or AYUV10bit */

	switch (format) {
	case HISI_FB_PIXEL_FORMAT_RGBA_1010102:
	case HISI_FB_PIXEL_FORMAT_BGRA_1010102:
		return true;

	default:
		return false;
	}
}

bool is_8bit(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_RGBA_8888:
    case HISI_FB_PIXEL_FORMAT_RGBX_8888:
    case HISI_FB_PIXEL_FORMAT_BGRA_8888:
	case HISI_FB_PIXEL_FORMAT_BGRX_8888:
	case HISI_FB_PIXEL_FORMAT_YCBCR_422_SP: /* NV16 */
	case HISI_FB_PIXEL_FORMAT_YCRCB_422_SP:
    case HISI_FB_PIXEL_FORMAT_YCBCR_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCRCB_420_SP: /* NV21*/

	/* YUV Planar */
	case HISI_FB_PIXEL_FORMAT_YCBCR_422_P:
	case HISI_FB_PIXEL_FORMAT_YCRCB_422_P:
	case HISI_FB_PIXEL_FORMAT_YCBCR_420_P:
	case HISI_FB_PIXEL_FORMAT_YCRCB_420_P: /* HISI_FB_PIXEL_FORMAT_YV12 */

	/* YUV Package */
	case HISI_FB_PIXEL_FORMAT_YUYV_422_PKG:
	case HISI_FB_PIXEL_FORMAT_UYVY_422_PKG:
	case HISI_FB_PIXEL_FORMAT_YVYU_422_PKG:
	case HISI_FB_PIXEL_FORMAT_VYUY_422_PKG:
		return true;

	default:
		return false;
	}
}

bool is_yuv_semiplanar(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCBCR_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCRCB_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCBCR_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCRCB_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_yuv_plane(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCBCR_422_P:
	case HISI_FB_PIXEL_FORMAT_YCRCB_422_P:
	case HISI_FB_PIXEL_FORMAT_YCBCR_420_P:
	case HISI_FB_PIXEL_FORMAT_YCRCB_420_P:
	case HISI_FB_PIXEL_FORMAT_YCBCR420_P_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCBCR422_P_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_yuv_format(uint32_t format)
{
	return (is_yuv_package(format) || is_yuv_semiplanar(format) || \
		is_yuv_plane(format) || format == DPU_RDFC_OUT_FORMAT_AYUV_10101010);
}

bool is_yuv_sp_420(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCBCR_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCRCB_420_SP:
	case HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_yuv_sp_422(uint32_t format)
{
	switch (format) {
	case HISI_FB_PIXEL_FORMAT_YCBCR_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCRCB_422_SP:
	case HISI_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT:
	case HISI_FB_PIXEL_FORMAT_YCRCB422_SP_10BIT:
		return true;

	default:
		return false;
	}
}

bool is_yuv_444(uint32_t format)
{
	return format == HISI_FB_PIXEL_FORMAT_YUVA444;
}

int dpu_transform_hal2dma(int transform, int chn_idx)
{
	int ret = -1;

	if (chn_idx >= DPU_WCHN_W0 && chn_idx <= DPU_WCHN_W2) {
		if (transform == HISI_FB_TRANSFORM_NOP) {
			ret = DPU_TRANSFORM_NOP;
		} else if (transform == (HISI_FB_TRANSFORM_ROT_90 | HISI_FB_TRANSFORM_FLIP_V) ) {
			ret = DPU_TRANSFORM_ROT;
		} else {
		    ret = -1;
			disp_pr_err("Transform %d is not supported\n", transform);
		}
	} else {
		// TODO:
	}

    disp_pr_info("Transform  = %d \n", transform);

	return ret;
}

static struct hal_to_dma_pixel_fmt wdma_fmt[] = {
	{HISI_FB_PIXEL_FORMAT_RGB_565, DPU_WDMA_PIXEL_FORMAT_RGB_565},
	{HISI_FB_PIXEL_FORMAT_BGR_565, DPU_WDMA_PIXEL_FORMAT_RGB_565},
	{HISI_FB_PIXEL_FORMAT_RGBX_4444, DPU_WDMA_PIXEL_FORMAT_XRGB_4444},
	{HISI_FB_PIXEL_FORMAT_BGRX_4444, DPU_WDMA_PIXEL_FORMAT_XRGB_4444},
	{HISI_FB_PIXEL_FORMAT_RGBA_4444, DPU_WDMA_PIXEL_FORMAT_ARGB_4444},
	{HISI_FB_PIXEL_FORMAT_BGRA_4444, DPU_WDMA_PIXEL_FORMAT_ARGB_4444},
	{HISI_FB_PIXEL_FORMAT_RGBX_5551, DPU_WDMA_PIXEL_FORMAT_XRGB_5551},
	{HISI_FB_PIXEL_FORMAT_BGRX_5551, DPU_WDMA_PIXEL_FORMAT_XRGB_5551},
	{HISI_FB_PIXEL_FORMAT_RGBA_5551, DPU_WDMA_PIXEL_FORMAT_ARGB_5551},
	{HISI_FB_PIXEL_FORMAT_BGRA_5551, DPU_WDMA_PIXEL_FORMAT_ARGB_5551},
	{HISI_FB_PIXEL_FORMAT_RGBX_8888, DPU_WDMA_PIXEL_FORMAT_XRGB_8888},
	{HISI_FB_PIXEL_FORMAT_BGRX_8888, DPU_WDMA_PIXEL_FORMAT_XRGB_8888},
	{HISI_FB_PIXEL_FORMAT_RGBA_8888, DPU_WDMA_PIXEL_FORMAT_ARGB_8888},
	{HISI_FB_PIXEL_FORMAT_BGRA_8888, DPU_WDMA_PIXEL_FORMAT_ARGB_8888},
	{HISI_FB_PIXEL_FORMAT_YUV_422_I, DPU_WDMA_PIXEL_FORMAT_YUYV_422_PKG},
	{HISI_FB_PIXEL_FORMAT_YUYV_422_PKG, DPU_WDMA_PIXEL_FORMAT_YUYV_422_PKG},
	{HISI_FB_PIXEL_FORMAT_YVYU_422_PKG, DPU_WDMA_PIXEL_FORMAT_YUYV_422_PKG},
	{HISI_FB_PIXEL_FORMAT_UYVY_422_PKG, DPU_WDMA_PIXEL_FORMAT_YUYV_422_PKG},
	{HISI_FB_PIXEL_FORMAT_VYUY_422_PKG, DPU_WDMA_PIXEL_FORMAT_YUYV_422_PKG},
	{HISI_FB_PIXEL_FORMAT_YCBCR_422_P, DPU_WDMA_PIXEL_FORMAT_YUV_422_P_HP},
	{HISI_FB_PIXEL_FORMAT_YCRCB_422_P, DPU_WDMA_PIXEL_FORMAT_YUV_422_P_HP},
	{HISI_FB_PIXEL_FORMAT_YCBCR_420_P, DPU_WDMA_PIXEL_FORMAT_YUV_420_P_HP},
	{HISI_FB_PIXEL_FORMAT_YCRCB_420_P, DPU_WDMA_PIXEL_FORMAT_YUV_420_P_HP},
	{HISI_FB_PIXEL_FORMAT_YCBCR_422_SP, DPU_WDMA_PIXEL_FORMAT_YUV_422_SP_HP},
	{HISI_FB_PIXEL_FORMAT_YCRCB_422_SP, DPU_WDMA_PIXEL_FORMAT_YUV_422_SP_HP},
	{HISI_FB_PIXEL_FORMAT_YCBCR_420_SP, DPU_WDMA_PIXEL_FORMAT_YUV_420_SP_HP},
	{HISI_FB_PIXEL_FORMAT_YCRCB_420_SP, DPU_WDMA_PIXEL_FORMAT_YUV_420_SP_HP},
	{HISI_FB_PIXEL_FORMAT_YUVA444, DPU_WDMA_PIXEL_FORMAT_AYUV_4444},
	{HISI_FB_PIXEL_FORMAT_RGBA_1010102, DPU_WDMA_PIXEL_FORMAT_RGBA_1010102},
	{HISI_FB_PIXEL_FORMAT_BGRA_1010102, DPU_WDMA_PIXEL_FORMAT_RGBA_1010102},
	{HISI_FB_PIXEL_FORMAT_Y410_10BIT, DPU_WDMA_PIXEL_FORMAT_Y410_10BIT},
	{HISI_FB_PIXEL_FORMAT_YUV422_10BIT, DPU_WDMA_PIXEL_FORMAT_YUV422_10BIT},
	{HISI_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT, DPU_WDMA_PIXEL_FORMAT_YUV420_SP_10BIT},
	{HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT, DPU_WDMA_PIXEL_FORMAT_YUV420_SP_10BIT},
	{HISI_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT, DPU_WDMA_PIXEL_FORMAT_YUV422_SP_10BIT},
	{HISI_FB_PIXEL_FORMAT_YCBCR420_P_10BIT, DPU_WDMA_PIXEL_FORMAT_YUV420_P_10BIT},
	{HISI_FB_PIXEL_FORMAT_YCBCR422_P_10BIT, DPU_WDMA_PIXEL_FORMAT_YUV422_P_10BIT},
};

static struct hal_to_dma_pixel_fmt sdma_fmt[] = {
	{HISI_FB_PIXEL_FORMAT_RGB_565, DPU_SDMA_PIXEL_FORMAT_BGR_565},
	{HISI_FB_PIXEL_FORMAT_BGR_565, DPU_SDMA_PIXEL_FORMAT_BGR_565},
	{HISI_FB_PIXEL_FORMAT_RGBX_4444, DPU_SDMA_PIXEL_FORMAT_XBGR_4444},
	{HISI_FB_PIXEL_FORMAT_BGRX_4444, DPU_SDMA_PIXEL_FORMAT_XRGB_4444},
	{HISI_FB_PIXEL_FORMAT_RGBA_4444, DPU_SDMA_PIXEL_FORMAT_ABGR_4444},
	{HISI_FB_PIXEL_FORMAT_BGRA_4444, DPU_SDMA_PIXEL_FORMAT_ARGB_4444},
	{HISI_FB_PIXEL_FORMAT_RGBX_5551, DPU_SDMA_PIXEL_FORMAT_XBGR_1555},
	{HISI_FB_PIXEL_FORMAT_BGRX_5551, DPU_SDMA_PIXEL_FORMAT_XRGB_1555},
	{HISI_FB_PIXEL_FORMAT_RGBA_5551, DPU_SDMA_PIXEL_FORMAT_ABGR_1555},
	{HISI_FB_PIXEL_FORMAT_BGRA_5551, DPU_SDMA_PIXEL_FORMAT_ARGB_1555},
	{HISI_FB_PIXEL_FORMAT_RGBX_8888, DPU_SDMA_PIXEL_FORMAT_XBGR_8888},
	{HISI_FB_PIXEL_FORMAT_BGRX_8888, DPU_SDMA_PIXEL_FORMAT_XRGB_8888},
	{HISI_FB_PIXEL_FORMAT_RGBA_8888, DPU_SDMA_PIXEL_FORMAT_ABGR_8888},
	{HISI_FB_PIXEL_FORMAT_BGRA_8888, DPU_SDMA_PIXEL_FORMAT_ARGB_8888},
	{HISI_FB_PIXEL_FORMAT_YUV_422_I, DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_PKG},
	{HISI_FB_PIXEL_FORMAT_YUYV_422_PKG, DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_PKG},
	{HISI_FB_PIXEL_FORMAT_YVYU_422_PKG, DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_PKG},
	{HISI_FB_PIXEL_FORMAT_UYVY_422_PKG, DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_PKG},
	{HISI_FB_PIXEL_FORMAT_VYUY_422_PKG, DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_PKG},
	{HISI_FB_PIXEL_FORMAT_YCBCR_422_P, DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_P},
	{HISI_FB_PIXEL_FORMAT_YCRCB_422_P, DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_P},
	{HISI_FB_PIXEL_FORMAT_YCBCR_420_P, DPU_SDMA_PIXEL_FORMAT_YUYV_420_8BIT_P},
	{HISI_FB_PIXEL_FORMAT_YCRCB_420_P, DPU_SDMA_PIXEL_FORMAT_YUYV_420_8BIT_P},
	{HISI_FB_PIXEL_FORMAT_YCBCR_422_SP, DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_SP},
	{HISI_FB_PIXEL_FORMAT_YCRCB_422_SP, DPU_SDMA_PIXEL_FORMAT_YUYV_422_8BIT_SP},
	{HISI_FB_PIXEL_FORMAT_YCBCR_420_SP, DPU_SDMA_PIXEL_FORMAT_YUYV_420_8BIT_SP},
	{HISI_FB_PIXEL_FORMAT_YCRCB_420_SP, DPU_SDMA_PIXEL_FORMAT_YUYV_420_8BIT_SP},
	{HISI_FB_PIXEL_FORMAT_RGBA_1010102, DPU_SDMA_PIXEL_FORMAT_RGBA_1010102},
	{HISI_FB_PIXEL_FORMAT_BGRA_1010102, DPU_SDMA_PIXEL_FORMAT_RGBA_1010102},
	{HISI_FB_PIXEL_FORMAT_YUV422_10BIT, DPU_SDMA_PIXEL_FORMAT_YUYV_422_10BIT_PKG},
	{HISI_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT, DPU_SDMA_PIXEL_FORMAT_YUYV_420_10BIT_SP},
	{HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT, DPU_SDMA_PIXEL_FORMAT_YUYV_420_10BIT_SP},
	{HISI_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT, DPU_SDMA_PIXEL_FORMAT_YUYV_422_10BIT_SP},
	{HISI_FB_PIXEL_FORMAT_YCBCR420_P_10BIT, DPU_SDMA_PIXEL_FORMAT_YUYV_420_10BIT_P},
	{HISI_FB_PIXEL_FORMAT_YCBCR422_P_10BIT, DPU_SDMA_PIXEL_FORMAT_YUYV_422_10BIT_P},
	{HISI_FB_PIXEL_FORMAT_D3_128, DPU_SDMA_PIXEL_FORMAT_D3_128},
};

static struct hal_to_dma_pixel_fmt dsd_fmt[] = {
	{HISI_FB_PIXEL_FORMAT_RGBA_8888, DPU_DSD_PIXEL_FORMAT_XRGB_8888},
	{HISI_FB_PIXEL_FORMAT_YUV444, DPU_DSD_PIXEL_FORMAT_YUV444},
	{HISI_FB_PIXEL_FORMAT_YUYV_422_PKG, DPU_DSD_PIXEL_FORMAT_YUYV_422_8BIT_PKG},
	{HISI_FB_PIXEL_FORMAT_YUYV_420_PKG, DPU_DSD_PIXEL_FORMAT_YUYV_422_8BIT_PKG},
	{HISI_FB_PIXEL_FORMAT_RGBA_1010102, DPU_DSD_PIXEL_FORMAT_RGBA_1010102},
	{HISI_FB_PIXEL_FORMAT_YUVA444, DPU_DSD_PIXEL_FORMAT_YUVA_1010102},
	{HISI_FB_PIXEL_FORMAT_YUV422_10BIT, DPU_DSD_PIXEL_FORMAT_YUYV_422_10BIT_PKG},
	{HISI_FB_PIXEL_FORMAT_YUYV_420_PKG_10BIT, DPU_DSD_PIXEL_FORMAT_YUYV_422_10BIT_PKG},
};

static struct hal_to_dma_pixel_fmt dsd_wrap_fmt[] = {
	{HISI_FB_PIXEL_FORMAT_RGBA_8888, DPU_DSD_WRAP_FORMAT_RGB_8BIT},
	{HISI_FB_PIXEL_FORMAT_YUV444, DPU_DSD_WRAP_FORMAT_YUV444_8BIT},
	{HISI_FB_PIXEL_FORMAT_YUYV_422_PKG, DPU_DSD_WRAP_FORMAT_YUV422_8BIT},
	{HISI_FB_PIXEL_FORMAT_YUYV_420_PKG, DPU_DSD_WRAP_FORMAT_YUV420_8BIT},
	{HISI_FB_PIXEL_FORMAT_RGBA_1010102, DPU_DSD_WRAP_FORMAT_RGB_10BIT},
	{HISI_FB_PIXEL_FORMAT_YUVA444, DPU_DSD_WRAP_FORMAT_YUV444_10BIT},
	{HISI_FB_PIXEL_FORMAT_YUV422_10BIT, DPU_DSD_WRAP_FORMAT_YUV422_10BIT},
	{HISI_FB_PIXEL_FORMAT_YUYV_420_PKG_10BIT, DPU_DSD_WRAP_FORMAT_YUV420_10BIT},
};

int dpu_pixel_format_hal2dma(int format, uint32_t dma_type)
{
	int i;
	int size;
	struct hal_to_dma_pixel_fmt *p_dma_fmt;

	if (dma_type == DPU_HAL2SDMA_PIXEL_FORMAT) {
		size = sizeof(sdma_fmt) / sizeof(sdma_fmt[0]);
		p_dma_fmt = sdma_fmt;
	} else if (dma_type == DPU_HAL2WDMA_PIXEL_FORMAT) {
		size = sizeof(wdma_fmt) / sizeof(wdma_fmt[0]);
		p_dma_fmt = wdma_fmt;
	} else if (dma_type == DPU_HAL2DSD_PIXEL_FORMAT) {
		size = sizeof(dsd_fmt) / sizeof(dsd_fmt[0]);
		p_dma_fmt = dsd_fmt;
	} else {
		disp_pr_err("dma type:%d error\n", dma_type);
		return -1;
	}

	for (i = 0; i < size; i++) {
		if (p_dma_fmt[i].hal_pixel_format == format)
			return p_dma_fmt[i].dma_pixel_format;
	}

	disp_pr_err("hal input format:%d not support!\n", format);
	return -1;
}

int dpu_pixel_format_hal2dsdwrap(int format)
{
	int i;
	int size;
	struct hal_to_dma_pixel_fmt *p_dma_fmt;

	size = sizeof(dsd_wrap_fmt) / sizeof(dsd_wrap_fmt[0]);
	p_dma_fmt = dsd_wrap_fmt;

	for (i = 0; i < size; i++) {
		if (p_dma_fmt[i].hal_pixel_format == format)
			return p_dma_fmt[i].dma_pixel_format;
	}

	disp_pr_err("hal input format:%d not support!\n", format);
	return DPU_DSD_WRAP_FORMAT_RGB_8BIT;
}

static struct hal_to_dfc_pixel_fmt dfc_fmt_static[] = {
	{HISI_FB_PIXEL_FORMAT_RGB_565, DPU_DFC_STATIC_PIXEL_FORMAT_BGR_565},
	{HISI_FB_PIXEL_FORMAT_RGBX_4444, DPU_DFC_STATIC_PIXEL_FORMAT_XBGR_4444},
	{HISI_FB_PIXEL_FORMAT_RGBA_4444, DPU_DFC_STATIC_PIXEL_FORMAT_ABGR_4444},
	{HISI_FB_PIXEL_FORMAT_RGBX_5551, DPU_DFC_STATIC_PIXEL_FORMAT_XBGR_1555},
	{HISI_FB_PIXEL_FORMAT_RGBA_5551, DPU_DFC_STATIC_PIXEL_FORMAT_ABGR_1555},
	{HISI_FB_PIXEL_FORMAT_RGBX_8888, DPU_DFC_STATIC_PIXEL_FORMAT_XBGR_8888},
	{HISI_FB_PIXEL_FORMAT_RGBA_8888, DPU_DFC_STATIC_PIXEL_FORMAT_ABGR_8888},
	{HISI_FB_PIXEL_FORMAT_RGBA_1010102, DPU_DFC_STATIC_PIXEL_FORMAT_RGBA_1010102},
	{HISI_FB_PIXEL_FORMAT_BGR_565, DPU_DFC_STATIC_PIXEL_FORMAT_BGR_565},
	{HISI_FB_PIXEL_FORMAT_BGRX_4444, DPU_DFC_STATIC_PIXEL_FORMAT_XRGB_4444},
	{HISI_FB_PIXEL_FORMAT_BGRA_4444, DPU_DFC_STATIC_PIXEL_FORMAT_ARGB_4444},
	{HISI_FB_PIXEL_FORMAT_BGRX_5551, DPU_DFC_STATIC_PIXEL_FORMAT_XRGB_1555},
	{HISI_FB_PIXEL_FORMAT_BGRA_5551, DPU_DFC_STATIC_PIXEL_FORMAT_ARGB_1555},
	{HISI_FB_PIXEL_FORMAT_BGRX_8888, DPU_DFC_STATIC_PIXEL_FORMAT_XRGB_8888},
	{HISI_FB_PIXEL_FORMAT_BGRA_8888, DPU_DFC_STATIC_PIXEL_FORMAT_ARGB_8888},
	{HISI_FB_PIXEL_FORMAT_BGRA_1010102, DPU_DFC_STATIC_PIXEL_FORMAT_RGBA_1010102},
	{HISI_FB_PIXEL_FORMAT_YUV_422_I, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV422},
	{HISI_FB_PIXEL_FORMAT_YUYV_422_PKG, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV422},
	{HISI_FB_PIXEL_FORMAT_YVYU_422_PKG, DPU_DFC_STATIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_UYVY_422_PKG, DPU_DFC_STATIC_PIXEL_FORMAT_UYVY422},
	{HISI_FB_PIXEL_FORMAT_VYUY_422_PKG, DPU_DFC_STATIC_PIXEL_FORMAT_VYUY422},
	{HISI_FB_PIXEL_FORMAT_YCBCR_422_SP, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV422},
	{HISI_FB_PIXEL_FORMAT_YCRCB_422_SP, DPU_DFC_STATIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_YCBCR_420_SP, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV422},
	{HISI_FB_PIXEL_FORMAT_YCRCB_420_SP, DPU_DFC_STATIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_YCBCR_422_P, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV422},
	{HISI_FB_PIXEL_FORMAT_YCBCR_420_P, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV422},
	{HISI_FB_PIXEL_FORMAT_YCRCB_422_P, DPU_DFC_STATIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_YCRCB_420_P, DPU_DFC_STATIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_YUVA444, DPU_DFC_STATIC_PIXEL_FORMAT_YUV444},
	{HISI_FB_PIXEL_FORMAT_Y410_10BIT, DPU_DFC_STATIC_PIXEL_FORMAT_UYVA_1010102},
	{HISI_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV_10},
	{HISI_FB_PIXEL_FORMAT_YUV422_10BIT, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV_10},
	{HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV_10},
	{HISI_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV_10},
	{HISI_FB_PIXEL_FORMAT_YCBCR420_P_10BIT, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV_10},
	{HISI_FB_PIXEL_FORMAT_YCBCR422_P_10BIT, DPU_DFC_STATIC_PIXEL_FORMAT_YUYV_10},
};

static struct hal_to_dfc_pixel_fmt dfc_fmt_dynamic[] = {
	{HISI_FB_PIXEL_FORMAT_RGB_565, DPU_DFC_DYNAMIC_PIXEL_FORMAT_BGR_565},
	{HISI_FB_PIXEL_FORMAT_RGBX_4444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XBGR_4444},
	{HISI_FB_PIXEL_FORMAT_RGBA_4444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ABGR_4444},
	{HISI_FB_PIXEL_FORMAT_RGBX_5551, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XBGR_1555},
	{HISI_FB_PIXEL_FORMAT_RGBA_5551, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ABGR_1555},
	{HISI_FB_PIXEL_FORMAT_RGBX_8888, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XBGR_8888},
	{HISI_FB_PIXEL_FORMAT_RGBA_8888, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ABGR_8888},
	{HISI_FB_PIXEL_FORMAT_RGBA_1010102, DPU_DFC_DYNAMIC_PIXEL_FORMAT_RGBA_1010102},
	{HISI_FB_PIXEL_FORMAT_BGR_565, DPU_DFC_DYNAMIC_PIXEL_FORMAT_BGR_565},
	{HISI_FB_PIXEL_FORMAT_BGRX_4444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XRGB_4444},
	{HISI_FB_PIXEL_FORMAT_BGRA_4444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ARGB_4444},
	{HISI_FB_PIXEL_FORMAT_BGRX_5551, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XRGB_1555},
	{HISI_FB_PIXEL_FORMAT_BGRA_5551, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ARGB_1555},
	{HISI_FB_PIXEL_FORMAT_BGRX_8888, DPU_DFC_DYNAMIC_PIXEL_FORMAT_XRGB_8888},
	{HISI_FB_PIXEL_FORMAT_BGRA_8888, DPU_DFC_DYNAMIC_PIXEL_FORMAT_ARGB_8888},
	{HISI_FB_PIXEL_FORMAT_BGRA_1010102, DPU_DFC_DYNAMIC_PIXEL_FORMAT_RGBA_1010102},
	{HISI_FB_PIXEL_FORMAT_YUV_422_I, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV422},
	{HISI_FB_PIXEL_FORMAT_YUYV_422_PKG, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV422},
	{HISI_FB_PIXEL_FORMAT_YVYU_422_PKG, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_UYVY_422_PKG, DPU_DFC_DYNAMIC_PIXEL_FORMAT_UYVY422},
	{HISI_FB_PIXEL_FORMAT_VYUY_422_PKG, DPU_DFC_DYNAMIC_PIXEL_FORMAT_VYUY422},
	{HISI_FB_PIXEL_FORMAT_YCBCR_422_SP, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV422_SP_8BIT},
	{HISI_FB_PIXEL_FORMAT_YCRCB_422_SP, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_YCBCR_420_SP, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV420_SP_8BIT},
	{HISI_FB_PIXEL_FORMAT_YCRCB_420_SP, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_YCBCR_422_P, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV422_P_8BIT},
	{HISI_FB_PIXEL_FORMAT_YCBCR_420_P, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV420_P_8BIT},
	{HISI_FB_PIXEL_FORMAT_YCRCB_422_P, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_YCRCB_420_P, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YVYU422},
	{HISI_FB_PIXEL_FORMAT_Y410_10BIT, DPU_DFC_DYNAMIC_PIXEL_FORMAT_UYVA_1010102},
	{HISI_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV_10},
	{HISI_FB_PIXEL_FORMAT_YUV422_10BIT, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUYV_10},
	{HISI_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV420_SP_10BIT},
	{HISI_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV422_SP_10BIT},
	{HISI_FB_PIXEL_FORMAT_YCBCR420_P_10BIT, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV420_P_10BIT},
	{HISI_FB_PIXEL_FORMAT_YCBCR422_P_10BIT, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUV422_P_10BIT},
	{HISI_FB_PIXEL_FORMAT_YUVA444, DPU_DFC_DYNAMIC_PIXEL_FORMAT_YUVA_1010102},
};

int dpu_pixel_format_hal2dfc(int format, int dfc_type)
{
	int i;
	int size;
	struct hal_to_dfc_pixel_fmt *p_dma_fmt = NULL;

	if (dfc_type == DFC_STATIC) {
		size = ARRAY_SIZE(dfc_fmt_static);
		p_dma_fmt = dfc_fmt_static;
	} else if (dfc_type == DFC_DYNAMIC) {
		size = ARRAY_SIZE(dfc_fmt_dynamic);
		p_dma_fmt = dfc_fmt_dynamic;
	} else {
		disp_pr_err("DFC type:%d error!\n", dfc_type);
		return -1;
	}

	for (i = 0; i < size; i++) {
		if (p_dma_fmt[i].hal_pixel_format == format)
			return p_dma_fmt[i].dfc_pixel_format;
	}

	disp_pr_err("hal input format:%d not support!\n", format);
	return -1;
}

bool is_offline_scene(uint8_t scene_id)
{
	if (scene_id >= DPU_SCENE_OFFLINE_0 && scene_id <= DPU_SCENE_OFFLINE_2) {
		return true;
	}

	return false;
}

static void save_file(char *filename, const char *buf, uint32_t buf_len)
{
	ssize_t write_len = 0;
	struct file *fd = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;

	if (!filename) {
		disp_pr_err("filename is NULL\n");
		return;
	}
	if (!buf) {
		disp_pr_err("buf is NULL\n");
		return;
	}

	fd = filp_open(filename, O_CREAT | O_RDWR, 0644);
	if (IS_ERR(fd)) {
		disp_pr_err("filp_open returned:filename %s, error %ld\n",
			filename, PTR_ERR(fd));
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	write_len = vfs_write(fd, (char __user *)buf, buf_len, &pos);

	pos = 0;
	set_fs(old_fs);
	filp_close(fd, NULL);
}

static int32_t g_shared_fd = -1;
static uint32_t g_buff_size = 0;
static struct dma_buf *g_dmabuf = NULL;

void dpu_dump_layer(struct pipeline_src_layer *src_layer, struct disp_wb_layer *layer)
{
	struct layer_img img;
	if (src_layer != NULL) {
		disp_pr_info("img");
		img = src_layer->img;
	} else if (layer != NULL) {
		disp_pr_info("dst");
		img = layer->dst;
	}

	disp_pr_info("offline play layer: viraddr:0x%x, format:%d, share_fd:%d, wxh:%dx%d",
		img.vir_addr, img.format,
		img.shared_fd, img.width, img.height);
	g_shared_fd = img.shared_fd;
	g_buff_size = img.buff_size;

	uint64_t iova = 0;
	struct dma_buf *dmabuf = hisi_dss_get_buffer_by_sharefd(&iova, img.shared_fd, img.buff_size);
	disp_pr_info("dma_buf.size:%d,img.size:%d, iova:0x%x, priv:0x%p\n",
		dmabuf->size, img.buff_size, iova, dmabuf->priv);
	g_dmabuf = dmabuf;
	g_buff_size = dmabuf->size;

	dma_buf_begin_cpu_access(dmabuf, DMA_FROM_DEVICE);
	/* offset in PAGE granularity */
	void *kaddr = dma_buf_kmap(dmabuf, 0);
	disp_pr_info("dma_buf.size:%d,img.size:%d, kaddr:0x%p\n",
		dmabuf->size, img.buff_size, kaddr);

	char filename[100];
#ifdef LOCAL_DIMMING_GOLDEN_TEST
	(void)sprintf_s(filename, 100, "/data/data/ld.bin");
	disp_pr_info("ld write addr:0x%p", (char *)(kaddr) + img.buff_size);
	int ld_size = 192 * 108 * 10 * 2;
	save_file(filename, (char *)(kaddr), (img.buff_size  + ld_size));
#else
	(void)sprintf_s(filename, 100, "/data/data/wh_wb_frame.bin");
	save_file(filename, (char *)(kaddr), img.buff_size);
#endif
	g_dump_frame_cnt++;

	dma_buf_kunmap(dmabuf, 0, NULL);
	dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
}

void dpu_init_dump_layer(struct pipeline_src_layer *src_layer, struct disp_wb_layer *layer)
{
	struct layer_img img;
	if (src_layer != NULL) {
		disp_pr_info("img");
		img = src_layer->img;
	} else if (layer != NULL) {
		disp_pr_info("dst");
		img = layer->dst;
	}
	if (get_ld_continue_frm_wb() && (g_dump_frame_cnt > 0))
		return;

	disp_pr_info("offline play layer: viraddr:0x%x, format:%d, share_fd:%d, wxh:%dx%d",
		img.vir_addr, img.format,
		img.shared_fd, img.width, img.height);

	uint64_t iova = 0;
	struct dma_buf *dmabuf = hisi_dss_get_buffer_by_sharefd(&iova, img.shared_fd, img.buff_size);
	disp_pr_info("dma_buf.size:%d,img.size:%d, iova:0x%x, priv:0x%p\n",
		dmabuf->size, img.buff_size, iova, dmabuf->priv);

	dma_buf_begin_cpu_access(dmabuf, DMA_FROM_DEVICE);
	void *kaddr = dma_buf_kmap(dmabuf, 0); // offset in PAGE granularity
	disp_pr_info("dma_buf.size:%d,img.size:%d, kaddr:0x%p\n",
		dmabuf->size, img.buff_size, kaddr);

	(void)memset_s(kaddr + img.buff_size, img.buff_size, 0, img.buff_size);
	char filename[100];

	(void)sprintf_s(filename, 100, "/data/data/ld_ori.bin");
	disp_pr_info("ld write addr:0x%p", (char *)(kaddr) + img.buff_size);
	int ld_size = 192 * 108 * 10 * 2;
	save_file(filename, (char *)(kaddr), (img.buff_size + ld_size));

	dma_buf_kunmap(dmabuf, 0, NULL);
	dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
}

void dpu_test_dump_wb(void)
{
	struct dma_buf *dmabuf = g_dmabuf;
	if (dmabuf == NULL)
		disp_pr_info("dmabuf is NULL");
	else
		disp_pr_info("dma_buf.size:%d,img.size:%d, priv:0x%p\n", dmabuf->size, g_buff_size, dmabuf->priv);

	dma_buf_begin_cpu_access(dmabuf, DMA_FROM_DEVICE);
	void *kaddr = dma_buf_kmap(dmabuf, /* offset in PAGE granularity */ 0);
	disp_pr_info("dma_buf.size:%d,img.size:%d, kaddr:0x%p\n",
		dmabuf->size, g_buff_size, kaddr);

	char filename[100];
	(void)sprintf_s(filename, 100, "/data/data/wh_wb_frame.bin");
    g_dump_frame_cnt++;
	save_file(filename, (char *)(kaddr), g_buff_size);

	dma_buf_kunmap(dmabuf, 0, NULL);
	dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
}
EXPORT_SYMBOL_GPL(dpu_test_dump_wb);
