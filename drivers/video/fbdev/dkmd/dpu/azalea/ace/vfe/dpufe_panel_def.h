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

#ifndef DPUFE_PANEL_DEF_H
#define DPUFE_PANEL_DEF_H

#define DTS_ACE_PANEL_INFO "ace_panel_info"
#define MAX_PANEL_NUM_EACH_DISP 2
#define MAX_DISP_CHN_NUM 2

/* for fold panel */
#define DISPLAY_PANEL_ID_MAX 2

/* for fb0 fb1 fb2 and so on */
enum {
	PRIMARY_PANEL_IDX,
	EXTERNAL_PANEL_IDX,
	AUXILIARY_PANEL_IDX,
};

/* 32 bit alignment */
struct panel_base_info {
	uint32_t xres;
	uint32_t yres;
	uint32_t width;
	uint32_t height;
	uint32_t panel_type;
};

struct platform_product_info {
	uint32_t max_hwc_mmbuf_size;
	uint32_t max_mdc_mmbuf_size;
	uint32_t fold_display_support;
	uint32_t dfr_support_value;
	uint32_t dummy_pixel_num;
	uint32_t ifbc_type;
	uint32_t need_two_panel_display;
	uint32_t virtual_fb_xres;
	uint32_t virtual_fb_yres;
	uint32_t p3_support;
	uint32_t hdr_flw_support;
	uint32_t post_hihdr_support;
	uint32_t spr_enable;
	uint32_t panel_id[DISPLAY_PANEL_ID_MAX];
	uint32_t xres[DISPLAY_PANEL_ID_MAX];
	uint32_t yres[DISPLAY_PANEL_ID_MAX];
	uint32_t width[DISPLAY_PANEL_ID_MAX];
	uint32_t height[DISPLAY_PANEL_ID_MAX];
	uint32_t dfr_method;
	uint32_t support_tiny_porch_ratio;
	uint32_t support_ddr_bw_adjust;
	uint32_t actual_porch_ratio;
	uint32_t bypass_device_compose;
	uint32_t dual_lcd_support;
};

// 32bit align
struct platform_panel_info {
	uint8_t disp_logical_pri_cnnt;
	uint8_t disp_phy_ext_cnnt;
	uint8_t disp_logical_ext_cnnt;
	uint8_t reserved1;
	struct panel_base_info disp_phy_pri_pres;
	struct panel_base_info disp_logical_pri_pres;
	struct panel_base_info disp_phy_ext_pres;
	struct panel_base_info disp_logical_ext_pres;
};

typedef struct fb_fix_var_screeninfo {
	uint32_t fix_type;
	uint32_t fix_xpanstep;
	uint32_t fix_ypanstep;
	uint32_t var_vmode;

	uint32_t var_blue_offset;
	uint32_t var_green_offset;
	uint32_t var_red_offset;
	uint32_t var_transp_offset;

	uint32_t var_blue_length;
	uint32_t var_green_length;
	uint32_t var_red_length;
	uint32_t var_transp_length;

	uint32_t var_blue_msb_right;
	uint32_t var_green_msb_right;
	uint32_t var_red_msb_right;
	uint32_t var_transp_msb_right;
	uint32_t bpp;
} fb_fix_var_screeninfo_t;

enum dpu_fb_pixel_format {
	DPU_FB_PIXEL_FORMAT_RGB_565 = 0,
	DPU_FB_PIXEL_FORMAT_RGBX_4444,
	DPU_FB_PIXEL_FORMAT_RGBA_4444,
	DPU_FB_PIXEL_FORMAT_RGBX_5551,
	DPU_FB_PIXEL_FORMAT_RGBA_5551,
	DPU_FB_PIXEL_FORMAT_RGBX_8888,
	DPU_FB_PIXEL_FORMAT_RGBA_8888,

	DPU_FB_PIXEL_FORMAT_BGR_565,
	DPU_FB_PIXEL_FORMAT_BGRX_4444,
	DPU_FB_PIXEL_FORMAT_BGRA_4444,
	DPU_FB_PIXEL_FORMAT_BGRX_5551,
	DPU_FB_PIXEL_FORMAT_BGRA_5551,
	DPU_FB_PIXEL_FORMAT_BGRX_8888,
	DPU_FB_PIXEL_FORMAT_BGRA_8888,

	DPU_FB_PIXEL_FORMAT_YUV_422_I,

	/* YUV Semi-planar */
	DPU_FB_PIXEL_FORMAT_YCBCR_422_SP, /* NV16 */
	DPU_FB_PIXEL_FORMAT_YCRCB_422_SP,
	DPU_FB_PIXEL_FORMAT_YCBCR_420_SP,
	DPU_FB_PIXEL_FORMAT_YCRCB_420_SP, /* NV21 */

	/* YUV Planar */
	DPU_FB_PIXEL_FORMAT_YCBCR_422_P,
	DPU_FB_PIXEL_FORMAT_YCRCB_422_P,
	DPU_FB_PIXEL_FORMAT_YCBCR_420_P,
	DPU_FB_PIXEL_FORMAT_YCRCB_420_P, /* DPU_FB_PIXEL_FORMAT_YV12 */

	/* YUV Package */
	DPU_FB_PIXEL_FORMAT_YUYV_422_PKG,
	DPU_FB_PIXEL_FORMAT_UYVY_422_PKG,
	DPU_FB_PIXEL_FORMAT_YVYU_422_PKG,
	DPU_FB_PIXEL_FORMAT_VYUY_422_PKG,

	/* 10bit */
	DPU_FB_PIXEL_FORMAT_RGBA_1010102,
	DPU_FB_PIXEL_FORMAT_BGRA_1010102,
	DPU_FB_PIXEL_FORMAT_Y410_10BIT,
	DPU_FB_PIXEL_FORMAT_YUV422_10BIT,

	DPU_FB_PIXEL_FORMAT_YCRCB420_SP_10BIT,
	DPU_FB_PIXEL_FORMAT_YCRCB422_SP_10BIT,
	DPU_FB_PIXEL_FORMAT_YCRCB420_P_10BIT,
	DPU_FB_PIXEL_FORMAT_YCRCB422_P_10BIT,

	DPU_FB_PIXEL_FORMAT_YCBCR420_SP_10BIT,
	DPU_FB_PIXEL_FORMAT_YCBCR422_SP_10BIT,
	DPU_FB_PIXEL_FORMAT_YCBCR420_P_10BIT,
	DPU_FB_PIXEL_FORMAT_YCBCR422_P_10BIT,

	DPU_FB_PIXEL_FORMAT_MAX,
};

#endif
