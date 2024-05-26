/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include "../dpu_overlay_utils.h"
#include "../../dpu_display_effect.h"
#include "../../dpu_dpe_utils.h"
#include "../../dpu_mmbuf_manager.h"
#include "../../dpu_dpm.h"
#include "../../dpu_fb_panel.h"
#include "../../dpu_mipi_dsi.h"

/* 128 bytes */
#define SMMU_RW_ERR_ADDR_SIZE 128

#define WORKQUEUE_NAME_SIZE 128

#define DSS_MODULE_POST_SCF 0
#define DSS_MODULE_CLD 1
#define DSS_MODULE_MCTRL_SYS 2
#define DSS_MODULE_SMMU 3

#define VIDEO_MODE 0
#define CMD_MODE 1

#define dpu_destroy_wq(wq) \
	do { \
		if ((wq) != NULL) { \
			destroy_workqueue(wq); \
			(wq) = NULL; \
		} \
	} while (0)

/* init when power on */
uint32_t g_underflow_count;  /*lint !e552*/

static int g_dss_sr_refcount;

struct cmdlist_idxs_sq {
	uint32_t cur;
	uint32_t prev;
	uint32_t prev_prev;
};

struct dsm_data {
	struct dpu_fb_data_type *dpufd;
	dss_overlay_t *pov_req;
	int ret;
	struct cmdlist_idxs_sq cmd_idx;
	uint32_t time_diff;
	uint32_t *read_value;
};

struct single_ch_module_init {
	enum dss_chn_module module_id;
	void (*reg_ch_module_init)(char __iomem *dss_base, uint32_t module_base,
		dss_module_reg_t *dss_module, uint32_t ch);
};

struct single_module_init {
	uint32_t module_offset;
	void (*reg_module_init)(char __iomem *dss_base, uint32_t module_base, dss_module_reg_t *dss_module);
};

enum vactive0_panel_type {
	PANEL_CMD,
	PANEL_CMD_RESET,
	PANEL_VIDEO,
};

void dpufb_dss_overlay_info_init(dss_overlay_t *ov_req)
{
	if (ov_req == NULL)
		return;

	memset(ov_req, 0, sizeof(dss_overlay_t));
	ov_req->release_fence = -1;
	ov_req->retire_fence = -1;
}


int dpu_module_init(struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	if (dpufd->index == MEDIACOMMON_PANEL_IDX)
		memcpy(&(dpufd->dss_module), &(dpufd->dss_mdc_module_default), sizeof(dss_module_reg_t));
	else
		memcpy(&(dpufd->dss_module), &(dpufd->dss_module_default), sizeof(dss_module_reg_t));

	return 0;
}

static void module_aif0_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->aif_ch_base[ch] = dss_base + module_base;
	dpu_aif_init(dss_module->aif_ch_base[ch], &(dss_module->aif[ch]));
}

static void module_aif1_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->aif1_ch_base[ch] = dss_base + module_base;
	dpu_aif_init(dss_module->aif1_ch_base[ch], &(dss_module->aif1[ch]));
}

static void module_mif_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->mif_ch_base[ch] = dss_base + module_base;
	dpu_mif_init(dss_module->mif_ch_base[ch], &(dss_module->mif[ch]), ch);
}

static void module_mctl_chn_mutex_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->mctl_ch_base[ch].chn_mutex_base = dss_base + module_base;
}

static void module_mctl_chn_flush_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->mctl_ch_base[ch].chn_flush_en_base = dss_base + module_base;
}

static void module_mctl_chn_ov_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->mctl_ch_base[ch].chn_ov_en_base = dss_base + module_base;
}

static void module_mctl_chn_starty_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->mctl_ch_base[ch].chn_starty_base = dss_base + module_base;
	dpu_mctl_ch_starty_init(dss_module->mctl_ch_base[ch].chn_starty_base,
		&(dss_module->mctl_ch[ch]));
}

static void module_mctl_chn_mod_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->mctl_ch_base[ch].chn_mod_dbg_base = dss_base + module_base;
	dpu_mctl_ch_mod_dbg_init(dss_module->mctl_ch_base[ch].chn_mod_dbg_base,
		&(dss_module->mctl_ch[ch]));
}

static void module_dma_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->dma_base[ch] = dss_base + module_base;
	if (ch < DSS_WCHN_W0 || ch == DSS_RCHN_V2)
		dpu_rdma_init(dss_module->dma_base[ch], &(dss_module->rdma[ch]));

	if ((ch == DSS_RCHN_V0) || (ch == DSS_RCHN_V1) || (ch == DSS_RCHN_V2)) {
		dpu_rdma_u_init(dss_module->dma_base[ch], &(dss_module->rdma[ch]));
		dpu_rdma_v_init(dss_module->dma_base[ch], &(dss_module->rdma[ch]));
	}
}

static void module_dfc_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->dfc_base[ch] = dss_base + module_base;
	dpu_dfc_init(dss_module->dfc_base[ch], &(dss_module->dfc[ch]));
}

static void module_scl_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->scl_base[ch] = dss_base + module_base;
	dpu_scl_init(dss_module->scl_base[ch], &(dss_module->scl[ch]));
}

static void module_pcsc_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->pcsc_base[ch] = dss_base + module_base;
	dpu_csc_init(dss_module->pcsc_base[ch], &(dss_module->pcsc[ch]));
}

static void module_arsr2p_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->arsr2p_base[ch] = dss_base + module_base;
	dpu_arsr2p_init(dss_module->arsr2p_base[ch], &(dss_module->arsr2p[ch]));
}

static void module_post_clip_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->post_clip_base[ch] = dss_base + module_base;
	dpu_post_clip_init(dss_module->post_clip_base[ch], &(dss_module->post_clip[ch]));
}

static void module_csc_init(char __iomem *dss_base, uint32_t module_base,
	dss_module_reg_t *dss_module, uint32_t ch)
{
	dss_module->csc_base[ch] = dss_base + module_base;
	dpu_csc_init(dss_module->csc_base[ch], &(dss_module->csc[ch]));
}

static void module_post_scf_init(char __iomem *dss_base, uint32_t module_base, dss_module_reg_t *dss_module)
{
	dss_module->post_scf_base = dss_base + module_base;
	dpu_post_scf_init(dss_base, dss_module->post_scf_base, &(dss_module->post_scf));
}


static void module_mctl_sys_init(char __iomem *dss_base, uint32_t module_base, dss_module_reg_t *dss_module)
{
	dss_module->mctl_sys_base = dss_base + module_base;
	dpu_mctl_sys_init(dss_module->mctl_sys_base, &(dss_module->mctl_sys));
}

static void module_smmu_init(char __iomem *dss_base, uint32_t module_base, dss_module_reg_t *dss_module)
{
	dss_module->smmu_base = dss_base + module_base;
	dpu_smmu_init(dss_module->smmu_base, &(dss_module->smmu));
}

static struct single_ch_module_init g_dss_ch_module_init[] = {
	{ MODULE_AIF0_CHN, module_aif0_init },
	{ MODULE_AIF1_CHN, module_aif1_init },
	{ MODULE_MIF_CHN, module_mif_init },
	{ MODULE_MCTL_CHN_MUTEX, module_mctl_chn_mutex_init },
	{ MODULE_MCTL_CHN_FLUSH_EN, module_mctl_chn_flush_init },
	{ MODULE_MCTL_CHN_OV_OEN, module_mctl_chn_ov_init },
	{ MODULE_MCTL_CHN_STARTY, module_mctl_chn_starty_init },
	{ MODULE_MCTL_CHN_MOD_DBG, module_mctl_chn_mod_init },
	{ MODULE_DMA, module_dma_init },
	{ MODULE_DFC, module_dfc_init },
	{ MODULE_SCL, module_scl_init },
	{ MODULE_PCSC, module_pcsc_init },
	{ MODULE_ARSR2P, module_arsr2p_init },
	{ MODULE_POST_CLIP, module_post_clip_init },
	{ MODULE_CSC, module_csc_init },
};

static struct single_module_init g_dss_module_init[] = {
	{ DSS_POST_SCF_OFFSET, module_post_scf_init},

	{ DSS_MCTRL_SYS_OFFSET, module_mctl_sys_init},
	{ DSS_SMMU_OFFSET, module_smmu_init},
};

int dpu_module_default(struct dpu_fb_data_type *dpufd)
{
	dss_module_reg_t *dss_module = NULL;
	uint32_t module_base;
	char __iomem *dss_base = NULL;
	uint32_t i;
	uint32_t j;

	dpu_check_and_return((dpufd == NULL), -EINVAL, ERR, "dpufd is NULL\n");

	dss_base = dpufd->dss_base;
	dpu_check_and_return((dss_base == NULL), -EINVAL, ERR, "dss_base is NULL\n");

	dss_module = &(dpufd->dss_module_default);
	memset(dss_module, 0, sizeof(dss_module_reg_t));

	for (i = 0; i < DSS_MCTL_IDX_MAX; i++) {
		module_base = g_dss_module_ovl_base[i][MODULE_MCTL_BASE];
		if (module_base != 0) {
			dss_module->mctl_base[i] = dss_base + module_base;
			dpu_mctl_init(dss_module->mctl_base[i], &(dss_module->mctl[i]));
		}
	}

	for (i = 0; i < DSS_OVL_IDX_MAX; i++) {
		module_base = g_dss_module_ovl_base[i][MODULE_OVL_BASE];
		if (module_base != 0) {
			dss_module->ov_base[i] = dss_base + module_base;
			dpu_ovl_init(dss_module->ov_base[i], &(dss_module->ov[i]), i);
		}
	}

	for (i = 0; i < DSS_CHN_MAX_DEFINE; i++) {
		if (i == DSS_WCHN_W2)
			continue;
		for (j = 0; j < ARRAY_SIZE(g_dss_ch_module_init); j++) {
			module_base = g_dss_module_base[i][g_dss_ch_module_init[j].module_id];
			if (module_base != 0)
				g_dss_ch_module_init[j].reg_ch_module_init(dss_base, module_base, dss_module, i);
		}
	}

	for (j = 0; j < ARRAY_SIZE(g_dss_module_init); j++) {
		module_base = g_dss_module_init[j].module_offset;
		if (module_base != 0)
			g_dss_module_init[j].reg_module_init(dss_base, module_base, dss_module);
	}

	return 0;
}

static void dpufb_dss_on(struct dpu_fb_data_type *dpufd, int enable_cmdlist)
{
	int prev_refcount;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	dpu_mmbuf_sem_pend();

	prev_refcount = g_dss_sr_refcount++;
	if (prev_refcount == 0) {
		/* dss qos on */
		dpu_qos_on(dpufd);
		/* mif on */
		dpu_mif_on(dpufd);
		/* smmu on */
		dpu_smmu_on(dpufd);
		/* scl coef load */
		dpu_scl_coef_on(dpufd, false, SCL_COEF_YUV_IDX);  /*lint !e747*/

		if (enable_cmdlist)
			dpu_cmdlist_qos_on(dpufd);
	}
	dpu_mmbuf_sem_post();

	DPU_FB_DEBUG("fb%d, -, g_dss_sr_refcount=%d\n", dpufd->index, g_dss_sr_refcount);
}


void dpufb_dss_off(struct dpu_fb_data_type *dpufd, bool is_lp)
{
	int new_refcount;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	if (dpufd->index == MEDIACOMMON_PANEL_IDX)
		return;

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	dpu_mmbuf_sem_pend();
	new_refcount = --g_dss_sr_refcount;
	if (new_refcount < 0)
		DPU_FB_ERR("dss new_refcount err\n");

	if (is_lp) {
		if (new_refcount == 0) {
			dpufd->ldi_data_gate_en = 0;

			memset(dpufd->ov_block_infos_prev_prev, 0,
				DPU_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
			memset(dpufd->ov_block_infos_prev, 0,
				DPU_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
			memset(dpufd->ov_block_infos, 0,
				DPU_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));

			dpufb_dss_overlay_info_init(&dpufd->ov_req);
			dpufb_dss_overlay_info_init(&dpufd->ov_req_prev);
			dpufb_dss_overlay_info_init(&dpufd->ov_req_prev_prev);
		}
	}

	if (is_lp) {
		dpu_mmbuf_sem_post();
		return;
	}

	if (new_refcount == 0)
		dpu_mmbuf_mmbuf_free_all(dpufd);

	dpu_mmbuf_sem_post();

	DPU_FB_DEBUG("fb%d, -, g_dss_sr_refcount=%d\n", dpufd->index, g_dss_sr_refcount);
}

static int dpu_overlay_fastboot(struct dpu_fb_data_type *dpufd)
{
	dss_overlay_t *pov_req_prev = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		pov_req_prev = &(dpufd->ov_req_prev);
		memset(pov_req_prev, 0, sizeof(dss_overlay_t));
		pov_req_prev->ov_block_infos_ptr = (uint64_t)(uintptr_t)(dpufd->ov_block_infos_prev);
		pov_req_prev->ov_block_nums = 1;
		pov_req_prev->ovl_idx = DSS_OVL0;
		pov_req_prev->release_fence = -1;
		pov_req_prev->retire_fence = -1;

		pov_h_block_infos = (dss_overlay_block_t *)(uintptr_t)pov_req_prev->ov_block_infos_ptr;
		pov_h_block = &(pov_h_block_infos[0]);
		pov_h_block->layer_nums = 1;

		layer = &(pov_h_block->layer_infos[0]);
		layer->img.mmu_enable = 0;
		layer->layer_idx = 0x0;
		layer->chn_idx = DSS_RCHN_D0;
		layer->need_cap = 0;

		memcpy(&(dpufd->dss_module_default.rdma[DSS_RCHN_D0]), &(dpufd->dss_module_default.rdma[DSS_RCHN_D3]),
			sizeof(dss_rdma_t));
		memcpy(&(dpufd->dss_module_default.dfc[DSS_RCHN_D0]), &(dpufd->dss_module_default.dfc[DSS_RCHN_D3]),
			sizeof(dss_dfc_t));
		memcpy(&(dpufd->dss_module_default.ov[DSS_OVL0].ovl_layer[0]),
			&(dpufd->dss_module_default.ov[DSS_OVL0].ovl_layer[1]), sizeof(dss_ovl_layer_t));

		memset(&(dpufd->dss_module_default.mctl_ch[DSS_RCHN_D0]), 0, sizeof(dss_mctl_ch_t));
		memset(&(dpufd->dss_module_default.mctl[DSS_OVL0]), 0, sizeof(dss_mctl_t));

		dpufd->dss_module_default.mctl_sys.chn_ov_sel[DSS_OVL0] = 0xFFFFFFFF;  /* default valve */
		dpufd->dss_module_default.mctl_sys.ov_flush_en[DSS_OVL0] = 0x0;

		if (is_mipi_cmd_panel(dpufd)) {
			if (dpufd->vactive0_start_flag == 0) {
				dpufd->vactive0_start_flag = 1;
				dpufd->vactive0_end_flag = 1;
			}
		}
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}

static void dpu_overlay_on_param_init(struct dpu_fb_data_type *dpufd)
{
	dpufd->vactive0_start_flag = 0;
	dpufd->vactive0_end_flag = 0;
	dpufd->crc_flag = 0;
	dpufd->resolution_rect.x = 0;
	dpufd->resolution_rect.y = 0;
	dpufd->resolution_rect.w = dpufd->panel_info.xres;
	dpufd->resolution_rect.h = dpufd->panel_info.yres;

	dpufb_dss_overlay_info_init(&dpufd->ov_req);
	dpufd->ov_req.frame_no = 0;
	memset(&dpufd->acm_ce_info, 0, sizeof(dpufd->acm_ce_info));
	memset(dpufd->prefix_ce_info, 0, sizeof(dpufd->prefix_ce_info));
	g_offline_cmdlist_idxs = 0;
}

static void dpu_overlay_reg_modules_init(struct dpu_fb_data_type *dpufd)
{
	static uint32_t index;

	if (g_dss_module_resource_initialized == 0) {
		dpu_module_default(dpufd);
		g_dss_module_resource_initialized = 1;
		dpufd->dss_module_resource_initialized = true;
		index = dpufd->index;
	} else {
		if (!dpufd->dss_module_resource_initialized) {
			if ((dpufd->index != index) && (dpufd_list[index]))
				memcpy(&(dpufd->dss_module_default), &(dpufd_list[index]->dss_module_default),
					sizeof(dpufd->dss_module_default));
			dpufd->dss_module_resource_initialized = true;
		}
	}
}

static bool dpu_overlay_need_udpate_frame(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->base_frame_mode && is_mipi_cmd_panel(dpufd)) {
		DPU_FB_INFO("AOD is enable, disable base frame\n");
		dpufd->vactive0_start_flag = 1;
		dpufd->vactive0_end_flag = 1;
		return false;
	}

	return true;
}

static int dpu_ovl_base_config_default(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	dss_rect_t *wb_ov_block_rect, int ovl_idx)
{
	int ret;
	dss_ovl_t *ovl = NULL;
	struct img_size img_info = {0};
	int block_size = 0x7FFF;  /* default max reg valve */

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is nullptr!\n");
	if ((ovl_idx < DSS_OVL0) || (ovl_idx >= DSS_OVL_IDX_MAX)) {
		DPU_FB_ERR("ovl_idx is invalid\n");
		return -EINVAL;
	}

	if (pov_req && pov_req->wb_layer_infos[0].chn_idx == DSS_WCHN_W2)  /* dpuv400 copybit no ovl */
		return 0;

	ovl = &(dpufd->dss_module.ov[ovl_idx]);
	dpufd->dss_module.ov_used[ovl_idx] = 1;

	ret = get_img_size_info(dpufd, pov_req, wb_ov_block_rect, &img_info);
	dpu_check_and_return((ret != 0), -ret, ERR, "get_img_size_info error!\n");

	set_ovl_struct(dpufd, ovl, img_info, block_size);

	return ret;
}


static int dpu_overlay_on_update_frame(struct dpu_fb_data_type *dpufd, int enable_cmdlist)
{
	int ret;
	int ovl_idx;
	int mctl_idx;
	uint32_t cmdlist_idxs = 0;
	struct ov_module_set_regs_flag ov_module_flag = { enable_cmdlist, true, 1, 0 };

	ovl_idx = (dpufd->index == PRIMARY_PANEL_IDX) ? DSS_OVL0 : DSS_OVL1;
	mctl_idx = (dpufd->index == PRIMARY_PANEL_IDX) ? DSS_MCTL0 : DSS_MCTL1;

	ret = dpu_module_init(dpufd);
	dpu_check_and_return((ret != 0), -1, ERR, "fb%d, fail to module_init! ret=%d\n", dpufd->index, ret);

	if (!dpu_overlay_need_udpate_frame(dpufd))
		return -1;

	if (enable_cmdlist) {
		dpufd->set_reg = dpu_cmdlist_set_reg;
		dpu_cmdlist_data_get_online(dpufd);
		cmdlist_idxs = (0x1 << (uint32_t)(ovl_idx + DSS_CMDLIST_OV0));
		dpu_cmdlist_add_nop_node(dpufd, cmdlist_idxs, 0, 0);
	} else {
		dpufd->set_reg = dpufb_set_reg;
		dpu_mctl_mutex_lock(dpufd, ovl_idx);
	}

	dpufd->ov_req_prev.ovl_idx = ovl_idx;
	ret = dpu_ovl_base_config_default(dpufd, NULL, NULL, ovl_idx);
	dpu_check_and_return((ret != 0), -1, ERR, "fb%d, fail to ovl_base_config! ret=%d\n", dpufd->index, ret);

	ret = dpu_mctl_ov_config(dpufd, NULL, ovl_idx, false, true);
	dpu_check_and_return((ret != 0), -1, ERR, "fb%d, fail to mctl_config! ret=%d\n", dpufd->index, ret);

	ret = dpu_ov_module_set_regs(dpufd, NULL, ovl_idx, ov_module_flag);
	dpu_check_and_return((ret != 0), -1, ERR, "fb%d, fail to module_config! ret=%d\n", dpufd->index, ret);

	if (enable_cmdlist) {
		dpu_cmdlist_flush_cache(dpufd, cmdlist_idxs);
		dpu_cmdlist_config_start(dpufd, mctl_idx, cmdlist_idxs, 0);
	} else {
		dpu_mctl_mutex_unlock(dpufd, ovl_idx);
	}

	single_frame_update(dpufd);
	enable_ldi(dpufd);
	dpufb_frame_updated(dpufd);
	dpufd->frame_count++;

	if (g_debug_ovl_cmdlist)
		dpu_cmdlist_dump_all_node(dpufd, NULL, cmdlist_idxs);

	DPU_FB_INFO("fb%u self flush one frame\n", dpufd->index);
	return 0;
}

int dpu_overlay_on(struct dpu_fb_data_type *dpufd, bool fastboot_enable)
{
	int mctl_idx = 0;
	int enable_cmdlist;

	dpu_check_and_return((dpufd == NULL), -EINVAL, ERR, "dpufd is NULL\n");

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	dpu_overlay_on_param_init(dpufd);
	if (dpufd->index == MEDIACOMMON_PANEL_IDX)
		return 0;

	if ((dpufd->index == PRIMARY_PANEL_IDX) || (dpufd->index == EXTERNAL_PANEL_IDX))
		dpufb_activate_vsync(dpufd);

	dpu_overlay_reg_modules_init(dpufd);
	enable_cmdlist = g_enable_ovl_cmdlist_online;
	/* dss on */
	dpufb_dss_on(dpufd, enable_cmdlist);

	if ((dpufd->index == PRIMARY_PANEL_IDX) || (dpufd->index == EXTERNAL_PANEL_IDX)) {
		mctl_idx = (dpufd->index == PRIMARY_PANEL_IDX) ? DSS_MCTL0 : DSS_MCTL1;
		if ((dpufd->index == EXTERNAL_PANEL_IDX) && dpufd->panel_info.fake_external)
			enable_cmdlist = 0;

		ldi_data_gate(dpufd, true);
		dpu_mctl_on(dpufd, mctl_idx, enable_cmdlist, fastboot_enable);

		if (fastboot_enable)
			dpu_overlay_fastboot(dpufd);
		else
			dpu_overlay_on_update_frame(dpufd, enable_cmdlist);

		dpufb_deactivate_vsync(dpufd);
	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		enable_cmdlist = g_enable_ovl_cmdlist_offline;
		dpu_mctl_on(dpufd, DSS_MCTL2, enable_cmdlist, 0);
		dpu_mctl_on(dpufd, DSS_MCTL3, enable_cmdlist, 0);
		dpu_mctl_on(dpufd, DSS_MCTL5, enable_cmdlist, 0);
	} else {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);
	return 0;
}

static void dpu_overlay_reset_param(struct dpu_fb_data_type *dpufd)
{
	dpufd->ldi_data_gate_en = 0;
	dpufd->masklayer_maxbacklight_flag = false;

	memset(dpufd->ov_block_infos_prev_prev, 0, DPU_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
	memset(dpufd->ov_block_infos_prev, 0, DPU_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));
	memset(dpufd->ov_block_infos, 0, DPU_OV_BLOCK_NUMS * sizeof(dss_overlay_block_t));

	dpufb_dss_overlay_info_init(&dpufd->ov_req);
	dpufb_dss_overlay_info_init(&dpufd->ov_req_prev);
	dpufb_dss_overlay_info_init(&dpufd->ov_req_prev_prev);

	memset(dpufd->effect_updated_flag, 0, sizeof(dpufd->effect_updated_flag));
}

static int dpu_overlay_off_cmdlist_config(struct dpu_fb_data_type *dpufd, int enable_cmdlist)
{
	int ret;
	int ovl_idx = (dpufd->index == PRIMARY_PANEL_IDX) ? DSS_OVL0 : DSS_OVL1;
	uint32_t cmdlist_pre_idxs = 0;
	uint32_t cmdlist_idxs = 0;
	dss_overlay_t *pov_req_prev = &(dpufd->ov_req_prev);
	struct ov_module_set_regs_flag ov_module_flag = { enable_cmdlist, true, 1, 0 };

	if (enable_cmdlist) {
		dpufd->set_reg = dpu_cmdlist_set_reg;

		dpu_cmdlist_data_get_online(dpufd);

		ret = dpu_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_pre_idxs, NULL);
		dpu_check_and_return((ret != 0), -1, ERR, "fb%d, fail to get_cmdlist_idxs! ret=%d\n",
			dpufd->index, ret);

		cmdlist_idxs = (1 << (uint32_t)(DSS_CMDLIST_OV0 + ovl_idx));
		cmdlist_pre_idxs &= (~(cmdlist_idxs));

		dpu_cmdlist_add_nop_node(dpufd, cmdlist_pre_idxs, 0, 0);
		dpu_cmdlist_add_nop_node(dpufd, cmdlist_idxs, 0, 0);
	} else {
		dpufd->set_reg = dpufb_set_reg;

		dpu_mctl_mutex_lock(dpufd, ovl_idx);
		cmdlist_pre_idxs = ~0;
	}

	dpu_prev_module_set_regs(dpufd, pov_req_prev, cmdlist_pre_idxs, enable_cmdlist, NULL);

	ret = dpu_ovl_base_config_default(dpufd, NULL, NULL, ovl_idx);
	dpu_check_and_return((ret != 0), -1, ERR, "fb%d, fail to ovl_base_config! ret=%d\n", dpufd->index, ret);

	ret = dpu_mctl_ov_config(dpufd, NULL, ovl_idx, false, true);
	dpu_check_and_return((ret != 0), -1, ERR, "fb%d, fail to mctl_config! ret=%d\n", dpufd->index, ret);

	ret = dpu_ov_module_set_regs(dpufd, NULL, ovl_idx, ov_module_flag);
	dpu_check_and_return((ret != 0), -1, ERR, "fb%d, fail to module_config! ret=%d\n", dpufd->index, ret);

	if (enable_cmdlist) {
		dpu_cmdlist_config_stop(dpufd, cmdlist_pre_idxs);

		cmdlist_idxs |= cmdlist_pre_idxs;
		dpu_cmdlist_flush_cache(dpufd, cmdlist_idxs);

		if (g_debug_ovl_cmdlist)
			dpu_cmdlist_dump_all_node(dpufd, NULL, cmdlist_idxs);

		dpu_cmdlist_config_start(dpufd, ovl_idx, cmdlist_idxs, 0);
	} else {
		dpu_mctl_mutex_unlock(dpufd, ovl_idx);
	}

	return 0;
}

static int dpu_overlay_off_update_frame(struct dpu_fb_data_type *dpufd)
{
	int ret;
	int enable_cmdlist;

	enable_cmdlist = g_enable_ovl_cmdlist_online;
	if ((dpufd->index == EXTERNAL_PANEL_IDX) && dpufd->panel_info.fake_external)
		enable_cmdlist = 0;

	ret = dpu_module_init(dpufd);
	dpu_check_and_return((ret != 0), -1, ERR, "fb%d, fail to module_init! ret=%d\n", dpufd->index, ret);

	dpufd->resolution_rect.x = 0;
	dpufd->resolution_rect.y = 0;
	dpufd->resolution_rect.w = dpufd->panel_info.xres;
	dpufd->resolution_rect.h = dpufd->panel_info.yres;

	if (dpu_overlay_off_cmdlist_config(dpufd, enable_cmdlist) < 0)
		return -1;

	ldi_data_gate(dpufd, true);

	single_frame_update(dpufd);
	dpufb_frame_updated(dpufd);

	if (!dpu_check_reg_reload_status(dpufd))
		mdelay(20);  /* delay 20ms */

	ldi_data_gate(dpufd, false);

	if (is_mipi_cmd_panel(dpufd))
		dpufd->ldi_data_gate_en = 1;

	dpufd->frame_count++;

	return 0;
}

int dpu_overlay_off(struct dpu_fb_data_type *dpufd)
{
	int ret;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");
	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if ((dpufd->index == PRIMARY_PANEL_IDX) || (dpufd->index == EXTERNAL_PANEL_IDX)) {
		dpufb_activate_vsync(dpufd);
		ret = dpu_vactive0_start_config(dpufd, &(dpufd->ov_req_prev));
		if (ret != 0) {
			DPU_FB_ERR("fb%d, dpu_vactive0_start_config failed! ret = %d\n", dpufd->index, ret);
			goto err_out;
		}
		dpu_overlay_off_update_frame(dpufd);
err_out:
		dpufb_deactivate_vsync(dpufd);
	}

	if (dpufd->index == AUXILIARY_PANEL_IDX)
		dpufb_dss_off(dpufd, true);
	else
		dpufb_dss_off(dpufd, false);

	dpu_overlay_reset_param(dpufd);

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}

int dpu_overlay_on_lp(struct dpu_fb_data_type *dpufd)
{
	int mctl_idx;
	int enable_cmdlist;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	enable_cmdlist = g_enable_ovl_cmdlist_online;
	if ((dpufd->index == EXTERNAL_PANEL_IDX) && dpufd->panel_info.fake_external)
		enable_cmdlist = 0;

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		mctl_idx = DSS_MCTL0;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		mctl_idx = DSS_MCTL1;
	} else {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
		return -1;
	}

	/* dss on */
	dpufb_dss_on(dpufd, enable_cmdlist);

	dpu_mctl_on(dpufd, mctl_idx, enable_cmdlist, 0);

	return 0;
}

int dpu_overlay_off_lp(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	if ((dpufd->index != PRIMARY_PANEL_IDX) && (dpufd->index != EXTERNAL_PANEL_IDX)) {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
		return -1;
	}

	dpufb_dss_off(dpufd, true);

	return 0;
}

static void dpu_overlay_init_param_init(struct dpu_fb_data_type *dpufd)
{
	dpufd->dss_module_resource_initialized = false;

	dpufd->vactive0_start_flag = 0;
	dpufd->vactive0_end_flag = 0;
	dpufd->ldi_data_gate_en = 0;
	init_waitqueue_head(&dpufd->vactive0_start_wq);
	dpufd->crc_flag = 0;

	dpufd->frame_update_flag = 0;

	dpufd->online_play_count = 0;
	dpufd->masklayer_maxbacklight_flag = false;

	memset(&dpufd->ov_req, 0, sizeof(dss_overlay_t));
	dpufd->ov_req.release_fence = -1;
	dpufd->ov_req.retire_fence = -1;
	memset(&dpufd->ov_req_prev, 0, sizeof(dss_overlay_t));
	dpufd->ov_req_prev.release_fence = -1;
	dpufd->ov_req_prev.retire_fence = -1;
	memset(&dpufd->dss_module, 0, sizeof(dss_module_reg_t));
	memset(&dpufd->dss_module_default, 0, sizeof(dss_module_reg_t));

	dpufd->resolution_rect.x = 0;
	dpufd->resolution_rect.y = 0;
	dpufd->resolution_rect.w = dpufd->panel_info.xres;
	dpufd->resolution_rect.h = dpufd->panel_info.yres;

	dpufd->ov_ioctl_handler = dpu_overlay_ioctl_handler;

	dpufd->dss_debug_wq = NULL;
	dpufd->ldi_underflow_wq = NULL;
	dpufd->rch2_ce_end_wq = NULL;
	dpufd->rch4_ce_end_wq = NULL;
	dpufd->dpp_ce_end_wq = NULL;
	dpufd->masklayer_backlight_notify_wq = NULL;
	dpufd->delayed_cmd_queue_wq = NULL;
}

static int dpu_overlay_init_common(struct dpu_fb_data_type *dpufd)
{
	char wq_name[WORKQUEUE_NAME_SIZE] = {0};

	if ((dpufd->index == PRIMARY_PANEL_IDX) ||
		(dpufd->index == EXTERNAL_PANEL_IDX && !dpufd->panel_info.fake_external)) {
		snprintf(wq_name, WORKQUEUE_NAME_SIZE, "fb%d_dss_debug", dpufd->index);
		dpufd->dss_debug_wq = create_singlethread_workqueue(wq_name);
		dpu_check_and_return((dpufd->dss_debug_wq == NULL), -EINVAL, ERR,
			"fb%d, create dss debug workqueue failed!\n", dpufd->index);
		INIT_WORK(&dpufd->dss_debug_work, dpu_debug_func);

		snprintf(wq_name, WORKQUEUE_NAME_SIZE, "fb%d_ldi_underflow", dpufd->index);
		dpufd->ldi_underflow_wq = create_singlethread_workqueue(wq_name);
		dpu_check_and_return((dpufd->ldi_underflow_wq == NULL), -EINVAL, ERR,
			"fb%d, create ldi underflow workqueue failed!\n", dpufd->index);
		INIT_WORK(&dpufd->ldi_underflow_work, dpu_ldi_underflow_handle_func);

		if (dpufd->panel_info.delayed_cmd_queue_support) {
			snprintf(wq_name, WORKQUEUE_NAME_SIZE, "fb%d_delayed_cmd_queue", dpufd->index);
			dpufd->delayed_cmd_queue_wq = create_singlethread_workqueue(wq_name);
			dpu_check_and_return((dpufd->delayed_cmd_queue_wq == NULL), -EINVAL, ERR,
				"fb%d, create delayed cmd queue workqueue failed!\n", dpufd->index);
			INIT_WORK(&dpufd->delayed_cmd_queue_work, mipi_dsi_delayed_cmd_queue_handle_func);
			DPU_FB_INFO("Init delayed_cmd_queue_wq!\n");
		}
	}

	return 0;
}

static int dpu_overlay_init_primary_panel(struct dpu_fb_data_type *dpufd)
{
	char wq_name[WORKQUEUE_NAME_SIZE] = {0};
	dpufd->set_reg = dpu_cmdlist_set_reg;
	dpufd->ov_online_play = dpu_ov_online_play;
	dpufd->ov_offline_play = NULL;
	dpufd->ov_media_common_play = NULL;
	dpufd->ov_wb_isr_handler = NULL;
	dpufd->ov_vactive0_start_isr_handler = dpu_vactive0_start_isr_handler;

	dpufd->crc_isr_handler = NULL;

	dpu_effect_init(dpufd);

	/* for gmp lut set */
	dpufd->gmp_lut_wq = NULL;
	if (dpufd->panel_info.gmp_support) {
		snprintf(wq_name, WORKQUEUE_NAME_SIZE, "fb0_gmp_lut");
		dpufd->gmp_lut_wq = create_singlethread_workqueue(wq_name);
		dpu_check_and_return((dpufd->gmp_lut_wq == NULL), -EINVAL, ERR, "create gmp lut workqueue failed!\n");
		INIT_WORK(&dpufd->gmp_lut_work, dpufb_effect_gmp_lut_workqueue_handler);
	}

	return 0;
}

static void dpu_overlay_init_external_panel(struct dpu_fb_data_type *dpufd)
{
	dpufd->set_reg = dpufb_set_reg;
	dpufd->ov_online_play = dpu_ov_online_play;
	dpufd->ov_offline_play = NULL;
	dpufd->ov_media_common_play = NULL;
	dpufd->ov_wb_isr_handler = NULL;
	dpufd->ov_vactive0_start_isr_handler = dpu_vactive0_start_isr_handler;

	dpufd->crc_isr_handler = NULL;
}

static void dpu_overlay_init_auxiliary_panel(struct dpu_fb_data_type *dpufd)
{
	dpufd->set_reg = dpu_cmdlist_set_reg;
	dpufd->ov_online_play = NULL;
	dpufd->ov_offline_play = NULL;
	dpufd->ov_media_common_play = NULL;
	dpufd->ov_wb_isr_handler = NULL;
	dpufd->ov_vactive0_start_isr_handler = NULL;

	dpufd->crc_isr_handler = NULL;
}

int dpu_overlay_init(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_return((dpufd == NULL), -EINVAL, ERR, "dpufd is NULL\n");
	dpu_check_and_return((dpufd->dss_base == NULL), -EINVAL, ERR, "dpufd->dss_base\n");

	dpu_overlay_init_param_init(dpufd);

	if (dpu_overlay_init_common(dpufd) < 0) {
		DPU_FB_ERR("dpu_overlay_init_common failed\n");
		return -EINVAL;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpu_overlay_init_primary_panel(dpufd) < 0) {
			DPU_FB_ERR("dpu_overlay_init_primary_panel failed\n");
			return -EINVAL;
		}
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		dpu_overlay_init_external_panel(dpufd);
	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		dpu_overlay_init_auxiliary_panel(dpufd);
	} else {
		DPU_FB_ERR("fb%d not support this device!\n", dpufd->index);
		return -EINVAL;
	}

	dpu_mmbuf_init(dpufd);

	return 0;
}

int dpu_overlay_deinit(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_return((dpufd == NULL), -EINVAL, ERR, "dpufd is NULL\n");

	if (dpufd->index == PRIMARY_PANEL_IDX)
		dpu_effect_deinit(dpufd);

	dpu_destroy_wq(dpufd->rch4_ce_end_wq);
	dpu_destroy_wq(dpufd->rch2_ce_end_wq);
	dpu_destroy_wq(dpufd->dpp_ce_end_wq);

	dpu_destroy_wq(dpufd->dss_debug_wq);
	dpu_destroy_wq(dpufd->ldi_underflow_wq);
	dpu_destroy_wq(dpufd->delayed_cmd_queue_wq);

	if (!(dpufd->index == EXTERNAL_PANEL_IDX && !dpufd->panel_info.fake_external))
		dpu_cmdlist_deinit(dpufd);

	dpu_mmbuf_deinit(dpufd);

	dpu_destroy_wq(dpufd->gmp_lut_wq);
	return 0;
}
