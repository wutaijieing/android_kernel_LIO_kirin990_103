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

#include "overlay/dpu_overlay_utils.h"
#include "dpu_dpe_utils.h"
#include "dpu_mmbuf_manager.h"
#include "dpu_mipi_dsi.h"
#include "overlay/init/dpu_init.h"
#include "dpu_fb_panel.h"
#include "overlay/dpu_overlay_utils_platform.h"
#include "overlay/dump/dpu_dump.h"
#include "chrdev/dpu_chrdev.h"

static int dpu_check_cmdlist_enabled(
	struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
	return g_enable_ovl_cmdlist_online;
}

static int dpu_check_cmdlist_prepare(
	struct dpu_fb_data_type *dpufd,
	uint32_t *out_cmdlist_pre_idxs,
	uint32_t *out_cmdlist_idxs,
	int enable_cmdlist)
{
	int ret;
	uint32_t cmdlist_pre_idxs = 0;
	uint32_t cmdlist_idxs = 0;
	dss_overlay_t *pov_req = NULL;
	dss_overlay_t *pov_req_prev = NULL;

	pov_req = &(dpufd->ov_req);
	pov_req_prev = &(dpufd->ov_req_prev);

	(void)dpu_module_init(dpufd);

	dpu_mmbuf_info_get_online(dpufd);
	dpufd->use_mmbuf_cnt = 0;
	dpufd->use_axi_cnt = 0;

	if (enable_cmdlist) {
		dpufd->set_reg = dpu_cmdlist_set_reg;

		dpu_cmdlist_data_get_online(dpufd);

		ret = dpu_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_pre_idxs, NULL);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, get_cmdlist_idxs pov_req_prev failed! ret=%d\n", dpufd->index, ret);
			return -1;
		}
		ret = dpu_cmdlist_get_cmdlist_idxs(pov_req, &cmdlist_pre_idxs, &cmdlist_idxs);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, get_cmdlist_idxs pov_req failed! ret=%d\n", dpufd->index, ret);
			return -1;
		}
		dpu_cmdlist_add_nop_node(dpufd, cmdlist_pre_idxs, 0, 0);
		dpu_cmdlist_add_nop_node(dpufd, cmdlist_idxs, 0, 0);
	} else {
		if (dpufd->index == PRIMARY_PANEL_IDX) {
			(void)dpu_cmdlist_get_cmdlist_idxs(pov_req_prev, &cmdlist_pre_idxs, NULL);
			(void)dpu_cmdlist_get_cmdlist_idxs(pov_req, &cmdlist_pre_idxs, &cmdlist_idxs);
			if (g_debug_ovl_cmdlist && (cmdlist_pre_idxs != 0))
				DPU_FB_INFO("cmdlist_pre_idxs(0x%x), cmdlist_idxs(0x%x).\n", cmdlist_pre_idxs, cmdlist_idxs);
		} else {
			cmdlist_pre_idxs = ~0;
		}
		dpufd->set_reg = dpufb_set_reg;
		dpu_mctl_mutex_lock(dpufd, pov_req->ovl_idx);
	}

	dpu_prev_module_set_regs(dpufd, pov_req_prev, cmdlist_pre_idxs, enable_cmdlist, NULL);
	*out_cmdlist_pre_idxs = cmdlist_pre_idxs;
	*out_cmdlist_idxs = cmdlist_idxs;

	return 0;
}

static int dpu_check_vsync_and_mipi_resource(struct dpu_fb_data_type *dpufd,
		dss_overlay_t *pov_req, bool *vsync_time_checked)
{
	int ret = 0;

	if (is_mipi_video_panel(dpufd)) {
		*vsync_time_checked = true;
		ret = dpu_check_vsync_timediff(dpufd, pov_req);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static bool dpu_online_play_need_continue(
	struct dpu_fb_data_type *dpufd,
	void __user *argp)
{
	void_unused(argp);
	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%d, panel is power off!\n", dpufd->index);
		dpufd->backlight.bl_updated = 0;
		return false;
	}

	if (g_debug_ldi_underflow) {
		if (g_err_status & (DSS_PDP_LDI_UNDERFLOW | DSS_SDP_LDI_UNDERFLOW)) {
			mdelay(DPU_COMPOSER_HOLD_TIME);
			return false;
		}
	}

	if (g_debug_ovl_online_composer_return)
		return false;

	if (dpu_online_play_bypass_check(dpufd)) {
		DPU_FB_INFO("fb%d online play is bypassed!\n", dpufd->index);
		return false;
	}

	return true;
}

static int dpu_online_play_prepare(
	struct dpu_fb_data_type *dpufd,
	const void __user *argp)
{
	int ret;
	dss_overlay_t *pov_req = NULL;
	DPU_FB_DEBUG("fb%u+\n", dpufd->index);

	pov_req = &(dpufd->ov_req);
	ret = dpu_overlay_get_ov_data_from_user(dpufd, pov_req, argp);
	if (ret != 0) {
		DPU_FB_ERR("fb%d, dpu_overlay_get_ov_data_from_user failed! ret=%d\n", dpufd->index, ret);
		return ret;
	}

	if (is_mipi_cmd_panel(dpufd)) {
		ret = dpu_vactive0_start_config(dpufd, pov_req);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, dpu_vactive0_start_config failed! ret=%d\n", dpufd->index, ret);
			return ret;
		}
	}

	if (g_debug_ovl_online_composer)
		dump_dss_overlay(dpufd, pov_req);

	return ret;
}

static void dpu_online_play_cmdlist_flush(
	struct dpu_fb_data_type *dpufd,
	uint32_t cmdlist_pre_idxs,
	uint32_t *cmdlist_idxs,
	int enable_cmdlist)
{
	if (enable_cmdlist) {
		g_online_cmdlist_idxs |= *cmdlist_idxs;

		/* add taskend for share channel */
		dpu_cmdlist_add_nop_node(dpufd, *cmdlist_idxs, 0, 0);

		/* remove ch cmdlist */
		dpu_cmdlist_config_stop(dpufd, cmdlist_pre_idxs);

		*cmdlist_idxs |= cmdlist_pre_idxs;
		dpu_cmdlist_flush_cache(dpufd, *cmdlist_idxs);
	}
}

static int dpu_online_play_cmdlist_start(
	struct dpu_fb_data_type *dpufd,
	uint32_t cmdlist_idxs,
	int enable_cmdlist)
{
	dss_overlay_t *pov_req = &(dpufd->ov_req);

	if (enable_cmdlist) {
		if (g_fastboot_enable_flag)
			set_reg(dpufd->dss_base + DSS_MCTRL_CTL0_OFFSET + MCTL_CTL_TOP, 0x1, 32, 0);

		if (dpu_cmdlist_config_start(dpufd, pov_req->ovl_idx, cmdlist_idxs, 0))
			return -EFAULT;
	} else {
		dpu_mctl_mutex_unlock(dpufd, pov_req->ovl_idx);
	}

	return 0;
}

static int dpu_online_play_h_block_handle(
	struct dpu_fb_data_type *dpufd,
	dss_overlay_block_t *pov_h_block,
	int ov_block_idx,
	bool *vsync_time_checked,
	int enable_cmdlist)
{
	int ret;
	uint32_t i;
	dss_layer_t *layer = NULL;
	dss_rect_ltrb_t clip_rect = {0};
	dss_rect_t aligned_rect = {0};
	dss_overlay_t *pov_req = NULL;
	struct dpu_ov_compose_rect ov_compose_rect = { NULL, NULL, &clip_rect, &aligned_rect };
	struct dpu_ov_compose_flag ov_compose_flag = { false, false, true, enable_cmdlist };

	pov_req = &(dpufd->ov_req);
	dpu_aif_handler(dpufd, pov_req, pov_h_block);

	ret = dpu_ovl_base_config(dpufd, pov_req, pov_h_block, NULL, ov_block_idx);
	if (ret != 0) {
		DPU_FB_ERR("fb%d, dpu_ovl_init failed! ret=%d\n", dpufd->index, ret);
		return ret;
	}

	if (ov_block_idx == 0) {
		ret = dpu_check_vsync_and_mipi_resource(dpufd, pov_req, vsync_time_checked);
		if (ret < 0) {
			DPU_FB_ERR("fb%d, check vsync failed! ret=%d\n", dpufd->index, ret);
			return ret;
		}
	}

	/* Go through all layers */
	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &(pov_h_block->layer_infos[i]);
		memset(ov_compose_rect.clip_rect, 0, sizeof(dss_rect_ltrb_t));
		memset(ov_compose_rect.aligned_rect, 0, sizeof(dss_rect_t));
		ov_compose_flag.rdma_stretch_enable = false;

		ret = dpu_ov_compose_handler(dpufd, pov_h_block, layer, &ov_compose_rect, &ov_compose_flag);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, dpu_ov_compose_handler failed! ret=%d\n", dpufd->index, ret);
			return ret;
		}
	}

	ret = dpu_mctl_ov_config(dpufd, pov_req, pov_req->ovl_idx, ov_compose_flag.has_base, (ov_block_idx == 0));
	if (ret != 0) {
		DPU_FB_ERR("fb%d, dpu_mctl_config failed! ret=%d\n", dpufd->index, ret);
		return ret;
	}
	return ret;
}

static int dpu_online_play_blocks_handle(
	struct dpu_fb_data_type *dpufd,
	bool *vsync_time_checked,
	int enable_cmdlist)
{
	int ret = 0;
	uint32_t m;
	dss_overlay_t *pov_req = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	struct ov_module_set_regs_flag ov_module_flag = { enable_cmdlist, false, 0, 0 };

	pov_req = &(dpufd->ov_req);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_req->ov_block_infos[m]);
		(void)dpu_module_init(dpufd);

		ret = dpu_online_play_h_block_handle(dpufd, pov_h_block, m, vsync_time_checked, enable_cmdlist);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, play_h_block_handle failed! ret=%d\n", dpufd->index, ret);
			return ret;
		}

		ov_module_flag.is_first_ov_block = (m == 0) ? true : false;
		ret = dpu_ov_module_set_regs(dpufd, pov_req, pov_req->ovl_idx, ov_module_flag);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, dpu_module_config failed! ret=%d\n", dpufd->index, ret);
			return ret;
		}
	}

	return ret;
}

static int dpu_online_play_config_start(
	struct dpu_fb_data_type *dpufd,
	struct dss_cmdlist_idxs *idxs_tmp,
	bool vsync_time_checked,
	bool *masklayer_maxbacklight_flag)
{
	int ret;
	dss_overlay_t *pov_req = NULL;
	dss_overlay_t *pov_req_prev = NULL;
	void_unused(masklayer_maxbacklight_flag);

	pov_req = &(dpufd->ov_req);
	pov_req_prev = &(dpufd->ov_req_prev);

	dpu_online_play_cmdlist_flush(dpufd,
		*(idxs_tmp->cmdlist_pre_idxs), idxs_tmp->cmdlist_idxs, *(idxs_tmp->enable_cmdlist));
	if (!g_fake_lcd_flag)
		dpu_unflow_handler(dpufd, pov_req, true);

	if (vsync_time_checked == false) {
		ret = dpu_check_vsync_timediff(dpufd, pov_req);
		if (ret < 0)
			return ret;
	}

	ret = dpu_online_play_cmdlist_start(dpufd, *(idxs_tmp->cmdlist_idxs), *(idxs_tmp->enable_cmdlist));
	if (ret != 0)
		return ret;

	single_frame_update(dpufd);
	dpufb_frame_updated(dpufd);

	return 0;
}

void dpu_online_play_post_handle(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is nullptr!\n");

	/* pass to hwcomposer handle, driver doesn't use it no longer */
	dpufd->ov_req.release_fence = -1;
	dpufd->ov_req.retire_fence = -1;
	dpufd->enter_idle = false;
	dpufd->emi_protect_check_count = 0;
	dpufd->frame_count++;

	memcpy(&dpufd->ov_req_prev_prev, &dpufd->ov_req_prev, sizeof(dss_overlay_t));
	memcpy(dpufd->ov_block_infos_prev_prev, dpufd->ov_block_infos_prev,
		dpufd->ov_req_prev.ov_block_nums * sizeof(dss_overlay_block_t));

	memcpy(&dpufd->ov_req_prev, &dpufd->ov_req, sizeof(dss_overlay_t));
	memcpy(dpufd->ov_block_infos_prev, dpufd->ov_block_infos,
		dpufd->ov_req.ov_block_nums * sizeof(dss_overlay_block_t));
	dpufd->vsync_ctrl.vsync_timestamp_prev = dpufd->vsync_ctrl.vsync_timestamp;
}

int dpu_online_play_config_locked(struct dpu_fb_data_type *dpufd)
{
	int ret;
	int enable_cmdlist;
	uint32_t cmdlist_idxs = 0;
	uint32_t cmdlist_pre_idxs = 0;
	bool vsync_time_checked = false;
	bool masklayer_maxbkl_flag = false;
	dss_overlay_t *pov_req = NULL;
	struct dss_cmdlist_idxs idxs_tmp = { &cmdlist_pre_idxs, &cmdlist_idxs, &enable_cmdlist };

	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is nullptr!\n");
	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%u is power off\n", dpufd->index);
		return 0;
	}

	pov_req = &(dpufd->ov_req);
	enable_cmdlist = dpu_check_cmdlist_enabled(dpufd);

	ret = dpu_check_cmdlist_prepare(dpufd, &cmdlist_pre_idxs, &cmdlist_idxs, enable_cmdlist);
	if (ret) {
		DPU_FB_ERR("online play prepare failed!\n");
		goto err_return;
	}

	ret = dpu_online_play_blocks_handle(dpufd, &vsync_time_checked, enable_cmdlist);
	if (ret) {
		DPU_FB_ERR("online play block config failed!\n");
		goto err_return;
	}

	ret = dpu_online_play_config_start(dpufd, &idxs_tmp, vsync_time_checked, &masklayer_maxbkl_flag);
	if (ret) {
		DPU_FB_ERR("online play start config failed!\n");
		goto err_return;
	}

	if (g_debug_ovl_cmdlist && enable_cmdlist)
		dpu_cmdlist_dump_all_node(dpufd, NULL, cmdlist_idxs);

	dpufb_masklayer_backlight_flag_config(dpufd, masklayer_maxbkl_flag);

	return ret;

err_return:
	return ret;
}

void dpufb_check_timediff(struct timeval *tv_begin, struct timeval *tv_end)
{
	uint32_t timediff;

	dpufb_get_timestamp(tv_end);
	timediff = dpufb_timestamp_diff(tv_begin, tv_end);
	if ((timediff >= g_online_comp_timeout) && (g_debug_online_vsync != 0))
		DPU_FB_INFO("online play config took %u us!\n", timediff);
}

int dpu_ov_online_play(
	struct dpu_fb_data_type *dpufd,
	void __user *argp)
{
	int ret;

	struct timeval tv[3] = { {0}, {0}, {0}};  /* timestamp diff */

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is nullptr!\n");
	DPU_FB_DEBUG("fb%d +\n", dpufd->index);

	dpu_check_and_return(!argp, -EINVAL, ERR, "argp is nullptr!\n");

	if (!dpu_online_play_need_continue(dpufd, argp)) {
		DPU_FB_INFO("fb%d skip online play!\n", dpufd->index);
		return 0;
	}

	dpufb_get_timestamp(&tv[0]);

	if (dpu_online_play_prepare(dpufd, argp))
		return 0;

	DPU_FB_DEBUG("fb[%d], frame_no[%u]\n", dpufd->index, dpufd->ov_req.frame_no);
	down(&dpufd->blank_sem0);
	ret = dpu_online_play_config_locked(dpufd);
	dpu_check_and_return(ret != 0, ret, ERR, "online config failed\n");
	up(&dpufd->blank_sem0);

	dpu_online_play_post_handle(dpufd);
	if (ret != 0) {
		if (g_underflow_count <= DSS_UNDERFLOW_COUNT) {
			g_underflow_count++;
			ret = ERR_RETURN_DUMP;
		}
	}

	dpufb_check_timediff(&tv[0], &tv[2]);

	DPU_FB_DEBUG("fb%d -\n", dpufd->index);
	return ret;
}

int dpu_online_play_bypass(struct dpu_fb_data_type *dpufd, const void __user *argp)
{
	int ret;
	int bypass;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is nullptr!\n");
	dpu_check_and_return(!argp, -EINVAL, ERR, "argp is nullptr!\n");

	/* only bypass primary display */
	if (dpufd->index != PRIMARY_PANEL_IDX)
		return -EINVAL;

	/* only active with cmd panel */
	if (!is_mipi_cmd_panel(dpufd))
		return -EINVAL;

	DPU_FB_INFO("+\n");

	ret = (int)copy_from_user(&bypass, argp, sizeof(bypass));
	if (ret) {
		DPU_FB_ERR("arg is invalid\n");
		return -EINVAL;
	}

	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%d, panel is power off!\n", dpufd->index);
		if (bypass) {
			DPU_FB_INFO("cannot bypss when power off!\n");
			return -EINVAL;
		}
	}
	(void)dpu_online_play_bypass_set(dpufd, bypass);

	DPU_FB_INFO("-\n");

	return 0;
}

bool dpu_online_play_bypass_set(struct dpu_fb_data_type *dpufd, int bypass)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is nullptr!\n");
		return false;
	}

	/* only bypass primary display */
	if (dpufd->index != PRIMARY_PANEL_IDX)
		return false;

	/* only active with cmd panel */
	if (!is_mipi_cmd_panel(dpufd))
		return false;

	DPU_FB_INFO("bypass = %d\n", bypass);

	if (bypass != 0) {
		if (dpufd->bypass_info.bypass_count > ONLINE_PLAY_BYPASS_MAX_COUNT) {
			DPU_FB_WARNING("already reach the max bypass count, disable bypass first\n");
			return false;
		}
		dpufd->bypass_info.bypass = true;
	} else {
		/* reset state, be available to start a new bypass cycle */
		dpufd->bypass_info.bypass = false;
		dpufd->bypass_info.bypass_count = 0;
	}

	return true;
}

bool dpu_online_play_bypass_check(struct dpu_fb_data_type *dpufd)
{
	bool ret = true;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is nullptr!\n");
		return false;
	}

	/* only bypass primary display */
	if (dpufd->index != PRIMARY_PANEL_IDX)
		return false;

	/* only active with cmd panel */
	if (!is_mipi_cmd_panel(dpufd))
		return false;

	if (g_debug_online_play_bypass) {
		if (g_debug_online_play_bypass < 0)
			g_debug_online_play_bypass = 0;
		if (dpufd->panel_power_on)
			dpu_online_play_bypass_set(dpufd, g_debug_online_play_bypass);
		g_debug_online_play_bypass = 0;
	}

	if (!dpufd->bypass_info.bypass)
		return false;

	dpufd->bypass_info.bypass_count++;
	if (dpufd->bypass_info.bypass_count > ONLINE_PLAY_BYPASS_MAX_COUNT) {
		/* not reset the bypass_count, wait the call of stopping bypass */
		dpufd->bypass_info.bypass = false;
		ret = false;
		DPU_FB_WARNING("reach the max bypass count, restore online play\n");
	}

	return ret;
}

