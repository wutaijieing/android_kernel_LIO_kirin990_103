/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "hisi_comp_utils.h"
#include "hisi_operator_tool.h"

/*******************************************************************************
 * DPU CSC
 */
#define CSC_ROW 3
#define CSC_COL 5
#define P3_PROCESS_NEEDED_LAYER (1U << 31);
static int g_csc_mode = 0;
struct dpu_csc_coe {
	int (*csc_coe_yuv2rgb)[CSC_COL];
	int (*csc_coe_rgb2yuv)[CSC_COL];
};

/*
 * [ p00 p01 p02 idc2 odc2 ]
 * [ p10 p11 p12 idc1 odc1 ]
 * [ p20 p21 p22 idc0 odc0 ]
 */

/* regular rgb<->yuv */
static int g_csc_coe_yuv2rgb601_wide[CSC_ROW][CSC_COL] = {
	{0x4000, 0x00000, 0x059ba, 0x000, 0x000},
	{0x4000, 0x1e9fa, 0x1d24c, 0x600, 0x000},
	{0x4000, 0x07168, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_rgb2yuv601_wide[CSC_ROW][CSC_COL] = {
	{0x01323, 0x02591, 0x0074c, 0x000, 0x000},
	{0x1f533, 0x1eacd, 0x02000, 0x000, 0x200},
	{0x02000, 0x1e534, 0x1facc, 0x000, 0x200},
};

static int g_csc_coe_yuv2rgb601_narrow[CSC_ROW][CSC_COL] = {
	{0x4a85, 0x00000, 0x06625, 0x7c0, 0x000},
	{0x4a85, 0x1e6ed, 0x1cbf8, 0x600, 0x000},
	{0x4a85, 0x0811a, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_rgb2yuv601_narrow[CSC_ROW][CSC_COL] = {
	{0x0106f, 0x02044, 0x00644, 0x000, 0x040},
	{0x1f684, 0x1ed60, 0x01c1c, 0x000, 0x200},
	{0x01c1c, 0x1e876, 0x1fb6e, 0x000, 0x200},
};

static int g_csc_coe_yuv2rgb709_wide[CSC_ROW][CSC_COL] = {
	{0x4000, 0x00000, 0x064ca, 0x000, 0x000},
	{0x4000, 0x1f403, 0x1e20a, 0x600, 0x000},
	{0x4000, 0x076c2, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_rgb2yuv709_wide[CSC_ROW][CSC_COL] = {
	{0x00d9b, 0x02dc6, 0x0049f, 0x000, 0x000},
	{0x1f8ab, 0x1e755, 0x02000, 0x000, 0x200},
	{0x02000, 0x1e2ef, 0x1fd11, 0x000, 0x200},
};

static int g_csc_coe_yuv2rgb709_narrow[CSC_ROW][CSC_COL] = {
	{0x4a85, 0x00000, 0x072bc, 0x7c0, 0x000},
	{0x4a85, 0x1f25a, 0x1dde5, 0x600, 0x000},
	{0x4a85, 0x08732, 0x00000, 0x600, 0x000},
};

static int g_csc_coe_rgb2yuv709_narrow[CSC_ROW][CSC_COL] = {
	{0x00baf, 0x02750, 0x003f8, 0x000, 0x040},
	{0x1f98f, 0x1ea55, 0x01c1c, 0x000, 0x200},
	{0x01c1c, 0x1e678, 0x1fd6c, 0x000, 0x200},
};

static int g_csc_coe_rgb2yuv2020[CSC_ROW][CSC_COL] = {
	{0x04000, 0x00000, 0x00000, 0x00000, 0x00000},
	{0x00000, 0x04000, 0x00000, 0x00600, 0x00000},
	{0x00000, 0x00000, 0x04000, 0x00600, 0x00000},
};

static int g_csc_coe_yuv2rgb2020[CSC_ROW][CSC_COL] = {
	{0x04A85, 0x00000, 0x06B6F, 0x007C0, 0x00000},
	{0x04A85, 0x1F402, 0x1D65F, 0x00600, 0x00000},
	{0x04A85, 0x08912, 0x00000, 0x00600, 0x00000},
};

/* RGB: srgb to P3 */
static int g_csc_coe_srgb2p3[CSC_ROW][CSC_COL] = {
	{0x034A3, 0x00B5D, 0x00000, 0x00000, 0x00000},
	{0x00220, 0x03DE0, 0x00000, 0x00000, 0x00000},
	{0x00118, 0x004A2, 0x03A46, 0x00000, 0x00000},
};

/* YUV:srgb <-> RGB:P3 */
static int g_csc_coe_yuv2p3601_narrow[CSC_ROW][CSC_COL] = {
	{0x4a85, 0x1fb8c, 0x04ac6, 0x7c0, 0x000},
	{0x4a85, 0x1e7c2, 0x1d116, 0x600, 0x000},
	{0x4a85, 0x073bc, 0x1fdfa, 0x600, 0x000},
};

static int g_csc_coe_yuv2p3601_wide[CSC_ROW][CSC_COL] = {
	{0x4000, 0x1fc17, 0x041af, 0x000, 0x000},
	{0x4000, 0x1e6cc, 0x1d6cb, 0x600, 0x000},
	{0x4000, 0x065aa, 0x1fe39, 0x600, 0x000},
};

static int g_csc_coe_yuv2p3709_narrow[CSC_ROW][CSC_COL] = {
	{0x4a85, 0x1fd94, 0x0584f, 0x7c0, 0x000},
	{0x4a85, 0x1f2ce, 0x1e2d6, 0x600, 0x000},
	{0x4a85, 0x07a1c, 0x1ff7e, 0x600, 0x000},
};

static int g_csc_coe_yuv2p3709_wide[CSC_ROW][CSC_COL] = {
	{0x4000, 0x1fddf, 0x04d93, 0x000, 0x000},
	{0x4000, 0x1f469, 0x1E661, 0x600, 0x000},
	{0x4000, 0x06b43, 0x1ff8d, 0x600, 0x000},
};

void dpu_wch_csc_init(const char __iomem *wch_base, struct dpu_csc *s_csc)
{
	if (!wch_base) {
		disp_pr_err("wch_base is NULL\n");
		return;
	}
	if (!s_csc) {
		disp_pr_err("s_csc is NULL\n");
		return;
	}

	memset(s_csc, 0, sizeof(struct dpu_csc));

	s_csc->idc0.value = inp32(DPU_WCH_CSC_IDC0_ADDR(wch_base));
	s_csc->idc2.value = inp32(DPU_WCH_CSC_IDC2_ADDR(wch_base));
	s_csc->odc0.value = inp32(DPU_WCH_CSC_ODC0_ADDR(wch_base));
	s_csc->odc2.value = inp32(DPU_WCH_CSC_ODC2_ADDR(wch_base));

	s_csc->p00.value = inp32(DPU_WCH_CSC_P00_ADDR(wch_base));
	s_csc->p01.value = inp32(DPU_WCH_CSC_P01_ADDR(wch_base));
	s_csc->p02.value = inp32(DPU_WCH_CSC_P02_ADDR(wch_base));
	s_csc->p10.value = inp32(DPU_WCH_CSC_P10_ADDR(wch_base));
	s_csc->p11.value = inp32(DPU_WCH_CSC_P11_ADDR(wch_base));
	s_csc->p12.value = inp32(DPU_WCH_CSC_P12_ADDR(wch_base));
	s_csc->p20.value = inp32(DPU_WCH_CSC_P20_ADDR(wch_base));
	s_csc->p21.value = inp32(DPU_WCH_CSC_P21_ADDR(wch_base));
	s_csc->p22.value = inp32(DPU_WCH_CSC_P22_ADDR(wch_base));
	s_csc->icg_module.value = inp32(DPU_WCH_CSC_ICG_MODULE_ADDR(wch_base));
}

void dpu_wch_pcsc_init(const char __iomem *wch_base, struct dpu_csc *s_csc)
{
	if (!wch_base) {
		disp_pr_err("wch_base is NULL\n");
		return;
	}
	if (!s_csc) {
		disp_pr_err("s_csc is NULL\n");
		return;
	}

	memset(s_csc, 0, sizeof(struct dpu_csc));

	/*lint -e529*/
	s_csc->idc0.value = inp32(DPU_WCH_PCSC_IDC0_ADDR(wch_base));
	s_csc->idc2.value = inp32(DPU_WCH_PCSC_IDC2_ADDR(wch_base));
	s_csc->odc0.value = inp32(DPU_WCH_PCSC_ODC0_ADDR(wch_base));
	s_csc->odc2.value = inp32(DPU_WCH_PCSC_ODC2_ADDR(wch_base));

	s_csc->p00.value = inp32(DPU_WCH_PCSC_P00_ADDR(wch_base));
	s_csc->p01.value = inp32(DPU_WCH_PCSC_P01_ADDR(wch_base));
	s_csc->p02.value = inp32(DPU_WCH_PCSC_P02_ADDR(wch_base));
	s_csc->p10.value = inp32(DPU_WCH_PCSC_P10_ADDR(wch_base));
	s_csc->p11.value = inp32(DPU_WCH_PCSC_P11_ADDR(wch_base));
	s_csc->p12.value = inp32(DPU_WCH_PCSC_P12_ADDR(wch_base));
	s_csc->p20.value = inp32(DPU_WCH_PCSC_P20_ADDR(wch_base));
	s_csc->p21.value = inp32(DPU_WCH_PCSC_P21_ADDR(wch_base));
	s_csc->p22.value = inp32(DPU_WCH_PCSC_P22_ADDR(wch_base));
	s_csc->icg_module.value = inp32(DPU_WCH_PCSC_ICG_MODULE_ADDR(wch_base));
	/*lint +e529*/
}


void dpu_wch_csc_set_reg(struct hisi_comp_operator *operator, char __iomem *wch_base, struct dpu_csc *s_csc)
{
	if (!wch_base) {
		disp_pr_err("wch_base is null\n");
		return;
	}

	if (!s_csc) {
		disp_pr_err("s_csc is null\n");
		return;
	}

    disp_pr_info("enter+++");
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_IDC0_ADDR(wch_base), s_csc->idc0.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_IDC2_ADDR(wch_base), s_csc->idc2.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_ODC0_ADDR(wch_base), s_csc->odc0.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_ODC2_ADDR(wch_base), s_csc->odc2.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_P00_ADDR(wch_base), s_csc->p00.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_P01_ADDR(wch_base), s_csc->p01.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_P02_ADDR(wch_base), s_csc->p02.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_P10_ADDR(wch_base), s_csc->p10.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_P11_ADDR(wch_base), s_csc->p11.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_P12_ADDR(wch_base), s_csc->p12.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_P20_ADDR(wch_base), s_csc->p20.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_P21_ADDR(wch_base), s_csc->p21.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_P22_ADDR(wch_base), s_csc->p22.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_CSC_ICG_MODULE_ADDR(wch_base), s_csc->icg_module.value);
    disp_pr_info("exit---");
}

void dpu_wch_pcsc_set_reg(struct hisi_comp_operator *operator, char __iomem *wch_base, struct dpu_csc *s_csc)
{
	if (!wch_base) {
		disp_pr_err("wch_base is null\n");
		return;
	}

	if (!s_csc) {
		disp_pr_err("s_csc is null\n");
		return;
	}
    disp_pr_info("enter+++");
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_IDC0_ADDR(wch_base), s_csc->idc0.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_IDC2_ADDR(wch_base), s_csc->idc2.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_ODC0_ADDR(wch_base), s_csc->odc0.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_ODC2_ADDR(wch_base), s_csc->odc2.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_P00_ADDR(wch_base), s_csc->p00.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_P01_ADDR(wch_base), s_csc->p01.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_P02_ADDR(wch_base), s_csc->p02.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_P10_ADDR(wch_base), s_csc->p10.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_P11_ADDR(wch_base), s_csc->p11.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_P12_ADDR(wch_base), s_csc->p12.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_P20_ADDR(wch_base), s_csc->p20.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_P21_ADDR(wch_base), s_csc->p21.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_P22_ADDR(wch_base), s_csc->p22.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_PCSC_ICG_MODULE_ADDR(wch_base), s_csc->icg_module.value);
    disp_pr_info("exit---");
}

static bool is_wb_pcsc_needed(struct disp_wb_layer *layer, uint32_t in_format)
{
	bool need_pcsc = false;
	bool need_scf =  layer->need_caps & CAP_SCL;

	if (is_yuv_format(in_format) && !is_yuv_format(layer->dst.format)) {
		need_pcsc = true;
	} else if (!is_yuv_format(in_format) && !is_yuv_format(layer->dst.format) && need_scf) {
		need_pcsc = true;
	}

    disp_pr_info("need_pcsc = %d", need_pcsc);
	return need_pcsc;
}

static bool is_wb_csc_needed(struct disp_wb_layer *layer, uint32_t in_format)
{
	bool need_csc = false;
	bool need_scf = layer->need_caps & CAP_SCL;

	if (layer->need_caps & CAP_UVUP)
		return false; /* uvup out is yuv */

	if (!is_yuv_format(in_format) && (is_yuv_format(layer->dst.format) || need_scf))
		need_csc = true;

	disp_pr_info("need csc = %d, scf = %d", need_csc, need_scf);

	return need_csc;
}

static void wch_csc_coef_array_config(struct dpu_csc *csc, int **p, int j)
{
	csc->icg_module.value = set_bits32(csc->icg_module.value, 0x1, 1, 0);
	/* The follow code get one by one array value to calculate csc coef value
	 * It contains lots of fixed numbers
	 */
	csc->idc0.value = set_bits32(csc->idc0.value, (uint32_t)(*((int *)p + 2 * j + 3)) |
		((uint32_t)(*((int *)p + 1 * j + 3)) << 16), 27, 0);
	csc->idc2.value = set_bits32(csc->idc2.value, *((int *)p + 0 * j + 3), 11, 0);

	csc->odc0.value = set_bits32(csc->odc0.value, (uint32_t)(*((int *)p + 2 * j + 4)) |
		((uint32_t)(*((int *)p + 1 * j + 4)) << 16), 27, 0);
	csc->odc2.value = set_bits32(csc->odc2.value, *((int *)p + 0 * j + 4), 11, 0);

	csc->p00.value = set_bits32(csc->p00.value, *((int *)p + 0 * j + 0), 17, 0);
	csc->p01.value = set_bits32(csc->p01.value, *((int *)p + 0 * j + 1), 17, 0);
	csc->p02.value = set_bits32(csc->p02.value, *((int *)p + 0 * j + 2), 17, 0);

	csc->p10.value = set_bits32(csc->p10.value, *((int *)p + 1 * j + 0), 17, 0);
	csc->p11.value = set_bits32(csc->p11.value, *((int *)p + 1 * j + 1), 17, 0);
	csc->p12.value = set_bits32(csc->p12.value, *((int *)p + 1 * j + 2), 17, 0);

	csc->p20.value = set_bits32(csc->p20.value, *((int *)p + 2 * j + 0), 17, 0);
	csc->p21.value = set_bits32(csc->p21.value, *((int *)p + 2 * j + 1), 17, 0);
	csc->p22.value = set_bits32(csc->p22.value, *((int *)p + 2 * j + 2), 17, 0);
}

static void wch_csc_get_regular_coef(uint32_t csc_mode, struct dpu_csc_coe *csc_coe)
{
	switch (csc_mode) {
	case DPU_CSC_601_WIDE:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb601_wide;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv601_wide;
		break;
	case DPU_CSC_601_NARROW:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb601_narrow;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv601_narrow;
		break;
	case DPU_CSC_709_WIDE:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb709_wide;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv709_wide;
		break;
	case DPU_CSC_709_NARROW:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb709_narrow;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv709_narrow;
		break;
	case DPU_CSC_2020:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb2020;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv2020;
		break;
	default:
		csc_coe->csc_coe_yuv2rgb = g_csc_coe_yuv2rgb601_narrow;
		csc_coe->csc_coe_rgb2yuv = g_csc_coe_rgb2yuv601_narrow;
		break;
	}
}

static void wch_csc_get_coef(struct hisi_composer_data *ov_data, struct disp_csc_info *csc_info,
	struct disp_wb_layer *wb_layer)
{
	struct dpu_csc *csc = NULL;
	struct dpu_csc_coe csc_coe;

	wch_csc_get_regular_coef(csc_info->csc_mode, &csc_coe);

	/* config wch csc pcsc */
	if (wb_layer) {
		if (ov_data->offline_module.csc_used[csc_info->chn_idx]) {
			csc = &(ov_data->offline_module.csc[csc_info->chn_idx]);
			wch_csc_coef_array_config(csc, (int **)(csc_coe.csc_coe_rgb2yuv), CSC_COL);
		}

		if (ov_data->offline_module.pcsc_used[csc_info->chn_idx]) {
			csc = &(ov_data->offline_module.pcsc[csc_info->chn_idx]);
			wch_csc_coef_array_config(csc, (int **)(csc_coe.csc_coe_yuv2rgb), CSC_COL);
		}
	}
}

int dpu_wch_csc_config(struct hisi_composer_data *ov_data,
	struct disp_wb_layer *wb_layer, uint32_t in_format)
{
	struct disp_csc_info csc_info;

	disp_check_and_return(!ov_data, -EINVAL, err, "ov_data is NULL\n");
	disp_check_and_return(!wb_layer, -EINVAL, err, "wb_layer is NULL\n");

	csc_info.chn_idx = wb_layer->wchn_idx;
	csc_info.format = wb_layer->dst.format;
	csc_info.csc_mode = g_csc_mode % DPU_CSC_MOD_MAX; // wb_layer->dst.csc_mode;
	g_csc_mode++;

	disp_pr_info("in_format = %d, csc_mode = %d", in_format, csc_info.csc_mode);
	if (is_wb_pcsc_needed(wb_layer, in_format) && csc_info.chn_idx == DPU_WCHN_W1) {
		ov_data->offline_module.pcsc_used[wb_layer->wchn_idx] = 1;
	}

	if (is_wb_csc_needed(wb_layer, in_format)) {
		ov_data->offline_module.csc_used[wb_layer->wchn_idx] = 1;
	}

	wch_csc_get_coef(ov_data, &csc_info, wb_layer);

	return 0;
}
