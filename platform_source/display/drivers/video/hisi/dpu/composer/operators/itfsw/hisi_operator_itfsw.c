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

#include "hisi_disp_gadgets.h"
#include "hisi_operator_itfsw.h"
#include "hisi_disp_config.h"
#include "hisi_disp_composer.h"
#include "dm/hisi_dm.h"
#include "hisi_operator_tool.h"

#define PIPE_SW_SIG_CTRL (0x0000)
#define SW_POS_CTRL_SIG_EN (0x0004)
#define PIPE_SW_DAT_CTRL (0x0008)
#define SW_POS_CTRL_DAT_EN (0x000C)

#define PIPE_SW_SIG_CTRL_N(n) (0x0000 + 0x1C * (n))
#define SW_POS_CTRL_SIG_EN_N(n) (0x0004 + 0x1C * (n))
#define PIPE_SW_DAT_CTRL_N(n) (0x0008 + 0x1C * (n))
#define SW_POS_CTRL_DAT_EN_N(n) (0x000C + 0x1C * (n))

#define DPP_CLRBAR_RED_VALUE (0x3FF00000)
#define DPP_CLRBAR_GREEN_VALUE (0x000FFC00)
#define DPP_CLRBAR_BLUE_VALUE (0x000003FF)
#define ITFSW_DATA_SEL (0x0068)
#define CLRBAR_START (0x006C)
#define MIPI_LDI_FRM_MSK_UP (0x0238)
#define MIPI_LDI_CTRL   (0x01B8)

static struct dpu_operator_offset g_itfsw_offset[] = {
	{DSS_ITF_CH0_OFFSET, 0},
	{DSS_ITF_CH1_OFFSET, 0},
	{DSS_ITF_CH2_OFFSET, 0},
	{DSS_ITF_CH3_OFFSET, 0},
};

static void hisi_dss_itfch_config_clrbar(char __iomem *dpu_base_addr, uint32_t scene_id)
{
	char __iomem *itfch_base = dpu_base_addr + g_itfsw_offset[scene_id].reg_offset; /* colorbar in itfch0 */

	disp_pr_info(" ++++ \n");

	// color bar width is xres/12
	// outp32(itfch_base, DSS_HEIGHT(pinfo->yres) << 16 | DSS_WIDTH(pinfo->xres));
	// outp32(itfch_base + 0x30, (DSS_WIDTH(pinfo->xres / 12) << 24) | (0 << 1) | 0x1);

	outp32(itfch_base, 1919 << 16 | 1079);
	outp32(itfch_base + 0x30, (89 << 24) | (0 << 1) | 0x1);

	dpu_set_reg(itfch_base + 0x34, DPP_CLRBAR_RED_VALUE, 30, 0);

	dpu_set_reg(itfch_base + 0x38, DPP_CLRBAR_GREEN_VALUE, 30, 0);
	dpu_set_reg(itfch_base + 0x3C, DPP_CLRBAR_BLUE_VALUE, 30, 0);
	dpu_set_reg(itfch_base + ITFSW_DATA_SEL, 0, 3, 0);
	dpu_set_reg(itfch_base + CLRBAR_START, 0x1, 1, 0);

	dpu_set_reg(dpu_base_addr + MIPI_LDI_FRM_MSK_UP, 0x1, 1, 0);
	dpu_set_reg(dpu_base_addr + MIPI_LDI_CTRL, 0x1, 1, 0);

	disp_pr_info(" ---- \n");
}

static void hisi_dss_itfch_config(char __iomem *dpu_base_addr, uint32_t scene_id)
{
	char __iomem *itfch_base = dpu_base_addr + g_itfsw_offset[scene_id].reg_offset; /* colorbar in itfch0 */

	disp_pr_info(" +++++++ \n");

	dpu_set_reg(itfch_base + 0x00, 0x77f0437, 32, 0); // REG_CTRL
	dpu_set_reg(itfch_base + 0x14, 0x1, 32, 0); // REG_CTRL
	dpu_set_reg(itfch_base + 0x68, 0, 32, 0); // REG_CTRL
	dpu_set_reg(itfch_base + 0x5C, 1, 1, 0); //REG_CTRL_FLUSH_EN
}

static void platform_reset_pipe_sw(char __iomem *dpu_base_addr, uint32_t pipe_sw_post_idx)
{
	disp_pr_info("++++\n");

	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + PIPE_SW_SIG_CTRL_N(pipe_sw_post_idx), 0x0, 8, 0);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + SW_POS_CTRL_SIG_EN_N(pipe_sw_post_idx), 0x0, 1, 0);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + PIPE_SW_DAT_CTRL_N(pipe_sw_post_idx), 0x0, 8, 0);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + SW_POS_CTRL_DAT_EN_N(pipe_sw_post_idx), 0x0, 1, 0);

	disp_pr_info("----\n");
}

uint32_t platform_init_pipe_sw(char __iomem *dpu_base_addr, uint32_t scene_id, uint32_t pipe_sw_post_idx)
{
	uint32_t scene_bit = BIT(scene_id);

	disp_pr_info(" ++++ \n");
	disp_pr_info(" dpu_base_addr:%p \n", dpu_base_addr);
	disp_pr_info(" scene_id:%d, scene_bit:%d\n", scene_id, scene_bit);

#ifdef CONFIG_HISI_FB_COLORBAR_USED
	hisi_dss_itfch_config_clrbar(dpu_base_addr, scene_id);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + PIPE_SW_SIG_CTRL_N(pipe_sw_post_idx), scene_bit, 8, 0);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + SW_POS_CTRL_SIG_EN_N(pipe_sw_post_idx), 0x1, 1, 0);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + PIPE_SW_DAT_CTRL_N(pipe_sw_post_idx), scene_bit, 8, 0);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + SW_POS_CTRL_DAT_EN_N(pipe_sw_post_idx), 0x1, 1, 0);
#else
	// config in itf_sw_operator
	// hisi_dss_itfch_config(dpu_base_addr, scene_id);

	/*
	* just for test dp:
	* dp and mipi use same pre channel now, so should switch post_ch between ldi and dp
	* 1. reset pipe_sw
	* 2. switch pipe
	*/
	if (pipe_sw_post_idx == PIPE_SW_POST_CH_DP0) {
		dpu_set_reg(DPU_DSI_LDI_CTRL_ADDR(dpu_base_addr), 0x0, 1, 0);
		platform_reset_pipe_sw(dpu_base_addr, PIPE_SW_POST_CH_DSI0);
		platform_reset_pipe_sw(dpu_base_addr, PIPE_SW_POST_CH_DP0);
	}

	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + PIPE_SW_SIG_CTRL_N(pipe_sw_post_idx), scene_bit, 8, 0);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + PIPE_SW_DAT_CTRL_N(pipe_sw_post_idx), scene_bit, 8, 0);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + SW_POS_CTRL_SIG_EN_N(pipe_sw_post_idx), 0x1, 1, 0);
	dpu_set_reg(dpu_base_addr + DSS_PIPE_SW_OFFSET + SW_POS_CTRL_DAT_EN_N(pipe_sw_post_idx), 0x1, 1, 0);
#endif

	return 0;
}

static void hisi_itfsw_set_regs(struct hisi_comp_operator *operator,
	struct hisi_itfsw_params *itfsw_params, char __iomem *dpu_base_addr)
{
	char __iomem *itfch_base = dpu_base_addr + operator->operator_offset;

	disp_pr_debug(" itfch_base:0x%p, operator_offset:%u \n", itfch_base, operator->operator_offset);

	hisi_module_set_reg(&operator->module_desc, DPU_ITF_CH_IMG_SIZE_ADDR(itfch_base),
		itfsw_params->img_size_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_ITF_CH_ITFSW_DATA_SEL_ADDR(itfch_base),
		itfsw_params->itfsw_data_sel_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_ITF_CH_DPP_CLIP_EN_ADDR(itfch_base),
		itfsw_params->dpp_clip_en_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_ITF_CH_REG_CTRL_FLUSH_EN_ADDR(itfch_base),
		itfsw_params->reg_ctrl_flush_en_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_ITF_CH_REG_CTRL_DEBUG_ADDR(itfch_base),
		itfsw_params->reg_ctrl_debug_union.value);
}

static int hisi_itfsw_set_params(struct hisi_itfsw_params *itfsw_params, struct post_online_info *dirty_region)
{
	disp_pr_debug(" ++++ \n");

	itfsw_params->img_size_union.value = 0;
	itfsw_params->img_size_union.reg.width_sub1 = dirty_region->dirty_rect.w - 1;
	itfsw_params->img_size_union.reg.height_sub1 = dirty_region->dirty_rect.h - 1;
	itfsw_params->dpp_clip_en_union.value = 0;
	itfsw_params->itfsw_data_sel_union.value = 0;
	itfsw_params->reg_ctrl_flush_en_union.value = 1;
	itfsw_params->reg_ctrl_debug_union.value = 0x400;

	return 0;
}

static void dpu_dm_itfsw_set_params(struct hisi_comp_operator *operator,
	struct hisi_dm_param *dm_param, struct post_online_info *dirty_region)
{
	struct hisi_dm_itfsw_info *itf = &dm_param->itfsw_info;

	disp_pr_debug(" ++++ \n");

	disp_pr_debug("dirty_rect.w:%d, dirty_rect.h:%d\n", dirty_region->dirty_rect.w, dirty_region->dirty_rect.h);

	itf->itf_input_img_width_union.value = 0;
	itf->itf_input_img_width_union.reg.itf_input_img_width = dirty_region->dirty_rect.w - 1;
	itf->itf_input_img_width_union.reg.itf_input_img_height = dirty_region->dirty_rect.h - 1;

	itf->itf_input_fmt_union.value = 0;
	itf->itf_input_fmt_union.reg.itf_input_fmt = ARGB10101010;//39; /* 16 means rgbg, 6 means argb8888*/
	itf->itf_input_fmt_union.reg.itf_layer_id = POST_LAYER_ID; /* 31 means no relationship to layer */
	itf->itf_input_fmt_union.reg.itf_sel = BIT(operator->scene_id);
	itf->itf_input_fmt_union.reg.itf_hidic_en = 0;
}

static int dpu_dm_itfsw_set_reg(struct hisi_comp_operator *operator, const struct hisi_dm_param *dm_param, char __iomem *dpu_base_addr)
{
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);
	const struct hisi_dm_itfsw_info *itf = &dm_param->itfsw_info;

	disp_pr_debug(" ++++ \n");

	hisi_module_set_reg(&operator->module_desc, DPU_DM_ITF_INPUT_IMG_WIDTH_ADDR(dm_base), itf->itf_input_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc, DPU_DM_ITF_INPUT_FMT_ADDR(dm_base), itf->itf_input_fmt_union.value);

	return 0;
}

static int hisi_itfsw_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
	void *in_dirty_region)
{
	struct hisi_operator_itfsw *itfsw = (struct hisi_operator_itfsw *)operator;
	struct post_online_info *dirty_region = (struct post_online_info *)in_dirty_region;
	struct hisi_dm_param *dm_param = composer_device->ov_data.dm_param;
	char __iomem *dpu_base_addr = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];
	struct hisi_itfsw_params *itfsw_params = NULL;
	int ret;

	itfsw_params = &itfsw->itfsw_params;
	ret = hisi_itfsw_set_params(itfsw_params, dirty_region);
	if (ret)
		return -1;

	dpu_dm_itfsw_set_params(operator, dm_param, dirty_region);
	dpu_dm_itfsw_set_reg(operator, dm_param, dpu_base_addr);

	hisi_itfsw_set_regs(operator, itfsw_params, composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);

    //hisi_dump_layer(src_layer,NULL, 2);

	return 0;
}

void hisi_itfsw_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct hisi_operator_itfsw **itfs = NULL;
	struct hisi_operator_itfsw *itfsw = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	itfs = kzalloc(sizeof(*itfs) * type_operator->operator_count, GFP_KERNEL);
	if (!itfs)
		return;

	for (i = 0; i < type_operator->operator_count; i++) {
		itfsw = kzalloc(sizeof(*itfsw), GFP_KERNEL);
		if (!itfsw)
			continue;

		itfs[i] = itfsw;
		base = &itfsw->base;

		hisi_operator_init(base, ops, cookie);

		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = hisi_itfsw_set_cmd_item;
		base->operator_offset = g_itfsw_offset[i].reg_offset;
		base->dm_offset = g_itfsw_offset[i].dm_offset;
		sema_init(&base->operator_sem, 1);

		base->out_data = kzalloc(sizeof(struct hisi_itfsw_out_data), GFP_KERNEL);
		if (!base->out_data) {
			kfree(itfsw);
			itfsw = NULL;
			itfs[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct hisi_itfsw_in_data), GFP_KERNEL);
		if (!base->in_data) {
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(itfsw);
			itfsw = NULL;
			itfs[i] = NULL;
			continue;
		}

		base->be_dm_counted = false;
	}

	type_operator->operators = (struct hisi_comp_operator **)(itfs);
}
