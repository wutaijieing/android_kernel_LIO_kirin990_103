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

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>

#include "hisi_operators_manager.h"
#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_disp_config.h"
#include "hisi_disp_composer.h"
#include "hisi_operator_tool.h"
#include "dm/hisi_dm.h"
#include "dsc/dpu_operator_dsc.h"
#include "gralloc/hisi_disp_iommu.h"

static void dpu_dsc_calc_dsc_ctrl(struct hisi_panel_info *pinfo, struct dpu_dsc_params *dsc_params)
{
	struct panel_dsc_info *panel_dsc_info = NULL;
	struct dsc_info *dsc_info = NULL;
	uint32_t dsc_en = 0;
	uint32_t dual_dsc_en = 0;
	uint32_t reset_ich_per_line = 0;

	disp_pr_info(" ++++ \n");
	panel_dsc_info = &(pinfo->panel_dsc_info);
	dsc_info = &(panel_dsc_info->dsc_info);
	if ((pinfo->ifbc_type == IFBC_TYPE_VESA2X_SINGLE) ||
		(pinfo->ifbc_type == IFBC_TYPE_VESA3X_SINGLE) ||
		(pinfo->ifbc_type == IFBC_TYPE_VESA3_75X_SINGLE) ||
		(pinfo->ifbc_type == IFBC_TYPE_VESA4X_SINGLE_SPR) ||
		(pinfo->ifbc_type == IFBC_TYPE_VESA2X_SINGLE_SPR)) {
		dsc_en = 0x1;
	} else {
		dsc_en = 0x1;
		dual_dsc_en = 1;
		//reset_ich_per_line = 1;
	}

	if(dsc_info->slice_width != 0 && dsc_info->pic_width / dsc_info->slice_width == 1)
		dual_dsc_en = 0;

	panel_dsc_info->dsc_en = (reset_ich_per_line << 2) | (dual_dsc_en << 1) | dsc_en;

	panel_dsc_info->dual_dsc_en = dual_dsc_en;

	if ((panel_dsc_info->dsc_info.native_422) == 0x1) {
		panel_dsc_info->idata_422 = 0x1;
		panel_dsc_info->dsc_info.convert_rgb = 0x0;
	} else {
		disp_pr_debug("[DSC] native_422 0\n");
		panel_dsc_info->idata_422 = 0x0;
		panel_dsc_info->dsc_info.convert_rgb = 0x1;
	}

	dsc_params->dsc_en_union.value = 0;
	dsc_params->dsc_en_union.reg.dsc_en = dsc_en;
	dsc_params->dsc_en_union.reg.dual_dsc_en = dual_dsc_en;
}

static void dpu_dsc_calculation(struct hisi_panel_info *pinfo)
{
	struct panel_dsc_info *panel_dsc_info = NULL;

	disp_pr_info(" ++++ \n");
	panel_dsc_info = &(pinfo->panel_dsc_info);
	panel_dsc_info->adjustment_bits =
		(8 - (panel_dsc_info->dsc_info.dsc_bpp * (panel_dsc_info->dsc_info.slice_width + 1 - 1)) % 8) % 8;
	disp_pr_debug("adjustment_bits = %d\n", panel_dsc_info->adjustment_bits);
	panel_dsc_info->adj_bits_per_grp = panel_dsc_info->dsc_info.dsc_bpp * 3 - 3;
	disp_pr_debug("adj_bits_per_grp = %d\n", panel_dsc_info->adj_bits_per_grp);
	panel_dsc_info->bits_per_grp = panel_dsc_info->dsc_info.dsc_bpp * 3;
	disp_pr_debug("bits_per_grp = %d\n", panel_dsc_info->bits_per_grp);

	panel_dsc_info->slices_per_line = 0;
	if (panel_dsc_info->dsc_info.native_422 == 0x1)
		panel_dsc_info->pic_line_grp_num =
			(((panel_dsc_info->dsc_info.slice_width + 1) >> 1) + 2) / 3 *
			(panel_dsc_info->slices_per_line + 1) - 1;
	else
		panel_dsc_info->pic_line_grp_num =
			((panel_dsc_info->dsc_info.slice_width + 3) / 3) * (panel_dsc_info->slices_per_line + 1) - 1;
	disp_pr_debug("pic_line_grp_num = %d\n", panel_dsc_info->pic_line_grp_num);
	panel_dsc_info->dsc_insert_byte_num = 0;
	if (panel_dsc_info->dsc_info.chunk_size % 6)
		panel_dsc_info->dsc_insert_byte_num = (6 - panel_dsc_info->dsc_info.chunk_size % 6) & 0x6;
	disp_pr_debug("dsc_insert_byte_num = %d\n", panel_dsc_info->dsc_insert_byte_num);
}

static void dpu_dsc_do_calculation(struct hisi_panel_info *pinfo, struct dpu_dsc_params *dsc_params)
{
	disp_check_and_no_retval((pinfo == NULL), err, "NULL Pointer!\n");
	disp_check_and_no_retval((dsc_params == NULL), err, "NULL Pointer!\n");

	disp_pr_info(" ++++ \n");

	dpu_dsc_calc_dsc_ctrl(pinfo, dsc_params);
	dpu_dsc_calculation(pinfo);
}

static void dpu_dsc_set_basic_params(struct dpu_dsc_params *dsc_params, struct dsc_info *dsc_info)
{
	dsc_params->dsc_pic_size_union.value = 0;
	dsc_params->dsc_pic_size_union.reg.pic_height = dsc_info->pic_height - 1;
	dsc_params->dsc_pic_size_union.reg.pic_width = dsc_info->pic_width- 1;

	dsc_params->dsc_slice_size_union.value = 0;
	dsc_params->dsc_slice_size_union.reg.slice_height = dsc_info->slice_height - 1;
	dsc_params->dsc_slice_size_union.reg.slice_width = dsc_info->slice_width - 1;

	dsc_params->dsc_chunk_size_union.value = 0;
	dsc_params->dsc_chunk_size_union.reg.chunk_size = dsc_info->chunk_size;

	dsc_params->dsc_initial_delay_union.value = 0;
	dsc_params->dsc_initial_delay_union.reg.initial_xmit_delay = dsc_info->initial_xmit_delay;
	dsc_params->dsc_initial_delay_union.reg.initial_dec_delay = dsc_info->initial_dec_delay;

	dsc_params->dsc_rc_param0_union.value = 0;
	dsc_params->dsc_rc_param0_union.reg.initial_scale_value = dsc_info->initial_scale_value;
	dsc_params->dsc_rc_param0_union.reg.scale_increment_interval = dsc_info->scale_increment_interval;

	dsc_params->dsc_rc_param1_union.value = 0;
	dsc_params->dsc_rc_param1_union.reg.scale_decrement_interval = dsc_info->scale_decrement_interval;
	dsc_params->dsc_rc_param1_union.reg.first_line_bpg_offset = dsc_info->first_line_bpg_offset;

	dsc_params->dsc_rc_param2_union.value = 0;
	dsc_params->dsc_rc_param2_union.reg.nfl_bpg_offset = dsc_info->nfl_bpg_offset;
	dsc_params->dsc_rc_param2_union.reg.slice_bpg_offset = dsc_info->slice_bpg_offset;

	dsc_params->dsc_rc_param3_union.value = 0;
	dsc_params->dsc_rc_param3_union.reg.initial_offset = dsc_info->initial_offset;
	dsc_params->dsc_rc_param3_union.reg.final_offset = dsc_info->final_offset;

	dsc_params->dsc_flatness_param_union.value = 0;
	dsc_params->dsc_flatness_param_union.reg.flatness_min_qp =  dsc_info->flatness_min_qp;
	dsc_params->dsc_flatness_param_union.reg.flatness_max_qp =  dsc_info->flatness_max_qp;

	dsc_params->dsc_rc_param4_union.value = 0;
	dsc_params->dsc_rc_param4_union.reg.rc_model_size = dsc_info->rc_model_size;
	dsc_params->dsc_rc_param4_union.reg.rc_edge_factor = dsc_info->rc_edge_factor;

	dsc_params->dsc_rc_param5_union.value = 0;
	dsc_params->dsc_rc_param5_union.reg.rc_quant_incr_limit0 = dsc_info->rc_quant_incr_limit0;
	dsc_params->dsc_rc_param5_union.reg.rc_quant_incr_limit1 = dsc_info->rc_quant_incr_limit1;
	dsc_params->dsc_rc_param5_union.reg.rc_tgt_offset_hi = dsc_info->rc_tgt_offset_hi;
	dsc_params->dsc_rc_param5_union.reg.rc_tgt_offset_lo = dsc_info->rc_tgt_offset_lo;
}

static void dpu_dsc_set_rc_thresh_params(struct dpu_dsc_params *dsc_params, struct dsc_info *dsc_info)
{
	dsc_params->dsc_rc_buf_thresh0_union.value = 0;
	dsc_params->dsc_rc_buf_thresh0_union.reg.rc_buf_thresh0 = dsc_info->rc_buf_thresh[0];
	dsc_params->dsc_rc_buf_thresh0_union.reg.rc_buf_thresh1 = dsc_info->rc_buf_thresh[1];
	dsc_params->dsc_rc_buf_thresh0_union.reg.rc_buf_thresh2 = dsc_info->rc_buf_thresh[2];
	dsc_params->dsc_rc_buf_thresh0_union.reg.rc_buf_thresh3 = dsc_info->rc_buf_thresh[3];

	dsc_params->dsc_rc_buf_thresh1_union.value = 0;
	dsc_params->dsc_rc_buf_thresh1_union.reg.rc_buf_thresh4 = dsc_info->rc_buf_thresh[4];
	dsc_params->dsc_rc_buf_thresh1_union.reg.rc_buf_thresh5 = dsc_info->rc_buf_thresh[5];
	dsc_params->dsc_rc_buf_thresh1_union.reg.rc_buf_thresh6 = dsc_info->rc_buf_thresh[6];
	dsc_params->dsc_rc_buf_thresh1_union.reg.rc_buf_thresh7 = dsc_info->rc_buf_thresh[7];

	dsc_params->dsc_rc_buf_thresh2_union.value = 0;
	dsc_params->dsc_rc_buf_thresh2_union.reg.rc_buf_thresh8 = dsc_info->rc_buf_thresh[8];
	dsc_params->dsc_rc_buf_thresh2_union.reg.rc_buf_thresh9 = dsc_info->rc_buf_thresh[9];
	dsc_params->dsc_rc_buf_thresh2_union.reg.rc_buf_thresh10 = dsc_info->rc_buf_thresh[10];
	dsc_params->dsc_rc_buf_thresh2_union.reg.rc_buf_thresh11 = dsc_info->rc_buf_thresh[11];

	dsc_params->dsc_rc_buf_thresh3_union.value = 0;
	dsc_params->dsc_rc_buf_thresh3_union.reg.rc_buf_thresh12 = dsc_info->rc_buf_thresh[12];
	dsc_params->dsc_rc_buf_thresh3_union.reg.rc_buf_thresh13 = dsc_info->rc_buf_thresh[13];
}

static void dpu_dsc_set_rc_range_params(struct dpu_dsc_params *dsc_params, struct dsc_info *dsc_info)
{
	dsc_params->dsc_rc_range_param0_union.value = 0;
	dsc_params->dsc_rc_range_param0_union.reg.range_min_qp0 = dsc_info->rc_range[0].min_qp;
	dsc_params->dsc_rc_range_param0_union.reg.range_max_qp0 = dsc_info->rc_range[0].max_qp;
	dsc_params->dsc_rc_range_param0_union.reg.range_bpg_offset0 = dsc_info->rc_range[0].offset;
	dsc_params->dsc_rc_range_param0_union.reg.range_min_qp1 = dsc_info->rc_range[1].min_qp;
	dsc_params->dsc_rc_range_param0_union.reg.range_max_qp1 = dsc_info->rc_range[1].max_qp;
	dsc_params->dsc_rc_range_param0_union.reg.range_bpg_offset1 = dsc_info->rc_range[1].offset;

	dsc_params->dsc_rc_range_param1_union.value = 0;
	dsc_params->dsc_rc_range_param1_union.reg.range_min_qp2 = dsc_info->rc_range[2].min_qp;
	dsc_params->dsc_rc_range_param1_union.reg.range_max_qp2 = dsc_info->rc_range[2].max_qp;
	dsc_params->dsc_rc_range_param1_union.reg.range_bpg_offset2 = dsc_info->rc_range[2].offset;
	dsc_params->dsc_rc_range_param1_union.reg.range_min_qp3 = dsc_info->rc_range[3].min_qp;
	dsc_params->dsc_rc_range_param1_union.reg.range_max_qp3 = dsc_info->rc_range[3].max_qp;
	dsc_params->dsc_rc_range_param1_union.reg.range_bpg_offset3 = dsc_info->rc_range[3].offset;

	dsc_params->dsc_rc_range_param2_union.value = 0;
	dsc_params->dsc_rc_range_param2_union.reg.range_min_qp4 = dsc_info->rc_range[4].min_qp;
	dsc_params->dsc_rc_range_param2_union.reg.range_max_qp4 = dsc_info->rc_range[4].max_qp;
	dsc_params->dsc_rc_range_param2_union.reg.range_bpg_offset4 = dsc_info->rc_range[4].offset;
	dsc_params->dsc_rc_range_param2_union.reg.range_min_qp5 = dsc_info->rc_range[5].min_qp;
	dsc_params->dsc_rc_range_param2_union.reg.range_max_qp5 = dsc_info->rc_range[5].max_qp;
	dsc_params->dsc_rc_range_param2_union.reg.range_bpg_offset5 = dsc_info->rc_range[5].offset;

	dsc_params->dsc_rc_range_param3_union.value = 0;
	dsc_params->dsc_rc_range_param3_union.reg.range_min_qp6 = dsc_info->rc_range[6].min_qp;
	dsc_params->dsc_rc_range_param3_union.reg.range_max_qp6 = dsc_info->rc_range[6].max_qp;
	dsc_params->dsc_rc_range_param3_union.reg.range_bpg_offset6 = dsc_info->rc_range[6].offset;
	dsc_params->dsc_rc_range_param3_union.reg.range_min_qp7 = dsc_info->rc_range[7].min_qp;
	dsc_params->dsc_rc_range_param3_union.reg.range_max_qp7 = dsc_info->rc_range[7].max_qp;
	dsc_params->dsc_rc_range_param3_union.reg.range_bpg_offset7 = dsc_info->rc_range[7].offset;

	dsc_params->dsc_rc_range_param4_union.value = 0;
	dsc_params->dsc_rc_range_param4_union.reg.range_min_qp8 = dsc_info->rc_range[8].min_qp;
	dsc_params->dsc_rc_range_param4_union.reg.range_max_qp8 = dsc_info->rc_range[8].max_qp;
	dsc_params->dsc_rc_range_param4_union.reg.range_bpg_offset8 = dsc_info->rc_range[8].offset;
	dsc_params->dsc_rc_range_param4_union.reg.range_min_qp9 = dsc_info->rc_range[9].min_qp;
	dsc_params->dsc_rc_range_param4_union.reg.range_max_qp9 = dsc_info->rc_range[9].max_qp;
	dsc_params->dsc_rc_range_param4_union.reg.range_bpg_offset9 = dsc_info->rc_range[9].offset;

	dsc_params->dsc_rc_range_param5_union.value = 0;
	dsc_params->dsc_rc_range_param5_union.reg.range_min_qp10 = dsc_info->rc_range[10].min_qp;
	dsc_params->dsc_rc_range_param5_union.reg.range_max_qp10 = dsc_info->rc_range[10].max_qp;
	dsc_params->dsc_rc_range_param5_union.reg.range_bpg_offset10 = dsc_info->rc_range[10].offset;
	dsc_params->dsc_rc_range_param5_union.reg.range_min_qp11 = dsc_info->rc_range[11].min_qp;
	dsc_params->dsc_rc_range_param5_union.reg.range_max_qp11 = dsc_info->rc_range[11].max_qp;
	dsc_params->dsc_rc_range_param5_union.reg.range_bpg_offset11 = dsc_info->rc_range[11].offset;

	dsc_params->dsc_rc_range_param6_union.value = 0;
	dsc_params->dsc_rc_range_param6_union.reg.range_min_qp12 = dsc_info->rc_range[12].min_qp;
	dsc_params->dsc_rc_range_param6_union.reg.range_max_qp12 = dsc_info->rc_range[12].max_qp;
	dsc_params->dsc_rc_range_param6_union.reg.range_bpg_offset12 = dsc_info->rc_range[12].offset;
	dsc_params->dsc_rc_range_param6_union.reg.range_min_qp13 = dsc_info->rc_range[13].min_qp;
	dsc_params->dsc_rc_range_param6_union.reg.range_max_qp13 = dsc_info->rc_range[13].max_qp;
	dsc_params->dsc_rc_range_param6_union.reg.range_bpg_offset13 = dsc_info->rc_range[13].offset;

	dsc_params->dsc_rc_range_param7_union.value = 0;
	dsc_params->dsc_rc_range_param7_union.reg.range_min_qp14 = dsc_info->rc_range[14].min_qp;
	dsc_params->dsc_rc_range_param7_union.reg.range_max_qp14 = dsc_info->rc_range[14].max_qp;
	dsc_params->dsc_rc_range_param7_union.reg.range_bpg_offset14 = dsc_info->rc_range[14].offset;
}

static void dpu_dsc_set_out_ctrl_params(struct dpu_dsc_params *dsc_params, struct panel_dsc_info *dsc)
{
	dsc_params->dsc_out_ctrl_union.value = 0;
	if (((dsc->dsc_info.chunk_size + dsc->dsc_insert_byte_num) % 6) == 0) {
		dsc_params->dsc_out_ctrl_union.reg.dsc_out_mode = 0;
		disp_pr_debug("dsc->dsc_insert_byte_num 6\n");
	} else if (((dsc->dsc_info.chunk_size + dsc->dsc_insert_byte_num) % 4) == 0) {
		dsc_params->dsc_out_ctrl_union.reg.dsc_out_mode = 1;
		disp_pr_debug("dsc->dsc_insert_byte_num 4\n");
	} else {
		disp_pr_err("chunk_size should be mode by 3 or 2, but chunk_size = %u\n", dsc->dsc_info.chunk_size);
		return;
	}
}

static void dpu_dsc_set_rc_regs(struct dpu_operator_dsc *operator_dsc, struct hisi_composer_device *composer_device)
{
	uint32_t dsc_offset = NULL;
	char __iomem *dsc_base = NULL;
	struct dpu_dsc_params *dsc_params = NULL;
	struct panel_dsc_info *panel_dsc_info = NULL;

	disp_pr_info(" ++++ \n");
	dsc_offset = operator_dsc->base.id_desc.info.idx ? DSS_DSC1_OFFSET : DSS_DSC_OFFSET;
	dsc_base = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU] + dsc_offset;
	dsc_params = &operator_dsc->dsc_params;

	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_PARAM0_ADDR(dsc_base),
		dsc_params->dsc_rc_param0_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_PARAM1_ADDR(dsc_base),
		dsc_params->dsc_rc_param1_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_PARAM2_ADDR(dsc_base),
		dsc_params->dsc_rc_param2_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_PARAM3_ADDR(dsc_base),
		dsc_params->dsc_rc_param3_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_FLATNESS_PARAM_ADDR(dsc_base),
		dsc_params->dsc_flatness_param_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_PARAM4_ADDR(dsc_base),
		dsc_params->dsc_rc_param4_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_PARAM5_ADDR(dsc_base),
		dsc_params->dsc_rc_param5_union.value);

	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_BUF_THRESH0_ADDR(dsc_base),
		dsc_params->dsc_rc_buf_thresh0_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_BUF_THRESH1_ADDR(dsc_base),
		dsc_params->dsc_rc_buf_thresh1_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_BUF_THRESH2_ADDR(dsc_base),
		dsc_params->dsc_rc_buf_thresh2_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_BUF_THRESH3_ADDR(dsc_base),
		dsc_params->dsc_rc_buf_thresh3_union.value);

	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_RANGE_PARAM0_ADDR(dsc_base),
		dsc_params->dsc_rc_range_param0_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_RANGE_PARAM1_ADDR(dsc_base),
		dsc_params->dsc_rc_range_param1_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_RANGE_PARAM2_ADDR(dsc_base),
		dsc_params->dsc_rc_range_param2_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_RANGE_PARAM3_ADDR(dsc_base),
		dsc_params->dsc_rc_range_param3_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_RANGE_PARAM4_ADDR(dsc_base),
		dsc_params->dsc_rc_range_param4_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_RANGE_PARAM5_ADDR(dsc_base),
		dsc_params->dsc_rc_range_param5_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_RANGE_PARAM6_ADDR(dsc_base),
		dsc_params->dsc_rc_range_param6_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_RC_RANGE_PARAM7_ADDR(dsc_base),
		dsc_params->dsc_rc_range_param7_union.value);
}

static int dpu_dsc_set_dsc_params(struct dpu_operator_dsc *operator_dsc, struct hisi_composer_device *composer_device)
{
	uint32_t slice_num = 0;
	uint32_t dsc_offset = NULL;
	char __iomem *dsc_base = NULL;
	struct dpu_dsc_params *dsc_params = NULL;
	struct panel_dsc_info *panel_dsc_info = NULL;

	disp_pr_info(" ++++ \n");
	dsc_offset = operator_dsc->base.id_desc.info.idx ? DSS_DSC1_OFFSET : DSS_DSC_OFFSET;
	dsc_base = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU] + dsc_offset;
	dsc_params = &operator_dsc->dsc_params;
	panel_dsc_info = &(composer_device->ov_data.fix_panel_info->panel_dsc_info);

	dpu_set_reg(DPU_DSC_REG_DEFAULT_ADDR(dsc_base), 0x1, 1, 0); /* 0x1 --> reset dsc reg */
	dpu_set_reg(DPU_DSC_REG_DEFAULT_ADDR(dsc_base), 0x0, 1, 0); /* 0x0 --> disreset dsc reg */

	dsc_params->dsc_pic_size_union.value = 0;
	dsc_params->dsc_pic_size_union.reg.pic_height = composer_device->ov_data.fix_panel_info->yres - 1;
	dsc_params->dsc_pic_size_union.reg.pic_width = composer_device->ov_data.fix_panel_info->xres - 1;

	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_PIC_SIZE_ADDR(dsc_base),
		dsc_params->dsc_pic_size_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_SLICE_SIZE_ADDR(dsc_base),
		dsc_params->dsc_slice_size_union.value);

	if (composer_device->ov_data.fix_panel_info->ifbc_type == IFBC_TYPE_NONE)
		return 0;

	dpu_dsc_do_calculation(composer_device->ov_data.fix_panel_info, dsc_params);

	dsc_params->dsc_version_union.value = 0;
	dsc_params->dsc_version_union.reg.dsc_version_major= panel_dsc_info->dsc_info.dsc_version_major;
	dsc_params->dsc_version_union.reg.dsc_version_minor= panel_dsc_info->dsc_info.dsc_version_minor;

	/* set dsc_params->dsc_en_union in dpu_dsc_do_calculation */

	dsc_params->dsc_ctrl_union.value = 0;
	dsc_params->dsc_ctrl_union.reg.bits_per_component = panel_dsc_info->dsc_info.dsc_bpc;
	dsc_params->dsc_ctrl_union.reg.linebuf_depth = panel_dsc_info->dsc_info.linebuf_depth;
	dsc_params->dsc_ctrl_union.reg.dsc_insert_byte_num = panel_dsc_info->dsc_insert_byte_num;
	dsc_params->dsc_ctrl_union.reg.convert_rgb = panel_dsc_info->dsc_info.convert_rgb;
	dsc_params->dsc_ctrl_union.reg.native_422 = panel_dsc_info->dsc_info.native_422;
	/* check this bit: dsc_params->dsc_ctrl_union.reg.vbr_enable */
	dsc_params->dsc_ctrl_union.reg.bits_per_pixel = 12; // 2x compress, 15 for rgb10bit
	dsc_params->dsc_ctrl_union.reg.block_pred_enable = panel_dsc_info->dsc_info.block_pred_enable;
	dsc_params->dsc_ctrl_union.reg.idata_422 = panel_dsc_info->idata_422;
	dsc_params->dsc_ctrl_union.reg.full_ich_err_precision = 0;

	dpu_dsc_set_basic_params(dsc_params, &panel_dsc_info->dsc_info);
	dpu_dsc_set_rc_thresh_params(dsc_params, &panel_dsc_info->dsc_info);
	dpu_dsc_set_rc_range_params(dsc_params, &panel_dsc_info->dsc_info);

	dsc_params->dsc_multi_slice_ctl_union.value = 0;
	dsc_params->dsc_multi_slice_ctl_union.reg.slices_per_line = 0;
	if (panel_dsc_info->dsc_info.slice_width != 0)
		slice_num = panel_dsc_info->dsc_info.pic_width / panel_dsc_info->dsc_info.slice_width;
	if (slice_num == 4)
		dsc_params->dsc_multi_slice_ctl_union.reg.slices_per_line = 1;

	dpu_dsc_set_out_ctrl_params(dsc_params, panel_dsc_info);

	dsc_params->dsc_12_union.value = 0;
	dsc_params->dsc_12_union.reg.flatness_det_thresh = 2 << (panel_dsc_info->dsc_info.dsc_bpc - 8);
	dsc_params->dsc_reg_ctrl_union.value = 0;
	dsc_params->dsc_reg_ctrl_union.reg.reg_ctrl_scene_id = operator_dsc->base.scene_id;
	dsc_params->dsc_reg_ctrl_flush_en_union.value = 0;
	dsc_params->dsc_reg_ctrl_flush_en_union.reg.reg_ctrl_flush_en = 1;

	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_VERSION_ADDR(dsc_base),
		dsc_params->dsc_version_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_EN_ADDR(dsc_base),
		dsc_params->dsc_en_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_CTRL_ADDR(dsc_base),
		dsc_params->dsc_ctrl_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_PIC_SIZE_ADDR(dsc_base),
		dsc_params->dsc_pic_size_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_SLICE_SIZE_ADDR(dsc_base),
		dsc_params->dsc_slice_size_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_CHUNK_SIZE_ADDR(dsc_base),
		dsc_params->dsc_chunk_size_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_INITIAL_DELAY_ADDR(dsc_base),
		dsc_params->dsc_initial_delay_union.value);

	dpu_dsc_set_rc_regs(operator_dsc, composer_device);

	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_MULTI_SLICE_CTL_ADDR(dsc_base),
		dsc_params->dsc_multi_slice_ctl_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_OUT_CTRL_ADDR(dsc_base),
		dsc_params->dsc_out_ctrl_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_12_ADDR(dsc_base),
		dsc_params->dsc_12_union.value);
	hisi_module_set_reg(&operator_dsc->base.module_desc, DPU_DSC_REG_CTRL_FLUSH_EN_ADDR(dsc_base),
		dsc_params->dsc_reg_ctrl_flush_en_union.value);

	return 0;
}

static int dpu_dsc_set_dm_dscinfo(struct hisi_comp_operator *operator,
		struct hisi_dm_dsc_info *dm_dsc_info, struct hisi_panel_info *fix_panel_info)
{
	struct panel_dsc_info *panel_dsc_info = NULL;

	panel_dsc_info = &(fix_panel_info->panel_dsc_info);

	disp_pr_info(" ++++ \n");

	if (fix_panel_info->ifbc_type != IFBC_TYPE_NONE) {
		// DPU_DM_DSC_INPUT_IMG_WIDTH_UNION
		dm_dsc_info->dsc_input_img_width_union.value = 0;
		dm_dsc_info->dsc_input_img_width_union.reg.dsc_input_img_height = panel_dsc_info->dsc_info.pic_height - 1;
		dm_dsc_info->dsc_input_img_width_union.reg.dsc_input_img_width = panel_dsc_info->dsc_info.pic_width - 1;
		// DPU_DM_DSC_OUTPUT_IMG_WIDTH_UNION
		dm_dsc_info->dsc_output_img_width_union.value = 0;
		dm_dsc_info->dsc_output_img_width_union.reg.dsc_output_img_height = panel_dsc_info->dsc_info.output_height - 1;
		dm_dsc_info->dsc_output_img_width_union.reg.dsc_output_img_width = panel_dsc_info->dsc_info.output_width - 1;
	} else {
		// DPU_DM_DSC_INPUT_IMG_WIDTH_UNION
		dm_dsc_info->dsc_input_img_width_union.value = 0;
		dm_dsc_info->dsc_input_img_width_union.reg.dsc_input_img_height = fix_panel_info->yres- 1;
		dm_dsc_info->dsc_input_img_width_union.reg.dsc_input_img_width = fix_panel_info->xres - 1;
		// DPU_DM_DSC_OUTPUT_IMG_WIDTH_UNION
		dm_dsc_info->dsc_output_img_width_union.value = 0;
		dm_dsc_info->dsc_output_img_width_union.reg.dsc_output_img_height = fix_panel_info->yres- 1;
		dm_dsc_info->dsc_output_img_width_union.reg.dsc_output_img_width = fix_panel_info->xres - 1;
	}

	// DPU_DM_DSC_SEL_UNION
	disp_pr_debug(" operator->in_data->next_order:%u \n", operator->in_data->next_order);
	disp_pr_debug(" hisi_get_op_id_by_op_type:%d \n", hisi_get_op_id_by_op_type(operator->in_data->next_order));
	disp_pr_debug(" dsc_sel:%d \n", BIT(operator->id_desc.info.idx));
	dm_dsc_info->dsc_sel_union.value = 0;
	dm_dsc_info->dsc_sel_union.reg.dsc_sel = BIT(operator->id_desc.info.idx);
	dm_dsc_info->dsc_sel_union.reg.dsc_layer_id = 31;
	dm_dsc_info->dsc_sel_union.reg.dsc_order0 = hisi_get_op_id_by_op_type(operator->in_data->next_order);
	dm_dsc_info->dsc_sel_union.reg.dsc_order1 = 31;
	// DPU_DM_DSC_RESERVED_UNION
	dm_dsc_info->dsc_reserved_union.value = 0;
	dm_dsc_info->dsc_reserved_union.reg.dsc_input_fmt = operator->in_data->format;
	dm_dsc_info->dsc_reserved_union.reg.dsc_output_fmt = operator->out_data->format;

	return 0;
}

static void dpu_dsc_set_regs(struct hisi_comp_operator *operator,
	struct hisi_dm_dsc_info *dsc_info, char __iomem *dpu_base_addr)
{
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);

	hisi_module_set_reg(&operator->module_desc, DPU_DM_DSC_INPUT_IMG_WIDTH_ADDR(dm_base), dsc_info->dsc_input_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_DSC_OUTPUT_IMG_WIDTH_ADDR(dm_base), dsc_info->dsc_output_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_DSC_SEL_ADDR(dm_base), dsc_info->dsc_sel_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_DSC_RESERVED_ADDR(dm_base), dsc_info->dsc_reserved_union.value);
}

static void dpu_dsc_flush_regs(char __iomem *dpu_base_addr)
{
	disp_pr_info(" ++++ \n");
	dpu_set_reg(DPU_DSC_REG_CTRL_FLUSH_EN_ADDR(dpu_base_addr + DSS_DSC_OFFSET), 1, 1, 0);
}

static int dpu_dsc_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
	void *in_dirty_region)
{
	int ret;
	struct dpu_operator_dsc *operator_dsc = NULL;
	struct hisi_dm_param *dm_param = NULL;
	struct post_online_info *dirty_region = (struct post_online_info *)in_dirty_region;
	struct hisi_dm_dsc_info *dm_dsc_info = NULL;
	char __iomem *dsc_base = NULL;
	uint32_t dsc_offset = NULL;
	operator_dsc = (struct dpu_operator_dsc *)operator;

	disp_pr_info(" ++++ \n");
	dsc_offset = operator_dsc->base.id_desc.info.idx ? DSS_DSC1_OFFSET : DSS_DSC_OFFSET;
	dsc_base = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU] + dsc_offset;

	dm_param = composer_device->ov_data.dm_param;
	dm_dsc_info = &dm_param->dsc_info[0];

	ret = dpu_dsc_set_dm_dscinfo(operator, dm_dsc_info, composer_device->ov_data.fix_panel_info);
	if (ret)
		return -1;

	if (composer_device->ov_data.fix_panel_info->ifbc_type != IFBC_TYPE_NONE) {
	dirty_region->dirty_rect.w = composer_device->ov_data.fix_panel_info->panel_dsc_info.dsc_info.output_width;
	dirty_region->dirty_rect.h = composer_device->ov_data.fix_panel_info->panel_dsc_info.dsc_info.output_height;
	} else {
		dirty_region->dirty_rect.w = composer_device->ov_data.fix_panel_info->xres;
		dirty_region->dirty_rect.h = composer_device->ov_data.fix_panel_info->yres;
	}

	ret = dpu_dsc_set_dsc_params(operator_dsc, composer_device);
	if (ret)
		return -1;

	dpu_dsc_set_regs(operator, dm_dsc_info, composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);
	if (composer_device->ov_data.fix_panel_info->ifbc_type != IFBC_TYPE_NONE) {
		dpu_dsc_flush_regs(composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);
	} else {
		dpu_set_reg(DPU_DSC_EN_ADDR(dsc_base), 0, 1, 0);
		dpu_set_reg(DPU_DSC_REG_CTRL_FLUSH_EN_ADDR(dsc_base), 1, 1, 0);
	}
	dpu_set_reg(DPU_DSC_REG_CTRL_DEBUG_ADDR(dsc_base), 1, 1, 10);
	disp_pr_info(" ---- \n");
	return 0;
}

static int dpu_dsc_build_data(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *pre_out_data,
		struct hisi_comp_operator *next_operator)
{
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;
	disp_pr_info(" ++++ \n");

	operator->in_data->rect.x = src_layer->src_rect.x;
	operator->in_data->rect.y = src_layer->src_rect.y;
	operator->in_data->rect.w = src_layer->src_rect.w;
	operator->in_data->rect.h = src_layer->src_rect.h;
	if (src_layer->is_cld_layer) {
		operator->in_data->rect.w = src_layer->cld_width;
		operator->in_data->rect.h = src_layer->cld_height;
	}

	if (g_local_dimming_en) {
		operator->in_data->format = RGB_10BIT;
	} else {
		operator->in_data->format = ARGB10101010;
	}

	memcpy(operator->out_data, operator->in_data, sizeof(*(operator->in_data)));

	dpu_operator_build_data(operator, layer, pre_out_data, next_operator);

	return 0;
}

void dpu_dsc_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct dpu_operator_dsc **dscs = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info(" ++++ \n");
	dscs = kzalloc(sizeof(*dscs) * type_operator->operator_count, GFP_KERNEL);
	if (!dscs)
		return;
	disp_pr_info(" type_operator->operator_count:%u \n", type_operator->operator_count);
	for (i = 0; i < type_operator->operator_count; i++) {
		disp_pr_info(" size of struct hisi_operator_dsc:%u \n", sizeof(*(dscs[i])));
		dscs[i] = kzalloc(sizeof(*(dscs[i])), GFP_KERNEL);
		if (!dscs[i])
			continue;

		base = &dscs[i]->base;
		hisi_operator_init(&dscs[i]->base, ops, cookie);

		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		disp_pr_ops(info, base->id_desc);

		base->set_cmd_item = dpu_dsc_set_cmd_item;
		base->build_data = dpu_dsc_build_data;

		base->out_data = kzalloc(sizeof(struct dpu_dsc_out_data), GFP_KERNEL);
		if (!base->out_data) {
			disp_pr_err("alloc out_data failed\n");
			kfree(dscs[i]);
			dscs[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct dpu_dsc_in_data), GFP_KERNEL);
		if (!base->in_data) {
			disp_pr_err("alloc in_data failed\n");
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(dscs[i]);
			dscs[i] = NULL;
			continue;
		}

		/* TODO: init other rmda operators */

		dscs[i]->base.be_dm_counted = false;
	}

	type_operator->operators = (struct hisi_comp_operator **)(dscs);
}
