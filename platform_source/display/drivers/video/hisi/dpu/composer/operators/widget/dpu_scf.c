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

#include "dpu_scf.h"
#include "hisi_disp_gadgets.h"

static void calc_hrz_component(const struct dpu_scf_data *input, struct scf_reg *config)
{
	const struct disp_rect *src_rect = &input->src_rect;
	const struct disp_rect *dst_rect = &input->dst_rect;
	bool enable_throwing = false;

	if (has_same_width(src_rect, dst_rect) || dst_rect->w <= 1) {
		config->en_hscl.reg.scf_en_hscl_en = false;
		return;
	}

	/* hscl scale down, do hscl first, order is 0,
	 * hscl scale up,   do hscl later, order is 1.
	 */
	config->h_v_order.reg.scf_h_v_order = src_rect->w < dst_rect->w ? 1 : 0;

	config->inc_hscl.reg.scf_inc_hscl = scf_ratio(src_rect->w, input->acc_hscl, dst_rect->w);
	config->acc_hscl.value = input->acc_hscl;
	config->acc_hscl1.value = 0; // TODO:

	/* 1, throwing scaler just be used for scaling down, and
	 * 2, when dst < src / 4.
	 */
	enable_throwing = scf_enable_throwing(src_rect->w, dst_rect->w);
	config->en_hscl_str.reg.scf_en_hscl_str = enable_throwing;
	config->en_hscl_str.reg.scf_en_hscl_str_a = enable_throwing && input->has_alpha;

	config->en_hscl.reg.scf_en_hscl_en = true;
}

static void calc_vrz_component(const struct dpu_scf_data *input, struct scf_reg *config)
{
	const struct disp_rect *src_rect = &input->src_rect;
	const struct disp_rect *dst_rect = &input->dst_rect;
	bool enable_throwing = false;

	if (has_same_height(src_rect, dst_rect) || dst_rect->h <= 1) {
		config->en_vscl.reg.scf_en_vscl_en = false;
		return;
	}

	config->inc_vscl.reg.scf_inc_vscl = scf_ratio(src_rect->h, input->acc_vscl, dst_rect->h);
	config->acc_vscl.value = input->acc_vscl;
	config->acc_vscl1.value = 0; // TODO:

	enable_throwing = scf_enable_throwing(src_rect->h, dst_rect->h);
	config->en_vscl_str.reg.scf_en_vscl_str = enable_throwing;
	config->en_vscl_str.reg.scf_en_vscl_str_a = enable_throwing && input->has_alpha;

	config->en_vscl.reg.scf_en_vscl_en = true;
}

static int scf_calc_configure(const struct dpu_scf_data *input, struct scf_reg *config)
{
	const struct disp_rect *src_rect = &input->src_rect;
	const struct disp_rect *dst_rect = &input->dst_rect;
	int ret = 0;

	if (has_same_dim(src_rect, dst_rect)) {
		ret = 0;
		goto out;
	}

	/* scf restraint */
	if (scf_rect_is_underflow(src_rect) || scf_rect_is_underflow(dst_rect)) {
		ret = -EINVAL;
		goto out;
	}

	/* TODO: other over flow restraint */

	config->src_rect.reg.scf_input_height = DPU_HEIGHT(src_rect->h);
	config->src_rect.reg.scf_input_width = DPU_HEIGHT(src_rect->w);
	config->dst_rect.reg.scf_output_height = DPU_HEIGHT(dst_rect->h);
	config->dst_rect.reg.scf_output_width = DPU_HEIGHT(dst_rect->w);
	calc_hrz_component(input, config);
	calc_vrz_component(input, config);

	return ret;

out:
	disp_pr_scf(input);
	config->en_hscl.reg.scf_en_hscl_en = false;
	config->en_vscl.reg.scf_en_vscl_en = false;
	return ret;
}

static void scf_set_reg(struct dpu_module_desc *module, const struct scf_reg *config, char __iomem *base)
{
	hisi_module_set_reg(module, DPU_ARSR_SCF_EN_HSCL_ADDR(base), config->en_hscl_str.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_EN_VSCL_ADDR(base), config->en_vscl_str.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_H_V_ORDER_ADDR(base), config->h_v_order.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_INPUT_WIDTH_HEIGHT_ADDR(base), config->src_rect.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_OUTPUT_WIDTH_HEIGHT_ADDR(base), config->dst_rect.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_EN_HSCL_ADDR(base), config->en_hscl.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_ACC_HSCL_ADDR(base), config->acc_hscl.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_ACC_HSCL1_ADDR(base), config->acc_hscl1.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_INC_HSCL_ADDR(base), config->inc_hscl.value);

	hisi_module_set_reg(module, DPU_ARSR_SCF_ACC_VSCL_ADDR(base), config->acc_vscl.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_ACC_VSCL1_ADDR(base), config->acc_vscl1.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_INC_VSCL_ADDR(base), config->inc_vscl.value);
	hisi_module_set_reg(module, DPU_ARSR_SCF_EN_MMP_ADDR(base), 1);
}

void dpu_scf_set_cmd_item(struct dpu_module_desc *module, struct dpu_scf_data *input)
{
	struct scf_reg config = {0};
	int ret;

	disp_pr_info("++++ ");

	disp_pr_scf(input);

	ret = scf_calc_configure(input, &config);
	if (ret) {
		disp_pr_err("calc ratio err!");
		disp_pr_scf(input);
		return;
	}

	scf_set_reg(module, &config, input->base);
	disp_pr_info("---- ");
}

