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

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_disp_composer.h"
#include "hisi_operator_dpp.h"
#include "hisi_dpu_module.h"
#include "hisi_disp_gadgets.h"
#include "hisi_operator_tool.h"
#include "hisi_dpp_local_dimming.h"
#include "test_dpp/hisi_display_effect_test.h"
#include "../hisi_operators_manager.h"
#include "../../../effect/display_effect_alsc.h"

static void hisi_dpp_set_dm_dppinfo(struct hisi_composer_data *ov_data, struct hisi_dm_dpp_info *dpp_info)
{
	disp_pr_info(" ++++ \n");

	dpp_info->dpp_input_img_width_union.reg.dpp_input_img_width = ov_data->fix_panel_info->xres - 1;
	dpp_info->dpp_input_img_width_union.reg.dpp_input_img_height =  ov_data->fix_panel_info->yres - 1;

	dpp_info->dpp_output_img_width_union.reg.dpp_output_img_width = ov_data->fix_panel_info->xres - 1;
	dpp_info->dpp_output_img_width_union.reg.dpp_output_img_height = ov_data->fix_panel_info->yres - 1;

	dpp_info->dpp_sel_union.reg.dpp_layer_id = 31;
#if defined(CONFIG_DPP_SCENE0)
	dpp_info->dpp_sel_union.reg.dpp_sel = 1; /* select dpp0 */
#else
	dpp_info->dpp_sel_union.reg.dpp_sel = 2; /* select dpp1 */
#endif
#ifdef CONFIG_DKMD_DPU_V720
	if (g_debug_en_ddic) {
		dpp_info->dpp_sel_union.reg.dpp_order0 = DPU_DDIC_OP_ID;
		disp_pr_debug("DPP next is ddic\n");
	} else {
		dpp_info->dpp_sel_union.reg.dpp_order0 = DPU_DSC_OP_ID;
	}
#else
	dpp_info->dpp_sel_union.reg.dpp_order0 = DPU_ITF_OP_ID;
#endif
	dpp_info->dpp_sel_union.reg.dpp_order1 = DPU_OP_INVALID_ID;

	if (g_local_dimming_en) {
		dpp_info->dpp_reserved_union.reg.dpp_input_fmt = ARGB10101010; // 0b111001
		dpp_info->dpp_reserved_union.reg.dpp_output_fmt = RGB_10BIT; // 0b100111
	} else {
		dpp_info->dpp_reserved_union.reg.dpp_input_fmt = DPU_RDFC_OUT_FORMAT_ARGB_10101010;
		dpp_info->dpp_reserved_union.reg.dpp_output_fmt = 39;
	}
}

static void hisi_dpp_flush_en(struct hisi_dm_dpp_info *dpp_info, char __iomem *dpu_base_addr)
{
#if defined(CONFIG_DPP_SCENE0)
	dpu_set_reg(DPU_DPP_REG_CTRL_FLUSH_EN_ADDR(dpu_base_addr + DSS_DPP0_OFFSET), 1, 1, 0);
	dpu_set_reg(DPU_DPP_REG_CTRL_DEBUG_ADDR(dpu_base_addr + DSS_DPP0_OFFSET), 1, 1, 10);
#else
	dpu_set_reg(DPU_DPP_REG_CTRL_FLUSH_EN_ADDR(dpu_base_addr + DSS_DPP1_OFFSET), 1, 1, 0);
	dpu_set_reg(DPU_DPP_REG_CTRL_DEBUG_ADDR(dpu_base_addr + DSS_DPP1_OFFSET), 1, 1, 10);
#endif
}
static void hisi_dpp_set_regs(struct hisi_comp_operator *operator,struct hisi_dm_param *dm_param,  char __iomem *dpu_base_addr)
{
	struct hisi_dm_dpp_info *dpp_info = &(dm_param->dpp_info[0]);
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);

	disp_pr_info(" ++++ \n");
#ifdef CONFIG_DKMD_DPU_V720
	uint32_t gmp_en_value = inp32(DPU_DPP_GMP_EN_ADDR(dpu_base_addr + DSS_DPP0_OFFSET));
	gmp_en_value = gmp_en_value | (g_gmp_bitext_mode << 1);
	disp_pr_debug("gmp_en_value:0x%x\n", gmp_en_value);
	// gmp_bitext_mode temp test for demura
	dpu_set_reg(DPU_DPP_GMP_EN_ADDR(dpu_base_addr + DSS_DPP0_OFFSET), gmp_en_value, 32, 0);
#endif

	if (g_debug_en_ddic) { // dither mode 0x3
		dpu_set_reg(DPU_DPP_DITHER_CTL1_ADDR(dpu_base_addr + DSS_DPP0_OFFSET), 0x27, 32, 0);
		disp_pr_debug(" set dither mode \n");
	}
	hisi_module_set_reg(&operator->module_desc, DPU_DM_DPP_INPUT_IMG_WIDTH_ADDR(dm_base), dpp_info->dpp_input_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_DPP_OUTPUT_IMG_WIDTH_ADDR(dm_base), dpp_info->dpp_output_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_DPP_SEL_ADDR(dm_base), dpp_info->dpp_sel_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_DPP_RESERVED_ADDR(dm_base), dpp_info->dpp_reserved_union.value);
}

static void set_alsc(struct hisi_composer_device *composer_device)
{
	struct hisi_comp_operator *dpp_op = NULL;
	union operator_id id_dpp;

	id_dpp.id = 0;
	id_dpp.info.type = COMP_OPS_DPP;
	id_dpp.info.idx= 0;

	dpp_op = hisi_disp_get_operator(id_dpp);
	hisi_alsc_set_reg((struct hisi_operator_dpp *)dpp_op,
		composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);
}

static int hisi_dpp_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
		void *layer)
{
	int ret;
	struct hisi_dm_param *dm_param = NULL;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer;

	disp_pr_info(" ++++ \n");
	dm_param = composer_device->ov_data.dm_param;

	hisi_dpp_set_dm_dppinfo(&(composer_device->ov_data), &(dm_param->dpp_info[0]));
	hisi_dpp_set_regs(operator, dm_param, composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);

#ifdef CONFIG_DPP_ALSC
	set_alsc(composer_device);
#endif
	if (hisi_fb_display_en_dpp_core)
		dpp_effect_test(composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);
	if (g_local_dimming_en)
		hisi_local_dimming_set_regs(operator, &(composer_device->ov_data),
			composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU], src_layer);

	hisi_dpp_flush_en(&(dm_param->dpp_info[0]), composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);

	disp_pr_info(" ---- \n");
	return 0;
}
void hisi_dpp_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct hisi_operator_dpp **dpp = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info("dpp ++++ ,type_operator->operator_count = %d\n",type_operator->operator_count);
	dpp = kzalloc(sizeof(*dpp) * type_operator->operator_count, GFP_KERNEL);
	if (!dpp)
		return;

	disp_pr_info(" type_operator->operator_count:%u \n", type_operator->operator_count);
	for (i = 0; i < type_operator->operator_count; i++) {
		disp_pr_info("size of struct hisi_operator_dpp:%u \n", sizeof(*(dpp[i])));
		dpp[i] = kzalloc(sizeof(*(dpp[i])), GFP_KERNEL);
		if (!dpp[i])
			continue;

		/* TODO: error check */
		hisi_operator_init(&dpp[i]->base, ops, cookie);

		base = &dpp[i]->base;
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		disp_pr_info(" base->id_desc.info.idx:%u, base->id_desc.info.type:%u \n", base->id_desc.info.idx, base->id_desc.info.type);
		base->set_cmd_item = hisi_dpp_set_cmd_item;
		sema_init(&dpp[i]->base.operator_sem, 1);

		base->out_data = kzalloc(sizeof(struct hisi_dpp_out_data), GFP_KERNEL);
		if (!base->out_data) {
			kfree(dpp[i]);
			dpp[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct hisi_dpp_in_data), GFP_KERNEL);
		if (!base->in_data) {
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(dpp[i]);
			dpp[i] = NULL;
			continue;
		}
		disp_pr_info("dpp base->out_data:%p,base->in_data %p \n",base->out_data, base->in_data);
		dpp[i]->base.be_dm_counted = false;
	}
	type_operator->operators = (struct hisi_comp_operator **)(dpp);
#ifdef CONFIG_DPP_ALSC
	hisi_alsc_init(*dpp);
#endif
}
