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

#include "hisi_disp_composer.h"
#include "dpu_offline_define.h"
#include "wch/dpu_wch_wdma.h"
#include "wch/dpu_wch_dither.h"
#include "wch/dpu_wch_post_clip.h"
#include "wch/dpu_wch_dfc.h"
#include "wch/dpu_wch_scf.h"
#include "wch/dpu_wch_csc.h"

struct wchn_module_init {
	enum dpu_wchn_module module_id;
	void (*reg_ch_module_init)(struct dpu_offline_module *offline_module, uint32_t ch);
};

static void module_dma_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	dpu_wdma_init(offline_module->wch_base[ch], &(offline_module->wdma[ch]));
}

static void module_dfc_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	dpu_wdfc_init(offline_module->wch_base[ch], &(offline_module->dfc[ch]));
}

static void module_scf_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	if (ch == DPU_WCHN_W0 || ch == DPU_WCHN_W2) {
		disp_pr_info("wch[%d] do not surport scf", ch);
		return;
	}

	offline_module->has_scf[ch] = 1;
	dpu_wb_scf_init(offline_module->wch_base[ch], &(offline_module->scf[ch]));
}

static void module_pcsc_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	if (ch == DPU_WCHN_W0 || ch == DPU_WCHN_W2) {
		disp_pr_info("wch[%d] do not surport pcsc", ch);
		return;
	}

	offline_module->has_pcsc[ch] = 1;
	dpu_wch_pcsc_init(offline_module->wch_base[ch], &(offline_module->pcsc[ch]));
}

static void module_post_clip_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	if (ch == DPU_WCHN_W0 || ch == DPU_WCHN_W2) {
		disp_pr_info("wch[%d] do not surport post_clip", ch);
		return;
	}

	offline_module->has_post_clip[ch] = 1;
	dpu_wb_post_clip_init(offline_module->wch_base[ch], &(offline_module->clip[ch]));
}

static void module_csc_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	dpu_wch_csc_init(offline_module->wch_base[ch], &(offline_module->csc[ch]));
}

static void module_rot_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	if (ch == DPU_WCHN_W0) {
		disp_pr_info("wch[%d] do not surport rot", ch);
		return;
	}

	offline_module->has_rot[ch] = 1;
	// TODO:
}

static void module_wrap_ctl_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	// TODO:
}

static void module_wlt_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
#ifndef CONFIG_DKMD_DPU_V720
	if (ch == DPU_WCHN_W0 || ch == DPU_WCHN_W2) {
#else
	if (ch == DPU_WCHN_W0) {
#endif
		disp_pr_info("wch[%d] do not surport wlt", ch);
		return;
	}
	offline_module->has_wlt[ch] = 1;
	// TODO:
}

static void module_scf_lut_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	if (ch == DPU_WCHN_W0 || ch == DPU_WCHN_W2) {
		disp_pr_info("wch[%d] do not surport scf lut", ch);
		return;
	}

	offline_module->has_scf_lut[ch] = 1;
	// TODO:
}

static void module_wdfc_dither_init(struct dpu_offline_module *offline_module, uint32_t ch)
{
	dpu_wdfc_dither_init(offline_module->wch_base[ch], &(offline_module->dither[ch]));
}

static struct wchn_module_init g_wchn_module_init[] = {
	{ WCH_MODULE_DMA, module_dma_init },
	{ WCH_MODULE_WLT, module_wlt_init },
	{ WCH_MODULE_DFC, module_dfc_init },
	{ WCH_MODULE_WDFC_DITHER, module_wdfc_dither_init },
	{ WCH_MODULE_SCF, module_scf_init },
	{ WCH_MODULE_PCSC, module_pcsc_init },
	{ WCH_MODULE_POST_CLIP, module_post_clip_init },
	{ WCH_MODULE_CSC, module_csc_init },
	{ WCH_MODULE_SCL_LUT, module_scf_lut_init },
	{ WCH_MODULE_ROT, module_rot_init },
	{ WCH_MODULE_WRAP_CTL, module_wrap_ctl_init }
};

static struct offline_isr_info *hisi_offline_isr_info_alloc(void)
{
	int i;
	struct offline_isr_info *isr_info = NULL;

	isr_info = (struct offline_isr_info *)kmalloc(sizeof(struct offline_isr_info), GFP_ATOMIC);
	if (isr_info) {
		memset(isr_info, 0, sizeof(struct offline_isr_info));
	} else {
		disp_pr_err("failed to kmalloc cmdlist_info!\n");
		return NULL;
	}

	sema_init(&(isr_info->wb_common_sem), 1);

	for (i = 0; i < WB_TYPE_MAX; i++) {
		sema_init(&(isr_info->wb_sem[i]), 1);
		init_waitqueue_head(&(isr_info->wb_wq[i]));
		isr_info->wb_done[i] = 0;
		isr_info->wb_flag[i] = 0;
	}

	return isr_info;
}

static void hisi_offline_dst_block_rect_init(struct hisi_composer_data *data)
{
	int i;

	for (i = 0; i < DPU_CMDLIST_BLOCK_MAX; i++) {
		data->ov_block_rects[i] = (struct disp_rect *)kmalloc(sizeof(struct disp_rect), GFP_ATOMIC);
		if (!data->ov_block_rects[i]) {
			disp_pr_err("ov_block_rects[%d] failed to alloc!\n", i);
			goto err_deint;
		}
	}

err_deint:
	// TODO:
	return;
}

void hisi_offline_init_param_init(struct hisi_composer_data *data)
{
	memset(&data->offline_module_default, 0, sizeof(struct dpu_offline_module));
	memset(&data->offline_module, 0, sizeof(struct dpu_offline_module));
	memset(&data->ov_req, 0, sizeof(struct ov_req_infos));

	data->offline_isr_info = hisi_offline_isr_info_alloc();

	hisi_offline_dst_block_rect_init(data);
}

int hisi_offline_reg_modules_init(struct hisi_composer_data *ov_data)
{
	struct dpu_offline_module *offline_module = NULL;
	uint32_t wch_base;
	char __iomem *dpu_base = NULL;
	uint32_t i;
	uint32_t j;

	disp_check_and_return(!ov_data, -EINVAL, err, "ov_data is NULL\n");

	offline_module = &(ov_data->offline_module_default);
	memset(offline_module, 0, sizeof(struct dpu_offline_module));

	dpu_base = ov_data->ip_base_addrs[DISP_IP_BASE_DPU];

	if (!dpu_base) {
		disp_pr_err("dpu_base is null");
		return -1;
	}

	disp_pr_info("enter+++");

	for (i = 0; i < DPU_WCHN_MAX_DEFINE; i++) {
		if (i == 0)
			continue; /* now not surport wch0 */

		for (j = 0; j < ARRAY_SIZE(g_wchn_module_init); j++) {
			wch_base = g_dpu_wch_base[i];
			if (wch_base != 0) {
				offline_module->wch_base[i] = dpu_base + wch_base;
				g_wchn_module_init[j].reg_ch_module_init(offline_module, i);
			}
		}
	}
	disp_pr_info("enter---");
	return 0;
}

int hisi_offline_module_default(struct hisi_composer_data *data)
{
	memcpy(&(data->offline_module), &(data->offline_module_default), sizeof(struct dpu_offline_module));

	return 0;
}
