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

#ifndef HISI_OPERATOR_DPP_H
#define HISI_OPERATOR_DPP_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_disp.h"
#include "hisi_composer_operator.h"
#include "../../effect/display_effect_alsc_interface.h"

#define GAMA_LUT_LENGTH  ((uint32_t)257)
#define LCP_IGM_LUT_LENGTH  ((uint32_t)257)
#define DPP_BUF_ROI_REGION_COUNT 2
#define LCP_XCC_LUT_LENGTH  ((uint32_t)12)

// acm algorithm info define, mem_ctrl decide by enable or not
struct acm_info {
	uint32_t acm_en;
	uint32_t param_mode;
	uint32_t sata_offset;
	uint32_t acm_hue_rlh01;
	uint32_t acm_hue_rlh23;
	uint32_t acm_hue_rlh45;
	uint32_t acm_hue_rlh67;
	uint32_t acm_hue_param01;
	uint32_t acm_hue_param23;
	uint32_t acm_hue_param45;
	uint32_t acm_hue_param67;
	uint32_t acm_hue_smooth0;
	uint32_t acm_hue_smooth1;
	uint32_t acm_hue_smooth2;
	uint32_t acm_hue_smooth3;
	uint32_t acm_hue_smooth4;
	uint32_t acm_hue_smooth5;
	uint32_t acm_hue_smooth6;
	uint32_t acm_hue_smooth7;
	uint32_t acm_color_choose;
	uint32_t acm_l_cont_en;
	uint32_t acm_lc_param01;
	uint32_t acm_lc_param23;
	uint32_t acm_lc_param45;
	uint32_t acm_lc_param67;
	uint32_t acm_l_adj_ctrl;
	uint32_t acm_capture_ctrl;
	uint32_t acm_capture_in;
	uint32_t acm_capture_out;
	uint32_t acm_ink_ctrl;
	uint32_t acm_ink_out;
};

/* two roi rect config for dpp roi mode
 * top_left > bottom_right,thisi roi rect configuration does not take effect
 * only one region takes effect, another area must be set to
 * top_left_x = 0
 * top_left_y = 0
 * bottom_right_x = 0xF
 * bottom_right_y = 0xF
 */
struct roi_rect {
	uint32_t enable;
	uint32_t top_left_x;
	uint32_t top_left_y;
	uint32_t bottom_right_x;
	uint32_t bottom_right_y;
};

/* gama_info is for kernel; gamma_info is for hal sdk*/
struct gama_info {
	uint32_t gama_enable;
	uint32_t para_mode;
	uint32_t gama_r_lut[GAMA_LUT_LENGTH];
	uint32_t gama_g_lut[GAMA_LUT_LENGTH];
	uint32_t gama_b_lut[GAMA_LUT_LENGTH];
};

struct gamma_info {
        uint32_t enable;
        uint32_t para_mode;
       // compat_pointer(gamma_r_table); // gamma lut length 257
       // compat_pointer(gamma_g_table);
       // compat_pointer(gamma_b_table);
};

struct degamma_info {
	uint32_t degamma_enable;
	uint32_t degamma_r_lut[LCP_IGM_LUT_LENGTH];
	uint32_t degamma_g_lut[LCP_IGM_LUT_LENGTH];
	uint32_t degamma_b_lut[LCP_IGM_LUT_LENGTH];
};

struct xcc_info {
	uint32_t xcc_enable;
	uint32_t xcc_table[LCP_XCC_LUT_LENGTH];
};

//hiace v3
struct hiace_v3_register {
	// highlight
	uint32_t highlight;
	uint32_t hl_hue_gain0;
	uint32_t hl_hue_gain1;
	uint32_t hl_hue_gain2;
	uint32_t hl_hue_gain3;
	uint32_t hl_hue_gain4;
	uint32_t hl_hue_gain5;
	uint32_t hl_sat;
	uint32_t hl_yout_maxth;
	uint32_t hl_yout;
	uint32_t hl_diff;

	// skin
	uint32_t skin_count;

	// sat
	uint32_t lre_sat;

	// local refresh
	uint32_t local_refresh_h;
	uint32_t local_refresh_v;
	uint32_t global_hist_en;
	uint32_t global_hist_start;
	uint32_t global_hist_size;

	// separator screen
	uint32_t screen_mode;
	uint32_t separator;
	uint32_t weight_min_max;

	// local_hist, fna protect
	uint32_t fna_en;
	uint32_t dbg_hiace;
};

struct hiace_info {
	int disp_panel_id;
	uint32_t image_info;
	uint32_t half_block_info;
	uint32_t xyweight;
	uint32_t lhist_sft;
	uint32_t roi_start_point;
	uint32_t roi_width_high;
	uint32_t roi_mode_ctrl;
	uint32_t roi_hist_stat_mode;
	uint32_t hue;
	uint32_t saturation;
	uint32_t value;
	uint32_t skin_gain;
	uint32_t up_low_th;
	uint32_t rgb_blend_weight;
	uint32_t fna_statistic;
	uint32_t up_cnt;
	uint32_t low_cnt;
	uint32_t sum_saturation;
	uint32_t lhist_en;
	uint32_t gamma_w;
	uint32_t gamma_r;
	uint32_t fna_addr;
	uint32_t fna_data;
	uint32_t update_fna;
	uint32_t fna_valid;
	uint32_t db_pipe_cfg;
	uint32_t db_pipe_ext_width;
	uint32_t db_pipe_full_img_width;
	uint32_t bypass_nr;
	uint32_t s3_some_brightness01;
	uint32_t s3_some_brightness23;
	uint32_t s3_some_brightness4;
	uint32_t s3_min_max_sigma;
	uint32_t s3_green_sigma03;
	uint32_t s3_green_sigma45;
	uint32_t s3_red_sigma03;
	uint32_t s3_red_sigma45;
	uint32_t s3_blue_sigma03;
	uint32_t s3_blue_sigma45;
	uint32_t s3_white_sigma03;
	uint32_t s3_white_sigma45;
	uint32_t s3_filter_level;
	uint32_t s3_similarity_coeff;
	uint32_t s3_v_filter_weight_adj;
	uint32_t s3_hue;
	uint32_t s3_saturation;
	uint32_t s3_value;
	uint32_t s3_skin_gain;
	uint32_t param_update;
	uint32_t enable;
	uint32_t enable_update; // 1 valid; 0 invalid
	uint32_t lut_update; // 1 valid; 0 invalid
	uint32_t thminv;
	unsigned int *lut;

	uint32_t hdr10_en;
	uint32_t loglum_max;
	uint32_t hist_modev;
	uint32_t end_point;
	uint32_t bypass_nr_gain;

	struct hiace_v3_register hiace_v3_regs;

	/*hiace HDR LUT*/
	uint32_t table_update;
};


struct dpu_effect_info {
	int disp_panel_id; /* 0: inner panel; 1: outer panel */
	uint32_t modules;

	struct acm_info acm;
	//struct arsr_info arsr[3]; /* 0: 1:1 arsr2p; 1: scale-up arsr2p; 2: scale-down arsr2p */
	struct gamma_info gamma;
	struct hiace_info hiace;
	struct roi_rect dpp_roi[DPP_BUF_ROI_REGION_COUNT]; // only surpport two roi region
};

struct hisi_dpp_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_dpp_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_operator_dpp {
	struct hisi_comp_operator base;

	/* other sdma data */
	struct dss_alsc alsc;
};

void hisi_dpp_init(struct hisi_operator_type *operators, struct dpu_module_ops ops, void *cookie);
#endif /* HISI_OPERATOR_DPP_H */
