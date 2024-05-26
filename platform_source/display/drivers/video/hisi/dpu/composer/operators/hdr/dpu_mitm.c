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

#include "dpu_hdr_def.h"
#include "dpu_mitm.h"
#include "dm/hisi_dm.h"
#include "hisi_dpu_module.h"
#include "hisi_disp_debug.h"
#include "hisi_disp.h"
#include "hisi_operator_tool.h"
#include <securec.h>
#include <linux/moduleparam.h>

static void dpu_mitm_set_reg(struct dpu_module_desc *module_desc, char __iomem *reg_base,
	uint32_t idx, struct wcg_mitm_info *mitm_info)
{
	int i;
	int y1 = 0;
	int y2 = 0;
	uint32_t temp_data = 0;

	disp_pr_debug("[MITM] idx %d \n", idx);

	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_DEGAMMA_STEP1_ADDR(reg_base, idx), mitm_info->itm_slf_degamma_step1);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_DEGAMMA_STEP2_ADDR(reg_base, idx), mitm_info->itm_slf_degamma_step2);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_DEGAMMA_POS1_ADDR(reg_base, idx), mitm_info->itm_slf_degamma_pos1);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_DEGAMMA_POS2_ADDR(reg_base, idx), mitm_info->itm_slf_degamma_pos2);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_DEGAMMA_POS3_ADDR(reg_base, idx), mitm_info->itm_slf_degamma_pos3);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_DEGAMMA_POS4_ADDR(reg_base, idx), mitm_info->itm_slf_degamma_pos4);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_DEGAMMA_NUM1_ADDR(reg_base, idx), mitm_info->itm_slf_degamma_num1);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_DEGAMMA_NUM2_ADDR(reg_base, idx), mitm_info->itm_slf_degamma_num2);

	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_COEF00_0_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_coef00);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_COEF01_0_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_coef01);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_COEF02_0_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_coef02);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_COEF10_0_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_coef10);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_COEF11_0_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_coef11);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_COEF12_0_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_coef12);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_COEF20_0_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_coef20);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_COEF21_0_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_coef21);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_COEF22_0_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_coef22);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_SCALE_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_scale);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMUT_CLIP_MAX_ADDR(reg_base, idx), mitm_info->itm_slf_gamut_clip_max);

	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_STEP1_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_step1);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_STEP2_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_step2);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_POS1_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_pos1);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_POS2_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_pos2);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_POS3_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_pos3);
	hisi_module_set_reg(module_desc, 
		DPU_RCH_OV_MITM_SLF_GAMMA_POS4_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_pos4);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_POS5_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_pos5);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_POS6_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_pos6);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_POS7_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_pos7);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_POS8_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_pos8);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_NUM1_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_num1);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_SLF_GAMMA_NUM2_ADDR(reg_base, idx), mitm_info->itm_slf_gamma_num2);
	hisi_module_set_reg(module_desc,
		DPU_RCH_OV_MITM_ALPHA_ADDR(reg_base, idx), 0x2000000);

	disp_pr_debug(" [mitm] set degamma \n");
	for(i = 0; i < MITM_LUT_GAMMA_SIZE/2; i++) {
		temp_data = mitm_info->itm_degamma_lut[i*2];
		temp_data = temp_data + (mitm_info->itm_degamma_lut[i*2+1] <<16);
		hisi_module_set_reg(module_desc, DPU_RCH_OV_MITM_DEGAMMA_LUT_ADDR(reg_base, y1, idx), temp_data);
		y1++;
	}

	disp_pr_debug(" [mitm] set gamma \n");

	for(i = 0; i < MITM_LUT_GAMMA_SIZE/2; i++) {
		temp_data = mitm_info->itm_gamma_lut[i*2];
		temp_data = temp_data + (mitm_info->itm_gamma_lut[i*2+1] <<16);
		hisi_module_set_reg(module_desc, DPU_RCH_OV_MITM_GAMMA_LUT_ADDR(reg_base, y2,  idx), temp_data);
		y2++;
	}
	disp_pr_debug("[mitm]  ------ \n");
}

void dpu_set_dm_mitem_reg(struct dpu_module_desc *module_desc,
	char __iomem *dpu_base,
	struct wcg_mitm_info *mitm_param)
{
	char __iomem *ov_base = NULL;

	ov_base = dpu_base + DSS_RCH_OV_OFFSET;
	disp_pr_debug(" ++++ \n");

	if (hisi_fb_display_effect_test == 1 ||
		hisi_fb_display_effect_test == 3 ||
		hisi_fb_display_effect_test == 5 ||
		hisi_fb_display_effect_test == 7) {
		dpu_mitm_set_reg(module_desc, ov_base, 0,
			mitm_param);
		disp_pr_debug(" ---00-- \n");
	} else if (hisi_fb_display_effect_test == 2 ||
		hisi_fb_display_effect_test == 4 ||
		hisi_fb_display_effect_test == 8 ||
		hisi_fb_display_effect_test == 10) {
		dpu_mitm_set_reg(module_desc, ov_base, 1,
			mitm_param);
		disp_pr_debug(" ---11-- \n");
	} else {
		disp_pr_debug(" ---00 -- 11-- \n");
		dpu_mitm_set_reg(module_desc, ov_base, 0,
			mitm_param);
		dpu_mitm_set_reg(module_desc, ov_base, 1,
			mitm_param);
	}
}

void dpu_ov_mitm_sel(struct pipeline_src_layer *layer, struct hisi_dm_layer_info *layer_info)
{
	disp_pr_debug(" hisi_fb_display_effect_test:%u \n", hisi_fb_display_effect_test);

	/* set mitm according to layer dataspace and hdr policy */
	// [0]:mitm_en              [1]:reserved                  [2]:mitm_slf_en
	// [3]:mitm_slf_degmm_en    [4]:mitm_slf_gamut_en     [5]:mitm_slf_gmm_en
	if(1 == hisi_fb_display_effect_test) {
		/* gamma disbale gamut disable, degamma use mitm0 */
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0xd;
		layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel =
		0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel =
		0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel =
		0;
	} else if (hisi_fb_display_effect_test == 2) {
		/* gamma disbale, gamut disable, degamma use mitm1 */
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0xd;
		layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel =
		1;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel =
		0; // means use mitm1 gammalut
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel =
		0; // means use mitm0 gamma
	} else if (hisi_fb_display_effect_test == 3) {
		/*  gamut disable,  degamma disbale, gamma use mitm0 */
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x25;
		layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel = 0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel = 0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel = 0;
	} else if (hisi_fb_display_effect_test == 4){
		/* gamut disable,  degamma disbale, gamma use mitm1 */
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x25;
		layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel =
		0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel =
		0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel =
		1;
	} else if (hisi_fb_display_effect_test == 5){
		/* all enable mitm 0 */
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x3d;
		layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel =
		0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel =
		0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel =
		0;
	} else if (hisi_fb_display_effect_test == 6){
		/* all enable mitm 1 */
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x3d;
		layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel = 1;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel = 1;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel = 1;
	} else if (hisi_fb_display_effect_test == 7){
		/* gamut enable  only mitm 0 */
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x15;
		layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel = 0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel = 0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel = 0;
	} else if (hisi_fb_display_effect_test == 8){
		/* gamut enable  only mitm1 */
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x15;
		layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel = 0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel = 1;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel = 0;
	} else if (hisi_fb_display_effect_test == 0x9) {
		if(layer->layer_id % 2) {
			/* all enable */
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x3d;
			layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel = 1;
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel = 0;
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel = 1;
		  } else {
			/* all enable */
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x3d;
			layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel = 0;
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel = 0;
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel = 0;
		  }
	} else if (hisi_fb_display_effect_test == 0xa) {
		if(layer->layer_id == 0 ||layer->layer_id == 1 ) {
			/* all enable */
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x3d;
			layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel = 0;
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel = 0;
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel = 0;
		} else {
			/* all enable */
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0x3d;
			layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel = 1;
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel = 1;
			layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel = 1;
		}
	} else {
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_en = 0;
		layer_info->layer_ov_dfc_cfg_union.reg.layer_ov_mitm_degamma_reg_sel = 0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gamut_reg_sel = 0;
		layer_info->layer_ov_pattern_a_union.reg.layer_ov_mitm_gama_reg_sel = 0;
	}
}