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
#ifndef HISI_DISP_ITFSW_H
#define HISI_DISP_ITFSW_H

#include "hisi_composer_operator.h"
#include "soc_dte_define.h"

struct hisi_itfsw_params {
	DPU_ITF_CH_IMG_SIZE_UNION img_size_union;
	DPU_ITF_CH_DPP_CLIP_TOP_UNION dpp_clip_top_union;
	DPU_ITF_CH_DPP_CLIP_BOTTOM_UNION dpp_clip_bottom_union;
	DPU_ITF_CH_DPP_CLIP_LEFT_UNION dpp_clip_left_union;
	DPU_ITF_CH_DPP_CLIP_RIGHT_UNION dpp_clip_right_union;
	DPU_ITF_CH_DPP_CLIP_EN_UNION dpp_clip_en_union;
	DPU_ITF_CH_DPP_CLIP_DBG_UNION dpp_clip_dbg_union;
	DPU_ITF_CH_WB_UNPACK_POST_CLIP_DISP_SIZE_UNION wb_unpack_post_clip_disp_size_union;
	DPU_ITF_CH_WB_UNPACK_POST_CLIP_CTL_HRZ_UNION wb_unpack_post_clip_ctl_hrz_union;
	DPU_ITF_CH_WB_UNPACK_EN_UNION wb_unpack_en_union;
	DPU_ITF_CH_WB_UNPACK_SIZE_UNION wb_unpack_size_union;
	DPU_ITF_CH_WB_UNPACK_IMG_FMT_UNION wb_unpack_img_fmt_union;
	DPU_ITF_CH_DPP_CLRBAR_CTRL_UNION dpp_clrbar_ctrl_union;
	DPU_ITF_CH_DPP_CLRBAR_1ST_CLR_UNION dpp_clrbar_1st_clr_union;
	DPU_ITF_CH_DPP_CLRBAR_2ND_CLR_UNION dpp_clrbar_2nd_clr_union;
	DPU_ITF_CH_DPP_CLRBAR_3RD_CLR_UNION dpp_clrbar_3rd_clr_union;
	DPU_ITF_CH_ITFSW_RD_SHADOW_UNION itfsw_rd_shadow_union;
	DPU_ITF_CH_HIDIC_MODE_UNION hidic_mode_union;
	DPU_ITF_CH_HIDIC_DIMENSION_UNION hidic_dimension_union;
	DPU_ITF_CH_HIDIC_CMP_RESERVED_UNION hidic_cmp_reserved_union;
	DPU_ITF_CH_HIDIC_DBG_OUT_HI_UNION hidic_dbg_out_hi_union;
	DPU_ITF_CH_HIDIC_DBG_OUT_LO_UNION hidic_dbg_out_lo_union;
	DPU_WCH_REG_CTRL_UNION reg_ctrl_union;
	DPU_WCH_REG_CTRL_FLUSH_EN_UNION reg_ctrl_flush_en_union;
	DPU_WCH_REG_CTRL_DEBUG_UNION reg_ctrl_debug_union;
	DPU_WCH_REG_CTRL_STATE_UNION reg_ctrl_state_union;
	DPU_ITF_CH_ITFSW_DATA_SEL_UNION itfsw_data_sel_union;
	DPU_ITF_CH_CLRBAR_START_UNION clrbar_start_union;
	DPU_ITF_CH_ITFCH_CLK_CTL_UNION itfch_clk_ctl_union;
	DPU_ITF_CH_VSYNC_DELAY_CNT_CTRL_UNION vsync_delay_cnt_ctrl_union;
	DPU_ITF_CH_VSYNC_DELAY_CNT_UNION vsync_delay_cnt_union;
};

struct hisi_itfsw_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_itfsw_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_operator_itfsw {
	struct hisi_comp_operator base;

	/* other overlay data */
	struct hisi_itfsw_params itfsw_params;
};

void hisi_itfsw_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie);
uint32_t platform_init_pipe_sw(char __iomem *dpu_base_addr, uint32_t scene_id, uint32_t pipe_sw_post_idx);
#endif
