/*
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include "dpu_hihdr_itm.h"

const uint32_t g_itm_degamma_step_map[8] = {5, 5, 5, 4, 3, 3, 3, 0};
const uint32_t g_itm_degamma_num_map[8] = {1, 10, 19, 28, 37, 55, 62, 63};
const uint32_t g_itm_degamma_pos_map[8] = {32, 320, 608, 752, 824, 968, 1022, 1023};
const uint32_t g_itm_degamma_lut_map[64] = {
0, 158, 337, 595, 940, 1380, 1923, 2572, 3334, 4213, 5215, 6343,7603, 8997, 10530, 12205, 14027, 15998,
18121, 20401, 21600, 22839, 24119, 25440, 26802, 28206, 29651, 31139, 32670, 33451, 34243, 35047, 35860,
36685, 37521, 38368, 39226, 40095, 40975, 41866, 42768, 43682, 44607, 45543, 46491, 47450, 48420, 49402,
50396, 51401, 52418, 53446, 54486, 55538, 56601, 57676, 58763, 59862, 60973, 62095, 63230, 64377, 65535, 65535};

const uint32_t g_itm_gamma_step_map[8] = {7, 7, 8, 9, 10, 11, 13, 0};
const uint32_t g_itm_gamma_num_map[8] = {1, 32, 42, 47, 52, 57, 62, 63};
const uint32_t g_itm_gamma_pos_map[8] = {128, 4096, 6656, 9216, 14336, 24576, 65534, 65535};
const uint32_t g_itm_gamma_lut_map[64] = {
0, 25, 50, 70, 86, 100, 113, 124, 134, 144, 153, 161, 169, 177, 184, 191, 198, 205, 211, 217, 223, 229,
234, 240, 245, 250, 255, 260, 265, 270, 274, 279, 283, 292, 301, 309, 317, 324, 332, 339, 346, 353, 360,
373, 385, 397, 409, 420, 442, 462, 481, 499, 517, 549, 580, 609, 636, 661, 753, 831, 901, 965, 1023, 1023};

const uint32_t g_itm_gamut_coef[9] = {842, 182, 0, 34, 990, 0, 18, 74, 932};

static void dpu_hihdr_itm_gamma_init(struct dpu_fb_data_type *dpufd, char __iomem *hdr_base)
{
	int i;

	/* step map */
	uint32_t step1 = (g_itm_gamma_step_map[3] << 24) | (g_itm_gamma_step_map[2] << 16) |
		(g_itm_gamma_step_map[1] << 8) | g_itm_gamma_step_map[0];
	uint32_t step2 = (g_itm_gamma_step_map[7] << 24) | (g_itm_gamma_step_map[6] << 16) |
		(g_itm_gamma_step_map[5] << 8) | g_itm_gamma_step_map[4];

	/* num map */
	uint32_t num1 =  (g_itm_gamma_num_map[3] << 24) | (g_itm_gamma_num_map[2] << 16) |
		(g_itm_gamma_num_map[1] << 8) | g_itm_gamma_num_map[0];
	uint32_t num2 =  (g_itm_gamma_num_map[7] << 24) | (g_itm_gamma_num_map[6] << 16) |
		(g_itm_gamma_num_map[5] << 8) | g_itm_gamma_num_map[4];

	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_STEP1, step1, 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_STEP2, step2, 32, 0);

	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_NUM1, num1, 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_NUM2, num2, 32, 0);

	/* pos map */
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_POS1, g_itm_gamma_pos_map[0], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_POS2, g_itm_gamma_pos_map[1], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_POS3, g_itm_gamma_pos_map[2], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_POS4, g_itm_gamma_pos_map[3], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_POS5, g_itm_gamma_pos_map[4], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_POS6, g_itm_gamma_pos_map[5], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_POS7, g_itm_gamma_pos_map[6], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMMA_POS8, g_itm_gamma_pos_map[7], 32, 0);

	/* gamma lut, 32 data */
	for (i = 0; i < 32; i++) {
		uint32_t temp_data = (g_itm_gamma_lut_map[i * 2 + 1] << 16) | g_itm_gamma_lut_map[i * 2];
		set_reg(hdr_base + DSS_ITM_LUT_GAMMA + (i * 4), temp_data, 32, 0);
	}
}

static void dpu_hihdr_itm_gamut_init(struct dpu_fb_data_type *dpufd, char __iomem *hdr_base)
{
	/* ces0 and csc1 are used for ROI. Here we set the same parameter. */
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF00_0, g_itm_gamut_coef[0], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF01_0, g_itm_gamut_coef[1], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF02_0, g_itm_gamut_coef[2], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF10_0, g_itm_gamut_coef[3], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF11_0, g_itm_gamut_coef[4], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF12_0, g_itm_gamut_coef[5], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF20_0, g_itm_gamut_coef[6], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF21_0, g_itm_gamut_coef[7], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF22_0, g_itm_gamut_coef[8], 32, 0);

	/* Algorithm precision */
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_SCALE, 10, 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_CLIP_MAX, 0xffff, 32, 0);

	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF00_1, g_itm_gamut_coef[0], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF01_1, g_itm_gamut_coef[1], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF02_1, g_itm_gamut_coef[2], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF10_1, g_itm_gamut_coef[3], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF11_1, g_itm_gamut_coef[4], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF12_1, g_itm_gamut_coef[5], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF20_1, g_itm_gamut_coef[6], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF21_1, g_itm_gamut_coef[7], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_GAMUT_COEF22_1, g_itm_gamut_coef[8], 32, 0);
}

static void dpu_hihdr_itm_degamma_init(struct dpu_fb_data_type *dpufd, char __iomem *hdr_base)
{
	int i;

	/* step map */
	uint32_t step1 =  (g_itm_degamma_step_map[3] << 24) | (g_itm_degamma_step_map[2] << 16) |
		(g_itm_degamma_step_map[1] << 8) | g_itm_degamma_step_map[0];
	uint32_t step2 =  (g_itm_degamma_step_map[7] << 24) | (g_itm_degamma_step_map[6] << 16) |
		(g_itm_degamma_step_map[5] << 8) | g_itm_degamma_step_map[4];

	/* num map */
	uint32_t num1 =  (g_itm_degamma_num_map[3] << 24) | (g_itm_degamma_num_map[2] << 16) |
		(g_itm_degamma_num_map[1] << 8) | g_itm_degamma_num_map[0];
	uint32_t num2 =  (g_itm_degamma_num_map[7] << 24) | (g_itm_degamma_num_map[6] << 16) |
		(g_itm_degamma_num_map[5] << 8) | g_itm_degamma_num_map[4];

	set_reg(hdr_base + DSS_ITM_SLF_DEGAMMA_STEP1, step1, 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_DEGAMMA_STEP2, step2, 32, 0);

	set_reg(hdr_base + DSS_ITM_SLF_DEGAMMA_NUM1, num1, 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_DEGAMMA_NUM2, num2, 32, 0);

	/* pos map */
	set_reg(hdr_base + DSS_ITM_SLF_DEGAMMA_POS1,
		(g_itm_degamma_pos_map[1] << 16) | g_itm_degamma_pos_map[0], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_DEGAMMA_POS2,
		(g_itm_degamma_pos_map[3] << 16) | g_itm_degamma_pos_map[2], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_DEGAMMA_POS3,
		(g_itm_degamma_pos_map[5] << 16) | g_itm_degamma_pos_map[4], 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_DEGAMMA_POS4,
		(g_itm_degamma_pos_map[7] << 16) | g_itm_degamma_pos_map[6], 32, 0);

	/* degamma lut, 64 data */
	for (i = 0; i < 64; i++) {
		set_reg(hdr_base + DSS_ITM_LUT_DEGAMMA + (i * 4), g_itm_degamma_lut_map[i], 32, 0);
	}
}

void dpu_hihdr_itm_init(struct dpu_fb_data_type *dpufd, char __iomem *hdr_base)
{
	dpu_check_and_no_retval(!hdr_base, ERR, "hdr_base is nullptr!\n");

	/* CSC static config: SRGB -> P3 */
	dpu_hihdr_itm_gamma_init(dpufd, hdr_base);
	dpu_hihdr_itm_gamut_init(dpufd, hdr_base);
	dpu_hihdr_itm_degamma_init(dpufd, hdr_base);
}

static inline void dpu_lut_set_reg(struct dpu_fb_data_type *dpufd, char __iomem *lut_base,
	uint32_t lut_size, uint32_t* lut)
{
	int i = 0;
	uint32_t lut_value = 0;
	for (i = 0; i < lut_size; i++) {
		lut_value = lut[i * 2 + 1] << 16 | lut[i * 2];
		dpufd->set_reg(dpufd, lut_base + (i * 4), lut_value, 32, 0);
	}
}

static void hihdr_itm_set_basic_reg(struct dpu_fb_data_type *dpufd,  hihdr_itm_info_t *itm_info, char __iomem *hdr_base)
{
	itm_basic_info_t *basic_info = NULL;

	if (g_debug_effect_hihdr)
		DPU_FB_INFO("[hihdr] itm basic +");

	dpu_check_and_no_retval((dpufd == NULL), ERR, "dpufd is NULL\n");
	dpu_check_and_no_retval((itm_info == NULL), ERR, "itm_info is NULL\n");
	dpu_check_and_no_retval((hdr_base == NULL), ERR, "hdr_base is NULL\n");

	basic_info = &itm_info->basic_info;

	set_reg(hdr_base + DSS_ITM_SLF_LCPROC_MIN_E,
		(basic_info->output_min_e << 16 | basic_info->input_min_e), 32, 0);


	set_reg(hdr_base + DSS_ITM_SLF_LCPROC_CLIP,
		(basic_info->ratiolutoutclip << 16 | basic_info->ratiolutinclip), 32, 0);

	set_reg(hdr_base + DSS_ITM_SLF_LCPROC_SCALE,
		((basic_info->lumaoutbit1 << 20) | (basic_info->lumaoutbit0 << 16)|
		(basic_info->chromaratiolutstep1 << 12) | (basic_info->chromaratiolutstep0 << 8)|
		(basic_info->lumaratiolutstep1 << 4)|(basic_info->lumaratiolutstep0 << 0)), 32, 0);

	set_reg(hdr_base + DSS_ITM_SLF_LCPROC_THRESHOLD,
		basic_info->chromalutthres << 16 | basic_info->lumalutthres, 32, 0);

	set_reg(hdr_base + DSS_ITM_SLF_LCPROC_OUT_BIT,
		(basic_info->chromaoutbit2 << 8 | basic_info->chromaoutbit1), 32, 0);

	set_reg(hdr_base + DSS_ITM_SLF_LCPROC_CRATIOCLIP,
		basic_info->chromaRatioclip, 13, 0);

	/* itm lut */
	dpu_lut_set_reg(dpufd, hdr_base + DSS_ITM_LUT_CHROMA,
		ITM_LUT_CHROMA_SIZE / 2, (uint32_t *)(&itm_info->dynamic_info.lut_chroma));

	dpu_lut_set_reg(dpufd, hdr_base + DSS_ITM_LUT_CHROMA0,
		ITM_LUT_CHROMA0_SIZE / 2, (uint32_t *)(&itm_info->dynamic_info.lut_chroma0));

	dpu_lut_set_reg(dpufd, hdr_base + DSS_ITM_LUT_CHROMA1,
		ITM_LUT_CHROMA1_SIZE / 2, (uint32_t *)(&itm_info->dynamic_info.lut_chroma1));

	dpu_lut_set_reg(dpufd, hdr_base + DSS_ITM_LUT_CHROMA2,
		ITM_LUT_CHROMA2_SIZE / 2, (uint32_t *)(&itm_info->dynamic_info.lut_chroma2));

	dpu_lut_set_reg(dpufd, hdr_base + DSS_ITM_LUT_LUMALUT0,
		ITM_LUT_LUMALUT0_SIZE / 2, (uint32_t *)(&itm_info->dynamic_info.lut_lumalut0));

	dpu_lut_set_reg(dpufd, hdr_base + DSS_ITM_LUT_CHROMALUT0,
		ITM_LUT_CHROMALUT0_SIZE / 2, (uint32_t *)(&itm_info->dynamic_info.lut_chromalut0));
}

static void hihdr_itm_set_dynamic_reg(struct dpu_fb_data_type *dpufd, hihdr_itm_info_t *itm_info, char __iomem *hdr_base)
{
	dpu_lut_set_reg(dpufd, hdr_base + DSS_ITM_LUT_LUMA,
		ITM_LUT_LUMA_SIZE / 2, (uint32_t *)(&itm_info->dynamic_info.lut_luma));
};

static bool hihdr_itm_check_basic_config(struct dpu_fb_data_type *dpufd,  hihdr_itm_info_t *itm_info, char __iomem *hdr_base)
{
	uint32_t lcproc_out_bit;
	uint32_t lcproc_clip;
	itm_basic_info_t *basic_info = NULL;

	dpu_check_and_return((dpufd == NULL), false, ERR, "pov_req is NULL.\n");
	dpu_check_and_return((itm_info == NULL), false, ERR, "pov_req is NULL.\n");

	basic_info = &itm_info->basic_info;

	lcproc_out_bit = (uint32_t) (basic_info->chromaoutbit2 << 8 | basic_info->chromaoutbit1);
	lcproc_clip = (uint32_t) (basic_info->ratiolutoutclip << 16 | basic_info->ratiolutinclip);

	if ((lcproc_out_bit == 0) || (lcproc_clip == 0)) {
		DPU_FB_WARNING("[hihdr] sdr hihdr status error!");
		return false;
	}

	if (g_debug_effect_hihdr) {
		DPU_FB_INFO("[hihdr] chromaoutbit2: %d; chromaoutbit1: %d ", basic_info->chromaoutbit2, basic_info->chromaoutbit1);
		DPU_FB_INFO("[hihdr] ratiolutoutclip: %d; ratiolutinclip: %d ", basic_info->ratiolutoutclip, basic_info->ratiolutinclip);
		DPU_FB_INFO("[hihdr] LCPROC_OUT_BIT: 0x%x; LCPROC_CLIP: 0x%x", inp32(hdr_base + DSS_ITM_SLF_LCPROC_OUT_BIT), inp32(hdr_base + DSS_ITM_SLF_LCPROC_CLIP));
	}

	if ((lcproc_out_bit != inp32(hdr_base + DSS_ITM_SLF_LCPROC_OUT_BIT)) ||
		(lcproc_clip != inp32(hdr_base + DSS_ITM_SLF_LCPROC_CLIP))) {
			dpufd->hihdr_basic_ready = dpufd->hihdr_basic_ready | HIHDR_SDR_BASIC_CONFIG_READY;
			DPU_FB_INFO("[hihdr] Basic need config again!! itm flag = %d", itm_info->flag);
			hihdr_itm_set_basic_reg(dpufd, itm_info, hdr_base);
			hihdr_itm_set_dynamic_reg(dpufd, itm_info, hdr_base);
	}

	return true;
}

static void dpu_hihdr_itm_sdr_enhance(struct dpu_fb_data_type *dpufd, hihdr_itm_info_t *itm_info,
	uint32_t itm_ctl, uint32_t itm_roi)
{
	char __iomem *hdr_base = dpufd->dss_base + DSS_HDR_OFFSET;

	if (g_debug_effect_hihdr)
		DPU_FB_INFO("[hihdr] itm flag = %d", itm_info->flag);

	switch (itm_info->flag) {
	case SDR_EH_BASIC:
		dpufd->hihdr_basic_ready = dpufd->hihdr_basic_ready | HIHDR_SDR_BASIC_CONFIG_READY;
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_SLF_CTRL, itm_ctl, 32, 0);
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_ROI, itm_roi, 32, 0);

		hihdr_itm_set_basic_reg(dpufd, itm_info, hdr_base);
		hihdr_itm_set_dynamic_reg(dpufd, itm_info, hdr_base);
		break;
	case SDR_EH_DYNAMIC:
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_SLF_CTRL, itm_ctl, 32, 0);
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_ROI, itm_roi, 32, 0);

		if (hihdr_itm_check_basic_config(dpufd, itm_info, hdr_base))
			hihdr_itm_set_dynamic_reg(dpufd, itm_info, hdr_base);

		break;
	case SDR_EH_CLOSE:
		dpufd->hihdr_basic_ready = dpufd->hihdr_basic_ready & (~HIHDR_SDR_BASIC_CONFIG_READY);
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_SLF_CTRL, itm_ctl, 32, 0);
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_ROI, itm_roi, 32, 0);
		break;
	case SDR_EH_HOLD:
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_SLF_CTRL, itm_ctl, 32, 0);
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_ROI, itm_roi, 32, 0);

		hihdr_itm_check_basic_config(dpufd, itm_info, hdr_base);
		break;
	case SDR_EH_DEFAULT:
	default:
		break;
	}
}

static void dpu_hihdr_itm_csc_trigger(struct dpu_fb_data_type *dpufd, hihdr_itm_info_t *itm_info,
	uint32_t itm_ctl, uint32_t itm_roi)
{
	char __iomem *hdr_base = dpufd->dss_base + DSS_HDR_OFFSET;

	if (g_debug_effect_hihdr)
		DPU_FB_INFO("[hihdr] itm csc flag = %d", itm_info->csc_flag);

	switch(itm_info->csc_flag) {
	case CSC_2_P3_START:
	case CSC_2_P3_STOP:
	case CSC_2_P3_HOLD:
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_SLF_CTRL, itm_ctl, 32, 0);
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_ROI, itm_roi, 32, 0);
		break;
	case CSC_DEFAULT:
	default:
		break;
	}
}

static void dpu_hihdr_itm_ctl(hihdr_itm_info_t *itm_info, uint32_t *out_itm_ctl, uint32_t *out_itm_roi)
{
	dpu_check_and_no_retval((out_itm_ctl == NULL), ERR, "out_itm_ctl is NULL\n");
	dpu_check_and_no_retval((out_itm_roi == NULL), ERR, "out_itm_roi is NULL\n");
	/* keep csc & enhance algorithm */
	if ((itm_info->flag == SDR_EH_DEFAULT || itm_info->flag == SDR_EH_DYNAMIC || itm_info->flag == SDR_EH_HOLD) &&
		(itm_info->csc_flag == CSC_DEFAULT || itm_info->csc_flag == CSC_2_P3_HOLD)) {
		if (g_debug_effect_hihdr) {
			DPU_FB_INFO("[hihdr] KEEP csc flag = %d, enhance flag = %d", itm_info->csc_flag, itm_info->flag);
			DPU_FB_INFO("[hihdr] KEEP out_itm_ctl = %d, out_itm_roi = %d", *out_itm_ctl, *out_itm_roi);
		}
	}

	/* csc & enhance algorithm close*/
	if ((itm_info->flag == SDR_EH_CLOSE || itm_info->flag == SDR_EH_DEFAULT) &&
		(itm_info->csc_flag == CSC_2_P3_STOP || itm_info->csc_flag == CSC_DEFAULT)) {
		*out_itm_ctl = 0;
		*out_itm_roi = 0;
		if (g_debug_effect_hihdr) {
			DPU_FB_INFO("[hihdr] CLOSE csc flag = %d, enhance flag = %d", itm_info->csc_flag, itm_info->flag);
			DPU_FB_INFO("[hihdr] CLOSE out_itm_ctl = %d, out_itm_roi = %d", *out_itm_ctl, *out_itm_roi);
		}
		return;
	}

	if (itm_info->flag == SDR_EH_BASIC || itm_info->flag == SDR_EH_DYNAMIC || itm_info->flag == SDR_EH_HOLD) {
		*out_itm_ctl = 0x1;
		*out_itm_roi = 0x200;
	}

	if (itm_info->csc_flag == CSC_2_P3_START || itm_info->csc_flag == CSC_2_P3_HOLD) {
		*out_itm_ctl = *out_itm_ctl | 0xF;
		*out_itm_roi = *out_itm_roi | 0x1;
	}

	if (g_debug_effect_hihdr) {
		DPU_FB_INFO("[hihdr] csc flag = %d, enhance flag = %d", itm_info->csc_flag, itm_info->flag);
		DPU_FB_INFO("[hihdr] out_itm_ctl = %d, out_itm_roi = %d", *out_itm_ctl, *out_itm_roi);
	}
}

void dpu_hihdr_itm_set_reg(struct dpu_fb_data_type *dpufd, hihdr_itm_info_t *itm_info)
{
	uint32_t itm_ctl = 0;
	uint32_t itm_roi = 0;

	dpu_check_and_no_retval((dpufd == NULL), ERR, "dpufd is NULL\n");
	dpu_check_and_no_retval((itm_info == NULL), ERR, "itm_info is NULL\n");
	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;

	dpu_hihdr_itm_ctl(itm_info, &itm_ctl, &itm_roi);
	dpu_hihdr_itm_csc_trigger(dpufd, itm_info, itm_ctl, itm_roi);
	dpu_hihdr_itm_sdr_enhance(dpufd, itm_info, itm_ctl, itm_roi);
}


