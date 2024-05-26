/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <platform_include/basicplatform/linux/hw_cmdline_parse.h>
#include <linux/miscdevice.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <linux/memory.h>
#include <uapi/linux/sched/types.h>

#include "dpu_display_effect.h"
#include "overlay/dpu_overlay_utils.h"
#include "dpu_dpe_utils.h"
#include "chrdev/dpu_chrdev.h"

#define MMAP_DEVICE_NAME "display_sharemem_map"
#define DTS_COMP_SHAREMEM_NAME "hisilicon,dpusharemem"
#define DEV_NAME_SHARE_MEM "display_share_mem"
#define XCC_COEF_LENGTH 12
#define DEBUG_EFFECT_LOG DPU_FB_ERR
#define round(x, y)  (((x) + (y) - 1) / (y))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-larger-than="

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

int g_factory_gamma_enable = 0;
struct mutex g_rgbw_lock;
uint8_t* share_mem_virt = NULL;
phys_addr_t share_mem_phy = 0;
const int JDI_TD4336_RT8555_RGBW_ID = 14;
const int SHARP_TD4336_RT8555_RGBW_ID = 15;
const int LG_NT36772A_RT8555_RGBW_ID = 16;
const int g_bl_timeinterval = 16670000;
const int blc_xcc_buf_count = 18;
const int display_effect_flag_max = 4;

/*lint -e838 -e778 -e845 -e712 -e527 -e30 -e142 -e715 -e655 -e550 +e559*/
static void dpu_effect_module_support(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	struct dss_effect *effect_ctrl = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return ;
	}

	pinfo = &(dpufd->panel_info);
	effect_ctrl = &(dpufd->effect_ctl);

	memset(effect_ctrl, 0, sizeof(struct dss_effect));

	effect_ctrl->acm_support = (pinfo->acm_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACM) != 0));

	effect_ctrl->ace_support = (pinfo->acm_ce_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACE) != 0));

	effect_ctrl->dither_support = (pinfo->dither_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_DITHER) != 0));

	effect_ctrl->lcp_xcc_support = (pinfo->xcc_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_LCP_XCC) != 0));

	effect_ctrl->lcp_gmp_support = (pinfo->gmp_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_LCP_GMP) != 0));

	effect_ctrl->lcp_igm_support = (pinfo->gamma_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_LCP_IGM) != 0));

	effect_ctrl->gamma_support = (pinfo->gamma_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_GAMA) != 0));

	effect_ctrl->hiace_support = (pinfo->hiace_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_HIACE) != 0));

	effect_ctrl->post_hihdr_support = pinfo->post_hihdr_support;

	effect_ctrl->arsr1p_sharp_support = (pinfo->arsr1p_sharpness_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_POST_SCF) != 0));

	effect_ctrl->arsr2p_sharp_support = (pinfo->prefix_sharpness2D_support);

	effect_ctrl->post_xcc_support = (pinfo->post_xcc_support
		&& (DPU_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_POST_XCC) != 0));
	effect_ctrl->dss_ready = true;
}

static int dpufb_effect_module_init_handler(void __user *argp)
{
	int ret;
	struct dpu_fb_data_type *dpufd_primary = NULL;

	dpufd_primary = dpufd_list[PRIMARY_PANEL_IDX];
	if (NULL == dpufd_primary) {
		DPU_FB_ERR("fb0 is not existed, return!\n");
		ret = -ENODEV;
		goto err_out;
	}

	if (argp == NULL) {
		DPU_FB_ERR("argp is null pointer\n");
		return -1;
	}
	ret = copy_to_user(argp, &(dpufd_primary->effect_ctl), sizeof(struct dss_effect));
	DPU_FB_INFO("fb0 effect_ctl:dss_ready=%d, arsr2p=%d, arsr1p=%d, acm=%d,"
		" ace=%d, hiace=%d, hihdr=%d, igm=%d, gmp=%d, xcc=%d, gamma=%d, dither=%d,"
		" post_xcc=%d\n",
		dpufd_primary->effect_ctl.dss_ready,
		dpufd_primary->effect_ctl.arsr2p_sharp_support,
		dpufd_primary->effect_ctl.arsr1p_sharp_support,
		dpufd_primary->effect_ctl.acm_support,
		dpufd_primary->effect_ctl.ace_support,
		dpufd_primary->effect_ctl.hiace_support,
		dpufd_primary->effect_ctl.post_hihdr_support,
		dpufd_primary->effect_ctl.lcp_igm_support,
		dpufd_primary->effect_ctl.lcp_gmp_support,
		dpufd_primary->effect_ctl.lcp_xcc_support,
		dpufd_primary->effect_ctl.gamma_support,
		dpufd_primary->effect_ctl.dither_support,
		dpufd_primary->effect_ctl.post_xcc_support);
	if (ret) {
		DPU_FB_ERR("failed to copy result of ioctl to user space.\n");
		goto err_out;
	}

err_out:
	return ret;
}

static int dpufb_effect_module_deinit_handler(const void __user *argp)
{
	int ret;
	struct dss_effect init_status;

	if (argp == NULL) {
		DPU_FB_ERR("argp is null pointer\n");
		return -1;
	}

	ret = copy_from_user(&init_status, argp, sizeof(struct dss_effect));
	if (ret) {
		DPU_FB_ERR("failed to copy data to kernel space.\n");
		goto err_out;
	}

err_out:
	return ret;
}

static int dpufb_effect_info_get_handler(void __user *argp)
{
	int ret = -EINVAL;
	struct dss_effect_info effect_info;
	struct dpu_fb_data_type *dpufd_primary = NULL;

	if (argp == NULL) {
		DPU_FB_ERR("argp is null pointer\n");
		return -1;
	}

	ret = copy_from_user(&effect_info, argp, sizeof(struct dss_effect_info));
	if (ret) {
		DPU_FB_ERR("failed to copy data from user.\n");
		goto err_out;
	}

	dpufd_primary = dpufd_list[PRIMARY_PANEL_IDX];
	if (dpufd_primary == NULL) {
		DPU_FB_ERR("fb0 is not existed, return!\n");
		ret = -ENODEV;
		goto err_out;
	}

	if (!dpufd_primary->panel_power_on) {
		DPU_FB_ERR("panel is power down, return!\n");
		ret = -EBADRQC;
		goto err_out;
	}

	if (!dpufd_primary->effect_ctl.dss_ready) {
		DPU_FB_ERR("dss is not ready\n");
		ret = -EBADRQC;
		goto err_out;
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_ARSR2P) {
		ret = dpu_effect_arsr2p_info_get(dpufd_primary, effect_info.arsr2p);
		if (ret) {
			DPU_FB_ERR("failed to get arsr2p info\n");
			goto err_out;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_ARSR1P) {
		ret = dpu_effect_arsr1p_info_get(dpufd_primary, effect_info.arsr1p);
		if (ret) {
			DPU_FB_ERR("failed to get arsr1p info\n");
			goto err_out;
		}
	}

	if (effect_info.modules & (DSS_EFFECT_MODULE_LCP_GMP | DSS_EFFECT_MODULE_LCP_IGM |
		DSS_EFFECT_MODULE_LCP_XCC | DSS_EFFECT_MODULE_POST_XCC)) {
		ret = dpu_effect_lcp_info_get(dpufd_primary, &effect_info.lcp);
		if (ret) {
			DPU_FB_ERR("failed to get lcp info\n");
			goto err_out;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_GAMMA) {
		ret = dpu_effect_gamma_info_get(dpufd_primary, &effect_info.gamma);
		if (ret) {
			DPU_FB_ERR("failed to get gamma info\n");
			goto err_out;
		}
	}

	ret = copy_to_user(argp, &effect_info, sizeof(struct dss_effect_info));
	if (ret) {
		DPU_FB_ERR("failed to copy result of ioctl to user space.\n");
		goto err_out;
	}

err_out:
	return ret;;
}

static int dpufb_effect_info_set_handler(const void __user *argp)
{
	int ret;
	struct dss_effect_info effect_info;
	struct dpu_fb_data_type *dpufd_primary = NULL;

	if (argp == NULL) {
		DPU_FB_ERR("argp is null pointer\n");
		return -1;
	}

	ret = copy_from_user(&effect_info, argp, sizeof(struct dss_effect_info));
	if (ret) {
		DPU_FB_ERR("failed to copy data to kernel space.\n");
		goto err_out;
	}

	if ((effect_info.disp_panel_id >= DISP_PANEL_NUM) ||
		(effect_info.disp_panel_id < 0)) {
		DPU_FB_ERR("disp_panel_id = %d is overflow.\n", effect_info.disp_panel_id);
		goto err_out;
	}
	dpufd_primary = dpufd_list[PRIMARY_PANEL_IDX];
	if (dpufd_primary == NULL) {
		DPU_FB_ERR("dpufd_primary is null or unexpected input fb\n");
		ret = -EBADRQC;
		goto err_out;
	}

	mutex_lock(&dpufd_primary->effect_lock);

	dpufd_primary->effect_info[effect_info.disp_panel_id].modules = effect_info.modules;

	if (effect_info.modules & DSS_EFFECT_MODULE_ARSR2P) {
		ret = dpu_effect_save_arsr2p_info(dpufd_primary, &effect_info);
		if (ret) {
			DPU_FB_ERR("failed to set arsr2p info\n");
			goto err_out_spin;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_ARSR1P) {
		ret = dpu_effect_save_arsr1p_info(dpufd_primary, &effect_info);
		if (ret) {
			DPU_FB_ERR("failed to set arsr1p info\n");
			goto err_out_spin;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_ACM) {
		ret = dpu_effect_save_acm_info(dpufd_primary, &effect_info);
		if (ret) {
			DPU_FB_ERR("failed to set acm info\n");
			goto err_out_spin;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_POST_XCC) {
		ret = dpu_effect_save_post_xcc_info(dpufd_primary, &effect_info);
		if (ret) {
			DPU_FB_ERR("failed to set post_xcc info\n");
			goto err_out_spin;
		}
	}

	if ((effect_info.modules & DSS_EFFECT_MODULE_LCP_GMP) ||
		(effect_info.modules & DSS_EFFECT_MODULE_LCP_IGM) ||
		(effect_info.modules & DSS_EFFECT_MODULE_LCP_XCC) ||
		(effect_info.modules & DSS_EFFECT_MODULE_GAMMA)) {

		ret = dpu_effect_dpp_buf_info_set(dpufd_primary, &effect_info, false);

		if (ret) {
			DPU_FB_ERR("failed to set dpp info\n");
			goto err_out_spin;
		}
	}

	/* the display effect is not allowed to set reg when the partical update */
	if (dpufd_primary->display_effect_flag < 5)
		dpufd_primary->display_effect_flag = 4;

err_out_spin:
	mutex_unlock(&dpufd_primary->effect_lock);

	DPU_FB_DEBUG("fb%d, modules = 0x%x, -.\n", dpufd_primary->index, effect_info.modules);

err_out:
	return ret;;
}

static int dpu_display_effect_ioctl_handler(struct dpu_fb_data_type *dpufd, unsigned int cmd, void __user *argp)
{
	int ret = -EINVAL;

	if (argp == NULL || dpufd == NULL) {
		DPU_FB_ERR("[effect]NULL pointer of argp or dpufd\n");
		goto err_out;
	}

	DPU_FB_DEBUG("[effect]fb%d, +\n", dpufd->index);

	switch (cmd) {
	case DPUFB_EFFECT_MODULE_INIT:
		ret = dpufb_effect_module_init_handler(argp);
		break;
	case DPUFB_EFFECT_MODULE_DEINIT:
		ret = dpufb_effect_module_deinit_handler(argp);
		break;
	case DPUFB_EFFECT_INFO_GET:
		ret = dpufb_effect_info_get_handler(argp);
		break;
	case DPUFB_EFFECT_INFO_SET:
		ret = dpufb_effect_info_set_handler(argp);
		break;
	default:
		DPU_FB_ERR("[effect]unknown cmd id\n");
		ret = -ENOSYS;
		break;
	};

	DPU_FB_DEBUG("[effect]fb%d, -\n", dpufd->index);

err_out:
	return ret;
}

static int effect_thread_fn(void *data)
{
	int ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	DPU_FB_INFO("[effect] thread + \n");

	dpufd = (struct dpu_fb_data_type *)data;
	if (!dpufd) {
		DPU_FB_ERR("dpufd is null pointer\n");
		return -1;
	}

	while(kthread_should_stop() == 0) {
		ret = wait_event_interruptible(dpufd->effect_wq, dpufd->effect_update); //lint !e578
		if (ret) {
			DPU_FB_ERR("wait_event_interruptible wrong \n");
			continue;
		}
		dpufd->effect_update = false;

		dpufb_effect_dpp_config(dpufd, false);
	}

	DPU_FB_INFO("[effect] thread - \n");

	return 0;
}

static int dpu_display_effect_thread_init(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->effect_thread)
		return 0;

	struct sched_param param = { .sched_priority = 1};
	dpufd->effect_thread = kthread_create(effect_thread_fn, dpufd, "effect_thread");

	if (IS_ERR(dpufd->effect_thread)) {
		DPU_FB_ERR("Unable to start kernel effect_thread.\n");
		dpufd->effect_thread = NULL;
		return -EINVAL;
	}

	init_waitqueue_head(&dpufd->effect_wq);
	sched_setscheduler_nocheck(dpufd->effect_thread, SCHED_FIFO, &param);
	wake_up_process(dpufd->effect_thread);
	return 0;
}

void dpu_display_effect_init(struct dpu_fb_data_type *dpufd)
{
	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return ;
	}

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpufd->panel_info.p3_support)
			dpu_display_effect_thread_init(dpufd);

		dpufd->display_effect_ioctl_handler = NULL;
		memset(dpufd->effect_updated_flag, 0, sizeof(dpufd->effect_updated_flag));
		mutex_init(&dpufd->effect_lock);

		dpu_effect_module_support(dpufd);
	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		dpufd->display_effect_ioctl_handler = dpu_display_effect_ioctl_handler;
	} else {
		dpufd->display_effect_ioctl_handler = NULL;
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);
}

void dpu_effect_set_reg(struct dpu_fb_data_type *dpufd)
{
	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return ;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;

	if (mutex_trylock(&dpufd->effect_lock) != 0) {
		if (dpufd->panel_info.smart_color_mode_support == 0) {
			dpu_effect_acm_set_reg(dpufd);
			dpu_effect_lcp_set_reg(dpufd);
		}

		dpu_effect_post_xcc_set_reg(dpufd);

		dpu_effect_gamma_set_reg(dpufd);
		mutex_unlock(&dpufd->effect_lock);
	}

	return;
}

int dpufb_runmode_check(display_engine_param_t* de_param)
{
	if(de_param == NULL){
		DPU_FB_ERR("de_param is null pointer\n");
		return -1;
	}
	if ((de_param->modules & DISPLAY_ENGINE_DDIC_RGBW) ||
		(de_param->modules & DISPLAY_ENGINE_HBM) ||
		(de_param->modules & DISPLAY_ENGINE_BLC) ||
		(de_param->modules & DISPLAY_ENGINE_DDIC_LOCAL_HBM)) {
		DPU_FB_DEBUG("dpufb_runmode_check ok\n");
		return 0;
	}

	return 0;
}

uint32_t get_fixed_point_offset(uint32_t half_block_size)
{
	uint32_t num = 2;
	uint32_t len = 2;

	while (len < half_block_size) {
		num++;
		len <<= 1;
	}
	return num;
}


int dpufb_hiace_roi_info_init(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req)
{
	struct dss_rect *hiace_roi = NULL;
	struct hiace_roi_info *hiace_roi_param = NULL;

	if (pov_req == NULL) {
		DPU_FB_ERR("pov_req is NULL!\n");
		return -1;
	}
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -1;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_DEBUG("[effect] fb%d, not support!", dpufd->index);
		return -EINVAL;
	}

	pov_req = &(dpufd->ov_req);
	hiace_roi_param = &(dpufd->auto_hiace_roi_info);

	if (dpufd->panel_info.hiace_support) {
		hiace_roi = &(pov_req->hiace_roi_rect);
		if (pov_req->hiace_roi_support) {
			if (pov_req->hiace_roi_enable) {
				hiace_roi_param->roi_top = hiace_roi->y;
				hiace_roi_param->roi_left = hiace_roi->x;
				hiace_roi_param->roi_bot = hiace_roi->y + hiace_roi->h;
				hiace_roi_param->roi_right = hiace_roi->x + hiace_roi->w;
			} else {
				hiace_roi_param->roi_top = 0;
				hiace_roi_param->roi_left = 0;
				hiace_roi_param->roi_bot = 0;
				hiace_roi_param->roi_right = 0;
			}
			hiace_roi_param->roi_enable = pov_req->hiace_roi_enable;
		}
	} else {
		hiace_roi_param->roi_top =
			((dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.roi_start_point >> 16) & 0x1fff);
		hiace_roi_param->roi_left =
			dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.roi_start_point & 0x1fff;
		hiace_roi_param->roi_bot = hiace_roi_param->roi_top +
			((dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.roi_width_high >> 16) & 0x1fff);
		hiace_roi_param->roi_right = hiace_roi_param->roi_left +
			(dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.roi_width_high & 0x1fff);
		hiace_roi_param->roi_enable = dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.roi_mode_ctrl;
	}
	return 0;
}

void dpufb_hiace_roi_reg_set(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req)
{
	char __iomem *hiace_base = NULL;
	dss_rect_t *rect = NULL;
	uint32_t start_point, width_high;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return;
	}
	if (pov_req == NULL) {
		DPU_FB_ERR("pov_req is NULL!\n");
		return;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_DEBUG("[effect] fb%d, not support!", dpufd->index);
		return;
	}

	hiace_base = dpufd->dss_base + DSS_HI_ACE_OFFSET;

	if (dpufd->fb_shutdown == true || dpufd->panel_power_on == false) {
		DPU_FB_ERR("[effect] fb_shutdown or panel power down");
		return;
	}

	if (pov_req->hiace_roi_support) {
		rect = &(pov_req->hiace_roi_rect);
		start_point = ((uint32_t)rect->y << 16) | ((uint32_t)rect->x +
			dpufd->panel_info.dummy_pixel_x_left);
		width_high = ((uint32_t)rect->h << 16) | ((uint32_t)rect->w);
		set_reg(hiace_base + DPE_ROI_START_POINT, start_point, 32, 0);
		set_reg(hiace_base + DPE_ROI_WIDTH_HIGH, width_high, 32, 0);
		set_reg(hiace_base + DPE_ROI_MODE_CTRL,
			pov_req->hiace_roi_enable, 32, 0);
		DPU_FB_DEBUG("start_point:0x%x, width_high:0x%x, roi_enable=%d\n",
			start_point, width_high, pov_req->hiace_roi_enable);
	} else {
		set_reg(hiace_base + DPE_ROI_START_POINT,
			dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.roi_start_point, 32, 0);
		set_reg(hiace_base + DPE_ROI_WIDTH_HIGH,
			dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.roi_width_high, 32, 0);
		set_reg(hiace_base + DPE_ROI_MODE_CTRL,
			dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.roi_mode_ctrl, 32, 0);
	}
}

void hiace_size_config(struct dpu_fb_data_type *dpufd, uint32_t width, uint32_t height)
{
	char __iomem *hiace_base = NULL;
	uint32_t half_block_w;
	uint32_t half_block_h;
	uint32_t fixbit_x;
	uint32_t fixbit_y;
	uint32_t reciprocal_x;
	uint32_t reciprocal_y;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	if (dpufd->panel_info.hiace_support == 0) {
		if (g_debug_effect & DEBUG_EFFECT_ENTRY)
			DEBUG_EFFECT_LOG("[effect] HIACE is not supported!\n");
		return;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("[effect] fb%d, not support!", dpufd->index);
		return;
	}

	hiace_base = dpufd->dss_base + DSS_HI_ACE_OFFSET;

	/* parameters */
	set_reg(hiace_base + DPE_IMAGE_INFO, (height << 16) | width, 32, 0);
	dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.image_info = (height << 16) | width;

	half_block_w = (width / (XBLOCKNUM * 2)) & 0x1ff;
	half_block_h = round(height, (YBLOCKNUM * 2)) & 0x1ff;
	set_reg(hiace_base + DPE_HALF_BLOCK_INFO,
		(half_block_h << 16) | half_block_w, 32, 0);

	dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.half_block_info = (half_block_h << 16) | half_block_w;

	fixbit_x = get_fixed_point_offset(half_block_w) & 0x1f;
	fixbit_y = get_fixed_point_offset(half_block_h) & 0x1f;
	reciprocal_x = ((1U << (fixbit_x + 8)) / (2 * MAX(half_block_w, 1))) & 0x3ff;
	reciprocal_y = ((1U << (fixbit_y + 8)) / (2 * MAX(half_block_h, 1))) & 0x3ff;
	set_reg(hiace_base + DPE_XYWEIGHT, (fixbit_y << 26) | (reciprocal_y << 16) |
		(fixbit_x << 10) | reciprocal_x, 32, 0);

	dpufd->effect_info[dpufd->panel_info.disp_panel_id].hiace.xyweight = (fixbit_y << 26) | (reciprocal_y << 16) |
		(fixbit_x << 10) | reciprocal_x;

	if (g_debug_effect & DEBUG_EFFECT_ENTRY)
		DEBUG_EFFECT_LOG("[effect] half_block_w:%d,half_block_h:%d,fixbit_x:%d,"
			"fixbit_y:%d, reciprocal_x:%d, reciprocal_y:%d\n",
			half_block_w, half_block_h, fixbit_x,
			fixbit_y, reciprocal_x, reciprocal_y);
}


static int share_mmap_map(struct file *filp, struct vm_area_struct *vma)
{
	void_unused(filp);
	unsigned long start = 0;
	unsigned long size = 0;

	if (vma == NULL) {
		DPU_FB_ERR("[shmmap] vma is null!\n");
		return -1;
	}

	if (share_mem_virt == NULL || share_mem_phy == 0) {
		DPU_FB_ERR("[shmmap] share memory is not alloced!\n");
		return 0;
	}

	start = (unsigned long)vma->vm_start;
	size = (unsigned long)(vma->vm_end - vma->vm_start);
	if (size > SHARE_MEMORY_SIZE)
		return -1;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	DPU_FB_INFO("[shmmap] my_map size = 0x%lx\n", size);
	if(remap_pfn_range(vma, start, __phys_to_pfn(share_mem_phy), size, vma->vm_page_prot)) {
		DPU_FB_ERR("[shmmap] remap_pfn_range error!\n");
		return -1;
	}
	return 0;
}

static struct file_operations mmap_dev_fops = {
	.owner = THIS_MODULE,
	.mmap = share_mmap_map,
};

static struct miscdevice misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MMAP_DEVICE_NAME,
	.fops = &mmap_dev_fops,
};

static int __init mmap_dev_init(void)
{
	int ret = 0;

	DPU_FB_INFO("[shmmap] dev_init \n");
	ret = misc_register(&misc_device);
	if (ret) {
		DPU_FB_ERR("[shmmap] misc_register ret = %d \n", ret);
		return ret;
	}

	return ret;
}

static int dpu_share_mem_probe(struct platform_device *pdev)
{
	DPU_FB_DEBUG("[shmmap] dpu_share_mem_probe \n");

	if (NULL == pdev) {
		DPU_FB_ERR("[shmmap] pdev is NULL");
		return -EINVAL;
	}

	share_mem_virt = (void *)dma_alloc_coherent(&pdev->dev, SHARE_MEMORY_SIZE, &share_mem_phy, GFP_KERNEL);
	if (share_mem_virt == NULL || share_mem_phy == 0) {
		DPU_FB_ERR("[shmmap] dma_alloc_coherent error! ");
		return -EINVAL;
	}

	mmap_dev_init();

	return 0;
}

static int dpu_share_mem_remove(struct platform_device *pdev)
{
	void_unused(pdev);
	return 0;
}

static const struct of_device_id dpu_share_match_table[] = {
	{
		.compatible = DTS_COMP_SHAREMEM_NAME,
		.data = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, dpu_share_match_table);

static struct platform_driver this_driver = {
	.probe = dpu_share_mem_probe,
	.remove = dpu_share_mem_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = DEV_NAME_SHARE_MEM,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dpu_share_match_table),
	},
};

static int __init dpu_share_mem_init(void)
{
	int ret = 0;
	DPU_FB_INFO("[shmmap] dpu_share_mem_init\n");
	ret = platform_driver_register(&this_driver);
	if (ret) {
		DPU_FB_ERR("[shmmap] platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(dpu_share_mem_init);

#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
/*lint +e838 +e778 +e845 +e712 +e527 +e30 +e142 +e715 +e655 +e550 +e559*/
