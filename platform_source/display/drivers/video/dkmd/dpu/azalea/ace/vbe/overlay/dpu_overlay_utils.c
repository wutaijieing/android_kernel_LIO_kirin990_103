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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "dpu_overlay_utils.h"
#include "../dpu_display_effect.h"
#include "../dpu_dpe_utils.h"
#include "../dpu_mmbuf_manager.h"
#include "../dpu_fb_panel.h"
#include "../dpu_iommu.h"
#include "../../include/dpu_communi_def.h"
#include "../fault_mgr/dpu_fault_mgr.h"

#define vsync_left_time(timediff) ((timediff) - 8000000)

struct prev_module_param {
	bool has_ovl;
	bool has_rot;
	int32_t ovl_idx;
	uint32_t chn_idx;
	uint32_t display_id;
};

static inline bool dpu_is_sharpness_support(int32_t width, int32_t height)
{
	/* width porch and height porch, width and height resolution */
	return ((width >= 16) && (width <= 1600) && (height >= 4) && (height <= 2560));
}

static int dpu_add_rch_cmdlist(struct dpu_fb_data_type *dpufd,  int chn_idx,
	uint32_t wb_type, bool enable_cmdlist)
{
	int ret = 0;
	uint32_t tmp = 0;
	union cmd_flag flag = {
		.bits.pending = 0,
		.bits.task_end = 0,
		.bits.last = 0,
	};
	void_unused(wb_type);

	if (enable_cmdlist) {
		dpufd->cmdlist_idx = dpu_get_cmdlist_idx_by_chnidx((uint32_t)chn_idx);
		tmp = (0x1 << (uint32_t)(dpufd->cmdlist_idx));

		/* FIXME:base, dim */
		/* add rch cmdlist */
		ret = dpu_cmdlist_add_new_node(dpufd, tmp, flag, 0);
		dpu_check_and_return((ret != 0), ret, ERR, "fb%d, dpu_cmdlist_add_new_node err:%d\n",
			dpufd->index, ret);
	}

	return ret;
}

static void dpu_dma_ch_set_reg(struct dpu_fb_data_type *dpufd, int chn_idx,
	dss_module_reg_t *dss_module)
{
	if (chn_idx < DSS_WCHN_W0 || chn_idx == DSS_RCHN_V2)
		dpu_rdma_set_reg(dpufd, dss_module->dma_base[chn_idx], &(dss_module->rdma[chn_idx]));

	if ((chn_idx == DSS_RCHN_V0) || (chn_idx == DSS_RCHN_V1) || (chn_idx == DSS_RCHN_V2)) {
		dpu_rdma_u_set_reg(dpufd, dss_module->dma_base[chn_idx], &(dss_module->rdma[chn_idx]));
		dpu_rdma_v_set_reg(dpufd, dss_module->dma_base[chn_idx], &(dss_module->rdma[chn_idx]));
	}
}

int dpu_ch_module_set_regs(struct dpu_fb_data_type *dpufd, int32_t mctl_idx, int chn_idx,
	uint32_t wb_type, bool enable_cmdlist)
{
	int i;
	dss_module_reg_t *dss_module = NULL;

	dpu_check_and_return((dpufd == NULL), -1, DEBUG, "dpufd is NULL!\n");
	dpu_check_and_return(((chn_idx < 0) || (chn_idx >= DSS_CHN_MAX_DEFINE)), -1, DEBUG,
		"chn_idx is out of the range!\n");

	i = chn_idx;
	dss_module = &(dpufd->dss_module);

	if (dpu_add_rch_cmdlist(dpufd, chn_idx, wb_type, enable_cmdlist) != 0)
		return -1;

	if (dss_module->mctl_ch_used[i] == 1)
		dpu_mctl_ch_set_reg(dpufd, &(dss_module->mctl_ch_base[i]),
			&(dss_module->mctl_ch[i]), mctl_idx, i);

	if (dss_module->smmu_used == 1)
		dpu_smmu_ch_set_reg(dpufd, dss_module->smmu_base, &(dss_module->smmu), i);

	if (dss_module->dma_used[i] == 1)
		dpu_dma_ch_set_reg(dpufd, i, dss_module);

	if (dss_module->aif_ch_used[i] == 1)
		dpu_aif_ch_set_reg(dpufd, dss_module->aif_ch_base[i], &(dss_module->aif[i]));

	if (dss_module->aif1_ch_used[i] == 1)
		dpu_aif_ch_set_reg(dpufd, dss_module->aif1_ch_base[i], &(dss_module->aif1[i]));

	if (dss_module->mif_used[i] == 1)
		dpu_mif_set_reg(dpufd, dss_module->mif_ch_base[i], &(dss_module->mif[i]), i);

	if (dss_module->dfc_used[i] == 1)
		dpu_dfc_set_reg(dpufd, dss_module->dfc_base[i], &(dss_module->dfc[i]));

	if (dss_module->scl_used[i] == 1)
		dpu_scl_set_reg(dpufd, dss_module->scl_base[i], &(dss_module->scl[i]));

	if (dpufd->dss_module.post_clip_used[i])
		dpu_post_clip_set_reg(dpufd, dss_module->post_clip_base[i], &(dss_module->post_clip[i]), i);

	if (dss_module->pcsc_used[i] == 1)
		dpu_csc_set_reg(dpufd, dss_module->pcsc_base[i], &(dss_module->pcsc[i]));

	if (dss_module->arsr2p_used[i] == 1)
		dpu_arsr2p_set_reg(dpufd, dss_module->arsr2p_base[i], &(dss_module->arsr2p[i]));

	if (dss_module->csc_used[i] == 1)
		dpu_csc_set_reg(dpufd, dss_module->csc_base[i], &(dss_module->csc[i]));

	if (dss_module->mctl_ch_used[i] == 1)
		dpu_mctl_sys_ch_set_reg(dpufd, &(dss_module->mctl_ch_base[i]),
			&(dss_module->mctl_ch[i]), i, true);

	return 0;
}

bool dpu_is_async_play(struct dpu_fb_data_type *dpufd)
{
	if (!is_mipi_cmd_panel(dpufd))
		return false;

	return true;
}

int dpu_ov_module_set_regs(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req, int ovl_idx,
	struct ov_module_set_regs_flag ov_module_flag)
{
	dss_module_reg_t *dss_module = NULL;
	int ret = 0;
	uint32_t tmp = 0;
	union cmd_flag flag = {
		.bits.pending = 0,
		.bits.task_end = ov_module_flag.task_end,
		.bits.last = ov_module_flag.last,
	};

	dpu_check_and_return((dpufd == NULL), -1, DEBUG, "dpufd is NULL!\n");

	/* dpuv400 copybit no ovl */
	if ((pov_req != NULL) && pov_req->wb_layer_infos[0].chn_idx == DSS_WCHN_W2)
		return 0;

	dss_module = &(dpufd->dss_module);
	dpu_check_and_return(((ovl_idx < 0) || (ovl_idx >= DSS_OVL_IDX_MAX)), -1, DEBUG,
		"ovl_idx More than Array Upper Limit!\n");

	if (ov_module_flag.enable_cmdlist) {
		/* add ov cmdlist */
		dpufd->cmdlist_idx = dpu_get_cmdlist_idx_by_ovlidx(ovl_idx);
		tmp = (0x1 << (uint32_t)(dpufd->cmdlist_idx));

		ret = dpu_cmdlist_add_new_node(dpufd, tmp, flag, 0);
		dpu_check_and_return((ret != 0), ret, ERR, "fb%d, dpu_cmdlist_add_new_node err:%d\n",
			dpufd->index, ret);
	}

	if (dss_module->mctl_used[ovl_idx] == 1)
		dpu_mctl_ov_set_reg(dpufd, dss_module->mctl_base[ovl_idx],
			&(dss_module->mctl[ovl_idx]), ovl_idx, ov_module_flag.enable_cmdlist);

	if (dss_module->ov_used[ovl_idx] == 1)
		dpu_ovl_set_reg(dpufd, dss_module->ov_base[ovl_idx], &(dss_module->ov[ovl_idx]), ovl_idx);

	if (dss_module->post_scf_used == 1)
		dpu_post_scf_set_reg(dpufd, dss_module->post_scf_base, &(dss_module->post_scf));

	if (ov_module_flag.is_first_ov_block) {
		dpu_dpp_acm_gm_set_reg(dpufd);
		dpu_effect_set_reg(dpufd);
	}

	if (dss_module->mctl_sys_used == 1)
		dpu_mctl_sys_set_reg(dpufd, dss_module->mctl_sys_base, &(dss_module->mctl_sys), ovl_idx);

	return 0;
}

static int dpu_layer_set_regs(struct dpu_fb_data_type *dpufd, uint32_t chn_idx)
{
	dss_module_reg_t *dss_module = NULL;

	dss_module = &(dpufd->dss_module);

	/* RCH default */
	dpu_chn_set_reg_default_value(dpufd, dss_module->dma_base[chn_idx]);

	/* remove smmu config by cmdlist, not for smmu without shadow register */
	dpu_smmu_ch_set_reg(dpufd, dss_module->smmu_base, &(dss_module->smmu), chn_idx);

	/* MIF */
	dpu_mif_set_reg(dpufd, dss_module->mif_ch_base[chn_idx], &(dss_module->mif[chn_idx]), chn_idx);

	/* AIF */
	dpu_aif_ch_set_reg(dpufd, dss_module->aif_ch_base[chn_idx], &(dss_module->aif[chn_idx]));

	/* MCTL */
	dss_module->mctl_ch[chn_idx].chn_mutex =
		set_bits32(dss_module->mctl_ch[chn_idx].chn_mutex, 1, 1, 0);
	dss_module->mctl_ch[chn_idx].chn_flush_en =
		set_bits32(dss_module->mctl_ch[chn_idx].chn_flush_en, 1, 1, 0);
	dss_module->mctl_ch[chn_idx].chn_ov_oen =
		set_bits32(dss_module->mctl_ch[chn_idx].chn_ov_oen, 0, 32, 0);
	dss_module->mctl_ch_used[chn_idx] = 1;

	dpu_mctl_sys_ch_set_reg(dpufd, &(dss_module->mctl_ch_base[chn_idx]),
		&(dss_module->mctl_ch[chn_idx]), chn_idx, false);

	return 0;
}

static void get_primary_online_mmbuf(int32_t ovl_idx, uint32_t chn_idx, dss_layer_t *layer)
{
	if (ovl_idx == DSS_OVL0) {
		g_primary_online_mmbuf[chn_idx].mmbuf.addr = layer->img.mmbuf_base;
		g_primary_online_mmbuf[chn_idx].mmbuf.size = layer->img.mmbuf_size;
		g_primary_online_mmbuf[chn_idx].ov_idx = ovl_idx;
	}
}

static void get_external_online_mmbuf(int32_t ovl_idx, uint32_t chn_idx, const dss_layer_t *layer)
{
	if (ovl_idx == DSS_OVL1) {
		g_external_online_mmbuf[chn_idx].mmbuf.addr = layer->img.mmbuf_base;
		g_external_online_mmbuf[chn_idx].mmbuf.size = layer->img.mmbuf_size;
		g_external_online_mmbuf[chn_idx].ov_idx = ovl_idx;
	}
}

static int dpu_block_set_regs(struct dpu_fb_data_type *dpufd, dss_overlay_block_t *pov_h_block,
	dss_mmbuf_t *offline_mmbuf,
	uint32_t *cmdlist_idxs_temp, int32_t ovl_idx)
{
	dss_layer_t *layer = NULL;
	uint32_t chn_idx = 0;
	uint32_t i = 0;
	int j = 0;
	uint32_t tmp = 0;

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &(pov_h_block->layer_infos[i]);
		chn_idx = layer->chn_idx;
		if (chn_idx >= DSS_CHN_MAX_DEFINE) {
			DPU_FB_ERR("chn_idx=%d, More than Array Upper Limit\n", chn_idx);
			return -EINVAL;
		}
		if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
			continue;

		if ((layer->need_cap & (CAP_AFBCD | CAP_HFBCD | CAP_HEBCD)) &&
			(layer->dst_rect.y >= pov_h_block->ov_block_rect.y)) {
			if (chn_idx < DSS_CHN_MAX_DEFINE) {
				get_primary_online_mmbuf(ovl_idx, chn_idx, layer);
				get_external_online_mmbuf(ovl_idx, chn_idx, layer);
			}
		}

		if ((ovl_idx == DSS_OVL2) || (ovl_idx == DSS_OVL3)) {
			if ((layer->need_cap & (CAP_AFBCD | CAP_HFBCD | CAP_HEBCD)) && j < DSS_CHN_MAX_DEFINE) {
					offline_mmbuf[j].addr = layer->img.mmbuf_base;
					offline_mmbuf[j].size = layer->img.mmbuf_size;
					j++;
			}
		}

		dpufd->cmdlist_idx = dpu_get_cmdlist_idx_by_chnidx(chn_idx);
		tmp = (0x1 << (uint32_t)(dpufd->cmdlist_idx));

		if ((*cmdlist_idxs_temp & tmp) != tmp)
			continue;
		*cmdlist_idxs_temp &= (~tmp);

		dpu_layer_set_regs(dpufd, chn_idx);
	}

	return 0;
}

static int dpu_mctl_sys_ov_set_reg(struct dpu_fb_data_type *dpufd, dss_module_reg_t *dss_module,
	int32_t ovl_idx)
{
	/* MCTL ov */
	dss_module->mctl_sys.chn_ov_sel_used[ovl_idx] = 1;
	dss_module->mctl_sys.wch_ov_sel_used[ovl_idx - DSS_OVL2] = 1;
	dss_module->mctl_sys.ov_flush_en_used[ovl_idx] = 1;
	dss_module->mctl_sys.ov_flush_en[ovl_idx] =
		set_bits32(dss_module->mctl_sys.ov_flush_en[ovl_idx], 0x1, 1, 0);
	dpu_mctl_sys_set_reg(dpufd, dss_module->mctl_sys_base, &(dss_module->mctl_sys), ovl_idx);

	return 0;
}

static void dpu_wb_layers_set_regs(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, struct prev_module_param *param)
{
	dss_wb_layer_t *wb_layer = NULL;
	uint32_t i;

	for (i = 0; i < pov_req->wb_layer_nums; i++) {
		wb_layer = &(pov_req->wb_layer_infos[i]);
		param->chn_idx = wb_layer->chn_idx;
		param->display_id = wb_layer->dst.display_id;
		if (wb_layer->transform & DPU_FB_TRANSFORM_ROT_90)
			param->has_rot = true;

		/* dpuv400 copybit */
		dpufd->cmdlist_idx = dpu_get_cmdlist_idx_by_chnidx(param->chn_idx);

		dpu_layer_set_regs(dpufd, param->chn_idx);
	}
}

int dpu_prev_module_set_regs(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, uint32_t cmdlist_pre_idxs, bool enable_cmdlist, int *use_comm_mmbuf)
{
	dss_module_reg_t *dss_module = NULL;
	struct prev_module_param param = {true, false, 0, 0, 0};
	uint32_t i = 0;
	int ret;
	uint32_t cmdlist_idxs_temp;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_mmbuf_t offline_mmbuf[DSS_CHN_MAX_DEFINE] = { {0} };
	union cmd_flag flag = {
		.bits.pending = 0,
		.bits.task_end = 1,
		.bits.last = 1,
	};

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is nullptr!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "pov_req is nullptr!\n");
	param.ovl_idx = pov_req->ovl_idx;
	dss_module = &(dpufd->dss_module);

	if (enable_cmdlist) {
		/* clear prev chn cmdlist reg default */
		if (pov_req->wb_enable) {
			ret = dpu_cmdlist_add_new_node(dpufd, cmdlist_pre_idxs, flag, 1);
		} else {
			flag.bits.task_end = 0;
			flag.bits.last = 0;
			ret = dpu_cmdlist_add_new_node(dpufd, cmdlist_pre_idxs, flag, 0);
		}
		dpu_check_and_return((ret != 0), ret, ERR, "fb%d, add_new_node err:%d\n", dpufd->index, ret);
	}
	if ((dpufd->index == MEDIACOMMON_PANEL_IDX) && (pov_req->wb_compose_type == DSS_WB_COMPOSE_COPYBIT))
		param.has_ovl = false;

	memset(offline_mmbuf, 0x0, sizeof(offline_mmbuf));
	cmdlist_idxs_temp = cmdlist_pre_idxs;
	pov_h_block_infos = pov_req->ov_block_infos;

	for (i = 0; i < pov_req->ov_block_nums; i++)
		dpu_block_set_regs(dpufd, &(pov_h_block_infos[i]), offline_mmbuf, &cmdlist_idxs_temp, param.ovl_idx);

	if (pov_req->wb_enable && ((param.ovl_idx > DSS_OVL1) || (!param.has_ovl))) {
		if (param.has_ovl) {
			dpufd->cmdlist_idx = dpu_get_cmdlist_idx_by_ovlidx(param.ovl_idx);
			/* OV default */
			dpu_ov_set_reg_default_value(dpufd, dss_module->ov_base[param.ovl_idx], param.ovl_idx);
		}

		dpu_wb_layers_set_regs(dpufd, pov_req, &param);

		if (param.has_ovl) {  /* dpuv400 copybit */
			dpufd->cmdlist_idx = dpu_get_cmdlist_idx_by_ovlidx(param.ovl_idx);

			dpu_mctl_sys_ov_set_reg(dpufd, dss_module, param.ovl_idx);
		}
		dpu_check_use_comm_mmbuf(param.display_id, use_comm_mmbuf, offline_mmbuf, param.has_rot);
	}

	return 0;
}

bool dpu_check_reg_reload_status(struct dpu_fb_data_type *dpufd)
{
	if (g_fpga_flag == 1) {
		/* android reboot test may cause dss underflow and clear ack timeout in fpga */
		/* delay 1s to ensure dss status is stable when display off */
		mdelay(1000);  /* delay 1000ms */
		DPU_FB_INFO("need delay 1s in fpga!\n");
	} else {
		mdelay(50);  /* delay 50ms */
	}

	(void)dpufd;

	return true;
}

bool dpu_check_crg_sctrl_status(struct dpu_fb_data_type *dpufd)
{
	uint32_t crg_state_check;
	uint32_t sctrl_mmbuf_dss_check;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	crg_state_check = inp32(dpufd->peri_crg_base + PERCLKEN3);
	if ((crg_state_check & 0x23000) != 0x23000) {  /* mask */
		DPU_FB_ERR("dss crg_clk_enable failed, crg_state_check = 0x%x\n", crg_state_check);
		return false;
	}

	crg_state_check = inp32(dpufd->peri_crg_base + PERRSTSTAT3);
	if ((crg_state_check | 0xfffff3ff) != 0xfffff3ff) {  /* mask */
		DPU_FB_ERR("dss crg_reset failed, crg_state_check = 0x%x\n", crg_state_check);
		return false;
	}

	crg_state_check = inp32(dpufd->peri_crg_base + ISOSTAT);
	if ((crg_state_check | 0xffffffbf) != 0xffffffbf) {  /* mask */
		DPU_FB_ERR("dss iso_disable failed, crg_state_check = 0x%x\n", crg_state_check);
		return false;
	}

	crg_state_check = inp32(dpufd->peri_crg_base + PERPWRSTAT);
	if ((crg_state_check & 0x20) != 0x20) {  /* mask */
		DPU_FB_ERR("dss subsys regulator_enabel failed, crg_state_check = 0x%x\n", crg_state_check);
		return false;
	}

	sctrl_mmbuf_dss_check = inp32(dpufd->sctrl_base + SCPERCLKEN1);
	if ((sctrl_mmbuf_dss_check & 0x1000000) != 0x1000000) {  /* mask */
		DPU_FB_ERR("dss subsys mmbuf_dss_clk_enabel failed, sctrl_mmbuf_dss_check = 0x%x\n",
			sctrl_mmbuf_dss_check);
		return false;
	}

	return true;
}

static uint32_t dpu_ldi_get_cmdlist_idxs(struct dpu_fb_data_type *dpufd)
{
	int ret;
	dss_overlay_t *pov_req_prev = NULL;
	dss_overlay_t *pov_req_prev_prev = NULL;
	uint32_t cmdlist_idxs_prev = 0;
	uint32_t cmdlist_idxs_prev_prev = 0;

	pov_req_prev = &(dpufd->ov_req_prev);
	pov_req_prev_prev = &(dpufd->ov_req_prev_prev);

	ret = dpu_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_idxs_prev, NULL);
	if (ret != 0)
		DPU_FB_ERR("fb%d, dpu_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret = %d\n", dpufd->index, ret);

	ret = dpu_cmdlist_get_cmdlist_idxs(pov_req_prev_prev, &cmdlist_idxs_prev_prev, NULL);
	if (ret != 0)
		DPU_FB_ERR("fb%d, dpu_cmdlist_get_cmdlist_idxs pov_req_prev_prev failed! ret = %d\n",
			dpufd->index, ret);

	DPU_FB_INFO("fb%d, -. cmdlist_idxs_prev = 0x%x, cmdlist_idxs_prev_prev = 0x%x\n",
		dpufd->index, cmdlist_idxs_prev, cmdlist_idxs_prev_prev);

	return (cmdlist_idxs_prev | cmdlist_idxs_prev_prev);
}

void dpu_ldi_underflow_handle_func(struct work_struct *work)
{
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t cmdlist_idxs;

	dpu_check_and_no_retval((work == NULL), ERR, "work is NULL!\n");
	dpufd = container_of(work, struct dpu_fb_data_type, ldi_underflow_work);
	dpu_check_and_no_retval((dpufd == NULL), ERR, "dpufd is NULL!\n");

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	down(&dpufd->blank_sem0);
	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%d, panel is power off!\n", dpufd->index);
		up(&dpufd->blank_sem0);
		return;
	}
	cmdlist_idxs = dpu_ldi_get_cmdlist_idxs(dpufd);
	dpufb_activate_vsync(dpufd);
	dpu_cmdlist_config_reset(dpufd, &(dpufd->ov_req_prev), cmdlist_idxs);
	dpufb_deactivate_vsync(dpufd);
	up(&dpufd->blank_sem0);
	DPU_FB_INFO("fb%d, -\n", dpufd->index);
}

static int dpu_check_iova_info(struct dpu_iova_info iova_info, uint64_t vir_addr, uint32_t buf_size)
{
	if (vir_addr < iova_info.iova_start ||
	    	vir_addr + buf_size > iova_info.iova_start + iova_info.iova_size) {
		DPU_FB_ERR("invalid vir_addr\n");
		return -1;
	}
	return 0;
}

static int dpu_check_layer_iova_validate(struct layer_info layer)
{
	int ret = -1;

	if (layer.disp_source == RDA_DISP)
		ret = dpu_check_iova_info(g_dpu_iova_info, layer.vir_addr, layer.buf_size);
	else if (layer.disp_source == NON_RDA_DISP)
		ret = dpu_check_iova_info(g_dpu_vm_iova_info, layer.vir_addr, layer.buf_size);
	else
		DPU_FB_ERR("invalid disp_source\n");

	if (ret != 0) {
		DPU_FB_ERR("disp_source:%d, layer%d check iova failed!\n", layer.disp_source, layer.layer_idx);
		dpu_submit_disp_fault(layer.disp_source, CHECK_IOVA_VALID_FAIL);
		return -1;
	}

	return ret;
}

static int dpu_get_layer_addr(struct layer_info *layer)
{
	int ret;
	uint64_t iova = 0;

	if (layer->need_cap & (CAP_BASE | CAP_DIM | CAP_PURE_COLOR))
		return 0;

	if (layer->disp_source != RDA_DISP)
		return 0;

	ret = dpu_get_buffer_by_dma_buf(&iova, layer->buf, layer->buf_size);
	if (ret != 0) {
		DPU_FB_ERR("get iova by dma_buf failed\n");
		dpu_submit_disp_fault(RDA_DISP, GET_IOVA_BY_DMA_BUF_FAIL);
		return -1;
	}

	layer->vir_addr = iova;
	return 0;
}

int dpu_load_layer_info(dss_overlay_block_t *ov_block_info, struct block_info *tmp_blk_info, uint32_t fb_index)
{
	uint32_t i;

	dpu_check_and_return((ov_block_info == NULL), -1, ERR, "ov_block_info is NULL point!\n");
	dpu_check_and_return((tmp_blk_info == NULL), -1, ERR, "tmp_blk_info is NULL point!\n");

	for (i = 0; i < ov_block_info->layer_nums; i++) {
		dpu_check_and_return(dpu_get_layer_addr(&(tmp_blk_info->layer_infos[i])) != 0, -1, ERR,
			"fb%u get layer addr failed\n", fb_index);
		if (dpu_check_layer_iova_validate(tmp_blk_info->layer_infos[i]) != 0)
			return -1;

		ov_block_info->layer_infos[i].img.format = tmp_blk_info->layer_infos[i].format;
		ov_block_info->layer_infos[i].img.width = tmp_blk_info->layer_infos[i].width;
		ov_block_info->layer_infos[i].img.height = tmp_blk_info->layer_infos[i].height;
		ov_block_info->layer_infos[i].img.bpp = tmp_blk_info->layer_infos[i].bpp;
		ov_block_info->layer_infos[i].img.buf_size = tmp_blk_info->layer_infos[i].buf_size;
		ov_block_info->layer_infos[i].img.stride = tmp_blk_info->layer_infos[i].stride;
		ov_block_info->layer_infos[i].img.stride_plane1 = tmp_blk_info->layer_infos[i].stride_plane1;
		ov_block_info->layer_infos[i].img.stride_plane2 = tmp_blk_info->layer_infos[i].stride_plane2;
		ov_block_info->layer_infos[i].img.offset_plane1 = tmp_blk_info->layer_infos[i].offset_plane1;
		ov_block_info->layer_infos[i].img.offset_plane2 = tmp_blk_info->layer_infos[i].offset_plane2;
		ov_block_info->layer_infos[i].img.csc_mode = tmp_blk_info->layer_infos[i].csc_mode;

		ov_block_info->layer_infos[i].img.vir_addr = tmp_blk_info->layer_infos[i].vir_addr;
		ov_block_info->layer_infos[i].img.shared_fd = tmp_blk_info->layer_infos[i].shared_fd;
		ov_block_info->layer_infos[i].src_rect = tmp_blk_info->layer_infos[i].src_rect;
		ov_block_info->layer_infos[i].dst_rect = tmp_blk_info->layer_infos[i].dst_rect;

		ov_block_info->layer_infos[i].transform = tmp_blk_info->layer_infos[i].transform;
		ov_block_info->layer_infos[i].blending = tmp_blk_info->layer_infos[i].blending;
		ov_block_info->layer_infos[i].glb_alpha = tmp_blk_info->layer_infos[i].glb_alpha;
		ov_block_info->layer_infos[i].color = tmp_blk_info->layer_infos[i].color;
		ov_block_info->layer_infos[i].layer_idx = tmp_blk_info->layer_infos[i].layer_idx;
		ov_block_info->layer_infos[i].chn_idx = tmp_blk_info->layer_infos[i].chn_idx;
		ov_block_info->layer_infos[i].need_cap = tmp_blk_info->layer_infos[i].need_cap;

		ov_block_info->layer_infos[i].img.afbc_header_addr = tmp_blk_info->layer_infos[i].afbc_header_addr;
		ov_block_info->layer_infos[i].img.afbc_header_offset = tmp_blk_info->layer_infos[i].afbc_header_offset;
		ov_block_info->layer_infos[i].img.afbc_header_stride = tmp_blk_info->layer_infos[i].afbc_header_stride;
		ov_block_info->layer_infos[i].img.afbc_payload_addr = tmp_blk_info->layer_infos[i].afbc_payload_addr;
		ov_block_info->layer_infos[i].img.afbc_payload_offset = tmp_blk_info->layer_infos[i].afbc_payload_offset;
		ov_block_info->layer_infos[i].img.afbc_payload_stride = tmp_blk_info->layer_infos[i].afbc_payload_stride;
		ov_block_info->layer_infos[i].img.afbc_scramble_mode = tmp_blk_info->layer_infos[i].afbc_scramble_mode;
		ov_block_info->layer_infos[i].img.mmbuf_base = tmp_blk_info->layer_infos[i].mmbuf_base;
		ov_block_info->layer_infos[i].img.mmbuf_size = tmp_blk_info->layer_infos[i].mmbuf_size;
		ov_block_info->layer_infos[i].img.mmu_enable = tmp_blk_info->layer_infos[i].mmu_enable;
	}

	return 0;
}

static int dpu_unpack_snd_overlay_info(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, const void __user *argp)
{
	int ret;
	struct dpu_core_disp_data tmp_ov_info;
	uint32_t m;


	ret = copy_from_user(&tmp_ov_info, argp, sizeof(struct dpu_core_disp_data));
	if (ret) {
		DPU_FB_ERR("fb%d, copy_from_user failed!\n", dpufd->index);
		return -EINVAL;
	}

	if (tmp_ov_info.magic_num != SND_INFO_MAGIC_NUM) {
		DPU_FB_ERR("wrong magic num:0x%x\n", tmp_ov_info.magic_num);
		return -1;
	}

	DPU_FB_DEBUG("overlay size : %u\n", sizeof(struct dpu_core_disp_data));
	pov_req->ov_block_nums = tmp_ov_info.ov_block_nums;
	pov_req->frame_no = tmp_ov_info.frame_no;
	pov_req->ovl_idx = tmp_ov_info.ovl_idx;

	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_req->ov_block_infos[m].layer_nums = tmp_ov_info.ov_block_infos[m].layer_nums;
		pov_req->ov_block_infos[m].ov_block_rect = tmp_ov_info.ov_block_infos[m].ov_block_rect;
		ret = dpu_load_layer_info(&pov_req->ov_block_infos[m], &tmp_ov_info.ov_block_infos[m], dpufd->index);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, dpu_unpack_layer_info failed!\n", dpufd->index);
			return -EINVAL;
		}
	}

	return 0;
}

int dpu_overlay_get_ov_data_from_user(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, const void __user *argp)
{
	int ret;
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");
	dpu_check_and_return(!pov_req, -EINVAL, ERR, "pov_req is NULL!\n");
	dpu_check_and_return(!argp, -EINVAL, ERR, "user data is invalid!\n");

	ret = dpu_unpack_snd_overlay_info(dpufd, pov_req, argp);
	if (ret) {
		DPU_FB_ERR("fb%d, unpack_snd_overlay_info failed!\n", dpufd->index);
		return -EINVAL;
	}

	ret = dpu_check_userdata(dpufd, pov_req, pov_req->ov_block_infos);
	if (ret != 0) {
		DPU_FB_ERR("fb%d, dpu_check_userdata failed!\n", dpufd->index);
		return -EINVAL;
	}

	return ret;
}

uint64_t dpu_get_vsync_timediff(struct dpu_fb_data_type *dpufd)
{
	uint64_t vsync_timediff = 0;
	struct dpu_panel_info *pinfo = NULL;
	uint32_t current_fps = 0;

	dpu_check_and_return((dpufd == NULL), 0, ERR, "dpufd is NULL\n");

	pinfo = &(dpufd->panel_info);

	if (pinfo->mipi.dsi_timing_support) {
		current_fps = (pinfo->fps == FPS_HIGH_60HZ) ? FPS_60HZ : pinfo->fps;
		if (current_fps != 0)
			vsync_timediff = 1000000000UL / (uint64_t)current_fps;
	} else {
		vsync_timediff = (uint64_t)(dpufd->panel_info.xres + dpufd->panel_info.ldi.h_back_porch +
			dpufd->panel_info.ldi.h_front_porch + dpufd->panel_info.ldi.h_pulse_width) *
			(dpufd->panel_info.yres + dpufd->panel_info.ldi.v_back_porch +
			dpufd->panel_info.ldi.v_front_porch + dpufd->panel_info.ldi.v_pulse_width) *
			1000000000UL / dpufd->panel_info.pxl_clk_rate;
	}

	DPU_FB_DEBUG("dsi_timing_support=%d, vsync_timediff=%llu, current_fps=%d, pinfo_fps=%d\n",
		pinfo->mipi.dsi_timing_support, vsync_timediff, current_fps, pinfo->fps);

	return vsync_timediff;
}

int dpu_check_vsync_timediff(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req)
{
	ktime_t prepare_timestamp;
	uint64_t vsync_timediff;
	uint64_t timestamp = 2000000;  /* 2ms */
	int ret;

	dpu_check_and_return((dpufd == NULL), -EINVAL, ERR, "dpufd is NULL\n");
	dpu_check_and_return((pov_req == NULL), -EINVAL, ERR, "pov_req is NULL\n");

	if (is_mipi_video_panel(dpufd)) {
		vsync_timediff = dpu_get_vsync_timediff(dpufd);
		if (vsync_timediff == 0) {
			DPU_FB_ERR("error vsync_timediff, maybe pinfo->fps is 0\n");
			return -1;
		}

		prepare_timestamp = (s64)systime_get();
		if ((ktime_to_ns(prepare_timestamp) > ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp)) &&
			((ktime_to_ns(prepare_timestamp) - ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp)) <
			(int64_t)(vsync_timediff - timestamp)) &&
			(ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp_prev) !=
			ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp))) {
			DPU_FB_DEBUG("fb%d, neednot wait next vstart! vsync_timediff=%llu, timestamp_diff=%llu.\n", \
				dpufd->index, vsync_timediff, \
				ktime_to_ns(prepare_timestamp) - ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp));
		} else {
			DPU_FB_DEBUG("fb%d, need wait next vstart! timestamp_diff=%llu\n", dpufd->index,
				ktime_to_ns(prepare_timestamp) - ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp));
			ret = dpu_vactive0_start_config(dpufd, pov_req);
			dpu_check_and_return((ret != 0), -1, ERR, "fb%d, dpu_vactive0_start_config fail ret=%d\n",
				dpufd->index, ret);
		}
	}

	return 0;
}

static bool check_time_enough_for_cfg(struct dpufb_vsync *vsync_ctrl, ktime_t cur_timestamp, uint64_t vsync_timediff)
{
	if (ktime_to_ns(cur_timestamp) <= ktime_to_ns(vsync_ctrl->vsync_timestamp))
		return false;
	if ((uint64_t)(ktime_to_ns(cur_timestamp) - ktime_to_ns(vsync_ctrl->vsync_timestamp)) >=
		vsync_left_time(vsync_timediff))
		return false;
	if (ktime_to_ns(vsync_ctrl->vsync_timestamp_prev) == ktime_to_ns(vsync_ctrl->vsync_timestamp))
		return false;
	return true;
}

bool dpu_check_vsync_valid(struct dpu_fb_data_type *dpufd)
{
	ktime_t cur_timestamp;
	uint64_t vsync_timediff;

	dpu_check_and_return(dpufd == NULL, false, ERR, "dpufd is NULL\n");

	if ((dpufd->frame_count == 1) && is_mipi_video_panel(dpufd)) {
		DPU_FB_INFO("video panel neednot to do check vsync in first frame\n");
		return true;
	}

	vsync_timediff = dpu_get_vsync_timediff(dpufd);
	dpu_check_and_return(vsync_timediff == 0, false, ERR, "error vsync_timediff, maybe pinfo->fps is 0\n");

	cur_timestamp = systime_get();
	if (check_time_enough_for_cfg(&dpufd->vsync_ctrl, cur_timestamp, vsync_timediff)) {
		DPU_FB_DEBUG("fb%u, expected vsync_timediff=%llu, actual timestamp_diff=%llu\n", \
			dpufd->index, vsync_left_time(vsync_timediff), \
			ktime_to_ns(cur_timestamp) - ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp));
		return true;
	}

	DPU_FB_ERR("fb%u, vsync check failed! expected vsync_timediff=%llu, actual timestamp_diff=%llu\n", \
		dpufd->index, vsync_left_time(vsync_timediff), \
		ktime_to_ns(cur_timestamp) - ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp));
	return false;
}

#pragma GCC diagnostic pop
