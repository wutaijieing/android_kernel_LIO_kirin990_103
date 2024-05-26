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

#include <linux/types.h>
#include <linux/dma-mapping.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_disp_composer.h"
#include "hisi_dpu_module.h"
#include "hisi_operators_manager.h"

#include "scl/arsr/dpu_operator_arsr.h"
#include "scl/dpu_scl.h"
#include "widget/dpu_csc.h"
#include "widget/dpu_dfc.h"
#include "widget/dpu_post_clip.h"
#include "widget/dpu_scf.h"
#include "widget/dpu_scf_lut.h"
#include "hisi_operator_tool.h"

#define ARSR_SCALE_H_V_INC_OFFSET     65536
#define ARSR_SCALE_DOWN_V_INC_OFFSET  32768    // 65536 / 2
#define ARSR_SCALE_INC(src, dst, offset) (((src) * 65536 + (offset)) / (dst))

#define ARSR2P_PHASE_NUM 9
#define ARSR2P_TAP4 4
#define ARSR2P_TAP6 6
static int arsr_post_en = 0;
struct dpu_arsr_data {
	char __iomem *base;
	struct disp_rect src_rect;
	struct disp_rect dst_rect;

	union operator_id arsr_id;
};

union arsr_lut_tap6 {
	struct {
		int32_t c0 : 9;
		int32_t c1 : 9;
		int32_t c2 : 9;
		int32_t    : 5;
	} reg;
	uint32_t value;
};

union arsr_lut_tap4 {
	struct  {
		int32_t c0 : 9;
		int32_t c1 : 9;
		int32_t    : 14;
	} reg;
	uint32_t value;
};

#define set_tap6_coef(v0, v1, v2) {.reg = {.c0 = v0, .c1 = v1, .c2 = v2}}
#define set_tap4_coef(v0, v1    ) {.reg = {.c0 = v0, .c1 = v1}}

struct _arsr_reg {
	DPU_ARSR_INPUT_WIDTH_HEIGHT_UNION src;
	DPU_ARSR_OUTPUT_WIDTH_HEIGHT_UNION dst;
	DPU_ARSR_IHLEFT1_UNION h_left;
	DPU_ARSR_IHRIGHT_UNION h_right;
	DPU_ARSR_IVTOP_UNION v_top;
	DPU_ARSR_IVBOTTOM_UNION v_bottom;
	DPU_ARSR_IHINC_UNION h_inc;
	DPU_ARSR_IVINC_UNION v_inc;
	DPU_ARSR_MODE_UNION mode;
};

/* c0, c1, c2, c3 */
static union arsr_lut_tap4 g_coef_tap4[] = {
	/* scl down */
	set_tap4_coef(31, 194), set_tap4_coef(31, 0),
	set_tap4_coef(23, 206), set_tap4_coef(44, -17),
	set_tap4_coef(14, 203), set_tap4_coef(57, -18),
	set_tap4_coef(6, 198),  set_tap4_coef(70, -18),
	set_tap4_coef(0, 190), set_tap4_coef(85, -19),
	set_tap4_coef(-5, 180), set_tap4_coef(99, -18),
	set_tap4_coef(-10, 170), set_tap4_coef(114, -18),
	set_tap4_coef(-13, 157), set_tap4_coef(129, -17),
	set_tap4_coef(-15, 143), set_tap4_coef(143, -15),

	/* scl up */
	set_tap4_coef(-3, 254), set_tap4_coef(6, -1),
	set_tap4_coef(-9, 255), set_tap4_coef(13, -3),
	set_tap4_coef(-18, 254), set_tap4_coef(27, -7),
	set_tap4_coef(-23, 245),  set_tap4_coef(44, -10),
	set_tap4_coef(-27, 233), set_tap4_coef(64, -14),
	set_tap4_coef(-29, 218), set_tap4_coef(85, -18),
	set_tap4_coef(-29, 198), set_tap4_coef(108, -21),
	set_tap4_coef(-29, 177), set_tap4_coef(132, -24),
	set_tap4_coef(-27, 155), set_tap4_coef(155, -27),
};

/* c0, c1, c2, c3, c4, c5 */
static union arsr_lut_tap6 g_coef_tap6[] = {
	/* scl down */
	set_tap6_coef(-22, 43, 214), set_tap6_coef(43, -22, 0),
	set_tap6_coef(-18, 29, 205), set_tap6_coef(53, -23, 10),
	set_tap6_coef(-16, 18, 203), set_tap6_coef(67, -25, 9),
	set_tap6_coef(-13, 9, 198), set_tap6_coef(80, -26, 8),
	set_tap6_coef(-10, 0, 191), set_tap6_coef(95, -27, 7),
	set_tap6_coef(-7, -7, 182), set_tap6_coef(109, -27, 6),
	set_tap6_coef(-5, -14, 174), set_tap6_coef(124, -27, 4),
	set_tap6_coef(-2, -18, 162), set_tap6_coef(137, -25, 2),
	set_tap6_coef(0, -22, 150), set_tap6_coef(150, -22, 0),

	/* scl up */
	set_tap6_coef(0, -3, 254), set_tap6_coef(6, -1, 0),
	set_tap6_coef(4, -12, 252), set_tap6_coef(15, -5, 2),
	set_tap6_coef(7, -22, 245), set_tap6_coef(31, -9, 4),
	set_tap6_coef(10, -29, 234), set_tap6_coef(49, -14, 6),
	set_tap6_coef(12, -34, 221), set_tap6_coef(68, -19, 8),
	set_tap6_coef(13, -37, 206), set_tap6_coef(88, -24, 10),
	set_tap6_coef(14, -38, 189), set_tap6_coef(108, -29, 12),
	set_tap6_coef(14, -38, 170), set_tap6_coef(130, -33, 13),
	set_tap6_coef(14, -36, 150), set_tap6_coef(150, -36, 14),
};

static uint32_t g_arsr_offset[] = {
	DPU_ARSR0_OFFSET, DPU_ARSR1_OFFSET
};

/***** arsr ********/
unsigned hisi_arsr_effect_en = 0;
module_param_named(arsr_effect_en, hisi_arsr_effect_en, int, 0644);
MODULE_PARM_DESC(arsr_effect_en, "hisi_arsr_effect_en");

unsigned hisi_fb_arsr_post_mode = 0x3e;
module_param_named(debug_arsr_post_mode, hisi_fb_arsr_post_mode, int, 0644);
MODULE_PARM_DESC(debug_arsr_post_mode, "hisi_fb_arsr_post_mode");

unsigned hisi_fb_arsr_pre_mode = 0x1;
module_param_named(debug_arsr_pre_mode, hisi_fb_arsr_pre_mode, int, 0644);
MODULE_PARM_DESC(debug_arsr_pre_mode, "hisi_fb_arsr_pre_mode");

unsigned hisi_fb_arsr_thres_y = (300 | (332 << 10) | (600 << 20));
module_param_named(debug_arsr_thres_y, hisi_fb_arsr_thres_y, int, 0644);
MODULE_PARM_DESC(debug_arsr_ta, "hisi_fb_arsr_thres_y");

unsigned hisi_fb_arsr_thres_u = (20 | (40 << 10) | (452 << 20));
module_param_named(debug_arsr_thres_u, hisi_fb_arsr_thres_u, int, 0644);
MODULE_PARM_DESC(debug_arsr_ta, "hisi_fb_arsr_thres_u");

unsigned hisi_fb_arsr_thres_v = (24 | (48 << 10) | (580 << 20));
module_param_named(debug_arsr_thres_v, hisi_fb_arsr_thres_v, int, 0644);
MODULE_PARM_DESC(debug_arsr_ta, "hisi_fb_arsr_thres_v");

unsigned hisi_fb_arsr_skin_cfg0 = (512 | (12 << 13));
module_param_named(debug_arsr_skin_cfg0, hisi_fb_arsr_skin_cfg0, int, 0644);
MODULE_PARM_DESC(debug_arsr_skin_cfg0, "hisi_fb_arsr_skin_cfg0");

unsigned hisi_fb_arsr_skin_cfg1 = (819);
module_param_named(debug_arsr_skin_cfg1, hisi_fb_arsr_skin_cfg1, int, 0644);
MODULE_PARM_DESC(debug_arsr_skin_cfg1, "hisi_fb_arsr_skin_cfg1");

unsigned hisi_fb_arsr_skin_cfg2 = (682);
module_param_named(debug_arsr_skin_cfg2, hisi_fb_arsr_skin_cfg2, int, 0644);
MODULE_PARM_DESC(debug_arsr_skin_cfg2, "hisi_fb_arsr_skin_cfg2");

unsigned hisi_fb_arsr_shoot_cfg1 = (372 | (20 << 16));
module_param_named(debug_arsr_shoot_cfg1, hisi_fb_arsr_shoot_cfg1, int, 0644);
MODULE_PARM_DESC(debug_arsr_shoot_cfg1, "hisi_fb_arsr_shoot_cfg1");

unsigned hisi_fb_arsr_shoot_cfg2 = ((8 << 16) | (-80 & 0x7ff));
module_param_named(debug_arsr_shoot_cfg2, hisi_fb_arsr_shoot_cfg2, int, 0644);
MODULE_PARM_DESC(debug_arsr_shoot_cfg2, "hisi_fb_arsr_shoot_cfg2");

unsigned hisi_fb_arsr_shoot_cfg3 = 15;
module_param_named(debug_arsr_shoot_cfg3, hisi_fb_arsr_shoot_cfg3, int, 0644);
MODULE_PARM_DESC(debug_arsr_shoot_cfg3, "hisi_fb_arsr_shoot_cfg3");

unsigned hisi_fb_arsr_sharp_cfg3 = (128 | (192 << 16));
module_param_named(debug_arsr_sharp_cfg3, hisi_fb_arsr_sharp_cfg3, int, 0644);
MODULE_PARM_DESC(debug_arsr_sharp_cfg3, "hisi_fb_arsr_sharp_cfg3");

unsigned hisi_fb_arsr_sharp_cfg4 = (40 | (160 << 16));
module_param_named(debug_arsr_sharp_cfg4, hisi_fb_arsr_sharp_cfg4, int, 0644);
MODULE_PARM_DESC(debug_arsr_sharp_cfg4, "hisi_fb_arsr_sharp_cfg4");

unsigned hisi_fb_arsr_sharp_cfg5 = 0;
module_param_named(debug_arsr_sharp_cfg5, hisi_fb_arsr_sharp_cfg5, int, 0644);
MODULE_PARM_DESC(debug_arsr_sharp_cfg5, "hisi_fb_arsr_sharp_cfg5");

unsigned hisi_fb_arsr_sharp_cfg6 = (8 | (8 << 16));
module_param_named(debug_arsr_sharp_cfg6, hisi_fb_arsr_sharp_cfg6, int, 0644);
MODULE_PARM_DESC(debug_arsr_sharp_cfg6, "hisi_fb_arsr_sharp_cfg6");

unsigned hisi_fb_arsr_sharp_cfg7 = (10 | (6 << 8));
module_param_named(debug_arsr_sharp_cfg7, hisi_fb_arsr_sharp_cfg7, int, 0644);
MODULE_PARM_DESC(debug_arsr_sharp_cfg7, "hisi_fb_arsr_sharp_cfg7");

unsigned hisi_fb_arsr_sharp_cfg8 = (16 | (160 << 16));
module_param_named(debug_arsr_sharp_cfg8, hisi_fb_arsr_sharp_cfg8, int, 0644);
MODULE_PARM_DESC(debug_arsr_sharp_cfg8, "hisi_fb_arsr_sharp_cfg8");

unsigned hisi_fb_arsr_sh_level = 0x20002;
module_param_named(debug_arsr_sh_level, hisi_fb_arsr_sh_level, int, 0644);
MODULE_PARM_DESC(debug_arsr_sh_level, "hisi_fb_arsr_sh_level");

unsigned hisi_fb_arsr_gain_low = 0x3C0078;
module_param_named(debug_arsr_gain_low, hisi_fb_arsr_gain_low, int, 0644);
MODULE_PARM_DESC(debug_arsr_gain_low, "hisi_fb_arsr_gain_low");

unsigned hisi_fb_arsr_gain_mid = 0x6400C8;
module_param_named(debug_arsr_gain_mid, hisi_fb_arsr_gain_mid, int, 0644);
MODULE_PARM_DESC(debug_arsr_gain_mid, "hisi_fb_arsr_gain_mid");

unsigned hisi_fb_arsr_gain_high = 0x5000A0;
module_param_named(debug_arsr_gain_high, hisi_fb_arsr_gain_high, int, 0644);
MODULE_PARM_DESC(debug_arsr_gain_high, "hisi_fb_arsr_gain_high");

unsigned hisi_fb_arsr_sloph_mf = 0x280;
module_param_named(debug_arsr_sloph_mf, hisi_fb_arsr_sloph_mf, int, 0644);
MODULE_PARM_DESC(debug_arsr_sloph_mf, "hisi_fb_arsr_sloph_mf");

unsigned hisi_fb_arsr_slopl_mf = 0x1400;
module_param_named(debug_arsr_slopl_mf, hisi_fb_arsr_slopl_mf, int, 0644);
MODULE_PARM_DESC(debug_arsr_slopl_mf, "hisi_fb_arsr_slopl_mf");

unsigned hisi_fb_arsr_sloph_hf = 0x140;
module_param_named(debug_arsr_sloph_hf, hisi_fb_arsr_sloph_hf, int, 0644);
MODULE_PARM_DESC(debug_arsr_sloph_hf, "hisi_fb_arsr_sloph_hf");

unsigned hisi_fb_arsr_slopl_hf = 0xA00;
module_param_named(debug_arsr_slopl_hf, hisi_fb_arsr_slopl_hf, int, 0644);
MODULE_PARM_DESC(debug_arsr_slopl_hf, "hisi_fb_arsr_slopl_hf");

unsigned hisi_fb_arsr_mf_lmt = 0x40;
module_param_named(debug_arsr_mf_lmt, hisi_fb_arsr_mf_lmt, int, 0644);
MODULE_PARM_DESC(debug_arsr_mf_lmt, "hisi_fb_arsr_mf_lmt");

unsigned hisi_fb_arsr_gain_mf =  0x12C012C;
module_param_named(debug_arsr_gain_mf, hisi_fb_arsr_gain_mf, int, 0644);
MODULE_PARM_DESC(debug_arsr_gain_mf, "hisi_fb_arsr_gain_mf");

unsigned hisi_fb_arsr_mf_b = 0;
module_param_named(debug_arsr_mf_b, hisi_fb_arsr_mf_b, int, 0644);
MODULE_PARM_DESC(debug_arsr_mf_b, "hisi_fb_arsr_mf_b");

unsigned hisi_fb_arsr_hf_lmt = 0x80;
module_param_named(debug_arsr_hf_lmt, hisi_fb_arsr_hf_lmt, int, 0644);
MODULE_PARM_DESC(debug_arsr_hf_lmt, "hisi_fb_arsr_hf_lmt");

unsigned hisi_fb_arsr_gain_hf = 0x104012C;
module_param_named(debug_arsr_gain_hf, hisi_fb_arsr_gain_hf, int, 0644);
MODULE_PARM_DESC(debug_arsr_gain_hf, "hisi_fb_arsr_gain_hf");

unsigned hisi_fb_arsr_hf_b = 0x1400;
module_param_named(debug_arsr_hf_b, hisi_fb_arsr_hf_b, int, 0644);
MODULE_PARM_DESC(debug_arsr_hf_b, "hisi_fb_arsr_hf_b");

unsigned hisi_fb_arsr_lf_ctrl = 0x100010;
module_param_named(debug_arsr_lf_ctrl, hisi_fb_arsr_lf_ctrl, int, 0644);
MODULE_PARM_DESC(debug_arsr_lf_ctrl, "hisi_fb_arsr_lf_ctrl");

unsigned hisi_fb_arsr_lf_var = 0x1800080;
module_param_named(debug_arsr_lf_var, hisi_fb_arsr_lf_var, int, 0644);
MODULE_PARM_DESC(debug_arsr_lf_var, "hisi_fb_arsr_lf_var");

unsigned hisi_fb_arsr_lf_ctrl_slop = 0;
module_param_named(debug_arsr_lf_ctrl_slop, hisi_fb_arsr_lf_ctrl_slop, int, 0644);
MODULE_PARM_DESC(debug_arsr_lf_ctrl_slop, "hisi_fb_arsr_lf_ctrl_slop");

unsigned hisi_fb_arsr_hf_select = 0;
module_param_named(debug_arsr_hf_select, hisi_fb_arsr_hf_select, int, 0644);
MODULE_PARM_DESC(debug_arsr_hf_select, "hisi_fb_arsr_hf_select");

unsigned hisi_fb_arsr_shar_cfg2_h = 0x10000C0;
module_param_named(debug_arsr_shar_cfg2_h, hisi_fb_arsr_shar_cfg2_h, int, 0644);
MODULE_PARM_DESC(debug_arsr_shar_cfg2_h, "hisi_fb_arsr_shar_cfg2_h");

unsigned hisi_fb_arsr_shar_cfg2_l = 0x200010;
module_param_named(debug_arsr_shar_cfg2_l, hisi_fb_arsr_shar_cfg2_l, int, 0644);
MODULE_PARM_DESC(debug_arsr_shar_cfg2_l, "hisi_fb_arsr_shar_cfg2_l");

unsigned hisi_fb_arsr_texture_analysys = (64 | (80 << 16));
module_param_named(debug_arsr_texture_analysys, hisi_fb_arsr_texture_analysys, int, 0644);
MODULE_PARM_DESC(debug_arsr_texture_analysys, "hisi_fb_arsr_texture_analysys");

unsigned hisi_fb_arsr_intplshootctrl = (8);
module_param_named(debug_arsr_intplshootctrl, hisi_fb_arsr_intplshootctrl, int, 0644);
MODULE_PARM_DESC(debug_arsr_intplshootctrl, "hisi_fb_arsr_intplshootctrl");

static void _arsr_effect_test(char __iomem * arsr_base)
{
	disp_pr_debug("[effect] hisi_fb_arsr_pre_mode=0x%x, arsr_base = %p\n", hisi_fb_arsr_pre_mode, arsr_base);
	dpu_set_reg(DPU_ARSR_MODE_ADDR(arsr_base), hisi_fb_arsr_pre_mode, 32, 0);
	dpu_set_reg(DPU_ARSR_SKIN_THRES_Y_ADDR (arsr_base), hisi_fb_arsr_thres_y, 32, 0);
	dpu_set_reg(DPU_ARSR_SKIN_THRES_U_ADDR (arsr_base) , hisi_fb_arsr_thres_u, 32, 0);
	dpu_set_reg(DPU_ARSR_SKIN_THRES_V_ADDR (arsr_base), hisi_fb_arsr_thres_v, 32, 0);
	dpu_set_reg(DPU_ARSR_SKIN_CFG0_ADDR(arsr_base), hisi_fb_arsr_skin_cfg0, 32, 0);
	dpu_set_reg(DPU_ARSR_SKIN_CFG1_ADDR(arsr_base), hisi_fb_arsr_skin_cfg1, 32, 0);
	dpu_set_reg(DPU_ARSR_SKIN_CFG2_ADDR(arsr_base), hisi_fb_arsr_skin_cfg2, 32, 0);
	dpu_set_reg(DPU_ARSR_SHOOT_CFG1_ADDR(arsr_base), hisi_fb_arsr_shoot_cfg1, 32, 0);
	dpu_set_reg(DPU_ARSR_SHOOT_CFG2_ADDR(arsr_base), hisi_fb_arsr_shoot_cfg2, 32, 0);
	dpu_set_reg(DPU_ARSR_SHOOT_CFG3_ADDR(arsr_base), hisi_fb_arsr_shoot_cfg3, 32, 0);
	dpu_set_reg(DPU_ARSR_SHARP_CFG3_ADDR(arsr_base), hisi_fb_arsr_sharp_cfg3, 32, 0);
	dpu_set_reg(DPU_ARSR_SHARP_CFG4_ADDR(arsr_base), hisi_fb_arsr_sharp_cfg4, 32, 0);
	dpu_set_reg(DPU_ARSR_SHARP_CFG6_ADDR(arsr_base), hisi_fb_arsr_sharp_cfg6, 32, 0);
	dpu_set_reg(DPU_ARSR_SHARP_CFG7_ADDR(arsr_base), hisi_fb_arsr_sharp_cfg7, 32, 0);
	dpu_set_reg(DPU_ARSR_SHARP_CFG8_ADDR(arsr_base), hisi_fb_arsr_sharp_cfg8, 32, 0);
	dpu_set_reg(DPU_ARSR_SHARPLEVEL_ADDR(arsr_base), hisi_fb_arsr_sh_level, 32, 0);
	dpu_set_reg(DPU_ARSR_SHPGAIN_LOW_ADDR(arsr_base), hisi_fb_arsr_gain_low, 32, 0);
	dpu_set_reg(DPU_ARSR_SHPGAIN_MID_ADDR(arsr_base), hisi_fb_arsr_gain_mid, 32, 0);
	dpu_set_reg(DPU_ARSR_SHPGAIN_HIGH_ADDR(arsr_base), hisi_fb_arsr_gain_high, 32, 0);
	dpu_set_reg(DPU_ARSR_GAINCTRLSLOPH_MF_ADDR(arsr_base), hisi_fb_arsr_sloph_mf, 32, 0);
	dpu_set_reg(DPU_ARSR_GAINCTRLSLOPL_MF_ADDR(arsr_base), hisi_fb_arsr_slopl_mf, 32, 0);
	dpu_set_reg(DPU_ARSR_GAINCTRLSLOPH_HF_ADDR(arsr_base), hisi_fb_arsr_sloph_hf, 32, 0);
	dpu_set_reg(DPU_ARSR_GAINCTRLSLOPL_HF_ADDR(arsr_base), hisi_fb_arsr_slopl_hf, 32, 0);
	dpu_set_reg(DPU_ARSR_MF_LMT_ADDR(arsr_base), hisi_fb_arsr_mf_lmt, 32, 0);
	dpu_set_reg(DPU_ARSR_GAIN_MF_ADDR(arsr_base), hisi_fb_arsr_gain_mf, 32, 0);
	dpu_set_reg(DPU_ARSR_MF_B_ADDR(arsr_base), hisi_fb_arsr_mf_b, 32, 0);
	dpu_set_reg(DPU_ARSR_HF_LMT_ADDR(arsr_base), hisi_fb_arsr_hf_lmt, 32, 0);
	dpu_set_reg(DPU_ARSR_GAIN_HF_ADDR(arsr_base), hisi_fb_arsr_gain_hf, 32, 0);
	dpu_set_reg(DPU_ARSR_HF_B_ADDR(arsr_base), hisi_fb_arsr_hf_b, 32, 0);
	dpu_set_reg(DPU_ARSR_LF_CTRL_ADDR(arsr_base), hisi_fb_arsr_lf_ctrl, 32, 0);
	dpu_set_reg(DPU_ARSR_LF_VAR_ADDR(arsr_base), hisi_fb_arsr_lf_var, 32, 0);
	dpu_set_reg(DPU_ARSR_LF_CTRL_SLOP_ADDR(arsr_base), hisi_fb_arsr_lf_ctrl_slop, 32, 0);
	dpu_set_reg(DPU_ARSR_HF_SELECT_ADDR(arsr_base), hisi_fb_arsr_hf_select, 32, 0);
	dpu_set_reg(DPU_ARSR_SHARP_CFG2_H_ADDR(arsr_base), hisi_fb_arsr_shar_cfg2_h, 32, 0);
	dpu_set_reg(DPU_ARSR_SHARP_CFG2_L_ADDR(arsr_base), hisi_fb_arsr_shar_cfg2_l, 32, 0);
	dpu_set_reg(DPU_ARSR_TEXTURW_ANALYSTS_ADDR(arsr_base), hisi_fb_arsr_texture_analysys, 32, 0);
	dpu_set_reg(DPU_ARSR_INTPLSHOOTCTRL_ADDR(arsr_base), hisi_fb_arsr_intplshootctrl, 32, 0);
}

static void _arsr_set_mode(const struct dpu_arsr_data *input, DPU_ARSR_MODE_UNION *mode)
{
	mode->reg.diintplen = 1;
	mode->reg.textureanalysisen = 1;

	mode->reg.prescaleren = 1;
	if (is_scale_up(input->src_rect.w, input->dst_rect.w))
		mode->reg.prescaleren = 0;

	/* TODO */
	mode->reg.data_format = 0;
	mode->reg.bypass = 0;
}

static void _arsr_set_lut(struct dpu_module_desc *module, char __iomem *base)
{
	uint32_t i = 0;
	uint32_t arry_len = ARRAY_SIZE(g_coef_tap6);

	disp_assert_if_cond(arry_len != ARRAY_SIZE(g_coef_tap4));

	for (i = 0; i < arry_len; i++) {
		hisi_module_set_reg(module, DPU_ARSR_COEFY_V_ADDR(base, i), g_coef_tap6[i].value);
		hisi_module_set_reg(module, DPU_ARSR_COEFY_H_ADDR(base, i), g_coef_tap6[i].value);
		hisi_module_set_reg(module, DPU_ARSR_COEFY_H_ADDR(base, i) + 0x100, g_coef_tap6[i].value);

		hisi_module_set_reg(module, DPU_ARSR_COEFA_V_ADDR(base, i), g_coef_tap4[i].value);
		hisi_module_set_reg(module, DPU_ARSR_COEFA_H_ADDR(base, i), g_coef_tap4[i].value);
		hisi_module_set_reg(module, DPU_ARSR_COEFA_H_ADDR(base, i) + 0x100, g_coef_tap4[i].value);

		hisi_module_set_reg(module, DPU_ARSR_COEFUV_V_ADDR(base, i), g_coef_tap4[i].value);
		hisi_module_set_reg(module, DPU_ARSR_COEFUV_H_ADDR(base, i), g_coef_tap4[i].value);
	}
}

static void _arsr_set_cmd_item(struct dpu_module_desc *module, struct dpu_arsr_data *input)
{
	struct _arsr_reg arsr_reg = {0};

	char __iomem *base = input->base;

	arsr_reg.src.reg.arsr_input_height = DPU_HEIGHT(input->src_rect.h);
	arsr_reg.src.reg.arsr_input_width = DPU_HEIGHT(input->src_rect.w);
	arsr_reg.dst.reg.arsr_output_height = DPU_HEIGHT(input->dst_rect.h);
	arsr_reg.dst.reg.arsr_output_width = DPU_HEIGHT(input->dst_rect.w);

	hisi_module_set_reg(module, DPU_ARSR_INPUT_WIDTH_HEIGHT_ADDR(base), arsr_reg.src.value);
	hisi_module_set_reg(module, DPU_ARSR_OUTPUT_WIDTH_HEIGHT_ADDR(base), arsr_reg.dst.value);

	/* TODO:
	 * calculate cordinate for block
	 *
	 *  left
	 */
	hisi_module_set_reg(module, DPU_ARSR_IHLEFT_ADDR(base), arsr_reg.h_left.value);
	hisi_module_set_reg(module, DPU_ARSR_IHLEFT1_ADDR(base), arsr_reg.h_left.value);

	/* right */
	arsr_reg.h_inc.value = ARSR_SCALE_INC(arsr_reg.src.reg.arsr_input_width, arsr_reg.dst.reg.arsr_output_width + 1, ARSR_SCALE_H_V_INC_OFFSET);
	arsr_reg.h_right.value = arsr_reg.dst.reg.arsr_output_width * arsr_reg.h_inc.value + arsr_reg.h_left.value;
	hisi_module_set_reg(module, DPU_ARSR_IHRIGHT_ADDR(base), arsr_reg.h_right.value);
	hisi_module_set_reg(module, DPU_ARSR_IHRIGHT1_ADDR(base), arsr_reg.h_right.value); // todo: get ihright1

	if (is_scale_down(arsr_reg.src.reg.arsr_input_height, arsr_reg.dst.reg.arsr_output_height))
		arsr_reg.v_inc.value = ARSR_SCALE_INC(arsr_reg.src.reg.arsr_input_height, arsr_reg.dst.reg.arsr_output_height, ARSR_SCALE_DOWN_V_INC_OFFSET);
	else
		arsr_reg.v_inc.value = ARSR_SCALE_INC(arsr_reg.src.reg.arsr_input_height, arsr_reg.dst.reg.arsr_output_height + 1, ARSR_SCALE_H_V_INC_OFFSET);

	/* top */
	hisi_module_set_reg(module, DPU_ARSR_IVTOP_ADDR(base), arsr_reg.v_top.value);

	/* bottom */
	arsr_reg.v_bottom.value = arsr_reg.dst.reg.arsr_output_height * arsr_reg.v_inc.value + arsr_reg.v_top.value;
	hisi_module_set_reg(module, DPU_ARSR_IVBOTTOM_ADDR(base), arsr_reg.v_bottom.value);
	hisi_module_set_reg(module, DPU_ARSR_IVBOTTOM1_ADDR(base), arsr_reg.v_bottom.value); // TODO: get ivbottom1

	/* INC */
	hisi_module_set_reg(module, DPU_ARSR_IHINC_ADDR(base), arsr_reg.h_inc.value);
	hisi_module_set_reg(module, DPU_ARSR_IVINC_ADDR(base), arsr_reg.v_inc.value);

	hisi_module_set_reg(module, DPU_ARSR_OFFSET_ADDR(base), 0);

	/* mode */
	_arsr_set_mode(input, &arsr_reg.mode);

	if (hisi_arsr_effect_en == 0)
		hisi_module_set_reg(module, DPU_ARSR_MODE_ADDR(base), arsr_reg.mode.value);
	else
		_arsr_effect_test(base);

	/* TODO: config skin, shoot, sharp texture coef */
}

static void arsr_set_dm_scl_section(struct dpu_operator_arsr *arsr, struct pipeline_src_layer *layer,
		struct hisi_dm_scl_info *scl_info, char __iomem *dpu_base_addr)
{
	struct dpu_module_desc *module = &arsr->base.module_desc;
	char __iomem *dm_scl_base = dpu_base_addr + dpu_get_dm_offset(arsr->base.scene_id) + arsr->base.dm_offset;

	/* scl input and output information must be calculated by build_data() */
	scl_info->scl0_input_img_width_union.reg.scl0_input_img_height = DPU_HEIGHT(arsr->base.in_data->rect.h);
	scl_info->scl0_input_img_width_union.reg.scl0_input_img_width = DPU_WIDTH(arsr->base.in_data->rect.w);

	scl_info->scl0_output_img_width_union.reg.scl0_output_img_height = DPU_HEIGHT(arsr->base.out_data->rect.h);
	scl_info->scl0_output_img_width_union.reg.scl0_output_img_width = DPU_WIDTH(arsr->base.out_data->rect.w);
	if (arsr_post_en == 1)
		scl_info->scl0_type_union.reg.scl0_layer_id = DPU_OP_INVALID_ID;
	else
		scl_info->scl0_type_union.reg.scl0_layer_id = layer->layer_id;

	scl_info->scl0_type_union.reg.scl0_order0 = hisi_get_op_id_by_op_type(arsr->base.in_data->next_order);
	scl_info->scl0_type_union.reg.scl0_sel = BIT(arsr->base.id_desc.info.idx % dpu_config_get_arsr_count());
	scl_info->scl0_type_union.reg.scl0_type = SCL_TYPE_ARSR;

	scl_info->scl0_threshold_union.reg.scl0_input_fmt = arsr->base.in_data->format;
	scl_info->scl0_threshold_union.reg.scl0_output_fmt = arsr->base.out_data->format;

	if (arsr_post_en == 1)
		scl_info->scl0_threshold_union.reg.scl0_pre_post_sel = 1; // asrr post
	else
		scl_info->scl0_threshold_union.reg.scl0_pre_post_sel = 0; // arsr pre

	scl_info->scl0_threshold_union.reg.scl0_threshold = SCL_THRESHOLD_DEFAULT_LEVEL;

	/* DM SCL Section */
	hisi_module_set_reg(module, DPU_DM_SCL0_INPUT_IMG_WIDTH_ADDR(dm_scl_base), scl_info->scl0_input_img_width_union.value);
	hisi_module_set_reg(module, DPU_DM_SCL0_OUTPUT_IMG_WIDTH_ADDR(dm_scl_base), scl_info->scl0_output_img_width_union.value);
	hisi_module_set_reg(module, DPU_DM_SCL0_TYPE_ADDR(dm_scl_base), scl_info->scl0_type_union.value);
	hisi_module_set_reg(module, DPU_DM_SCL0_THRESHOLD_ADDR(dm_scl_base), scl_info->scl0_threshold_union.value);
}

static void arsr_set_regs(struct dpu_operator_arsr *arsr, struct pipeline_src_layer *src_layer,
		char __iomem *dpu_base_addr)
{
	char __iomem *arsr_base = dpu_base_addr + arsr->base.operator_offset;
	struct dpu_module_desc *module = &arsr->base.module_desc;
	DPU_ARSR_REG_CTRL_UNION arsr_reg_ctrl;
	struct dpu_dfc_data dfc = {0};
	struct dpu_csc_data csc = {0};
	struct dpu_csc_data post_csc = {0};
	struct dpu_scf_data scf = {0};
	struct dpu_post_clip_data post_clip = {0};
	struct dpu_arsr_data _arsr = {0};

	/* 1, DFC */
	memset(&dfc, 0, sizeof(dfc));
	dfc.dfc_base = arsr_base;
	dfc.format = arsr->base.in_data->format;
	dfc.src_rect = arsr->base.in_data->rect;
	dfc.dst_rect = dfc.src_rect;
	dfc.enable_icg_submodule = has_40bit(arsr->base.in_data->format) ? 0 : 1;

	if (dfc.enable_icg_submodule)
		dpu_dfc_set_cmd_item(module, &dfc);
	else
		hisi_module_set_reg(module, DPU_ARSR_DFC_ICG_MODULE_ADDR(arsr_base), 0);

	/*
	 * csc
	 */
	csc.base = arsr_base;
	csc.csc_mode = DEFAULT_CSC_MODE;
	csc.out_format = AYUV10101010; // arsrpost in rgb out rgb need en csc
	csc.direction = dpu_csc_get_direction(dfc.format, csc.out_format);

	dpu_csc_set_cmd_item(module, &csc);

	/*
	 * SCF: just can do horizontal scale down.
	 */
	memset(&scf, 0, sizeof(scf));
	scf.src_rect = dfc.dst_rect;
	scf.dst_rect = scf.src_rect;
	if (is_scale_down(scf.src_rect.w, arsr->base.out_data->rect.w)) {
		scf.base = arsr_base;
		scf.has_alpha = has_alpha(csc.out_format);
		scf.acc_hscl = 0;
		scf.acc_vscl = 0;
		scf.dst_rect.w = arsr->base.out_data->rect.w;
		dpu_scf_set_cmd_item(module, &scf);
	} else {
		hisi_module_set_reg(module, DPU_ARSR_SCF_EN_VSCL_ADDR(arsr_base), 0);
		hisi_module_set_reg(module, DPU_ARSR_SCF_EN_HSCL_ADDR(arsr_base), 0);
	}

	/*
	 * arsr: can do horizontal scale up, and verizontal scale up and down.
	 */
	memset(&_arsr, 0, sizeof(_arsr));
	_arsr.src_rect = scf.dst_rect;
	_arsr.dst_rect = arsr->base.out_data->rect;
	_arsr.base = arsr_base;
	_arsr.arsr_id = arsr->base.id_desc;
	_arsr_set_cmd_item(module, &_arsr);

	/*
	 * post_csc
	 */
	post_csc.base = arsr_base;
	post_csc.csc_mode = csc.csc_mode;
	post_csc.direction = dpu_csc_get_direction(csc.out_format, arsr->base.out_data->format);
	post_csc.out_format = arsr->base.out_data->format;
	dpu_post_csc_set_cmd_item(module, &post_csc);

	/*
	 * TODO: post clip
	 */
	dpu_post_clip_set_cmd_item(module, &post_clip);

	/*
	 * TODO: scl lut must be move to power on
	 */
	dpu_scf_lut_set_cmd(module, arsr_base);

	/*
	 * TOD: ARSR LUT must be move to power on
	 */
	_arsr_set_lut(module, arsr_base);

	/* scf top reg */
	arsr_reg_ctrl.reg.reg_ctrl_scene_id = arsr->base.scene_id;
	hisi_module_set_reg(module, DPU_ARSR_REG_CTRL_ADDR(arsr_base), arsr_reg_ctrl.value);
	hisi_module_set_reg(module, DPU_ARSR_REG_CTRL_FLUSH_EN_ADDR(arsr_base), 0x1);

	disp_pr_info("scene_id=%d, layer_id=%d", arsr->base.scene_id, src_layer->layer_id);
}

static int arsr_build_data(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *pre_out_data,
		 struct hisi_comp_operator *next_operator)
{
	struct dpu_operator_arsr *arsr = (struct dpu_operator_arsr *)operator;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;
	struct dpu_arsr_in_data *in_data = NULL;
	struct dpu_arsr_out_data *out_data = NULL;

	disp_pr_info(" ++++ \n");

	in_data = (struct dpu_arsr_in_data *)arsr->base.in_data;
	disp_assert_if_cond(in_data == NULL);
	arsr_post_en = 0;
	if (src_layer->dst_rect.w == 0 && src_layer->dst_rect.h == 0) {
		arsr_post_en = 1;
		disp_pr_debug(" arsr_post_en %d \n", arsr_post_en);
	}
	out_data = (struct dpu_arsr_out_data *)arsr->base.out_data;
	disp_assert_if_cond(out_data == NULL);

	/* in data */
	if (next_operator) {
		disp_pr_info(" next_operator->id_desc.info.type:%u \n", next_operator->id_desc.info.type);
		in_data->base.next_order = next_operator->id_desc.info.type;
	}

	if (pre_out_data) {
		in_data->base.format = pre_out_data->format;
		in_data->base.rect = pre_out_data->rect;
	} else {
		in_data->base.format = dpu_pixel_format_hal2dfc(src_layer->img.format, DFC_STATIC);
		in_data->base.rect = src_layer->src_rect;
	}

	/* out data */
	/* TODO: vscf output format maybe is not ARGB, and out rect is not layer dest rect, such as offline block segment */
	if (arsr_post_en == 0) { // arsr pre
		out_data->base.rect = src_layer->dst_rect;
		if (src_layer->need_caps & CAP_HDR)
			out_data->base.format = ARGB10101010;
		else
			out_data->base.format = AYUV10101010;
	}
	disp_pr_debug(" out w=%d, h=%d arsr_post_en = %d\n", out_data->base.rect.w, out_data->base.rect.h, arsr_post_en);

	dpu_operator_build_data(operator, layer, pre_out_data, next_operator);

	dpu_disp_print_pipeline_data(&in_data->base);
	dpu_disp_print_pipeline_data(&out_data->base);

	return 0;
}

static int arsr_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
	void *layer)
{
	struct hisi_dm_param *dm_param = NULL;
	struct dpu_operator_arsr *arsr = NULL;
	struct pipeline_src_layer *src_layer = NULL;
	struct hisi_dm_scl_info *scl_info = NULL;
	char __iomem *dpu_base_addr = NULL;

	disp_pr_info(" ++++ scl_idx %d \n", dpu_get_scl_idx());

	arsr = (struct dpu_operator_arsr *)operator;
	dm_param = composer_device->ov_data.dm_param;

	src_layer = (struct pipeline_src_layer *)layer;
	scl_info = &dm_param->scl_info[dpu_get_scl_idx()];
	dpu_base_addr = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];
	arsr->base.dm_offset = dpu_scl_alloc_dm_node();

	if (arsr_post_en == 1) { // arsr post
		arsr->base.out_data->rect.w = composer_device->ov_data.fix_panel_info->xres;
		arsr->base.out_data->rect.h = composer_device->ov_data.fix_panel_info->yres;
		arsr->base.out_data->format = ARGB10101010;
	}
	/* calculate dm parament */
	arsr_set_dm_scl_section(arsr, src_layer, scl_info, dpu_base_addr);
	/* set dm regs and vscf regs */
	arsr_set_regs(arsr, src_layer, dpu_base_addr);
	composer_device->ov_data.layer_ov_format = operator->out_data->format; // sdma->arsr->ov

	disp_pr_info(" ---- \n");
	return 0;
}

void dpu_arsr_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct dpu_operator_arsr **arsrs = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info(" ++++ \n");
	disp_pr_info(" type_operator->operator_count:%u \n", type_operator->operator_count);

	arsrs = kzalloc(sizeof(*arsrs) * type_operator->operator_count, GFP_KERNEL);
	if (!arsrs)
		return;

	for (i = 0; i < type_operator->operator_count; i++) {
		disp_pr_info(" size of struct dpu_operator_arsr:%u \n", sizeof(*(arsrs[i])));
		arsrs[i] = kzalloc(sizeof(*(arsrs[i])), GFP_KERNEL);
		if (!arsrs[i])
			continue;

		base = &arsrs[i]->base;

		/* TODO: error check */
		hisi_operator_init(base, ops, cookie);
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = arsr_set_cmd_item;
		base->build_data = arsr_build_data;

		/* seq is bigger than count, seq will be 0,1,2,3， count is 2 */
		base->operator_offset = g_arsr_offset[base->id_desc.info.idx];

		sema_init(&base->operator_sem, 1);
		disp_pr_ops(info, base->id_desc);

		base->out_data = kzalloc(sizeof(struct dpu_arsr_out_data), GFP_KERNEL);
		if (!base->out_data) {
			kfree(arsrs[i]);
			arsrs[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct dpu_arsr_in_data), GFP_KERNEL);
		if (!base->in_data) {
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(arsrs[i]);
			arsrs[i] = NULL;
			continue;
		}

		/* TODO: init other rmda operators */

		base->be_dm_counted = false;
	}

	type_operator->operators = (struct hisi_comp_operator **)(arsrs);
}
