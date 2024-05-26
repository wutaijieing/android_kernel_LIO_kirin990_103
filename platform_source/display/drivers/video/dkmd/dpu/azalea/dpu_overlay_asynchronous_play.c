/* Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
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
#pragma GCC diagnostic ignored "-Winteger-overflow"

#include "overlay/dpu_overlay_utils.h"
#include "dpu_dpe_utils.h"
#include "dpu_mmbuf_manager.h"
#ifdef CONFIG_POWER_DUBAI_RGB_STATS
#include "product/rgb_stats/dpu_fb_rgb_stats.h"
#endif
struct dpu_ov_asynch_play_param      {
	uint32_t cmdlist_pre_idxs;
	uint32_t cmdlist_idxs;
	int enable_cmdlist;
	int need_skip;
	uint32_t timediff;
	bool masklayer_maxbacklight_flag;
	timeval_compatible tv0;
	timeval_compatible tv1;
	timeval_compatible returntv0;
};

#if defined(CONFIG_DPU_FB_AOD)
extern bool dpu_aod_get_aod_status(void);
#endif

#define MAX_WAIT_ASYNC_TIMES 25

#ifdef SUPPORT_DUMP_REGISTER_INFO
static void dump_dss_register_info(struct dpu_fb_data_type *dpufd,
	int ret, timeval_compatible tv0, timeval_compatible tv1)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL Pointer\n");
		return;
	}

	dpufb_activate_vsync(dpufd);

	DPU_FB_ERR("fb%d, wait asyn vactive0 start flag timeout, ret=%d, "
		"async_flag=%d, vactive0_flag=%d, timediff=%u us, itf_ints=0x%x, dpp_dbg=0x%x, "
		"online_fill_lvl=0x%x, ldi_vstate=0x%x, phy_status=0x%x\n",
		dpufd->index, ret, dpufd->asynchronous_vactive0_start_flag,
		dpufd->vactive0_start_flag, dpufb_timestamp_diff(&tv0, &tv1),
		inp32(dpufd->mipi_dsi0_base + MIPI_LDI_CPU_ITF_INTS),
		inp32(dpufd->dss_base + DSS_DISP_CH0_OFFSET + DISP_CH_DBG_CNT),
		inp32(dpufd->dss_base + DSS_DBUF0_OFFSET + DBUF_ONLINE_FILL_LEVEL),
		inp32(dpufd->mipi_dsi0_base + MIPI_LDI_VSTATE),
		inp32(dpufd->mipi_dsi0_base + MIPIDSI_PHY_STATUS_OFFSET));

	dpufb_deactivate_vsync(dpufd);
}
#endif

#if defined(CONFIG_CDC_MULTI_DISPLAY)
static void dpu_set_async_vactive0_start_flag(struct dpu_fb_data_type *dpufd)
{
	unsigned long flags = 0;

	if (!is_mipi_cmd_panel(dpufd)) {
		spin_lock_irqsave(&dpufd->buf_sync_ctrl.refresh_lock, flags);
		if (dpufd->buf_sync_ctrl.refresh)
			dpufd->asynchronous_vactive0_start_flag = 0;
		spin_unlock_irqrestore(&dpufd->buf_sync_ctrl.refresh_lock, flags);
	}
}
#else
#define dpu_set_async_vactive0_start_flag(dpufd)
#endif

int dpu_get_release_and_retire_fence(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	int ret = 0;
	int times = 0;
	uint32_t timeout_interval = g_debug_wait_asy_vactive0_thr;
	struct dss_fence fence_fd;
	timeval_compatible tv0;
	timeval_compatible tv1;

	dpu_check_and_return(((!dpufd) || (!argp)), -EINVAL, ERR, "NULL Pointer!\n");

	dpu_check_and_return(!dpufd->panel_power_on, -EINVAL, ERR, "fb%d, panel is power off!\n", dpufd->index);

	fence_fd.release_fence = -1;
	fence_fd.retire_fence = -1;

	if (dpu_is_async_play(dpufd)) {
		dpu_set_async_vactive0_start_flag(dpufd);

		dpufb_get_timestamp(&tv0);
		if (dpufd->asynchronous_vactive0_start_flag == 0) {
			while (1) {
				ret = wait_event_interruptible_timeout(dpufd->asynchronous_play_wq, /*lint !e578*/
						dpufd->asynchronous_vactive0_start_flag,
						msecs_to_jiffies(timeout_interval)); /*lint !e666*/
				if ((ret == -ERESTARTSYS) && (times++ < MAX_WAIT_ASYNC_TIMES))
					mdelay(2);  /* ms */
				else
					break;
			}

			if (ret <= 0) {
				dpufb_get_timestamp(&tv1);
				/* resync fence with hwcomposer */
				dpufb_buf_sync_suspend(dpufd);
#ifdef SUPPORT_DUMP_REGISTER_INFO
				dump_dss_register_info(dpufd, ret, tv0, tv1);
#endif
				ret = -EFAULT;
				goto err_return;
			}
		}
		dpufd->asynchronous_vactive0_start_flag = 0;
	}

	ret = dpufb_buf_sync_create_fence(dpufd, &fence_fd.release_fence, &fence_fd.retire_fence);
	if (ret != 0)
		DPU_FB_INFO("fb%d, dpu_create_fence failed!\n", dpufd->index);

	if (copy_to_user((struct dss_fence __user *)argp, &fence_fd, sizeof(struct dss_fence))) {
		DPU_FB_ERR("fb%d, copy_to_user failed.\n", dpufd->index);
		dpufb_buf_sync_close_fence(&fence_fd.release_fence, &fence_fd.retire_fence);
		ret = -EFAULT;
	}

	return ret;
err_return:
	if (is_mipi_cmd_panel(dpufd))
		dpufd->asynchronous_vactive0_start_flag = 1;

	return ret;
}

static void dpu_free_reserve_buffer(struct dpu_fb_data_type *dpufd,
	int free_buffer_refcount)
{
	struct dpu_fb_data_type *external_fb = dpufd_list[EXTERNAL_PANEL_IDX];

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer!\n");
		return;
	}

	if ((dpufd->index == PRIMARY_PANEL_IDX) && (free_buffer_refcount > 1)) {
		if (g_logo_buffer_base && g_logo_buffer_size) {
			dpufb_free_logo_buffer(dpufd);
			DPU_FB_INFO("dss_free_buffer_refcount = %d!\n", free_buffer_refcount);
		}

		/* wakeup DP when android system has been startup */
		if (external_fb) {
			external_fb->dp.dptx_gate = true;
			if (external_fb->dp_wakeup)
				external_fb->dp_wakeup(external_fb);
		}
	}
}

static void dpu_preserve_ov_req(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	int enable_cmdlist, uint32_t cmdlist_idxs)
{
	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer!\n");
		return;
	}

	if (g_debug_ovl_cmdlist && enable_cmdlist)
		dpu_cmdlist_dump_all_node(dpufd, NULL, cmdlist_idxs);

	if ((pov_req->ov_block_nums < 0) || (pov_req->ov_block_nums > DPU_OV_BLOCK_NUMS)) {
		DPU_FB_ERR("ov_block_nums[%d] is out of range!\n", pov_req->ov_block_nums);
		return;
	}

	dpufd->frame_count++;
	memcpy(&dpufd->ov_req_prev_prev, &dpufd->ov_req_prev, sizeof(dss_overlay_t));
	memcpy(dpufd->ov_block_infos_prev_prev, dpufd->ov_block_infos_prev,
		dpufd->ov_req_prev.ov_block_nums * sizeof(dss_overlay_block_t));
	dpufd->ov_req_prev_prev.ov_block_infos_ptr = (uint64_t)(uintptr_t)(dpufd->ov_block_infos_prev_prev);

	memcpy(&dpufd->ov_req_prev, pov_req, sizeof(dss_overlay_t));
	memcpy(dpufd->ov_block_infos_prev, dpufd->ov_block_infos,
		pov_req->ov_block_nums * sizeof(dss_overlay_block_t));
	dpufd->ov_req_prev.ov_block_infos_ptr = (uint64_t)(uintptr_t)(dpufd->ov_block_infos_prev);
	dpufd->vsync_ctrl.vsync_timestamp_prev = dpufd->vsync_ctrl.vsync_timestamp;
}

static int ov_composer_first_block_process(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, bool *vsync_time_checked, bool first_block)
{
	int ret;

	ret = dpu_dirty_region_dbuf_config(dpufd, pov_req);
	dpu_check_and_return((ret != 0), -EINVAL, ERR,
		"fb%d, dirty_region_dbuf_config failed! ret = %d\n", dpufd->index, ret);

	ret = dpu_post_scf_config(dpufd, pov_req);
	dpu_check_and_return((ret != 0), -EINVAL, ERR,
		"fb%d, post_scf_config failed! ret = %d\n", dpufd->index, ret);

#if defined(CONFIG_DPU_FB_V510) || defined(CONFIG_DPU_FB_V350) || \
	defined(CONFIG_DPU_FB_V345) || defined(CONFIG_DPU_FB_V346)
	ret = dpu_effect_arsr1p_config(dpufd, pov_req);
	dpu_check_and_return((ret != 0), -EINVAL, ERR,
		"fb%d, effect_arsr1p_config failed! ret = %d\n", dpufd->index, ret);

#if defined(CONFIG_EFFECT_HIACE)
	ret = dpu_effect_hiace_config(dpufd);
	dpu_check_and_return((ret != 0), -EINVAL, ERR,
		"fb%d, effect_hiace_config failed! ret = %d\n", dpufd->index, ret);
#endif
#endif

	if (is_mipi_video_panel(dpufd) || is_dp_panel(dpufd)) {
		*vsync_time_checked = true;
		if (dpufb_wait_for_mipi_resource_available(dpufd, pov_req) < 0)
			return -EINVAL;
		if (dpu_check_vsync_timediff(dpufd, pov_req) < 0)
			return -EINVAL;
	}

	return ret;
}

static int dpu_ov_layers_composer(struct dpu_fb_data_type *dpufd, dss_overlay_block_t *pov_h_block,
	struct dpu_ov_compose_rect *ov_compose_rect, struct dpu_ov_compose_flag *ov_compose_flag)
{
	uint32_t i;
	int ret;
	dss_layer_t *layer = NULL;

	for (i = 0; i < pov_h_block->layer_nums; i++) {
		layer = &(pov_h_block->layer_infos[i]);
		memset(ov_compose_rect->clip_rect, 0, sizeof(dss_rect_ltrb_t));
		memset(ov_compose_rect->aligned_rect, 0, sizeof(dss_rect_t));
		ov_compose_flag->rdma_stretch_enable = false;

		ret = dpu_ov_compose_handler(dpufd, pov_h_block, layer, ov_compose_rect, ov_compose_flag);
		dpu_check_and_return((ret != 0), -EINVAL, ERR,
			"fb%d, ov_compose failed! ret = %d\n", dpufd->index, ret);
	}

	return 0;
}

static int dpu_set_ov_composer_info(struct dpu_fb_data_type *dpufd, int enable_cmdlist, bool *vsync_time_checked)
{
	uint32_t m;
	int ret;
	dss_rect_t aligned_rect;
	dss_rect_ltrb_t clip_rect;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_overlay_t *pov_req = &(dpufd->ov_req);
	struct dpu_ov_compose_rect ov_compose_rect = { NULL, NULL, &clip_rect, &aligned_rect };
	struct dpu_ov_compose_flag ov_compose_flag = { false, false, true, enable_cmdlist };
	struct ov_module_set_regs_flag ov_module_flag = { enable_cmdlist, false, 0, 0 };

	pov_h_block_infos = (dss_overlay_block_t *)(uintptr_t)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		ret = dpu_module_init(dpufd);
		dpu_check_and_return((ret != 0), -EINVAL, ERR,
			"fb%d, module_init failed! ret = %d\n", dpufd->index, ret);

		dpu_aif_handler(dpufd, pov_req, pov_h_block);
		ret = dpu_ovl_base_config(dpufd, pov_req, pov_h_block, NULL, m);
		dpu_check_and_return((ret != 0), -EINVAL, ERR,
			"fb%d, ovl_base_config failed! ret = %d\n", dpufd->index, ret);

#if defined(CONFIG_VIDEO_IDLE)
		if (m == 0)
			(void)dpufb_video_panel_idle_display_ctrl(dpufd, pov_req->video_idle_status);
#endif

		/* Go through all layers */
		ret = dpu_ov_layers_composer(dpufd, pov_h_block, &ov_compose_rect, &ov_compose_flag);
		if (ret != 0)
			return -EINVAL;

		ret = dpu_mctl_ov_config(dpufd, pov_req, pov_req->ovl_idx, ov_compose_flag.has_base, (m == 0));
		dpu_check_and_return((ret != 0), -EINVAL, ERR,
			"fb%d, mctl_config failed! ret = %d\n", dpufd->index, ret);

		ret = ov_composer_first_block_process(dpufd, pov_req, vsync_time_checked, (m == 0));
		dpu_check_and_return((ret != 0), -EINVAL, ERR,
			"fb%d, first_block_process failed! ret = %d\n", dpufd->index, ret);

		ov_module_flag.is_first_ov_block = (m == 0);
		ret = dpu_ov_module_set_regs(dpufd, pov_req, pov_req->ovl_idx, ov_module_flag);
		dpu_check_and_return((ret != 0), -EINVAL, ERR,
			"fb%d, ov_module_set_regs failed! ret = %d\n", dpufd->index, ret);
	}

	return 0;
}

static int dpu_wait_vactive0_start_flag(struct dpu_fb_data_type *dpufd,
	const void __user *argp, int free_buffer_refcount, dss_overlay_t *pov_req, int *need_skip)
{
	uint32_t timediff;
	timeval_compatible tv1;
	timeval_compatible tv2;
	int ret;
	int wait_1vsync = 0;
	unsigned long flags = 0;

	if (g_debug_ovl_online_composer_timediff & 0x4)
		dpufb_get_timestamp(&tv1);

	ret = dpu_overlay_get_ov_data_from_user(dpufd, pov_req, argp);
	if (ret != 0) {
		DPU_FB_ERR("fb%d, dpu_overlay_get_ov_data_from_user failed! ret=%d\n", dpufd->index, ret);
		*need_skip = 1;
		return -EINVAL;
	}

	if ((is_mipi_video_panel(dpufd)) && (dpufd->online_play_count == 1)) {
		wait_1vsync = 1;
		DPU_FB_INFO("video panel wait a vsync when first frame displayed!\n");
	}

	if (is_mipi_cmd_panel(dpufd) || (free_buffer_refcount < 2) || wait_1vsync) {
		ret = dpu_vactive0_start_config(dpufd, pov_req);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, dpu_vactive0_start_config failed! ret=%d\n", dpufd->index, ret);
			*need_skip = 1;
			return -EINVAL;
		}

		if (is_mipi_cmd_panel(dpufd)) {
			spin_lock_irqsave(&dpufd->buf_sync_ctrl.refresh_lock, flags);
			dpufd->buf_sync_ctrl.refresh++;
			spin_unlock_irqrestore(&dpufd->buf_sync_ctrl.refresh_lock, flags);
		}
	}

	if (g_debug_ovl_online_composer_timediff & 0x4) {
		dpufb_get_timestamp(&tv2);
		timediff = dpufb_timestamp_diff(&tv1, &tv2);
		if (timediff >= g_debug_ovl_online_composer_time_threshold)
			DPU_FB_INFO("ONLINE_VACTIVE_TIMEDIFF is %u us!\n", timediff);
	}

	return 0;
}

static int dpu_ov_async_play_early_check(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	dpu_check_and_return(!argp, -EINVAL, ERR, "argp is NULL\n");

#if defined(CONFIG_DPU_FB_AOD)
	if ((dpufd->index == EXTERNAL_PANEL_IDX) && dpu_aod_get_aod_status()) {
		DPU_FB_INFO("In aod mode!\n");
		return 0;
	}
#endif

	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%d, panel is power off!\n", dpufd->index);
		dpufd->backlight.bl_updated = 0;
		return 0;
	}

	if (g_debug_ldi_underflow) {
		if (g_err_status & (DSS_PDP_LDI_UNDERFLOW | DSS_SDP_LDI_UNDERFLOW)) {
			mdelay(DPU_COMPOSER_HOLD_TIME);
			return 0;
		}
	}

	if (g_debug_ovl_online_composer_return)
		return 0;

	return 1;
}

static int dpu_ov_async_play_early_prepare(struct dpu_fb_data_type *dpufd, void __user *argp,
	struct list_head *lock_list, struct dpu_ov_asynch_play_param *asynch_play_param)
{
	dss_overlay_t *pov_req = &(dpufd->ov_req);
	dss_overlay_t *pov_req_prev = &(dpufd->ov_req_prev);
	int ret;

	ret = dpufb_layerbuf_lock(dpufd, pov_req, lock_list);
	dpu_check_and_return(ret, ret, ERR, "fb%d, dpufb_layerbuf_lock failed! ret=%d\n", dpufd->index, ret);

	dpu_handle_cur_ovl_req(dpufd, pov_req);

	ret = dpu_module_init(dpufd);
	dpu_check_and_return(ret, ret, ERR, "fb%d, dpu_module_init failed! ret=%d\n", dpufd->index, ret);

	dpu_mmbuf_info_get_online(dpufd);

	if (asynch_play_param->enable_cmdlist) {
		dpufd->set_reg = dpu_cmdlist_set_reg;

		dpu_cmdlist_data_get_online(dpufd);

		ret = dpu_cmdlist_get_cmdlist_idxs(pov_req_prev, &asynch_play_param->cmdlist_pre_idxs, NULL);
		dpu_check_and_return(ret, ret, ERR,
			"fb%d, dpu_cmdlist_get_cmdlist_idxs pov_req_prev failed! ret=%d\n", dpufd->index, ret);

		ret = dpu_cmdlist_get_cmdlist_idxs(pov_req, &asynch_play_param->cmdlist_pre_idxs,
			&asynch_play_param->cmdlist_idxs);
		dpu_check_and_return(ret, ret, ERR, "fb%d, dpu_cmdlist_get_cmdlist_idxs pov_req failed! ret=%d\n",
			dpufd->index, ret);

		dpu_cmdlist_add_nop_node(dpufd, asynch_play_param->cmdlist_pre_idxs, 0, 0);
		dpu_cmdlist_add_nop_node(dpufd, asynch_play_param->cmdlist_idxs, 0, 0);
	} else {
		dpufd->set_reg = dpufb_set_reg;
		dpu_mctl_mutex_lock(dpufd, pov_req->ovl_idx);
		asynch_play_param->cmdlist_pre_idxs = ~0;
	}

	return 0;
}

static int dpu_ov_async_play_set_reg(struct dpu_fb_data_type *dpufd,
	struct dpu_ov_asynch_play_param *asynch_play_param)
{
	dss_overlay_t *pov_req = &(dpufd->ov_req);
	dss_overlay_t *pov_req_prev = &(dpufd->ov_req_prev);
	bool vsync_time_checked = false;
	unsigned long flags = 0;
	int ret;

	dpu_prev_module_set_regs(dpufd, pov_req_prev, asynch_play_param->cmdlist_pre_idxs,
		asynch_play_param->enable_cmdlist, NULL);

	ret = dpu_set_ov_composer_info(dpufd, asynch_play_param->enable_cmdlist, &vsync_time_checked);
	if (ret)
		return ret;

	dpu_sec_mctl_set_regs(dpufd);

	if (asynch_play_param->enable_cmdlist) {
		g_online_cmdlist_idxs |= asynch_play_param->cmdlist_idxs;
		/* add taskend for share channel */
		dpu_cmdlist_add_nop_node(dpufd, asynch_play_param->cmdlist_idxs, 0, 0);

		/* remove ch cmdlist */
		dpu_cmdlist_config_stop(dpufd, asynch_play_param->cmdlist_pre_idxs);

		asynch_play_param->cmdlist_idxs |= asynch_play_param->cmdlist_pre_idxs;
		dpu_cmdlist_flush_cache(dpufd, asynch_play_param->cmdlist_idxs);
	}

	if (!g_fake_lcd_flag)
		dpu_unflow_handler(dpufd, pov_req, true);

	if (!vsync_time_checked) {
		ret = dpu_check_vsync_timediff(dpufd, pov_req);
		if (ret)
			return ret;
	}

	if (!is_mipi_cmd_panel(dpufd)) {
		spin_lock_irqsave(&dpufd->buf_sync_ctrl.refresh_lock, flags);
		dpufd->buf_sync_ctrl.refresh++;
		spin_unlock_irqrestore(&dpufd->buf_sync_ctrl.refresh_lock, flags);
	}

	if (is_mipi_cmd_panel(dpufd))
		(void)dpufb_wait_for_mipi_resource_available(dpufd, pov_req);

	dpufb_mask_layer_backlight_config(dpufd, pov_req_prev, pov_req,
		&asynch_play_param->masklayer_maxbacklight_flag);

	if (asynch_play_param->enable_cmdlist) {
		ret = dpu_cmdlist_config_start(dpufd, pov_req->ovl_idx, asynch_play_param->cmdlist_idxs, 0);
		if (ret)
			return ret;
	} else {
		dpu_mctl_mutex_unlock(dpufd, pov_req->ovl_idx);
	}

	if (dpufd->panel_info.dirty_region_updt_support)
		dpu_dirty_region_updt_config(dpufd, pov_req);

	/* cpu config drm layer */
	dpu_drm_layer_online_config(dpufd, pov_req_prev, pov_req);

	dpufb_dc_backlight_config(dpufd);

	return 0;
}

static int dpu_ov_async_play_finish(struct dpu_fb_data_type *dpufd, void __user *argp,
	int *free_buffer_refcount, struct list_head *lock_list, struct dpu_ov_asynch_play_param *asynch_play_param)
{
	uint64_t ov_block_infos_ptr;
	uint64_t ov_hihdr_infos_ptr;
	dss_overlay_t *pov_req = &(dpufd->ov_req);
	timeval_compatible returntv1;

#if defined(CONFIG_POWER_DUBAI_RGB_STATS)
	if ((dpufd->index == PRIMARY_PANEL_IDX) && (dpufd->online_play_count > 1))
		dpufb_rgb_read_register(dpufd);
#endif

	dpu_vactive0_end_isr_handler(dpufd);

	single_frame_update(dpufd);

	dpufb_frame_updated(dpufd);

	ov_block_infos_ptr = pov_req->ov_block_infos_ptr;
	ov_hihdr_infos_ptr = pov_req->ov_hihdr_infos_ptr;
	pov_req->ov_block_infos_ptr = (uint64_t)0;  /* clear ov_block_infos_ptr */
	pov_req->ov_hihdr_infos_ptr = (uint64_t)0;  /* clear ov_hihdr_infos_ptr */

	dpufb_get_timestamp(&returntv1);
	pov_req->online_wait_timediff = dpufb_timestamp_diff(&asynch_play_param->returntv0, &returntv1);

	if (copy_to_user((struct dss_overlay_t __user *)argp, pov_req, sizeof(dss_overlay_t))) {
		DPU_FB_ERR("fb%d, copy_to_user failed.\n", dpufd->index);
		return -EFAULT;
	}

	if (dpufd->ov_req_prev.ov_block_nums > DPU_OV_BLOCK_NUMS) {
		DPU_FB_ERR("ov_block_nums %d is out of range!\n", dpufd->ov_req_prev.ov_block_nums);
		return -EFAULT;
	}

	pov_req->ov_block_infos_ptr = ov_block_infos_ptr;
	pov_req->ov_hihdr_infos_ptr = ov_hihdr_infos_ptr;

	dpufd->enter_idle = false;
	dpufd->emi_protect_check_count = 0;
	dpufb_deactivate_vsync(dpufd);

	dpufb_layerbuf_flush(dpufd, lock_list);

	dpufb_masklayer_backlight_flag_config(dpufd, asynch_play_param->masklayer_maxbacklight_flag);

	dpu_free_reserve_buffer(dpufd, *free_buffer_refcount);
	(*free_buffer_refcount)++;

	dpu_preserve_ov_req(dpufd, pov_req, asynch_play_param->enable_cmdlist, asynch_play_param->cmdlist_idxs);

	return 0;
}

int dpu_ov_asynchronous_play(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	static int free_buffer_refcount;
	dss_overlay_t *pov_req = NULL;
	int ret;
	struct list_head lock_list;
	struct dpu_ov_asynch_play_param asynch_play_param = {0};

	ret = dpu_ov_async_play_early_check(dpufd, argp);
	if (ret != 1)
		return ret;

	pov_req = &(dpufd->ov_req);
	INIT_LIST_HEAD(&lock_list);

	if (g_debug_ovl_online_composer_timediff & 0x2)
		dpufb_get_timestamp(&asynch_play_param.tv0);

	dpufb_get_timestamp(&asynch_play_param.returntv0);

	asynch_play_param.enable_cmdlist = g_enable_ovl_cmdlist_online;
	if ((dpufd->index == EXTERNAL_PANEL_IDX) && dpufd->panel_info.fake_external)
		asynch_play_param.enable_cmdlist = 0;

	dpufb_activate_vsync(dpufd);

	dpufb_snd_cmd_before_frame(dpufd);

	if (dpu_wait_vactive0_start_flag(dpufd, argp, free_buffer_refcount, pov_req, &asynch_play_param.need_skip))
		goto err_return;

	down(&dpufd->blank_sem0);

	if (dpu_ov_async_play_early_prepare(dpufd, argp, &lock_list, &asynch_play_param))
		goto err_return;

	if (dpu_ov_async_play_set_reg(dpufd, &asynch_play_param))
		goto err_return;

	if (dpu_ov_async_play_finish(dpufd, argp, &free_buffer_refcount, &lock_list, &asynch_play_param))
		goto err_return;

	if (g_debug_ovl_online_composer_timediff & 0x2) {
		dpufb_get_timestamp(&asynch_play_param.tv1);
		asynch_play_param.timediff = dpufb_timestamp_diff(&asynch_play_param.tv0, &asynch_play_param.tv1);
		if (asynch_play_param.timediff >= g_debug_ovl_online_composer_time_threshold)
			DPU_FB_INFO("ONLINE_TIMEDIFF is %u us!\n", asynch_play_param.timediff);
	}
	up(&dpufd->blank_sem0);

	return 0;

err_return:
	if (is_mipi_cmd_panel(dpufd))
		dpufd->vactive0_start_flag = 1;

	dpufb_layerbuf_lock_exception(dpufd, &lock_list);
	dpufb_deactivate_vsync(dpufd);
	if (!asynch_play_param.need_skip)
		up(&dpufd->blank_sem0);

	return ret;
}

#pragma GCC diagnostic pop

