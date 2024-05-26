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

#include "dsc_output_calc.h"
#include "dkmd_log.h"
#include "dpu/soc_dpu_define.h"

/* single mipi */
struct mipi_ifbc_division g_mipi_ifbc_division[IFBC_TYPE_MAX] = {
	/* none */
	{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
	/* vesa2x_1pipe */
	{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_5, PXL0_DIV2_GT_EN_CLOSE,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_1, PXL0_DSI_GT_EN_3},
	/* vesa3x_1pipe */
	{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_5, PXL0_DIV2_GT_EN_CLOSE,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3},
	/* vesa2x_2pipe */
	{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_6, PXL0_DIV2_GT_EN_OPEN,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_1, PXL0_DSI_GT_EN_3},
	/* vesa3x_2pipe */
	{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_6, PXL0_DIV2_GT_EN_OPEN,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3},
	/* vesa3.75x_2pipe */
	{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_6, PXL0_DIV2_GT_EN_OPEN,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3},
	/* SPR+DSC1.2 need special calulation for compression.
		* It calucate the div in get_hsize_after_spr_dsc().
		*/
	/* vesa2.66x_pipe depend on SPR */
	{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
	/* vesa2.66x_pipe depend on SPR */
	{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
	/* vesa2x_pipe depend on SPR */
	{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
	/* vesa2x_pipe depend on SPR */
	{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
	/* vesa3.75x_1pipe */
	{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_5, PXL0_DIV2_GT_EN_CLOSE,
		PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3}
};

static bool is_ifbc_single_slice(uint32_t ifbc_type)
{
	return ((ifbc_type == IFBC_TYPE_VESA2X_SINGLE) ||
		(ifbc_type == IFBC_TYPE_VESA3X_SINGLE) ||
		(ifbc_type == IFBC_TYPE_VESA4X_SINGLE_SPR) ||
		(ifbc_type == IFBC_TYPE_VESA2X_SINGLE_SPR) ||
		(ifbc_type == IFBC_TYPE_VESA3_75X_SINGLE));
}

void dsc_calculation(struct dsc_calc_info *dsc, uint32_t ifbc_type)
{
	dpu_pr_debug("+\n");

	dsc->adjustment_bits = (8 - (dsc->dsc_info.dsc_bpp * (dsc->dsc_info.slice_width + 1 - 1)) % 8) % 8;
	dsc->adj_bits_per_grp = dsc->dsc_info.dsc_bpp * 3 - 3;
	dsc->bits_per_grp = dsc->dsc_info.dsc_bpp * 3;
	dsc->slices_per_line = 0;

	if (dsc->dsc_info.native_422 == 0x1)
		dsc->pic_line_grp_num = (((dsc->dsc_info.slice_width + 1 - 1) >> 1) + 2) / 3 * (dsc->slices_per_line + 1) - 1;
	else
		dsc->pic_line_grp_num = ((dsc->dsc_info.slice_width + 3 - 1) / 3) * (dsc->slices_per_line + 1) - 1;

	dsc->dsc_insert_byte_num = 0;
	if (dsc->dsc_info.chunk_size % 6)
		dsc->dsc_insert_byte_num = (6 - dsc->dsc_info.chunk_size % 6);

	dsc->dual_dsc_en = is_ifbc_single_slice(ifbc_type) ? 0 : 1;
	dsc->dsc_en = (dsc->dual_dsc_en << 1) | 0x1;

	if ((dsc->dsc_info.native_422) == 0x1) {
		dsc->idata_422 = 0x1;
		dsc->dsc_info.convert_rgb = 0x0;
	} else {
		dsc->idata_422 = 0x0;
		dsc->dsc_info.convert_rgb = 0x1;
	}
}

/* calculate dsc1.2 output hsize */
static uint32_t get_hsize_after_spr_dsc(struct dsc_calc_info *dsc, uint32_t input_width, uint32_t panel_xres)
{
	uint32_t hsize = input_width;

	dpu_check_and_return((!dsc), hsize, err, "dsc is invalid\n");

	// no dsc+spr or dsc1.1 should directly return
	if (((dsc->dsc_en == 0) && (dsc->spr_en == 0)) ||
		((dsc->dsc_en != 0) && (dsc->dsc_info.dsc_version_minor == 1))) {
		dpu_pr_info("[dsc]no dsc+spr or dsc1.1, output_width=%d\n", hsize);
		return hsize;
	}

	dpu_check_and_return((dsc->dsc_info.dsc_bpc == 0), hsize, err, "dsc.bpc is 0\n");
	dpu_check_and_return((input_width == 0), hsize, err, "input_width is 0\n");
	dpu_check_and_return((panel_xres / input_width == 0), hsize, err,
		"xres = %d, input_width is %d \n", panel_xres, input_width);

	/*
	 * bits_per_component * 3: 3 means GPR888 have 3 component
	 * pinfo->vesa_dsc.bits_per_pixel / 2: 2 means YUV422 BPP nead plus 2 config.
	 */
	if ((dsc->dsc_en != 0) && (dsc->spr_en != 0)) { /* dsc1.2+spr */
		hsize = ((dsc->dsc_info.chunk_size + dsc->dsc_insert_byte_num) * (dsc->dual_dsc_en + 1)) * 8 / DSC_OUTPUT_MODE;
		hsize = hsize / (panel_xres / input_width);
	} else if (dsc->spr_en != 0) { /* only spr */
		hsize = input_width * 2 / 3;
	} else if (dsc->dsc_en != 0) { /* only dsc1.2 */
		hsize = (input_width * dsc->dsc_info.dsc_bpp) / (dsc->dsc_info.dsc_bpc * 3);
	}
	dpu_pr_info("[dsc]dsc1.2 or spr, output_width=%d\n", hsize);
	return hsize;
}

uint32_t get_dsc_out_width(struct dsc_calc_info *dsc, uint32_t ifbc_type, uint32_t input_width, uint32_t panel_xres)
{
	uint32_t xres_div;
	uint32_t output_width = input_width;

	dpu_check_and_return((!dsc), output_width, err, "dsc is invalid\n");
	dpu_check_and_return((ifbc_type >= IFBC_TYPE_MAX), output_width, err, "ifbc_type is invalid\n");

	xres_div = g_mipi_ifbc_division[ifbc_type].xres_div;

	if ((output_width % xres_div) > 0)
		dpu_pr_info("xres: %d is not division_h: %d pixel aligned!\n", output_width, xres_div);

	if ((dsc->dsc_info.dsc_bpc == DSC_10BPC) && (ifbc_type == IFBC_TYPE_VESA3X_DUAL)) {
		output_width = output_width * 30 / 24 / xres_div;
	} else if ((output_width % xres_div) > 0) {
		output_width = ((dsc->dsc_info.chunk_size + dsc->dsc_insert_byte_num) * (dsc->dual_dsc_en + 1)) / xres_div;
	} else {
		output_width /= xres_div;
	}

	/* SPR+DSC1.2 need special calulation for compression.
	 * It calucate the div in get_hsize_after_spr_dsc().
	 */
	return get_hsize_after_spr_dsc(dsc, output_width, panel_xres);
}

uint32_t get_dsc_out_height(struct dsc_calc_info *dsc, uint32_t ifbc_type, uint32_t input_height)
{
	uint32_t yres_div;
	uint32_t output_height = input_height;

	dpu_check_and_return((!dsc), output_height, err, "dsc is invalid\n");
	dpu_check_and_return((ifbc_type >= IFBC_TYPE_MAX), output_height, err, "ifbc_type is invalid\n");

	yres_div = g_mipi_ifbc_division[ifbc_type].yres_div;

	if ((output_height % yres_div) > 0)
		dpu_pr_info("yres: %d is not division_v: %d pixel aligned!\n", output_height, yres_div);

	output_height /= yres_div;
	dpu_pr_info("[dsc]output_height=%d\n", output_height);
	return output_height;
}