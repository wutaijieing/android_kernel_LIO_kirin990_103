/* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include "hisi_comp_utils.h"
#include "hisi_disp_gadgets.h"
#include "hisi_composer_operator.h"
#include "dpu_offline_define.h"
#include "hisi_operator_tool.h"

int dpu_wdfc_dither_init(const char __iomem *dither_base, struct dpu_wdfc_dither *s_dither)
{
	if (!dither_base) {
		disp_pr_err("wch_base is NULL\n");
		return -1;
	}

	if (!dither_base) {
		disp_pr_err("s_dither is NULL\n");
		return -1;
	}

	memset(s_dither, 0, sizeof(struct dpu_wdfc_dither));

    s_dither->ctl1.value = inp32(DPU_WCH_DITHER_CTL1_ADDR(dither_base));
	s_dither->tri_thd10.value = inp32(DPU_WCH_DITHER_TRI_THD10_ADDR(dither_base));
	s_dither->tri_thd10_uni.value = inp32(DPU_WCH_DITHER_TRI_THD10_UNI_ADDR(dither_base));
	s_dither->bayer_ctl.value = inp32(DPU_WCH_BAYER_CTL_ADDR(dither_base));
	s_dither->bayer_matrix_part1.value = inp32(DPU_WCH_BAYER_MATRIX_PART1_ADDR(dither_base));
	s_dither->bayer_matrix_part0.value = inp32(DPU_WCH_BAYER_MATRIX_PART0_ADDR(dither_base));

	return 0;
}

static bool is_wdfc_dither_needed(struct disp_wb_layer *layer, uint32_t in_format)
{
	bool need_dither = false;

	if (is_10bit(in_format) && is_8bit(layer->dst.format)) {
        disp_pr_info("need wdfc dither");
		need_dither = true;
	}
    disp_pr_info("no need wdfc dither");
	return need_dither;
}

int dpu_wdfc_dither_config(struct post_offline_info *offline_info, struct hisi_composer_data *ov_data,
	struct disp_wb_layer *layer, uint32_t in_format)
{
	struct dpu_wdfc_dither *dither = NULL;
	int ret;

	disp_check_and_return((offline_info == NULL || layer == NULL), -EINVAL, err, "NULL ptr.\n");
	disp_pr_info("enter+++");

	dither = &(ov_data->offline_module.dither[layer->wchn_idx]);

    if (is_wdfc_dither_needed(layer, in_format)) {
        ov_data->offline_module.wdfc_dither_used[layer->wchn_idx] = 1;
    }

	/* en_dither and  add high frequency noise to 10bits input */
    dither->ctl1.value = set_bits32(dither->ctl1.value, 0x3, 6, 0);
	/* default value threshold of trigger dither */
	dither->tri_thd10.value = set_bits32(dither->tri_thd10.value, 0x02008020, 30, 0);
	/* default value threshold of GB channel */
	dither->tri_thd10_uni.value = set_bits32(dither->tri_thd10_uni.value, 0x00010040, 30, 0);
	/* bayer_offset */
	dither->bayer_ctl.value = set_bits32(dither->bayer_ctl.value, 0x0, 28, 0);

	/* default value bayer algorithm parameters */
	dither->bayer_matrix_part1.value = set_bits32(dither->bayer_matrix_part1.value, 0x5D7F91B3, 32, 0);
	/* default value bayer algorithm parameters */
	dither->bayer_matrix_part0.value = set_bits32(dither->bayer_matrix_part0.value, 0x6E4CA280, 32, 0);
	disp_pr_info("exit---");
	return 0;
}

void dpu_wdfc_dither_set_reg(struct hisi_comp_operator *operator, char __iomem *wch_base, struct dpu_wdfc_dither *s_dither)
{
	disp_check_and_no_retval((wch_base == NULL), err, "wch_base is NULL!\n");
	disp_check_and_no_retval((s_dither == NULL), err, "s_dither is NULL!\n");
	disp_pr_info("enter++++");

	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DITHER_CTL1_ADDR(wch_base), s_dither->ctl1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DITHER_TRI_THD10_ADDR(wch_base), s_dither->tri_thd10.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_DITHER_TRI_THD10_UNI_ADDR(wch_base), s_dither->tri_thd10_uni.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_BAYER_CTL_ADDR(wch_base), s_dither->bayer_ctl.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_BAYER_MATRIX_PART1_ADDR(wch_base), s_dither->bayer_matrix_part1.value);
	hisi_module_set_reg(&operator->module_desc, DPU_WCH_BAYER_MATRIX_PART0_ADDR(wch_base), s_dither->bayer_matrix_part0.value);
	disp_pr_info("exit----");
}
