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

#include "dsc_config.h"
#include "dkmd_log.h"
#include "dkmd_object.h"
#include "dpu/soc_dpu_define.h"

static void dss_dsc_version_cfg(char __iomem * dsc_base, struct dsc_calc_info *dsc)
{
	uint32_t version = (dsc->dsc_info.dsc_version_minor == DSC_V_1_2) ? 2 : 1;

	set_reg(DPU_DSC_EN_ADDR(dsc_base), dsc->dsc_en, 2, 0);
	set_reg(DPU_DSC_VERSION_ADDR(dsc_base), version, 4, 0);
}

static void dss_dsc_ctrl_cfg(char __iomem * dsc_base, struct dsc_calc_info *dsc)
{
	DPU_DSC_CTRL_UNION ctrl = { .value = 0 };

	ctrl.reg.bits_per_component = dsc->dsc_info.dsc_bpc;
	ctrl.reg.linebuf_depth = dsc->dsc_info.linebuf_depth;
	ctrl.reg.dsc_insert_byte_num = dsc->dsc_insert_byte_num;
	ctrl.reg.convert_rgb = dsc->dsc_info.convert_rgb;
	ctrl.reg.native_422 = dsc->dsc_info.native_422;
	ctrl.reg.bits_per_pixel = dsc->dsc_info.dsc_bpp;
	ctrl.reg.block_pred_enable = dsc->dsc_info.block_pred_enable;
	ctrl.reg.idata_422 = dsc->idata_422;
	set_reg(DPU_DSC_CTRL_ADDR(dsc_base), ctrl.value, 32, 0);

	if (dsc->dsc_info.dsc_version_minor == DSC_V_1_2)
		set_reg(DPU_DSC_12_ADDR(dsc_base), 0x2, 10, 0);
}

static void dss_dsc_basic_cfg(char __iomem * dsc_base, struct dsc_calc_info *dsc)
{
	DPU_DSC_PIC_SIZE_UNION pic_size = { .value = 0 };
	DPU_DSC_SLICE_SIZE_UNION slice_size = { .value = 0 };
	DPU_DSC_INITIAL_DELAY_UNION initial_delay = { .value = 0 };
	DPU_DSC_RC_PARAM0_UNION rc_param0 = { .value = 0 };
	DPU_DSC_RC_PARAM1_UNION rc_param1 = { .value = 0 };
	DPU_DSC_RC_PARAM2_UNION rc_param2 = { .value = 0 };
	DPU_DSC_RC_PARAM3_UNION rc_param3 = { .value = 0 };
	DPU_DSC_RC_PARAM4_UNION rc_param4 = { .value = 0 };
	DPU_DSC_RC_PARAM5_UNION rc_param5 = { .value = 0 };
	DPU_DSC_FLATNESS_PARAM_UNION flatness = { .value = 0 };

	pic_size.reg.pic_height = dsc->dsc_info.pic_height - 1;
	pic_size.reg.pic_width = dsc->dsc_info.pic_width - 1;
	set_reg(DPU_DSC_PIC_SIZE_ADDR(dsc_base), pic_size.value, 32, 0);

	slice_size.reg.slice_height = dsc->dsc_info.slice_height - 1;
	slice_size.reg.slice_width = dsc->dsc_info.slice_width - 1;
	set_reg(DPU_DSC_SLICE_SIZE_ADDR(dsc_base), slice_size.value, 32, 0);

	// chunk_size
	set_reg(DPU_DSC_CHUNK_SIZE_ADDR(dsc_base), dsc->dsc_info.chunk_size, 16, 0);

	// initial_xmit_delay, initial_dec_delay = hrd_delay -initial_xmit_delay
	initial_delay.reg.initial_xmit_delay = dsc->dsc_info.initial_xmit_delay;
	initial_delay.reg.initial_dec_delay = dsc->dsc_info.initial_dec_delay;
	set_reg(DPU_DSC_INITIAL_DELAY_ADDR(dsc_base), initial_delay.value, 32, 0);

	rc_param0.reg.initial_scale_value = dsc->dsc_info.initial_scale_value;
	rc_param0.reg.scale_increment_interval = dsc->dsc_info.scale_increment_interval;
	set_reg(DPU_DSC_RC_PARAM0_ADDR(dsc_base), rc_param0.value, 32, 0);

	rc_param1.reg.scale_decrement_interval = dsc->dsc_info.scale_decrement_interval;
	rc_param1.reg.first_line_bpg_offset = dsc->dsc_info.first_line_bpg_offset;
	set_reg(DPU_DSC_RC_PARAM1_ADDR(dsc_base), rc_param1.value, 32, 0);

	rc_param2.reg.nfl_bpg_offset = dsc->dsc_info.nfl_bpg_offset;
	rc_param2.reg.slice_bpg_offset = dsc->dsc_info.slice_bpg_offset;
	set_reg(DPU_DSC_RC_PARAM2_ADDR(dsc_base), rc_param2.value, 32, 0);

	rc_param3.reg.initial_offset = dsc->dsc_info.initial_offset;
	rc_param3.reg.final_offset = dsc->dsc_info.final_offset;
	set_reg(DPU_DSC_RC_PARAM3_ADDR(dsc_base), rc_param3.value, 32, 0);

	flatness.reg.flatness_min_qp = dsc->dsc_info.flatness_min_qp;
	flatness.reg.flatness_max_qp = dsc->dsc_info.flatness_max_qp;
	if (dsc->dsc_info.dsc_version_minor == DSC_V_1_2) {
		flatness.reg.somewhat_flat_qp = 0x1;
		set_reg(DPU_DSC_FLATNESS_PARAM_ADDR(dsc_base), flatness.value, 24, 0);
	} else {
		flatness.reg.somewhat_flat_qp = 0x0;
		flatness.reg.very_flat_qp = 0x0;
		set_reg(DPU_DSC_FLATNESS_PARAM_ADDR(dsc_base), flatness.value, 32, 0);
	}

	rc_param4.reg.rc_model_size = dsc->dsc_info.rc_model_size;
	rc_param4.reg.rc_edge_factor = dsc->dsc_info.rc_edge_factor;
	set_reg(DPU_DSC_RC_PARAM4_ADDR(dsc_base), rc_param4.value, 24, 0);

	rc_param5.reg.rc_quant_incr_limit0 = dsc->dsc_info.rc_quant_incr_limit0;
	rc_param5.reg.rc_quant_incr_limit1 = dsc->dsc_info.rc_quant_incr_limit1;
	rc_param5.reg.rc_tgt_offset_hi = dsc->dsc_info.rc_tgt_offset_hi;
	rc_param5.reg.rc_tgt_offset_lo = dsc->dsc_info.rc_tgt_offset_lo;
	set_reg(DPU_DSC_RC_PARAM5_ADDR(dsc_base), rc_param5.value, 24, 0);
}

static void dss_dsc_rc_thresh_cfg(char __iomem * dsc_base, struct dsc_calc_info *dsc)
{
	int i;
	DPU_DSC_RC_BUF_THRESH0_UNION thresh0;
	DPU_DSC_RC_BUF_THRESH3_UNION thresh3 = { .value = 0 };

	/* config rc_thresh 0 to 2, increase address offset is 4 */
	for (i = 0; i <= 8; i += 4) {
		thresh0.value = 0;
		thresh0.reg.rc_buf_thresh3 = dsc->dsc_info.rc_buf_thresh[3 + i];
		thresh0.reg.rc_buf_thresh2 = dsc->dsc_info.rc_buf_thresh[2 + i];
		thresh0.reg.rc_buf_thresh1 = dsc->dsc_info.rc_buf_thresh[1 + i];
		thresh0.reg.rc_buf_thresh0 = dsc->dsc_info.rc_buf_thresh[i];
		set_reg((DPU_DSC_RC_BUF_THRESH0_ADDR(dsc_base) + i), thresh0.value, 32, 0);
		dpu_pr_err("dsc rc_thresh[%d]=%d", (DPU_DSC_RC_BUF_THRESH0_ADDR(dsc_base) + i), thresh0.value);
	}

	/* config rc_thresh3 */
	thresh3.reg.rc_buf_thresh13 = dsc->dsc_info.rc_buf_thresh[13];
	thresh3.reg.rc_buf_thresh12 = dsc->dsc_info.rc_buf_thresh[12];
	set_reg(DPU_DSC_RC_BUF_THRESH3_ADDR(dsc_base), thresh3.value, 32, 0);
}

static void dss_dsc_rc_range_cfg(char __iomem * dsc_base, struct dsc_calc_info *dsc)
{
	int i;
	DPU_DSC_RC_RANGE_PARAM0_UNION rc_range0;
	DPU_DSC_RC_RANGE_PARAM7_UNION rc_range7 = { .value = 0 };

	/* config rc_range 0 to 6, increase address offset is 4 */
	for (i = 0; i <= 12; i += 2) {
		rc_range0.value = 0;
		rc_range0.reg.range_bpg_offset1 = dsc->dsc_info.rc_range[1 + i].offset;
		rc_range0.reg.range_max_qp1 = dsc->dsc_info.rc_range[1 + i].max_qp;
		rc_range0.reg.range_min_qp1 = dsc->dsc_info.rc_range[1 + i].min_qp;
		rc_range0.reg.range_bpg_offset0 = dsc->dsc_info.rc_range[i].offset;
		rc_range0.reg.range_max_qp0 = dsc->dsc_info.rc_range[i].max_qp;
		rc_range0.reg.range_min_qp0 = dsc->dsc_info.rc_range[i].min_qp;
		set_reg((DPU_DSC_RC_RANGE_PARAM0_ADDR(dsc_base) + 2 * i), rc_range0.value, 32, 0);
		dpu_pr_err("dsc rc_range[%d]=%d", (DPU_DSC_RC_RANGE_PARAM0_ADDR(dsc_base) + 2 * i), rc_range0.value);
	}

	rc_range7.reg.range_bpg_offset14 = dsc->dsc_info.rc_range[14].offset;
	rc_range7.reg.range_max_qp14 = dsc->dsc_info.rc_range[14].max_qp;
	rc_range7.reg.range_min_qp14 = dsc->dsc_info.rc_range[14].min_qp;
	set_reg(DPU_DSC_RC_RANGE_PARAM7_ADDR(dsc_base), rc_range7.value, 32, 0);
}

static void dss_dsc_multi_slice_cfg(char __iomem * dsc_base, struct dsc_calc_info *dsc)
{
	set_reg(DPU_DSC_MULTI_SLICE_CTL_ADDR(dsc_base), 0, 1, 0);
}

static void dss_dsc_out_mode_cfg(char __iomem * dsc_base, struct dsc_calc_info *dsc)
{
	if (((dsc->dsc_info.chunk_size + dsc->dsc_insert_byte_num) % 6) == 0) {
		set_reg(DPU_DSC_OUT_CTRL_ADDR(dsc_base), 0x0, 1, 0);
	} else if (((dsc->dsc_info.chunk_size + dsc->dsc_insert_byte_num) % 4) == 0) {
		set_reg(DPU_DSC_OUT_CTRL_ADDR(dsc_base), 0x1, 1, 0);
		dpu_pr_debug("dsc->dsc_insert_byte_num 4\n");
	} else {
		dpu_pr_err("chunk_size should be mode by 3 or 2, but chunk_size = %u\n", dsc->dsc_info.chunk_size);
		return;
	}
}

static void dss_dsc_normal_cfg(char __iomem * dsc_base, struct dsc_calc_info *dsc)
{
	/* DSC_MEM_CTRL default is 0, will be 0x88888888 in power control module
	 * DSC_CLK_SEL default is 0xFFFFFFFF, will be 0 for low power state in power control module
	 */
	set_reg(DPU_DSC_CLK_EN_ADDR(dsc_base), 0xF, 32, 0);
	set_reg(DPU_DSC_ST_DATAIN_ADDR(dsc_base), 0x0, 28, 0);
	set_reg(DPU_DSC_ST_DATAOUT_ADDR(dsc_base), 0x0, 16, 0);
	set_reg(DPU_DSC_ST_SLC_POS_ADDR(dsc_base), 0x0, 28, 0);
	set_reg(DPU_DSC_ST_PIC_POS_ADDR(dsc_base), 0x0, 28, 0);
	set_reg(DPU_DSC_ST_FIFO_ADDR(dsc_base), 0x0, 14, 0);
	set_reg(DPU_DSC_ST_LINEBUF_ADDR(dsc_base), 0x0, 24, 0);
	set_reg(DPU_DSC_ST_ITFC_ADDR(dsc_base), 0x0, 10, 0);
}

static void dss_dsc_cfg(char __iomem * dsc_base, struct dsc_calc_info *dsc)
{
	dpu_pr_info("+\n");
	dss_dsc_version_cfg(dsc_base, dsc);
	dss_dsc_ctrl_cfg(dsc_base, dsc);
	dss_dsc_basic_cfg(dsc_base, dsc);
	dss_dsc_rc_thresh_cfg(dsc_base, dsc);
	dss_dsc_rc_range_cfg(dsc_base, dsc);
	dss_dsc_multi_slice_cfg(dsc_base, dsc);
	dss_dsc_out_mode_cfg(dsc_base, dsc);
	dss_dsc_normal_cfg(dsc_base, dsc);
}

bool is_ifbc_vesa_panel(uint32_t ifbc_type)
{
	if ((ifbc_type == IFBC_TYPE_VESA2X_SINGLE) ||
		(ifbc_type == IFBC_TYPE_VESA3X_SINGLE) ||
		(ifbc_type == IFBC_TYPE_VESA2X_DUAL) ||
		(ifbc_type == IFBC_TYPE_VESA3X_DUAL) ||
		(ifbc_type == IFBC_TYPE_VESA3_75X_DUAL) ||
		(ifbc_type == IFBC_TYPE_VESA4X_SINGLE_SPR) ||
		(ifbc_type == IFBC_TYPE_VESA4X_DUAL_SPR) ||
		(ifbc_type == IFBC_TYPE_VESA3_75X_SINGLE) ||
		(ifbc_type == IFBC_TYPE_VESA2X_SINGLE_SPR) ||
		(ifbc_type == IFBC_TYPE_VESA2X_DUAL_SPR))
		return true;

	return false;
}

void dsc_init(struct dsc_calc_info *dsc, char __iomem * dsc_base)
{
	if (!dsc || !dsc_base) {
		dpu_pr_err("dsc or dsc_base is null!\n");
		return;
	}

	if (is_dsc_enabled(dsc))
		dss_dsc_cfg(dsc_base, dsc);
}

bool is_dsc_enabled(struct dsc_calc_info *dsc)
{
	if (!dsc) {
		dpu_pr_err("dsc is null!\n");
		return false;
	}

	return ((dsc->dsc_en & 0x1) == 0x1);
}