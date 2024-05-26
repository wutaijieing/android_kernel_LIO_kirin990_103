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
#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_operators_manager.h"
#include "hisi_composer_operator.h"
#include "sdma/hisi_operator_sdma.h"
#include "dpp/hisi_operator_dpp.h"
#include "hdr/dpu_operator_hdr.h"
#include "ddic/hisi_operator_ddic.h"
#include "overlayer/hisi_operator_overlayer.h"
#include "cld/dpu_disp_cld.h"
#include "srot/dpu_disp_srot.h"
#include "hisi_dpu_module.h"
#include "itfsw/hisi_operator_itfsw.h"
#include "dsc/dpu_operator_dsc.h"
#include "wch/dpu_operator_wch.h"
#include "scl/vscf/dpu_operator_vscf.h"
#include "scl/arsr/dpu_operator_arsr.h"
#include "dsd/hisi_operator_dsd.h"
#include "uvup/hisi_operator_uvup.h"

#ifdef CONFIG_DKMD_DPU_V720
static init_operator_cb g_operators_init_tbl[COMP_OPS_TYPE_MAX] = {
	hisi_sdma_init, NULL, NULL, dpu_arsr_init, NULL, dpu_hdr_init, dpu_cld_init, hisi_uvup_init,
	hisi_overlayer_init, hisi_dpp_init, hisi_ddic_init, dpu_dsc_init, NULL, NULL, NULL, dpu_wch_init,
	hisi_itfsw_init, hisi_dsd_init
};
#else
static init_operator_cb g_operators_init_tbl[COMP_OPS_TYPE_MAX] = {
	hisi_sdma_init, NULL, dpu_srot_init, dpu_arsr_init, dpu_vscf_init, NULL, dpu_cld_init, NULL,
	hisi_overlayer_init, hisi_dpp_init, NULL, dpu_dsc_init, NULL, NULL, NULL, dpu_wch_init,
	hisi_itfsw_init
};
#endif

static bool g_operators_be_inited;
static struct list_head g_operators_type_list;

int dpu_operator_build_data(struct hisi_comp_operator *operator, void *layer, struct hisi_pipeline_data *_pre_out_data,
	struct hisi_comp_operator *next_operator)
{
	struct pipeline_src_layer *src_layer;
	struct hisi_pipeline_data *pre_out_data;

	if (!operator || !layer) {
		disp_pr_err("operator or layer NULL pointer\n");
		return -1;
	}

	// if (_pre_out_data) {
	// 	pre_out_data = (struct hisi_pipeline_data *)_pre_out_data;

	// 	operator->in_data->rect.x = pre_out_data->rect.x;
	// 	operator->in_data->rect.y = pre_out_data->rect.y;
	// 	operator->in_data->rect.w = pre_out_data->rect.w;
	// 	operator->in_data->rect.h = pre_out_data->rect.h;
	// 	operator->in_data->format = pre_out_data->format;
	// 	disp_pr_info("pre_out_data [x y w h format next_order]=[%d %d %u %u %d %u]\n",
	// 		pre_out_data->rect.x, pre_out_data->rect.y, pre_out_data->rect.w, pre_out_data->rect.h,
	// 		pre_out_data->format, pre_out_data->next_order);
	// }

	if (next_operator) {
		next_operator->in_data->rect.x = operator->out_data->rect.x;
		next_operator->in_data->rect.y = operator->out_data->rect.y;
		next_operator->in_data->rect.w = operator->out_data->rect.w;
		next_operator->in_data->rect.h = operator->out_data->rect.h;
		next_operator->in_data->format = operator->out_data->format;

		operator->in_data->next_order = next_operator->id_desc.info.type;
		operator->out_data->next_order = next_operator->id_desc.info.type;

		disp_pr_debug("next_operator %u [x=%d y=%d w=%d h=%d format=%u]", next_operator->id_desc.info.type,
			next_operator->in_data->rect.x, next_operator->in_data->rect.y, next_operator->in_data->rect.w, next_operator->in_data->rect.h,
			next_operator->in_data->format);
	}

	return 0;
}


struct hisi_comp_operator *hisi_disp_get_operator(union operator_id id_desc)
{
	struct hisi_comp_operator *operator = NULL;
	struct hisi_operator_type *node = NULL;
	struct hisi_operator_type *_node_ = NULL;
	uint32_t i;

	if (id_desc.info.type >= COMP_OPS_TYPE_MAX)
		return NULL;

	disp_pr_debug(" id:0x%x, type:%u ", id_desc.id, id_desc.info.type);
	list_for_each_entry_safe(node, _node_, &g_operators_type_list, list_node) {
		if (node->type != id_desc.info.type)
			continue;

		disp_pr_debug(" node->operator_count:%u ", node->operator_count);
		for (i = 0; i < node->operator_count; i++) {
			if (!node->operators)
				break;

			operator = node->operators[i];
			disp_pr_debug("operator->id_desc.id:0x%x", operator->id_desc.id);

			if (operator && operator->id_desc.id == id_desc.id)
				return operator;
		}

		return NULL;
	}

	return NULL;
}

/* TODO: operators_dtsi is read from dts, one bit is one operator
 * such as, bit(COMP_OPS_SDMA) | bit(COMP_OPS_OVERLAYER)
 * [0,4],[1,2],
 *
 */
void hisi_disp_init_operators(union operator_type_desc operator_type_descs[], uint32_t count, void *cookie)
{
	union operator_type_desc desc;
	struct hisi_operator_type *operators = NULL;
	struct dpu_module_ops *module_context_ops = NULL;
	init_operator_cb init_operators = NULL;
	uint32_t i;

	if (g_operators_be_inited)
		return;

	disp_pr_info(" ++++ ");
	disp_pr_info(" count:%u ", count);

	INIT_LIST_HEAD(&g_operators_type_list);
	for (i = 0; i < count; i++) {
		desc.type_desc = operator_type_descs[i].type_desc;
		disp_pr_info(" desc.type_desc:0x%x ", desc.type_desc);
		disp_pr_info(" desc.detail.type:%u ", desc.detail.type);
		disp_pr_info(" desc.detail.count:%u ", desc.detail.count);

		init_operators = g_operators_init_tbl[desc.detail.type];
		if (!init_operators)
			continue;

		operators = kzalloc(sizeof(*operators), GFP_KERNEL);
		if (!operators)
			continue;

		operators->type = desc.detail.type;
		operators->operator_count = desc.detail.count;

		disp_pr_info(" type:%u, operator_count:%u, i:%u ", operators->type, operators->operator_count, i);
		module_context_ops = hisi_module_get_context_ops();
		if (!module_context_ops) {
			disp_pr_info("module_context_ops NULL");
			continue;
		}
		init_operators(operators, *module_context_ops, cookie);

		list_add_tail(&operators->list_node, &g_operators_type_list);
	}

	g_operators_be_inited = true;
	disp_pr_info(" ---- ");
	return;
}

struct list_head *hisi_disp_get_operator_list(void)
{
	if (!g_operators_be_inited)
		return NULL;

	return &g_operators_type_list;
}
