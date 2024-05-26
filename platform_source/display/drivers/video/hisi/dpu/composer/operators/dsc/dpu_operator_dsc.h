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

#ifndef DPU_OPERATOR_DSC_H
#define DPU_OPERATOR_DSC_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_composer_operator.h"

struct dpu_dsc_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct dpu_dsc_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct dpu_dsc_params {
	DPU_DSC_VERSION_UNION dsc_version_union;
	DPU_DSC_PPS_IDENTIFIER_UNION dsc_pps_indentifier_union;
	DPU_DSC_EN_UNION dsc_en_union;
	DPU_DSC_CTRL_UNION dsc_ctrl_union;
	DPU_DSC_PIC_SIZE_UNION dsc_pic_size_union;
	DPU_DSC_SLICE_SIZE_UNION dsc_slice_size_union;
	DPU_DSC_CHUNK_SIZE_UNION dsc_chunk_size_union;
	DPU_DSC_INITIAL_DELAY_UNION dsc_initial_delay_union;
	DPU_DSC_RC_PARAM0_UNION dsc_rc_param0_union;
	DPU_DSC_RC_PARAM1_UNION dsc_rc_param1_union;
	DPU_DSC_RC_PARAM2_UNION dsc_rc_param2_union;
	DPU_DSC_RC_PARAM3_UNION dsc_rc_param3_union;
	DPU_DSC_FLATNESS_PARAM_UNION dsc_flatness_param_union;
	DPU_DSC_RC_PARAM4_UNION dsc_rc_param4_union;
	DPU_DSC_RC_PARAM5_UNION dsc_rc_param5_union;
	DPU_DSC_RC_BUF_THRESH0_UNION dsc_rc_buf_thresh0_union;
	DPU_DSC_RC_BUF_THRESH1_UNION dsc_rc_buf_thresh1_union;
	DPU_DSC_RC_BUF_THRESH2_UNION dsc_rc_buf_thresh2_union;
	DPU_DSC_RC_BUF_THRESH3_UNION dsc_rc_buf_thresh3_union;
	DPU_DSC_RC_RANGE_PARAM0_UNION dsc_rc_range_param0_union;
	DPU_DSC_RC_RANGE_PARAM1_UNION dsc_rc_range_param1_union;
	DPU_DSC_RC_RANGE_PARAM2_UNION dsc_rc_range_param2_union;
	DPU_DSC_RC_RANGE_PARAM3_UNION dsc_rc_range_param3_union;
	DPU_DSC_RC_RANGE_PARAM4_UNION dsc_rc_range_param4_union;
	DPU_DSC_RC_RANGE_PARAM5_UNION dsc_rc_range_param5_union;
	DPU_DSC_RC_RANGE_PARAM6_UNION dsc_rc_range_param6_union;
	DPU_DSC_RC_RANGE_PARAM7_UNION dsc_rc_range_param7_union;
	DPU_DSC_ADJUSTMENT_BITS_UNION dsc_adjustment_bits_union;
	DPU_DSC_BITS_PER_GRP_UNION dsc_bits_per_grp_union;
	DPU_DSC_MULTI_SLICE_CTL_UNION dsc_multi_slice_ctl_union;
	DPU_DSC_OUT_CTRL_UNION dsc_out_ctrl_union;
	DPU_DSC_CLK_SEL_UNION dsc_clk_sel_union;
	DPU_DSC_CLK_EN_UNION dsc_clk_en_union;
	DPU_DSC_MEM_CTRL_UNION dsc_mem_ctrl_union;
	DPU_DSC_ST_DATAIN_UNION dsc_st_datain_union;
	DPU_DSC_ST_DATAOUT_UNION dsc_st_dataout_union;
	DPU_DSC_ST_SLC_POS_UNION dsc_st_slc_pos_union;
	DPU_DSC_ST_PIC_POS_UNION dsc_st_pic_pos_union;
	DPU_DSC_ST_FIFO_UNION dsc_st_fifo_union;
	DPU_DSC_ST_LINEBUF_UNION dsc_st_linebuf_union;
	DPU_DSC_ST_ITFC_UNION dsc_st_itfc_union;
	DPU_DSC_REG_DEBUG_UNION dsc_reg_debug_union;
	DPU_DSC_12_UNION dsc_12_union;
	/* csc/padding/txip */
	DPU_DSC_DATAPACK_CTRL_UNION dsc_datapack_ctrl_union;
	DPU_DSC_DATAPACK_SIZE_UNION dsc_datapack_size_union;
	DPU_DSC_RD_SHADOW_UNION dsc_rd_shadow_union;
	DPU_DSC_REG_DEFAULT_UNION dsc_reg_default_union;
	DPU_DSC_TOP_CLK_SEL_UNION dsc_top_clk_sel_union;
	DPU_DSC_TOP_CLK_EN_UNION dsc_top_clk_en_union;
	DPU_DSC_REG_CTRL_UNION dsc_reg_ctrl_union;
	DPU_DSC_REG_CTRL_FLUSH_EN_UNION dsc_reg_ctrl_flush_en_union;
	/* debug items */
};

struct dpu_operator_dsc {
	struct hisi_comp_operator base;

	/* other dsc data */
	struct dpu_dsc_params dsc_params;
};

void dpu_dsc_init(struct hisi_operator_type *operators, struct dpu_module_ops ops, void *cookie);
#endif /* DPU_OPERATOR_DSC_H */
