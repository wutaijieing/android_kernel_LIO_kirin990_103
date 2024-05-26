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

#include "dpu_display_effect.h"
#include "dpu_fb.h"
#include "chrdev/dpu_chrdev.h"
#include "global_ddr_map.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
// lint -e747, -e838, -e774
typedef struct time_interval {
	long start; /* microsecond */
	long stop;
} time_interval_t;

typedef struct delay_record {
	const char *name;
	long max;
	long min;
	long sum;
	int count;
} delay_record_t;

#define DEBUG_EFFECT_LOG DPU_FB_ERR

static bool g_is_effect_init;
static struct mutex g_ce_service_lock;
static ce_service_t g_hiace_service;

static bool g_is_effect_lock_init;
static spinlock_t g_gmp_effect_lock;
static spinlock_t g_igm_effect_lock;
static spinlock_t g_xcc_effect_lock;
static spinlock_t g_gama_effect_lock;
static spinlock_t g_post_xcc_effect_lock;
static spinlock_t g_hiace_table_lock;
static spinlock_t g_roi_lock;
static spinlock_t g_dpproi_effect_lock;

struct mutex g_dpp_buf_lock_arr[DISP_PANEL_NUM][DPP_BUF_MAX_COUNT - 1];
struct mutex g_dpp_ch0_lock;
struct mutex g_dpp_ch1_lock;

extern struct mutex g_rgbw_lock;
uint8_t g_dpp_cmdlist_state = 0;

uint32_t g_post_xcc_table_temp[12] = {
	0x0, 0x8000, 0x0, 0x0, 0x0, 0x0,
	0x8000, 0x0, 0x0, 0x0, 0x0, 0x8000
};


uint32_t g_enable_effect = ENABLE_EFFECT_HIACE | ENABLE_EFFECT_BL;
uint32_t g_debug_effect;
/*
* dpp chanel default select register
* 0: none roi region use CH0
* 1: none roi region use CH1
*/
uint32_t g_dyn_sw_default;

#define SCREEN_OFF_BLC_DELTA (-10000)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic pop


static int dpu_effect_copy_to_user(uint32_t *table_dst,
	uint32_t *table_src, uint32_t table_length)
{
	unsigned long table_size;

	if (table_dst == NULL) {
		DPU_FB_ERR("[effect]table_dst is NULL!\n");
		return -EINVAL;
	}

	if (table_src == NULL) {
		DPU_FB_ERR("[effect]table_src is NULL!\n");
		return -EINVAL;
	}

	if (table_length == 0) {
		DPU_FB_ERR("[effect]table_length is 0!\n");
		return -EINVAL;
	}

	table_size = (unsigned long)table_length * BYTES_PER_TABLE_ELEMENT;

	if (copy_to_user(table_dst, table_src, table_size)) {
		DPU_FB_ERR("[effect]failed to copy table to user.\n");
		return -EINVAL;
	}

	return 0;
}

void dpu_effect_init(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	hiace_alg_parameter_t *param = NULL;
	int i;
	int j;

	if (!g_is_effect_lock_init) {
		spin_lock_init(&g_gmp_effect_lock);
		spin_lock_init(&g_igm_effect_lock);
		spin_lock_init(&g_xcc_effect_lock);
		spin_lock_init(&g_gama_effect_lock);
		spin_lock_init(&g_post_xcc_effect_lock);
		spin_lock_init(&g_hiace_table_lock);
		spin_lock_init(&g_dpproi_effect_lock);
		spin_lock_init(&g_roi_lock);

		for (i = 0; i < DISP_PANEL_NUM; ++i) {
			for (j = 0; j < DPP_BUF_MAX_COUNT - 1; ++j)
				mutex_init(&g_dpp_buf_lock_arr[i][j]);
		}

		mutex_init(&g_dpp_ch0_lock);
		mutex_init(&g_dpp_ch1_lock);
		mutex_init(&g_rgbw_lock);
		g_is_effect_lock_init = true;
	}

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo->hiace_support == 0) {
		if (g_debug_effect & DEBUG_EFFECT_ENTRY)
			DEBUG_EFFECT_LOG("[effect] HIACE is not supported!\n");

		return;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("[effect] fb%d, not support!", dpufd->index);
		return;
	}

	for (i = 0; (!g_is_effect_init) && (i < DISP_PANEL_NUM); ++i) {
		memset(&dpufd->hiace_info[i], 0, sizeof(dss_ce_info_t));
		dpufd->hiace_info[i].algorithm_result = 1;
		mutex_init(&(dpufd->hiace_info[i].hist_lock));
		mutex_init(&(dpufd->hiace_info[i].lut_lock));
	}

	param = &pinfo->hiace_param;

	if (!g_is_effect_init) {
		mutex_init(&g_ce_service_lock);
		mutex_init(&(dpufd->al_ctrl.ctrl_lock));
		mutex_init(&(dpufd->ce_ctrl.ctrl_lock));
		mutex_init(&(dpufd->bl_ctrl.ctrl_lock));
		mutex_init(&(dpufd->bl_enable_ctrl.ctrl_lock));
		mutex_init(&(dpufd->metadata_ctrl.ctrl_lock));
		dpufd->bl_enable_ctrl.ctrl_bl_enable = 1;

		param->iWidth = (int)pinfo->xres;
		param->iHeight = (int)pinfo->yres;
		param->iMode = 0;
		param->bitWidth = 10;
		param->iMinBackLight = (int)dpufd->panel_info.bl_min;
		param->iMaxBackLight = (int)dpufd->panel_info.bl_max;
		param->iAmbientLight = -1;

		memset(&g_hiace_service, 0, sizeof(g_hiace_service));
		init_waitqueue_head(&g_hiace_service.wq_hist);

		if (g_debug_effect & DEBUG_EFFECT_ENTRY)
			DEBUG_EFFECT_LOG("[effect] width:%d, height:%d, minbl:%d, maxbl:%d\n",
				param->iWidth, param->iHeight,
				param->iMinBackLight, param->iMaxBackLight);

		g_is_effect_init = true;
	} else {
		if (g_debug_effect & DEBUG_EFFECT_ENTRY)
			DEBUG_EFFECT_LOG("[effect] bypass\n");
	}
}

void dpu_effect_deinit(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	int i;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo->hiace_support == 0) {
		if (g_debug_effect & DEBUG_EFFECT_ENTRY)
			DEBUG_EFFECT_LOG("[effect] HIACE is not supported!\n");
		return;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("[effect] fb%d, not support!", dpufd->index);
		return;
	}

	if (g_is_effect_lock_init) {
		g_is_effect_lock_init = false;
		mutex_destroy(&g_rgbw_lock);
	}
	/* avoid  using  mutex_lock() but hist_lock was destoried
	 * by mutex_destory in  dpu_effect_deinit
	 */
	down(&dpufd->hiace_hist_lock_sem);
	if (g_is_effect_init) {
		g_is_effect_init = false;

		for (i = 0; i < DISP_PANEL_NUM; ++i) {
			mutex_destroy(&(dpufd->hiace_info[i].hist_lock));
			mutex_destroy(&(dpufd->hiace_info[i].lut_lock));
		}

		mutex_destroy(&(dpufd->al_ctrl.ctrl_lock));
		mutex_destroy(&(dpufd->ce_ctrl.ctrl_lock));
		mutex_destroy(&(dpufd->bl_ctrl.ctrl_lock));
		mutex_destroy(&(dpufd->bl_enable_ctrl.ctrl_lock));
		mutex_destroy(&(dpufd->metadata_ctrl.ctrl_lock));

		mutex_destroy(&g_ce_service_lock);
	} else {
		if (g_debug_effect & DEBUG_EFFECT_ENTRY)
			DEBUG_EFFECT_LOG("[effect] bypass\n");
	}
	up(&dpufd->hiace_hist_lock_sem);
}

/*
 * GM IGM
 */

/* lint -e571, -e573, -e737, -e732, -e850, -e730, -e713, -e574, -e732, -e845, -e570,
-e774 -e568 -e587 -e685 */
int dpu_effect_arsr2p_info_get(struct dpu_fb_data_type *dpufd,
	struct arsr2p_info *arsr2p)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (arsr2p == NULL) {
		DPU_FB_ERR("fb%d, arsr2p is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->effect_ctl.arsr2p_sharp_support) {
		DPU_FB_INFO("fb%d, arsr2p is not supported!\n", dpufd->index);
		return 0;
	}

	memcpy(&arsr2p[0],
		&(dpufd->dss_module_default.arsr2p[DSS_RCHN_V0].arsr2p_effect),
		sizeof(struct arsr2p_info));
	memcpy(&arsr2p[1],
		&(dpufd->dss_module_default.arsr2p[DSS_RCHN_V0].arsr2p_effect_scale_up),
		sizeof(struct arsr2p_info));
	memcpy(&arsr2p[2],
		&(dpufd->dss_module_default.arsr2p[DSS_RCHN_V0].arsr2p_effect_scale_down),
		sizeof(struct arsr2p_info));
	arsr2p[0].sharp_enable = dpufd->panel_info.prefix_sharpness2D_support;
	arsr2p[1].sharp_enable = dpufd->panel_info.prefix_sharpness2D_support;
	arsr2p[2].sharp_enable = dpufd->panel_info.prefix_sharpness2D_support;

	return 0;
}

static void effect_arsr_post_init_param(struct arsr1p_info *arsr_para)
{
	if (arsr_para == NULL)
		return;

	arsr_para->skin_thres_y = (600 << 20) | (332 << 10) | 300; /* 0x2585312C */
	arsr_para->skin_thres_u = (452 << 20) | (40 << 10) | 20;   /* 0x1C40A014 */
	arsr_para->skin_thres_v = (580 << 20) | (48 << 10) | 24;   /* 0x2440C018 */
	arsr_para->skin_cfg0 = (12 << 13) | 512;                   /* 0x00018200 */
	arsr_para->skin_cfg1 = 819;                                /* 0x00000333 */
	arsr_para->skin_cfg2 = 682;                                /* 0x000002AA */
	arsr_para->shoot_cfg1 = (20 << 16) | 341;                  /* 0x00140155 */
	arsr_para->shoot_cfg2 = (-80 & 0x7ff) | (16 << 16);        /* 0x001007B0 */
	arsr_para->shoot_cfg3 = 20;                                /* 0x00000014 */
	arsr_para->sharp_cfg3 = HIGH16(0xA0) | LOW16(0x60);        /* 0x00A00060 */
	arsr_para->sharp_cfg4 = HIGH16(0x60) | LOW16(0x20);        /* 0x00600020 */
	arsr_para->sharp_cfg5 = 0;
	arsr_para->sharp_cfg6 = HIGH16(0x4) | LOW16(0x8);          /* 0x00040008 */
	arsr_para->sharp_cfg7 = (6 << 8) | 10;                     /* 0x0000060A */
	arsr_para->sharp_cfg8 = HIGH16(0xA0) | LOW16(0x10);        /* 0x00A00010 */

	arsr_para->sharp_level = 0x0020002;
	arsr_para->sharp_gain_low = 0x3C0078;
	arsr_para->sharp_gain_mid = 0x6400C8;
	arsr_para->sharp_gain_high = 0x5000A0;
	arsr_para->sharp_gainctrl_sloph_mf = 0x280;
	arsr_para->sharp_gainctrl_slopl_mf = 0x1400;
	arsr_para->sharp_gainctrl_sloph_hf = 0x140;
	arsr_para->sharp_gainctrl_slopl_hf = 0xA00;
	arsr_para->sharp_mf_lmt = 0x40;
	arsr_para->sharp_gain_mf = 0x12C012C;
	arsr_para->sharp_mf_b = 0;
	arsr_para->sharp_hf_lmt = 0x80;
	arsr_para->sharp_gain_hf = 0x104012C;
	arsr_para->sharp_hf_b = 0x1400;
	arsr_para->sharp_lf_ctrl = 0x100010;
	arsr_para->sharp_lf_var = 0x1800080;
	arsr_para->sharp_lf_ctrl_slop = 0;
	arsr_para->sharp_hf_select = 0;
	arsr_para->sharp_cfg2_h = 0x10000C0;
	arsr_para->sharp_cfg2_l = 0x200010;
	arsr_para->texture_analysis = 0x500040;
	arsr_para->intplshootctrl = 0x8;
}

int dpu_effect_arsr1p_info_get(struct dpu_fb_data_type *dpufd,
	struct arsr1p_info *arsr1p)
{
	struct arsr1p_info *arsr1p_param = NULL;
	struct arsr1p_info *arsr1p_rog_fhd = NULL;
	struct arsr1p_info *arsr1p_rog_hd = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (arsr1p == NULL) {
		DPU_FB_ERR("fb%d, arsr1p is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->effect_ctl.arsr1p_sharp_support) {
		DPU_FB_INFO("fb%d, arsr1p lcd is not supported!\n", dpufd->index);
		return 0;
	}

	/* arsr1p normal init */
	arsr1p_param = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].arsr1p[0]);
	arsr1p_param->sharp_enable = dpufd->panel_info.arsr1p_sharpness_support;
	arsr1p_param->skin_enable = arsr1p_param->sharp_enable;
	arsr1p_param->shoot_enable = arsr1p_param->skin_enable;
	effect_arsr_post_init_param(arsr1p_param);
	memcpy(&(arsr1p[0]), arsr1p_param, sizeof(struct arsr1p_info));

	/* arsr1p rog fhd init */
	arsr1p_rog_fhd = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].arsr1p[1]);
	arsr1p_rog_fhd->sharp_enable = dpufd->panel_info.arsr1p_sharpness_support;
	arsr1p_rog_fhd->skin_enable = arsr1p_rog_fhd->sharp_enable;
	arsr1p_rog_fhd->shoot_enable = arsr1p_rog_fhd->skin_enable;
	if (!(dpufd->effect_info[dpufd->panel_info.disp_panel_id].arsr1p_rog_initialized & ARSR1P_ROG_FHD_FLAG)) {
		effect_arsr_post_init_param(arsr1p_rog_fhd);
		dpufd->effect_info[dpufd->panel_info.disp_panel_id].arsr1p_rog_initialized |= ARSR1P_ROG_FHD_FLAG;
	}
	memcpy(&(arsr1p[1]), arsr1p_rog_fhd, sizeof(struct arsr1p_info));

	/* arsr1p rog hd init */
	arsr1p_rog_hd = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].arsr1p[2]);
	arsr1p_rog_hd->sharp_enable = dpufd->panel_info.arsr1p_sharpness_support;
	arsr1p_rog_hd->skin_enable = arsr1p_rog_hd->sharp_enable;
	arsr1p_rog_hd->shoot_enable = arsr1p_rog_hd->skin_enable;
	if (!(dpufd->effect_info[dpufd->panel_info.disp_panel_id].arsr1p_rog_initialized & ARSR1P_ROG_HD_FLAG)) {
		effect_arsr_post_init_param(arsr1p_rog_hd);
		dpufd->effect_info[dpufd->panel_info.disp_panel_id].arsr1p_rog_initialized |= ARSR1P_ROG_HD_FLAG;
	}
	memcpy(&(arsr1p[2]), arsr1p_rog_hd, sizeof(struct arsr1p_info));

	return 0;
}

int dpu_effect_lcp_info_get(struct dpu_fb_data_type *dpufd,
	struct lcp_info *lcp)
{
	int ret = 0;
	struct dpu_panel_info *pinfo = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (lcp == NULL) {
		DPU_FB_ERR("fb%d, lcp is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	if (dpufd->effect_ctl.lcp_gmp_support &&
		(pinfo->gmp_lut_table_len == LCP_GMP_LUT_LENGTH)) {
		ret = dpu_effect_copy_to_user(lcp->gmp_table_low32,
			pinfo->gmp_lut_table_low32bit, LCP_GMP_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gmp_table_low32 to user!\n",
				dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(lcp->gmp_table_high4,
			pinfo->gmp_lut_table_high4bit, LCP_GMP_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gmp_table_high4 to user!\n",
				dpufd->index);
			goto err_ret;
		}
	}

	if (dpufd->effect_ctl.lcp_xcc_support &&
		(pinfo->xcc_table_len == LCP_XCC_LUT_LENGTH)) {
		ret = dpu_effect_copy_to_user(lcp->xcc0_table,
			pinfo->xcc_table, LCP_XCC_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy xcc0_table to user!\n",
				dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(lcp->xcc1_table,
			pinfo->xcc_table, LCP_XCC_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy xcc1_table to user!\n",
				dpufd->index);
			goto err_ret;
		}
	}

	if (dpufd->effect_ctl.lcp_igm_support &&
		(pinfo->igm_lut_table_len == LCP_IGM_LUT_LENGTH)) {
		ret = dpu_effect_copy_to_user(lcp->igm_r_table,
				pinfo->igm_lut_table_R, LCP_IGM_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy igm_r_table to user!\n",
				dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(lcp->igm_g_table,
			pinfo->igm_lut_table_G, LCP_IGM_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy igm_g_table to user!\n",
				dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(lcp->igm_b_table,
			pinfo->igm_lut_table_B, LCP_IGM_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy igm_b_table to user!\n",
				dpufd->index);
			goto err_ret;
		}
	}

	if (dpufd->effect_ctl.post_xcc_support &&
		(pinfo->post_xcc_table_len == POST_XCC_LUT_LENGTH)) {
		ret = dpu_effect_copy_to_user(lcp->post_xcc_table,
			pinfo->post_xcc_table, POST_XCC_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy post_xcc_table to user!\n",
				dpufd->index);
			goto err_ret;
		}
	}

err_ret:
	return ret;
}

int dpu_effect_gamma_info_get(struct dpu_fb_data_type *dpufd,
	struct gamma_info *gamma)
{
	struct dpu_panel_info *pinfo = NULL;
	int ret = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (gamma == NULL) {
		DPU_FB_ERR("fb%d, gamma is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->effect_ctl.gamma_support) {
		DPU_FB_INFO("fb%d, gamma is not supported!\n", dpufd->index);
		return 0;
	}

	pinfo = &(dpufd->panel_info);

	if (pinfo->gamma_lut_table_len == GAMA_LUT_LENGTH) {
		gamma->para_mode = 0;

		ret = dpu_effect_copy_to_user(gamma->gamma_r_table,
			pinfo->gamma_lut_table_R, GAMA_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gamma_r_table to user!\n",
				dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(gamma->gamma_g_table,
			pinfo->gamma_lut_table_G, GAMA_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gamma_g_table to user!\n",
				dpufd->index);
			goto err_ret;
		}

		ret = dpu_effect_copy_to_user(gamma->gamma_b_table,
			pinfo->gamma_lut_table_B, GAMA_LUT_LENGTH);
		if (ret) {
			DPU_FB_ERR("fb%d, failed to copy gamma_b_table to user!\n",
				dpufd->index);
			goto err_ret;
		}
	}

err_ret:
	return ret;
}

int dpu_effect_save_arsr2p_info(struct dpu_fb_data_type *dpufd, struct dss_effect_info *effect_info_src)
{
	uint32_t i;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (effect_info_src == NULL) {
		DPU_FB_ERR("fb%d, effect_info_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->effect_ctl.arsr2p_sharp_support) {
		DPU_FB_INFO("fb%d, arsr2p lcd is not supported!\n", dpufd->index);
		return 0;
	}

	if (effect_info_src->disp_panel_id < 0 || effect_info_src->disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("effect_info_src->disp_panel_id wrong index!\n");
		return 0;
	}

	for (i = 0; i < ARSR2P_MAX_NUM; i++) {
		if (effect_info_src->arsr2p[i].update == 1)
			memcpy(&(dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i]),
				&(effect_info_src->arsr2p[i]), sizeof(struct arsr2p_info));
	}

	/* use effect_init_update_status to set arsr1p update status in dual-panel display effect */
	if (effect_info_src->arsr2p[0].update)
		dpufd->effect_init_update[effect_info_src->disp_panel_id].arsr2p_update_normal = 1;
	if (effect_info_src->arsr2p[1].update)
		dpufd->effect_init_update[effect_info_src->disp_panel_id].arsr2p_update_scale_up = 1;
	if (effect_info_src->arsr2p[2].update)
		dpufd->effect_init_update[effect_info_src->disp_panel_id].arsr2p_update_scale_down = 1;

	dpufd->effect_updated_flag[effect_info_src->disp_panel_id].arsr2p_effect_updated = true;

	// debug info
	for (i = 0; i < ARSR2P_MAX_NUM; i++)
		if (dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].update)
			DPU_FB_INFO("arsr2p mode %d: enable : %u, lcd_enable:%u, "
				"shoot_enable:%u, skin_enable:%u, update: %u\n",
				i, dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].enable,
				dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].sharp_enable,
				dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].shoot_enable,
				dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].skin_enable,
				dpufd->effect_info[effect_info_src->disp_panel_id].arsr2p[i].update);

	return 0;
}

int dpu_effect_save_arsr1p_info(struct dpu_fb_data_type *dpufd, struct dss_effect_info *effect_info_src)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!\n");
		return -EINVAL;
	}

	if (effect_info_src == NULL) {
		DPU_FB_ERR("fb%d, effect_info_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->effect_ctl.arsr1p_sharp_support) {
		DPU_FB_INFO("fb%d, arsr1p lcd is not supported!\n", dpufd->index);
		return 0;
	}

	if (effect_info_src->disp_panel_id < 0 || effect_info_src->disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("effect_info_src->disp_panel_id wrong index!\n");
		return 0;
	}

	effect_info_src->arsr1p[0].enable = (effect_info_src->arsr1p[0].para_mode) & 0x1;
	memcpy(dpufd->effect_info[effect_info_src->disp_panel_id].arsr1p, effect_info_src->arsr1p,
		sizeof(struct arsr1p_info) * ARSR1P_INFO_SIZE);
	dpufd->effect_updated_flag[effect_info_src->disp_panel_id].arsr1p_effect_updated = true;

	if (effect_info_src->arsr1p[1].update == 1)
		dpufd->effect_info[effect_info_src->disp_panel_id].arsr1p_rog_initialized |= ARSR1P_ROG_FHD_FLAG;
	// cppcheck-suppress *
	if (effect_info_src->arsr1p[2].update == 1)
		dpufd->effect_info[effect_info_src->disp_panel_id].arsr1p_rog_initialized |= ARSR1P_ROG_HD_FLAG;

	/* use effect_init_update to set arsr1p update status in dual-panel display effect */
	if (effect_info_src->arsr1p[0].update)
		dpufd->effect_init_update[effect_info_src->disp_panel_id].arsr1p_update_normal = 1;
	if (effect_info_src->arsr1p[1].update)
		dpufd->effect_init_update[effect_info_src->disp_panel_id].arsr1p_update_fhd = 1;
	// cppcheck-suppress *
	if (effect_info_src->arsr1p[2].update)
		dpufd->effect_init_update[effect_info_src->disp_panel_id].arsr1p_update_hd = 1;

	return 0;
}

int dpu_effect_save_acm_info(struct dpu_fb_data_type *dpufd, struct dss_effect_info *effect_info_src)
{
	(void)dpufd;
	(void)effect_info_src;
	return 0;
}

int dpu_effect_save_post_xcc_info(struct dpu_fb_data_type *dpufd, struct dss_effect_info *effect_info_src)
{
	struct post_xcc_info *post_xcc_dst = NULL;
	struct dss_effect *effect = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL!\n");
		return -EINVAL;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("[effect]display effect lock is not init!\n");
		return -EINVAL;
	}

	if (effect_info_src == NULL) {
		DPU_FB_ERR("[effect]fb%d, effect_info_src is NULL!\n", dpufd->index);
		return -EINVAL;
	}

	if (effect_info_src->disp_panel_id < 0 || effect_info_src->disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("effect_info_src->disp_panel_id wrong index!\n");
		return 0;
	}

	post_xcc_dst = &(dpufd->effect_info[effect_info_src->disp_panel_id].post_xcc);
	effect = &(dpufd->effect_ctl);

	if (!effect->post_xcc_support) {
		DPU_FB_INFO("[effect]fb%d, post_xcc is not supported!\n", dpufd->index);
		return 0;
	}

	spin_lock(&g_post_xcc_effect_lock);

	post_xcc_dst->enable = effect_info_src->lcp.post_xcc_enable;

	if (post_xcc_dst->enable)
		if (copy_from_user(post_xcc_dst->post_xcc_table,
			effect_info_src->lcp.post_xcc_table, POST_XCC_LUT_LENGTH * sizeof(uint32_t))) {
			DPU_FB_ERR("[effect]fb%d, failed to set post_xcc_table!\n",
				dpufd->index);
			goto err_ret;
		}

	dpufd->effect_updated_flag[effect_info_src->disp_panel_id].post_xcc_effect_updated = true;

	spin_unlock(&g_post_xcc_effect_lock);
	return 0;

err_ret:
	spin_unlock(&g_post_xcc_effect_lock);
	return -EINVAL;
}

void dpu_effect_print_gmp_lut_info(const uint32_t * const lut_low32bit, const uint32_t * const lut_high4bit)
{
	dpu_check_and_no_retval((!lut_low32bit || !lut_high4bit), ERR, "NULL pointer\n");

	DPU_FB_DEBUG("[effect] gmp lut index [%d %d %d %d %d]=[0x%llx 0x%llx 0x%llx 0x%llx 0x%llx]\n",
		LCP_GMP_LUT_CHECK_BLACK, LCP_GMP_LUT_CHECK_BLUE, LCP_GMP_LUT_CHECK_GREEN,
		LCP_GMP_LUT_CHECK_RED, LCP_GMP_LUT_CHECK_WHITE,
		(((uint64_t)lut_high4bit[LCP_GMP_LUT_CHECK_BLACK] << 32) | lut_low32bit[LCP_GMP_LUT_CHECK_BLACK]),
		(((uint64_t)lut_high4bit[LCP_GMP_LUT_CHECK_BLUE] << 32) | lut_low32bit[LCP_GMP_LUT_CHECK_BLUE]),
		(((uint64_t)lut_high4bit[LCP_GMP_LUT_CHECK_GREEN] << 32) | lut_low32bit[LCP_GMP_LUT_CHECK_GREEN]),
		(((uint64_t)lut_high4bit[LCP_GMP_LUT_CHECK_RED] << 32) | lut_low32bit[LCP_GMP_LUT_CHECK_RED]),
		(((uint64_t)lut_high4bit[LCP_GMP_LUT_CHECK_WHITE] << 32) | lut_low32bit[LCP_GMP_LUT_CHECK_WHITE]));
}

static int effect_dbm_save_gmp_info(struct gmp_info *dst_gmp,
	struct lcp_info *lcp_src)
{
	if (dst_gmp == NULL) {
		DPU_FB_ERR("[effect]dst_gmp is NULL!\n");
		return -EINVAL;
	}

	if (lcp_src == NULL) {
		DPU_FB_ERR("[effect]lcp_src is NULL!\n");
		return -EINVAL;
	}

	dst_gmp->gmp_mode = lcp_src->gmp_mode;
	/* only update gmp lut when gmp is enabled */
	if (lcp_src->gmp_mode & GMP_ENABLE) {
		if (copy_from_user(dst_gmp->gmp_lut_high4bit,
			lcp_src->gmp_table_high4,
			(LCP_GMP_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT))) {
			DPU_FB_ERR("[effect]failed to copy gmp high4bit table from user\n");
			return -EINVAL;
		}

		if (copy_from_user(dst_gmp->gmp_lut_low32bit,
			lcp_src->gmp_table_low32,
			(LCP_GMP_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT))) {
			DPU_FB_ERR("[effect]failed to copy gmp low32bit table from user\n");
			return -EINVAL;
		}
		dpu_effect_print_gmp_lut_info(dst_gmp->gmp_lut_low32bit, dst_gmp->gmp_lut_high4bit);
	}

	return 0;
}

void dpu_effect_print_degamma_lut_info(const struct degamma_info * const dst_degamma)
{
	uint32_t i;

	dpu_check_and_no_retval(!dst_degamma, ERR, "dst_degamma is NULL\n");

	for (i = LCP_IGM_LUT_CHECK_START; i < LCP_IGM_LUT_CHECK_LEN; i += LCP_IGM_LUT_CHECK_STEP)
		DPU_FB_DEBUG("[effect] igamma index %d [R G B]=[%u %u %u]\n", i,
			dst_degamma->degamma_r_lut[i], dst_degamma->degamma_g_lut[i], dst_degamma->degamma_b_lut[i]);
}

static int effect_dbm_save_degamma_info(struct degamma_info *dst_degamma,
	struct lcp_info *lcp_src)
{
	if (dst_degamma == NULL) {
		DPU_FB_ERR("[effect]dst_degamma is NULL!\n");
		return -EINVAL;
	}

	if (lcp_src == NULL) {
		DPU_FB_ERR("[effect]lcp_src is NULL!\n");
		return -EINVAL;
	}

	dst_degamma->degamma_enable = lcp_src->igm_enable;

	DPU_FB_DEBUG("dst_degamma->degamma_enable=%u\n", dst_degamma->degamma_enable);

	if (lcp_src->igm_enable) {
		if (copy_from_user(dst_degamma->degamma_r_lut,
			lcp_src->igm_r_table,
			LCP_IGM_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
			DPU_FB_ERR("[effect]failed to set igm_r_table!\n");
			return -EINVAL;
		}

		if (copy_from_user(dst_degamma->degamma_g_lut,
			lcp_src->igm_g_table,
			LCP_IGM_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
			DPU_FB_ERR("[effect]failed to set igm_g_table!\n");
			return -EINVAL;
		}

		if (copy_from_user(dst_degamma->degamma_b_lut,
			lcp_src->igm_b_table,
			LCP_IGM_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
			DPU_FB_ERR("[effect]failed to set igm_b_table!\n");
			return -EINVAL;
		}

		dpu_effect_print_degamma_lut_info(dst_degamma);
	}

	return 0;
}

static int effect_dbm_save_xcc_info(struct xcc_info *dst_xcc,
	struct lcp_info *lcp_src)
{
	if (dst_xcc == NULL) {
		DPU_FB_ERR("[effect]dst_xcc is NULL!\n");
		return -EINVAL;
	}

	if (lcp_src == NULL) {
		DPU_FB_ERR("[effect]lcp_src is NULL!\n");
		return -EINVAL;
	}

	struct xcc_info *tmp_xcc = dst_xcc;

	tmp_xcc->xcc_enable = lcp_src->xcc0_enable;

	if (lcp_src->xcc0_enable) {
		if (copy_from_user(tmp_xcc->xcc_table,
			lcp_src->xcc0_table, LCP_XCC_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
			DPU_FB_ERR("[effect]failed to set xcc_table 0!\n");
			return -EINVAL;
		}
	}

	tmp_xcc = dst_xcc + 1;

	tmp_xcc->xcc_enable = lcp_src->xcc1_enable;
	if (lcp_src->xcc1_enable) {
		if (copy_from_user(tmp_xcc->xcc_table,
			lcp_src->xcc1_table, LCP_XCC_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
			DPU_FB_ERR("[effect]failed to set xcc_table 1!\n");
			return -EINVAL;
		}
	}

	return 0;
}

static void effect_dbm_gamma_lut_set_normal(struct gama_info *gama_dst,
	const struct dpu_panel_info *pinfo)
{
	if (pinfo->gamma_lut_table_R != NULL) {
		memcpy(gama_dst->gama_r_lut, pinfo->gamma_lut_table_R,
			GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT);
		DPU_FB_DEBUG("[effect]normal mode to set gamma_r_table!\n");
	}

	if (pinfo->gamma_lut_table_G != NULL) {
		memcpy(gama_dst->gama_g_lut, pinfo->gamma_lut_table_G,
			GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT);
		DPU_FB_DEBUG("[effect]normal mode to set gamma_g_table!\n");
	}

	if (pinfo->gamma_lut_table_B != NULL) {
		memcpy(gama_dst->gama_b_lut, pinfo->gamma_lut_table_B,
			GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT);
		DPU_FB_DEBUG("[effect]normal mode to set gamma_b_table!\n");
	}
}

static void effect_dbm_gamma_lut_set_cinema(struct gama_info *gama_dst,
	const struct dpu_panel_info *pinfo)
{
	if (pinfo->cinema_gamma_lut_table_R != NULL) {
		memcpy(gama_dst->gama_r_lut, pinfo->cinema_gamma_lut_table_R,
			GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT);
		DPU_FB_DEBUG("[effect]cinema mode to set gamma_r_table!\n");
	}

	if (pinfo->cinema_gamma_lut_table_G != NULL) {
		memcpy(gama_dst->gama_g_lut, pinfo->cinema_gamma_lut_table_G,
			GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT);
		DPU_FB_DEBUG("[effect]cinema mode to set gamma_g_table!\n");
	}

	if (pinfo->cinema_gamma_lut_table_B != NULL) {
		memcpy(gama_dst->gama_b_lut, pinfo->cinema_gamma_lut_table_B,
			GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT);
		DPU_FB_DEBUG("[effect]cinema mode to set gamma_b_table!\n");
	}
}

void dpu_effect_print_gamma_lut_info(const struct gama_info * const gama_dst)
{
	static int gamma_lut_check_idx_array[] = { GAMA_LUT_CHECK_0, GAMA_LUT_CHECK_1, GAMA_LUT_CHECK_2,
		GAMA_LUT_CHECK_3, GAMA_LUT_CHECK_4, GAMA_LUT_CHECK_5, GAMA_LUT_CHECK_6, GAMA_LUT_CHECK_7 };
	uint32_t i;

	dpu_check_and_no_retval(!gama_dst, ERR, "gama_dst is NULL\n");

	for (i = 0; i < ARRAY_SIZE(gamma_lut_check_idx_array); ++i)
		DPU_FB_DEBUG("[effect] gamma index %d [R G B]=[%u %u %u]\n", gamma_lut_check_idx_array[i],
			gama_dst->gama_r_lut[gamma_lut_check_idx_array[i]],
			gama_dst->gama_g_lut[gamma_lut_check_idx_array[i]],
			gama_dst->gama_b_lut[gamma_lut_check_idx_array[i]]);
}

static int effect_dbm_gamma_lut_set_user(struct gama_info *gama_dst, const struct gamma_info *gamma_src)
{
	if (copy_from_user(gama_dst->gama_r_lut, gamma_src->gamma_r_table,
		GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
		DPU_FB_ERR("[effect]user mode failed to copy gamma_r_table from user!\n");
		return -EINVAL;
	}

	if (copy_from_user(gama_dst->gama_g_lut, gamma_src->gamma_g_table,
		GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
		DPU_FB_ERR("[effect]user mode failed to copy gamma_g_table from user!\n");
		return -EINVAL;
	}

	if (copy_from_user(gama_dst->gama_b_lut, gamma_src->gamma_b_table,
		GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
		DPU_FB_ERR("[effect]user mode failed to copy gamma_b_table from user!\n");
		return -EINVAL;
	}

	dpu_effect_print_gamma_lut_info(gama_dst);

	return 0;
}

static int effect_dbm_gamma_lut_set_user_roi(struct gama_info *gama_dst,
	const struct gamma_info *gamma_src)
{
	if (copy_from_user(gama_dst->gama_r_lut, gamma_src->gamma_roi_r_table,
		GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
		DPU_FB_ERR("[effect]user mode failed to copy gamma_roi_r_table from user!\n");
		return -EINVAL;
	}

	if (copy_from_user(gama_dst->gama_g_lut, gamma_src->gamma_roi_g_table,
		GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
		DPU_FB_ERR("[effect]user mode failed to copy gamma_roi_g_table from user!\n");
		return -EINVAL;
	}

	if (copy_from_user(gama_dst->gama_b_lut, gamma_src->gamma_roi_b_table,
		GAMA_LUT_LENGTH * BYTES_PER_TABLE_ELEMENT)) {
		DPU_FB_ERR("[effect]user mode failed to copy gamma_roi_b_table from user!\n");
		return -EINVAL;
	}

	return 0;
}

static int effect_dbm_gamma_lut_set(struct gama_info *gama_dst,
	struct gamma_info *gamma_src, struct dpu_panel_info *pinfo, bool roi_flag)
{
	if (gama_dst == NULL) {
		DPU_FB_ERR("[effect]gama_dst is NULL!\n");
		return -EINVAL;
	}

	if (gamma_src == NULL) {
		DPU_FB_ERR("[effect]gamma_src is NULL!\n");
		return -EINVAL;
	}

	if (pinfo == NULL) {
		DPU_FB_ERR("[effect]pinfo is NULL!\n");
		return -EINVAL;
	}

	if (gamma_src->para_mode == GAMMA_PARA_MODE_NORMAL) {
		/* Normal mode */
		effect_dbm_gamma_lut_set_normal(gama_dst, pinfo);
	} else if (gamma_src->para_mode == GAMMA_PARA_MODE_CINEMA) {
		/* Cinema mode */
		effect_dbm_gamma_lut_set_cinema(gama_dst, pinfo);
	} else if (gamma_src->para_mode == GAMMA_PARA_MODE_USER) {
		/* User mode */
		if (roi_flag)
			return effect_dbm_gamma_lut_set_user_roi(gama_dst, gamma_src);
		else
			return effect_dbm_gamma_lut_set_user(gama_dst, gamma_src);
	} else {
		DPU_FB_ERR("[effect]not supported gamma para_mode!\n");
		return -EINVAL;
	}

	return 0;
}

static int effect_dbm_save_gama_info(struct gama_info *gama_dst,
		struct gamma_info *gamma_src, struct dpu_panel_info *pinfo, bool roi_flag)
{
	if (gama_dst == NULL) {
		DPU_FB_ERR("[effect]gama_dst is NULL!\n");
		return -EINVAL;
	}

	if (gamma_src == NULL) {
		DPU_FB_ERR("[effect]gamma_src is NULL!\n");
		return -EINVAL;
	}

	if (pinfo == NULL) {
		DPU_FB_ERR("[effect]pinfo is NULL!\n");
		return -EINVAL;
	}

	if (roi_flag)
		gama_dst->gama_enable = gamma_src->roi_enable;
	else
		gama_dst->gama_enable = gamma_src->enable;

	gama_dst->para_mode = gamma_src->para_mode;
	if (effect_dbm_gamma_lut_set(gama_dst, gamma_src, pinfo, roi_flag) < 0)
		return -EINVAL;

	return 0;
}


static void effect_dbm_call_workqueue(struct dpu_fb_data_type *dpufd)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL!");
		return;
	}

	if (dpufd->effect_thread) {
		DPU_FB_DEBUG("call effect thread!\n");
		dpufd->effect_update = true;
		wake_up_interruptible(&dpufd->effect_wq);
		return;
	}

	/* call queue work to config reg */
	if (dpufd->gmp_lut_wq) {
		DPU_FB_DEBUG("call gmp workqueue!\n");
		queue_work(dpufd->gmp_lut_wq, &dpufd->gmp_lut_work);
	}
}

/* if success,return 0; else, return 1 */
static uint32_t effect_dbm_gmp_info_set(struct dpp_buf_maneger *dpp_buff_mngr,
		struct lcp_info *lcp_src, uint32_t buff_sel, int disp_panel_id)
{
	int ret = 0;
	uint32_t another_buf = 0;
	struct gmp_info *dst_gmp = NULL;
	bool roi_flag = false;

	if (dpp_buff_mngr == NULL) {
		DPU_FB_ERR("[effect]dpp_buff_mngr is NULL!");
		return DPP_EFFECT_UPDATE_NONE;
	}

	if (lcp_src == NULL)
		DPU_FB_DEBUG("[effect]there is no new gmp parameters this time!");

	if (buff_sel == DPP_EXTENAL_ROI_BUF)
		roi_flag = true;

	/* get dst gmp buffer to save gmp info */
	dst_gmp = &(dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp);
	if (lcp_src != NULL) {
		ret = effect_dbm_save_gmp_info(dst_gmp, lcp_src);
		if (ret != 0) {
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_save_status = DPP_BUF_INVALIED;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_cfg_status = DPP_BUF_READY_FOR_NONE;
			DPU_FB_ERR("[effect]save gmp info fail! buff_sel:%d\n", buff_sel);
			return DPP_EFFECT_UPDATE_NONE;
		}

		/* sdk updated gmp lut, driver need to config the lut reg */
		dpp_buff_mngr->online_mode_available = FALSE;
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_save_status = DPP_BUF_NEW_UPDATED;
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_cfg_status = DPP_BUF_READY_FOR_BOTH;
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_effect_roi = lcp_src->effect_area.gmp_area;
	} else {
		if (!roi_flag) {
			another_buf = buff_sel ? DPP_GENERAL_BUF_0 : DPP_GENERAL_BUF_1;
			/* only copy once */
			if (dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gmp_buf_save_status != DPP_BUF_NEW_UPDATED) {
				DPU_FB_DEBUG("[effect]no need to copy! buff_sel=%d, save_status:%d!\n",
					buff_sel, dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gmp_buf_save_status);
				return DPP_EFFECT_UPDATE_NONE;
			}
			memcpy(dst_gmp, &(dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gmp), sizeof(*dst_gmp));

			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_effect_roi =
					dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gmp_effect_roi;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_cfg_status =
					dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gmp_buf_cfg_status;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_save_status = DPP_BUF_FROM_ANOTHER;
			dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gmp_buf_save_status = DPP_BUF_COPIED_ONCE;
		}
	}

	DPU_FB_DEBUG("[effect]disp_panel_id:%d gmp buff_sel:%d, roi:0x%x, cfg status:%d, save status:%d, gmp_mode:%d!\n",
		disp_panel_id, buff_sel, dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_effect_roi,
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_cfg_status,
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_save_status,dst_gmp->gmp_mode);
	return DPP_EFFECT_UPDATE_SUCCESS;
}

/* if success,return 0; else, return 1 */
static uint32_t effect_dbm_igm_info_set(struct dpp_buf_maneger *dpp_buff_mngr,
		struct lcp_info *lcp_src, uint32_t buff_sel, int disp_panel_id)
{
	struct degamma_info *dst_degamma = NULL;
	int ret;
	uint32_t another_buf = 0;
	bool roi_flag = false;

	if (dpp_buff_mngr == NULL) {
		DPU_FB_ERR("[effect]dpp_buff_mngr is NULL!");
		return DPP_EFFECT_UPDATE_NONE;
	}

	if (lcp_src == NULL)
		DPU_FB_DEBUG("[effect] there is no new igm parameters this time");

	if (buff_sel == DPP_EXTENAL_ROI_BUF)
		roi_flag = true;

	if (disp_panel_id < 0 || disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("disp_panel_id wrong index!\n");
		return 0;
	}

	if (buff_sel >= DPP_BUF_MAX_COUNT) {
		DPU_FB_INFO("buff_sel wrong index!\n");
		return 0;
	}

	/* get dst degamma buffer to save degamma info */
	dst_degamma = &(dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degamma);

	if (lcp_src != NULL) {
		ret = effect_dbm_save_degamma_info(dst_degamma, lcp_src);
		if (ret != 0) {
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_save_status = DPP_BUF_INVALIED;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_cfg_status = DPP_BUF_READY_FOR_NONE;
			DPU_FB_ERR("[effect]save igm info fail! buff_sel:%d\n", buff_sel);
			return DPP_EFFECT_UPDATE_NONE;
		}
		/* sdk updated degamma lut, driver need to config the lut reg */
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_effect_roi = lcp_src->effect_area.igm_area;
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_cfg_status = DPP_BUF_READY_FOR_BOTH;
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_save_status = DPP_BUF_NEW_UPDATED;
	} else {
		if (!roi_flag) {
			another_buf = buff_sel ? DPP_GENERAL_BUF_0 : DPP_GENERAL_BUF_1;
			if (dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].degama_buf_save_status != DPP_BUF_NEW_UPDATED) {
				DPU_FB_DEBUG("[effect]no need to copy! buff_sel=%d, save_status:%d!\n",
					buff_sel, dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].degama_buf_save_status);
				return DPP_EFFECT_UPDATE_NONE;
			}
			memcpy(dst_degamma, &(dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].degamma), sizeof(*dst_degamma));

			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_effect_roi =
				dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].degama_effect_roi;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_cfg_status =
				dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].degama_buf_cfg_status;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_save_status = DPP_BUF_FROM_ANOTHER;
			dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].degama_buf_save_status = DPP_BUF_COPIED_ONCE;
		}
	}

	DPU_FB_DEBUG("[effect]degamma buff_sel:%d, roi:0x%x, cfg status:%d, save status:%d!\n",
		buff_sel, dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_effect_roi,
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_cfg_status,
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_save_status);
	return DPP_EFFECT_UPDATE_SUCCESS;
}

/* if success,return 0; else, return 1 */
static uint32_t effect_dbm_xcc_info_set(struct dpp_buf_maneger *dpp_buff_mngr, struct lcp_info *lcp_src,
	uint32_t buff_sel, int disp_panel_id)
{
	int ret;
	struct xcc_info *dst_xcc = NULL;
	uint32_t another_buf = 0;
	bool roi_flag = false;

	if (dpp_buff_mngr == NULL) {
		DPU_FB_ERR("[effect]dpp_buff_mngr is NULL!");
		return DPP_EFFECT_UPDATE_NONE;
	}

	if (lcp_src == NULL)
		DPU_FB_DEBUG("[effect] there is no new xcc parameters this time");

	if (buff_sel == DPP_EXTENAL_ROI_BUF)
		roi_flag = true;

	if (disp_panel_id < 0 || disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("disp_panel_id wrong index!\n");
		return 0;
	}

	if (buff_sel >= DPP_BUF_MAX_COUNT) {
		DPU_FB_INFO("buff_sel wrong index!\n");
		return 0;
	}

	/* get dst xcc buffer to save xcc info */
	dst_xcc = dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc;

	if (lcp_src != NULL) {
		ret = effect_dbm_save_xcc_info(dst_xcc, lcp_src);
		if (ret != 0) {
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_save_status = DPP_BUF_INVALIED;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_cfg_status = DPP_BUF_READY_FOR_NONE;
			DPU_FB_ERR("[effect]save xcc info fail! buff_sel:%d\n", buff_sel);
			return DPP_EFFECT_UPDATE_NONE;
		}

		// sdk updated xcc lut, driver need to config the lut reg
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc0_effect_roi = lcp_src->effect_area.xcc0_area;
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc1_effect_roi = lcp_src->effect_area.xcc1_area;
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_save_status = DPP_BUF_NEW_UPDATED;
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_cfg_status = DPP_BUF_READY_FOR_BOTH;
	} else {
		if (!roi_flag) {
			another_buf = buff_sel ? DPP_GENERAL_BUF_0 : DPP_GENERAL_BUF_1;
			if (dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].xcc_buf_save_status != DPP_BUF_NEW_UPDATED) {
				DPU_FB_DEBUG("[effect]no need to copy! buff_sel=%d, save_status:%d!\n",
					buff_sel, dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].xcc_buf_save_status);
				return DPP_EFFECT_UPDATE_NONE;
			}
			memcpy(dst_xcc, dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].xcc, sizeof(*dst_xcc) * 2);

			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc0_effect_roi =
				dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].xcc0_effect_roi;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc1_effect_roi =
				dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].xcc1_effect_roi;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_cfg_status =
				dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].xcc_buf_cfg_status;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_save_status = DPP_BUF_FROM_ANOTHER;
			dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].xcc_buf_save_status = DPP_BUF_COPIED_ONCE;
		}
	}

	DPU_FB_DEBUG("[effect]xcc buff_sel:%d, roi0:0x%x, roi1:0x%x, cfg status:%d, save status:%d!\n",
		buff_sel, dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc0_effect_roi,
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc1_effect_roi,
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_cfg_status,
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_save_status);
	return DPP_EFFECT_UPDATE_SUCCESS;
}

/* if success,return 0; else, return 1 */
static uint32_t effect_dbm_gamma_info_set(struct dpp_buf_maneger *dpp_buff_mngr,
	struct gamma_info *gamma_src, struct dpu_panel_info *pinfo, uint32_t buff_sel, int disp_panel_id)
{
	struct gama_info *gama_dst = NULL;
	int ret;
	uint32_t another_buf = 0;
	bool roi_flag = false;

	if (dpp_buff_mngr == NULL) {
		DPU_FB_ERR("[effect]dpp_buff_mngr is NULL!");
		return DPP_EFFECT_UPDATE_NONE;
	}

	if (gamma_src == NULL || pinfo == NULL)
		DPU_FB_DEBUG("[effect] there is no new gama parameters this time");

	if (buff_sel == DPP_EXTENAL_ROI_BUF)
		roi_flag = true;

	if (disp_panel_id < 0 || disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("disp_panel_id wrong index!\n");
		return 0;
	}

	if (buff_sel >= DPP_BUF_MAX_COUNT) {
		DPU_FB_INFO("buff_sel wrong index!\n");
		return 0;
	}

	/* get dst gama buffer to save gama info */
	gama_dst = &(dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama);

	if (gamma_src != NULL) {
		ret = effect_dbm_save_gama_info(gama_dst, gamma_src, pinfo, roi_flag);
		if (ret != 0) {
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_save_status = DPP_BUF_INVALIED;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_cfg_status = DPP_BUF_READY_FOR_NONE;
			DPU_FB_ERR("[effect]save gama info fail! buff_sel=%d\n", buff_sel);
			return DPP_EFFECT_UPDATE_NONE;
		}

		/* sdk updated gama lut, driver need to config the lut reg */
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_save_status = DPP_BUF_NEW_UPDATED;
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_cfg_status = DPP_BUF_READY_FOR_BOTH;
	} else {
		if (!roi_flag) {
			another_buf = buff_sel ? DPP_GENERAL_BUF_0 : DPP_GENERAL_BUF_1;
			if (dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gama_buf_save_status != DPP_BUF_NEW_UPDATED) {
				DPU_FB_DEBUG("[effect]no need to copy! buff_sel=%d, save_status:%d!\n",
					buff_sel, dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gama_buf_save_status);
				return DPP_EFFECT_UPDATE_NONE;
			}
			memcpy(gama_dst, &(dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gama), sizeof(*gama_dst));

			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_effect_roi =
				dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gama_effect_roi;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_cfg_status =
				dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gama_buf_cfg_status;
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_save_status = DPP_BUF_FROM_ANOTHER;
			dpp_buff_mngr->buf_info_list[disp_panel_id][another_buf].gama_buf_save_status = DPP_BUF_COPIED_ONCE;
		}
	}

	DPU_FB_DEBUG("[effect]buff_sel:%d, gamma roi:0x%x, cfg status:%d, save status:%d\n",
		buff_sel, dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_effect_roi,
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_cfg_status,
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_save_status);
	return DPP_EFFECT_UPDATE_SUCCESS;
}

/*lint -e456*/
static bool effect_dbm_dpp_buf_trylock(uint32_t buff_sel, int disp_panel_id)
{
	if (buff_sel == DPP_GENERAL_BUF_0 || buff_sel == DPP_GENERAL_BUF_1) {
		return mutex_trylock(&g_dpp_buf_lock_arr[disp_panel_id][buff_sel]) != 0 ? true : false;
	} else if (buff_sel == DPP_EXTENAL_ROI_BUF) {
		return true;
	} else {
		DPU_FB_ERR("[effect]buff_sel %d is wrong!\n", buff_sel);
		return false;
	}
}

static void effect_dbm_dpp_buf_lock(uint32_t buff_sel, int disp_panel_id)
{
	if (disp_panel_id < 0 || disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("disp_panel_id wrong index!\n");
		return;
	}

	if (buff_sel >= DPP_BUF_MAX_COUNT - 1) {
		DPU_FB_INFO("buff_sel wrong index!\n");
		return;
	}

	if (buff_sel == DPP_GENERAL_BUF_0 || buff_sel == DPP_GENERAL_BUF_1)
		mutex_lock(&g_dpp_buf_lock_arr[disp_panel_id][buff_sel]);
	else if (buff_sel == DPP_EXTENAL_ROI_BUF)
		DPU_FB_DEBUG("[effect]buff_sel %d needn't lock!\n", buff_sel);
	else
		DPU_FB_ERR("[effect]buff_sel %d is wrong!\n", buff_sel);
}

static void effect_dbm_dpp_buf_unlock(uint32_t buff_sel, int disp_panel_id)
{
	if (disp_panel_id < 0 || disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("disp_panel_id wrong index!\n");
		return;
	}

	if (buff_sel >= DPP_BUF_MAX_COUNT - 1) {
		DPU_FB_INFO("buff_sel wrong index!\n");
		return;
	}

	if (buff_sel == DPP_GENERAL_BUF_0 || buff_sel == DPP_GENERAL_BUF_1)
		mutex_unlock(&g_dpp_buf_lock_arr[disp_panel_id][buff_sel]);
	else if (buff_sel == DPP_EXTENAL_ROI_BUF)
		DPU_FB_DEBUG("[effect]buff_sel %d needn't unlock!\n", buff_sel);
	else
		DPU_FB_ERR("[effect]buff_sel %d is wrong!\n", buff_sel);
}

static bool effect_dbm_dpp_chn_trylock(uint32_t dst_channel)
{
	if (dst_channel == 0)
		return mutex_trylock(&g_dpp_ch0_lock) != 0 ? true : false;
	if (dst_channel == 1)
		return mutex_trylock(&g_dpp_ch1_lock) != 0 ? true : false;

	DPU_FB_ERR("[effect]dst_channel %d is wrong!\n", dst_channel);
	return FALSE;
}

static void effect_dbm_dpp_chn_lock(uint32_t dst_channel)
{
	if (dst_channel == 0)
		mutex_lock(&g_dpp_ch0_lock);
	else if (dst_channel == 1)
		mutex_lock(&g_dpp_ch1_lock);
	else
		DPU_FB_ERR("[effect]dst_channel %d is wrong!\n", dst_channel);
}

static void effect_dbm_dpp_chn_unlock(uint32_t dst_channel)
{
	if (dst_channel == 0)
		mutex_unlock(&g_dpp_ch0_lock);
	else if (dst_channel == 1)
		mutex_unlock(&g_dpp_ch1_lock);
	else
		DPU_FB_ERR("[effect]dst_channel %d is wrong!\n", dst_channel);
}
/* lint +e454, +e455, +e456 */

static void dpu_effect_change_dpp_status(struct dpu_fb_data_type *dpufd,
	uint8_t channel_status)
{
	uint32_t dst_channel = 0;
	uint32_t *ch_status = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL!\n");
		return;
	}

	ch_status = dpufd->effect_info[dpufd->panel_info.disp_panel_id].dpp_chn_status;
	dst_channel = (g_dyn_sw_default & 0x1) ? 0 : 1;
	effect_dbm_dpp_chn_lock(dst_channel);

	*(ch_status + dst_channel) = channel_status;

	effect_dbm_dpp_chn_unlock(dst_channel);

	return;
}

static int dpu_effect_dpp_save_buf_info(struct dpu_fb_data_type *dpufd,
	struct dss_effect_info *effect_info_src, uint32_t buff_sel, bool roi_flag)
{
	uint32_t ret = DPP_EFFECT_UPDATE_NONE;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;
	struct dpu_panel_info *pinfo = NULL;

	dpp_buff_mngr = &(dpufd->effect_info[effect_info_src->disp_panel_id].dpp_buf);

	effect_dbm_dpp_buf_lock(buff_sel, effect_info_src->disp_panel_id);
	DPU_FB_INFO("disp_panel_id=%d buff_sel=%u roi_flag=%d modules=0x%x\n",
		effect_info_src->disp_panel_id, buff_sel, roi_flag, effect_info_src->modules);
	if ((effect_info_src->modules & DSS_EFFECT_MODULE_LCP_GMP) &&
		dpufd->effect_ctl.lcp_gmp_support)
		ret &= effect_dbm_gmp_info_set(dpp_buff_mngr,
			&effect_info_src->lcp, buff_sel, effect_info_src->disp_panel_id);
	else if (dpufd->effect_ctl.lcp_gmp_support)
		ret &= effect_dbm_gmp_info_set(dpp_buff_mngr, NULL, buff_sel, effect_info_src->disp_panel_id);

	if ((effect_info_src->modules & DSS_EFFECT_MODULE_LCP_IGM) &&
		dpufd->effect_ctl.lcp_igm_support)
		ret &= effect_dbm_igm_info_set(dpp_buff_mngr, &effect_info_src->lcp, buff_sel, effect_info_src->disp_panel_id);
	else if (dpufd->effect_ctl.lcp_igm_support)
		ret &= effect_dbm_igm_info_set(dpp_buff_mngr, NULL, buff_sel, effect_info_src->disp_panel_id);

	if ((effect_info_src->modules & DSS_EFFECT_MODULE_LCP_XCC) &&
		dpufd->effect_ctl.lcp_xcc_support)
		ret &= effect_dbm_xcc_info_set(dpp_buff_mngr, &effect_info_src->lcp, buff_sel, effect_info_src->disp_panel_id);
	else if (dpufd->effect_ctl.lcp_xcc_support)
		ret &= effect_dbm_xcc_info_set(dpp_buff_mngr, NULL, buff_sel, effect_info_src->disp_panel_id);

	if ((effect_info_src->modules & DSS_EFFECT_MODULE_GAMMA) &&
		(dpufd->effect_ctl.gamma_support)) {
		pinfo = &(dpufd->panel_info);
		ret &= effect_dbm_gamma_info_set(dpp_buff_mngr, &effect_info_src->gamma, pinfo, buff_sel,
			effect_info_src->disp_panel_id);
	} else if (dpufd->effect_ctl.gamma_support) {
		ret &= effect_dbm_gamma_info_set(dpp_buff_mngr, NULL, NULL, buff_sel, effect_info_src->disp_panel_id);
	}
	effect_dbm_dpp_buf_unlock(buff_sel, effect_info_src->disp_panel_id);

	return ret;
}

int dpu_effect_set_dpp_buf_info(struct dpu_fb_data_type *dpufd, const struct dss_effect_info * const effect_info,
	uint32_t buff_sel, bool roi_flag, int update_flag)
{
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;
	int disp_panel_id;
	int ret = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL!\n");
		return -EINVAL;
	}

	if (effect_info->disp_panel_id < 0 || effect_info->disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("effect_info->disp_panel_id wrong index!\n");
		return 0;
	}

	if (dpufd->panel_info.disp_panel_id < 0 || dpufd->panel_info.disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("dpufd->panel_info.disp_panel_id wrong index!\n");
		return 0;
	}

	if (buff_sel >= DPP_BUF_MAX_COUNT) {
		DPU_FB_INFO("buff_sel wrong index!\n");
		return 0;
	}

	disp_panel_id = dpufd->panel_info.disp_panel_id;
	dpp_buff_mngr = &(dpufd->effect_info[effect_info->disp_panel_id].dpp_buf);

	effect_dbm_dpp_buf_lock(buff_sel, effect_info->disp_panel_id);

	DPU_FB_DEBUG("disp_panel_id=%d buff_sel=%u roi_flag=%d update_flag=%d\n", \
		effect_info->disp_panel_id, buff_sel, roi_flag, update_flag);

	/* gmp has updated, need to call workqueue to set reg */
	if (update_flag == DPP_EFFECT_UPDATE_NONE) {
		/* have no effect module updated */
		DPU_FB_INFO("[effect]effect_dbm_gmp_info_set fail, roi_flag=%d\n",
			roi_flag);
		effect_dbm_dpp_buf_unlock(buff_sel, effect_info->disp_panel_id);
		return -EINVAL;
	}

	if (!roi_flag)
		/* the update before dpp_latest_buf must before effect_dbm_call_workqueue */
		dpp_buff_mngr->latest_buf[effect_info->disp_panel_id] = buff_sel;

	/* We just save buf info and do not set reg if updating disp_panel_id is not current panel */
	if (effect_info->disp_panel_id != disp_panel_id) {
		effect_dbm_dpp_buf_unlock(buff_sel, effect_info->disp_panel_id);
		return 0;
	}

	dpu_effect_change_dpp_status(dpufd, DPP_CHN_UPDATE_READY);
	if (dpp_buff_mngr->online_mode_available == TRUE) {
		/* if the latest gmp parameters have been congiged to both dpp chn,
			* can use online mode config other modules.
			*/
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].dpp_config_mode = DPP_BUF_ONLINE_CONFIG;
	} else {
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].dpp_config_mode = DPP_BUF_WORKQ_CONFIG;
		effect_dbm_call_workqueue(dpufd);
	}

	effect_dbm_dpp_buf_unlock(buff_sel, disp_panel_id);
	return ret;
}

int dpu_effect_dpp_buf_info_set(struct dpu_fb_data_type *dpufd,
	struct dss_effect_info *effect_info, bool roi_flag)
{
	uint32_t ret = DPP_EFFECT_UPDATE_NONE;
	uint32_t buff_sel;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL!\n");
		return -EINVAL;
	}

	if (effect_info == NULL) {
		DPU_FB_ERR("[effect]effect_info is NULL!\n");
		return -EINVAL;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("[effect] fb%d, not support!\n", dpufd->index);
		return -EINVAL;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("[effect]display effect lock is not init!\n");
		return -EINVAL;
	}
	if (effect_info->disp_panel_id < 0 || effect_info->disp_panel_id >= DISP_PANEL_NUM) {
		DPU_FB_INFO("effect_info->disp_panel_id wrong index!\n");
		return 0;
	}

	dpp_buff_mngr = &(dpufd->effect_info[effect_info->disp_panel_id].dpp_buf);

	if (roi_flag)
		buff_sel = DPP_EXTENAL_ROI_BUF;
	else
		buff_sel = dpp_buff_mngr->latest_buf[effect_info->disp_panel_id] ? DPP_GENERAL_BUF_0 : DPP_GENERAL_BUF_1;

	ret = dpu_effect_dpp_save_buf_info(dpufd, effect_info, buff_sel, roi_flag);
	DPU_FB_INFO("[effect]buff_sel=%d roi_flag=%d dpp_latest_buf=%d modules=0x%x! "
		"disp_panel_id=%d, save_buf_info result=%d\n",
		buff_sel, roi_flag, dpp_buff_mngr->latest_buf[effect_info->disp_panel_id],  effect_info->modules,
		effect_info->disp_panel_id, ret);

	return dpu_effect_set_dpp_buf_info(dpufd, effect_info, buff_sel, roi_flag, ret);
}

void dpu_effect_acm_set_reg(struct dpu_fb_data_type *dpufd)
{
	(void)dpufd;
}
/*lint -e679*/

static bool effect_lcp_gmp_set_reg(char __iomem *gmp_lut_base,
	uint32_t *gmp_lut_low32, uint32_t *gmp_lut_high4)
{
	uint32_t i;

	DPU_FB_DEBUG("[effect] +\n");
	if (gmp_lut_base == NULL) {
		DPU_FB_DEBUG("[effect]gmp_lut_base is NULL!\n");
		return false;
	}

	if (gmp_lut_low32 == NULL) {
		DPU_FB_DEBUG("[effect]gmp_table_low32 is NULL!\n");
		return false;
	}
	if (gmp_lut_high4 == NULL) {
		DPU_FB_DEBUG("[effect]gmp_table_high4 is NULL!\n");
		return false;
	}

	for (i = 0; i < LCP_GMP_LUT_LENGTH; i++) {
		set_reg(gmp_lut_base + i * 2 * 4, gmp_lut_low32[i], 32, 0);
		set_reg(gmp_lut_base + i * 2 * 4 + 4, gmp_lut_high4[i], 4, 0);
	}

	dpu_effect_print_gmp_lut_info(gmp_lut_low32, gmp_lut_high4);

	return true;
}

static void effect_lcp_igm_set_reg(char __iomem *degamma_lut_base,
	struct degamma_info *degamma)
{
	uint32_t cnt;

	if (degamma_lut_base == NULL) {
		DPU_FB_DEBUG("[effect]degamma_lut_base is NULL!\n");
		return;
	}

	if (degamma == NULL) {
		DPU_FB_DEBUG("[effect]degamma is NULL!\n");
		return;
	}

	for (cnt = 0; cnt < LCP_IGM_LUT_LENGTH; cnt = cnt + 2) {
		set_reg(degamma_lut_base + (U_DEGAMA_R_COEF + cnt * 2),
			degamma->degamma_r_lut[cnt], 12, 0);
		if (cnt != LCP_IGM_LUT_LENGTH - 1)
			set_reg(degamma_lut_base + (U_DEGAMA_R_COEF + cnt * 2),
				degamma->degamma_r_lut[cnt+1], 12, 16);

		set_reg(degamma_lut_base + (U_DEGAMA_G_COEF + cnt * 2),
			degamma->degamma_g_lut[cnt], 12, 0);
		if (cnt != LCP_IGM_LUT_LENGTH - 1)
			set_reg(degamma_lut_base + (U_DEGAMA_G_COEF + cnt * 2),
				degamma->degamma_g_lut[cnt+1], 12, 16);

		set_reg(degamma_lut_base + (U_DEGAMA_B_COEF + cnt * 2),
			degamma->degamma_b_lut[cnt], 12, 0);
		if (cnt != LCP_IGM_LUT_LENGTH - 1)
			set_reg(degamma_lut_base + (U_DEGAMA_B_COEF + cnt * 2),
				degamma->degamma_b_lut[cnt+1], 12, 16);
	}

	dpu_effect_print_degamma_lut_info(degamma);
}

static void effect_lcp_xcc_set_reg(char __iomem *xcc_base,
	struct xcc_info *xcc)
{
	int cnt;

	if (xcc_base == NULL) {
		DPU_FB_DEBUG("[effect]xcc_base is NULL!\n");
		return;
	}

	if (xcc == NULL) {
		DPU_FB_DEBUG("[effect]xcc is NULL!\n");
		return;
	}

	for (cnt = 0; cnt < XCC_COEF_LEN; cnt++)
		set_reg(xcc_base + XCC_COEF_00 + cnt * 4, xcc->xcc_table[cnt], 17, 0);
}

void dpu_effect_gamma_set_reg(struct dpu_fb_data_type *dpufd)
{
	(void)dpufd;
}

void dpu_effect_post_xcc_set_reg(struct dpu_fb_data_type *dpufd)
{
	struct dss_effect *effect = NULL;
	char __iomem *post_xcc_base = NULL;
	struct post_xcc_info *post_xcc_param = NULL;
	uint32_t cnt = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL!");
		return;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("[effect]display effect lock is not init!\n");
		return;
	}

	effect = &dpufd->effect_ctl;
	post_xcc_base = dpufd->dss_base + DSS_DPP_POST_XCC_OFFSET;

	post_xcc_param = &(dpufd->effect_info[dpufd->panel_info.disp_panel_id].post_xcc);
	if (post_xcc_param == NULL) {
		DPU_FB_ERR("[effect]post_xcc_param is NULL!");
		return;
	}

	/* Update POST XCC Coef */
	if (effect->post_xcc_support &&
		dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].post_xcc_effect_updated &&
		!dpufd->mask_layer_xcc_flag) {
		if (spin_can_lock(&g_post_xcc_effect_lock)) {
			spin_lock(&g_post_xcc_effect_lock);
			if (post_xcc_param->post_xcc_table == NULL) {
				DPU_FB_ERR("[effect]post_xcc_table is NULL!\n");
				spin_unlock(&g_post_xcc_effect_lock);
				dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].post_xcc_effect_updated = false;
				return;
			}

			for (cnt = 0; cnt < POST_XCC_LUT_LENGTH; cnt++)
				set_reg(post_xcc_base + POST_XCC_COEF_00 + cnt * 4,
					post_xcc_param->post_xcc_table[cnt], 17, 0);

			set_reg(post_xcc_base + POST_XCC_EN, post_xcc_param->enable, 1, 0);
			dpufd->panel_info.dc_switch_xcc_updated =
				(dpufd->de_info.amoled_param.dc_brightness_dimming_enable_real !=
				dpufd->de_info.amoled_param.dc_brightness_dimming_enable) ||
				(dpufd->de_info.amoled_param.amoled_diming_enable !=
				dpufd->de_info.amoled_param.amoled_enable_from_hal);
			dpufd->effect_updated_flag[dpufd->panel_info.disp_panel_id].post_xcc_effect_updated = false;
			spin_unlock(&g_post_xcc_effect_lock);
		} else {
			DPU_FB_INFO("[effect]post_xcc effect param is being updated,"
				" delay set reg to next frame!\n");
		}
	}
}

/*
 * dppsw:0x20101 ifbcsw:0x403
 * ICH_0---DPP_0----OCH_0
 *      \----DPP_1---/
 */
static void effect_dpp_ch_both_connect(struct dpu_fb_data_type *dpufd,
	char __iomem *disp_glb_base)
{
	uint32_t sw_sig_ctrl;
	uint32_t sw_dat_ctrl;
	struct dpu_panel_info *pinfo = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL\n");
		return;
	}

	if (disp_glb_base == NULL) {
		DPU_FB_ERR("[effect]disp_glb_base is NULL\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	sw_sig_ctrl = inp32(disp_glb_base + DPPSW_SIG_CTRL);
	sw_dat_ctrl = inp32(disp_glb_base + DPPSW_DAT_CTRL);


	if ((sw_sig_ctrl != sw_dat_ctrl) || (sw_dat_ctrl != 0x20101)) {
		dpufd->set_reg(dpufd, disp_glb_base + DPPSW_SIG_CTRL, 0x20101, 32, 0);
		dpufd->set_reg(dpufd, disp_glb_base + DPPSW_DAT_CTRL, 0x20101, 32, 0);
	}


	sw_sig_ctrl = inp32(disp_glb_base + IFBCSW_SIG_CTRL);
	sw_dat_ctrl = inp32(disp_glb_base + IFBCSW_DAT_CTRL);
	/* sw_sig_ctrl must equle to sw_dat_ctrl, and must be 0x403, if not, re-config */
	if ((sw_sig_ctrl != sw_dat_ctrl) || (sw_dat_ctrl != 0x403)) {
		dpufd->set_reg(dpufd, disp_glb_base + IFBCSW_SIG_CTRL, 0x403, 32, 0);
		dpufd->set_reg(dpufd, disp_glb_base + IFBCSW_DAT_CTRL, 0x403, 32, 0);
	}
}


static int effect_dpp_ch_config_gmp(struct dpu_fb_data_type *dpufd,
	uint32_t config_ch, uint32_t buff_sel)
{
	uint32_t gmp_enable;
	char __iomem *gmp_base = NULL;
	char __iomem *gmp_lut_base = NULL;
	struct gmp_info *gmp = NULL;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;
	int disp_panel_id;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL\n");
		return -EINVAL;
	}
	if (dpufd->effect_ctl.lcp_gmp_support != true) {
		DPU_FB_DEBUG("[effect]gmp is not support!");
		return -EINVAL;
	}

	disp_panel_id = dpufd->panel_info.disp_panel_id;
	dpp_buff_mngr = &(dpufd->effect_info[disp_panel_id].dpp_buf);

	if (dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_save_status == DPP_BUF_INVALIED) {
		DPU_FB_DEBUG("[effect]gmp_buf_save_status is valid!"
			" buff_sel = %d, config_ch = %d\n",
			buff_sel, config_ch);
		return 0;
	}

	if (!(dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_cfg_status & (1 << config_ch))) {
		DPU_FB_DEBUG("[effect]gmp no need to config!");
		return 0;
	}

	gmp_base = dpufd->dss_base + DSS_DPP_GMP_OFFSET + config_ch * 0x40000;
	gmp_lut_base = dpufd->dss_base + DSS_DPP_GMP_LUT_OFFSET + config_ch * 0x10000;
	gmp = &(dpufd->effect_info[disp_panel_id].dpp_buf.buf_info_list[disp_panel_id][buff_sel].gmp);

	gmp_enable = gmp->gmp_mode & GMP_ENABLE;
	if (gmp_enable) {
		bool ret = effect_lcp_gmp_set_reg(gmp_lut_base,
			gmp->gmp_lut_low32bit, gmp->gmp_lut_high4bit);
		if (ret)
			set_reg(gmp_base + GMP_EN, gmp_enable, 1, 0);

		if (gmp->gmp_mode & GMP_LAST_PARAM_SCENE)
			gmp->gmp_mode &= ~GMP_LAST_PARAM_SCENE;
	} else {
		/* disable  GMP */
		set_reg(gmp_base + GMP_EN, gmp_enable, 1, 0);
	}

	dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_cfg_status &= ~((uint32_t)1 << config_ch);
	if (dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp_buf_cfg_status == DPP_BUF_READY_FOR_NONE)
		dpp_buff_mngr->online_mode_available = TRUE;

	return 0;
}

static int effect_dpp_ch_config_degamma(struct dpu_fb_data_type *dpufd,
	uint32_t config_ch, uint32_t buff_sel)
{
	uint32_t degamma_enable;
	char __iomem *degamma_base = NULL;
	char __iomem *degamma_lut_base = NULL;
	struct degamma_info *degamma = NULL;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;
	int disp_panel_id;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return -EINVAL;
	}

	if (dpufd->effect_ctl.lcp_igm_support != true) {
		DPU_FB_DEBUG("[effect]degamma is not support!");
		return 0;
	}

	disp_panel_id = dpufd->panel_info.disp_panel_id;
	dpp_buff_mngr = &(dpufd->effect_info[disp_panel_id].dpp_buf);

	if (dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_save_status == DPP_BUF_INVALIED) {
		DPU_FB_DEBUG("[effect]degama_buf_save_status is valid!"
			" buff_sel = %d, config_ch = %d\n", buff_sel, config_ch);
		return 0;
	}

	if (!(dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_cfg_status & (1 << config_ch))) {
		DPU_FB_DEBUG("[effect]degamma no need to config config_ch=%d!", config_ch);
		return 0;
	}

	degamma_base = dpufd->dss_base + DSS_DPP_DEGAMMA_OFFSET + config_ch * 0x40000;
	degamma_lut_base = dpufd->dss_base + DSS_DPP_DEGAMMA_LUT_OFFSET + config_ch * 0x40000;
	degamma = &(dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degamma);

	degamma_enable = degamma->degamma_enable;
	if (degamma_enable)
		effect_lcp_igm_set_reg(degamma_lut_base, degamma);

	set_reg(degamma_base + DEGAMA_EN, degamma->degamma_enable, 1, 0);

	dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].degama_buf_cfg_status &= ~((uint32_t)1 << config_ch);

	return 0;
}

static int effect_dpp_ch_config_xcc(struct dpu_fb_data_type *dpufd,
	uint32_t config_ch, uint32_t buff_sel, bool roi_mode)
{
	void_unused(roi_mode);
	uint32_t xcc_enable;
	char __iomem *xcc_base = NULL;
	struct xcc_info *xcc = NULL;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;
	int disp_panel_id;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL\n");
		return -EINVAL;
	}

	if (dpufd->effect_ctl.lcp_xcc_support != true) {
		DPU_FB_DEBUG("[effect]xcc is not support!");
		return 0;
	}

	disp_panel_id = dpufd->panel_info.disp_panel_id;
	dpp_buff_mngr = &(dpufd->effect_info[disp_panel_id].dpp_buf);

	if (dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_save_status == DPP_BUF_INVALIED) {
		DPU_FB_DEBUG("[effect]xcc_buf_save_status is valid!"
			" buff_sel = %d, config_ch = %d\n", buff_sel, config_ch);
		return 0;
	}

	if (!dpufd->mask_layer_xcc_flag &&
		!(dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_cfg_status & (1 << config_ch))) {
		DPU_FB_DEBUG("[effect]xcc no need to config!");
		return 0;
	}

	xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET + config_ch * 0x40000;
	xcc = dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc;

	xcc_enable = xcc->xcc_enable;
	if (xcc_enable)
		effect_lcp_xcc_set_reg(xcc_base, xcc);

	set_reg(xcc_base + XCC_EN, xcc->xcc_enable, 1, 0);

	dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].xcc_buf_cfg_status &= ~((uint32_t)1 << config_ch);

	return 0;
}

static void effect_dpp_ch_gama_lut(char __iomem *gama_lut_base,
	struct gama_info *gama)
{
	uint32_t cnt;

	if (gama_lut_base == NULL) {
		DPU_FB_ERR("[effect]gama_lut_base is NULL\n");
		return;
	}

	if (gama == NULL) {
		DPU_FB_ERR("[effect]gama is NULL\n");
		return;
	}

	for (cnt = 0; cnt < GAMA_LUT_LENGTH; cnt = cnt + 2) {
		set_reg(gama_lut_base + (U_GAMA_R_COEF + cnt * 2),
			gama->gama_r_lut[cnt], 12, 0);
		if (cnt != GAMA_LUT_LENGTH - 1)
			set_reg(gama_lut_base + (U_GAMA_R_COEF + cnt * 2),
				gama->gama_r_lut[cnt+1], 12, 16);

		set_reg(gama_lut_base + (U_GAMA_G_COEF + cnt * 2),
			gama->gama_g_lut[cnt], 12, 0);
		if (cnt != GAMA_LUT_LENGTH - 1)
			set_reg(gama_lut_base + (U_GAMA_G_COEF + cnt * 2),
				gama->gama_g_lut[cnt+1], 12, 16);

		set_reg(gama_lut_base + (U_GAMA_B_COEF + cnt * 2),
			gama->gama_b_lut[cnt], 12, 0);
		if (cnt != GAMA_LUT_LENGTH - 1)
			set_reg(gama_lut_base + (U_GAMA_B_COEF + cnt * 2),
				gama->gama_b_lut[cnt+1], 12, 16);
	}

	dpu_effect_print_gamma_lut_info(gama);
}

static int effect_dpp_ch_config_gama(struct dpu_fb_data_type *dpufd,
	uint32_t config_ch, uint32_t buff_sel)
{
	uint32_t gama_enable;
	char __iomem *gama_base = NULL;
	char __iomem *gama_lut_base = NULL;
	struct gama_info *gama = NULL;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;
	int disp_panel_id;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL\n");
		return -EINVAL;
	}

	if (dpufd->effect_ctl.gamma_support != true) {
		DPU_FB_DEBUG("[effect]gama is not support!");
		return 0;
	}

	disp_panel_id = dpufd->panel_info.disp_panel_id;
	dpp_buff_mngr = &(dpufd->effect_info[disp_panel_id].dpp_buf);

	if (dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_save_status == DPP_BUF_INVALIED) {
		DPU_FB_DEBUG("[effect]gama_buf_save_status is valid! buff_sel = %d, config_ch = %d\n", buff_sel, config_ch);
		return 0;
	}

	if (!(dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_cfg_status & (1 << config_ch))) {
		DPU_FB_DEBUG("[effect]gama no need to config config_ch=%d!", config_ch);
		return 0;
	}

	gama_base = dpufd->dss_base + DSS_DPP_GAMA_OFFSET + config_ch * 0x40000;
	gama_lut_base = dpufd->dss_base + DSS_DPP_GAMA_LUT_OFFSET + config_ch * 0x40000;
	gama = &(dpufd->effect_info[disp_panel_id].dpp_buf.buf_info_list[disp_panel_id][buff_sel].gama);

	gama_enable = gama->gama_enable;
	DPU_FB_DEBUG("[effect] gama_enable=%d!", gama_enable);
	if (gama_enable)
		effect_dpp_ch_gama_lut(gama_lut_base, gama);

	set_reg(gama_base + GAMA_EN, gama->gama_enable, 1, 0);

	dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gama_buf_cfg_status &= ~((uint32_t)1 << config_ch);

	return 0;
}

static void dpu_effect_dpp_sw_channel(struct dpu_fb_data_type *dpufd,
	char __iomem *disp_glb_base, uint32_t buff_sel)
{
	uint32_t dst_channel = 0;
	uint32_t *ch_status = NULL;
	struct gmp_info *gmp = NULL;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;
	int disp_panel_id;


	disp_panel_id = dpufd->panel_info.disp_panel_id;
	ch_status = dpufd->effect_info[disp_panel_id].dpp_chn_status;
	dpp_buff_mngr = &(dpufd->effect_info[disp_panel_id].dpp_buf);

	if (effect_dbm_dpp_buf_trylock(buff_sel, disp_panel_id)) {
		dst_channel = (inp32(disp_glb_base + DYN_SW_DEFAULT) & 0x1) ? 0 : 1;

		/* the workqueue is doing the config to this channel */
		if (effect_dbm_dpp_chn_trylock(dst_channel)) {
			gmp = &(dpufd->effect_info[disp_panel_id].dpp_buf.buf_info_list[disp_panel_id][buff_sel].gmp);
			DPU_FB_DEBUG("[effect]1 buf_sel=%d, dst_channel=%d, "
				"dst_ch_status=%d, another_ch_status=%d, gmp_mode=%d\n",
				buff_sel, dst_channel, *(ch_status + dst_channel),
				*(ch_status + ((~dst_channel) & 0x1)), gmp->gmp_mode);


			{
				if ((*(ch_status + dst_channel) == DPP_CHN_CONFIG_DOING) ||
					(*(ch_status + dst_channel) == DPP_CHN_NEED_CONFIG)) {
					DPU_FB_DEBUG("this channel status is %d, this channel no need chang\n",
						*(ch_status + dst_channel));
					goto nor_exit;
				}

				/* this channel is config ready or configing in other place, so do nothing and return */
				if (*(ch_status + dst_channel) == DPP_CHN_CONFIG_READY) {
					DPU_FB_DEBUG("this channel status is %d, workqueue has done,"
						"gmp_mode:%d, gmp_call_wq_scene:%d	dts:%d\n",
						*(ch_status + dst_channel), gmp->gmp_mode,
						dpp_buff_mngr->gmp_call_wq_scene, dst_channel);
					dpufd->set_reg(dpufd, disp_glb_base + DYN_SW_DEFAULT, dst_channel, 1, 0);
					g_dyn_sw_default = dst_channel;
					*(ch_status + ((~dst_channel) & 0x1)) = DPP_CHN_NEED_CONFIG;
					goto nor_exit;
				}

				if (dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].dpp_config_mode == DPP_BUF_WORKQ_CONFIG) {
					DPU_FB_DEBUG("this buf:%d is workqueue config mode! the dst_channel:%d status is %d\n",
						buff_sel, dst_channel, *(ch_status + dst_channel));
					goto nor_exit;
				}

				*(ch_status + dst_channel) = DPP_CHN_CONFIG_DOING;

				if (effect_dpp_ch_config_degamma(dpufd, dst_channel, buff_sel))
					goto err_exit;

				if (effect_dpp_ch_config_gama(dpufd, dst_channel, buff_sel))
					goto err_exit;

				if (effect_dpp_ch_config_xcc(dpufd, dst_channel, buff_sel, false))
					goto err_exit;

				*(ch_status + dst_channel) = DPP_CHN_CONFIG_READY;
				*(ch_status + ((~dst_channel) & 0x1)) = DPP_CHN_NEED_CONFIG;

				/* switch dpp channel */
				dpufd->set_reg(dpufd, disp_glb_base + DYN_SW_DEFAULT, dst_channel, 1, 0);
				g_dyn_sw_default = dst_channel;
				goto nor_exit;
			}
		}  else {
			DPU_FB_INFO("[effect] latest_buf:%d, dst_channel %d is being updated,"
				" delay set reg to next frame!\n", buff_sel, dst_channel);
			goto non_exit;
		}
	} else {
		DPU_FB_INFO("[effect] lcp buff %d is being updated,"
			" delay set reg to next frame!\n", buff_sel);
	}

	return;

err_exit:
	DPU_FB_INFO("[effect] dpp config error");
	*(ch_status + dst_channel) = DPP_CHN_NEED_CONFIG;

nor_exit:
	DPU_FB_DEBUG("[effect]2 buf_sel=%d, dst_channel=%d,"
		" dst_ch_status=%d, another_ch_status=%d\n",
		buff_sel, dst_channel, *(ch_status + dst_channel),
		*(ch_status + ((~dst_channel) & 0x1)));
	effect_dbm_dpp_chn_unlock(dst_channel);

non_exit:
	effect_dbm_dpp_buf_unlock(buff_sel, disp_panel_id);
	return;
}

void dpu_effect_lcp_set_reg(struct dpu_fb_data_type *dpufd)
{
	uint32_t buff_sel;
	char __iomem *disp_glb_base = NULL;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;
	int disp_panel_id;

	if (dpufd == NULL) {
		DPU_FB_ERR("[effect]dpufd is NULL!");
		return;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_INFO("[effect]fb%d not surpport!\n", dpufd->index);
		return;
	}

	if (!g_is_effect_lock_init) {
		DPU_FB_INFO("display effect lock is not init!\n");
		return;
	}

	disp_panel_id = dpufd->panel_info.disp_panel_id;
	disp_glb_base = dpufd->dss_base + DSS_DISP_GLB_OFFSET;
	dpp_buff_mngr = &(dpufd->effect_info[disp_panel_id].dpp_buf);

	/* check dpp config status, make sure both dpp is connectted */
	effect_dpp_ch_both_connect(dpufd, disp_glb_base);

	down(&dpufd->effect_gmp_sem);

	/* in dync switch config mode */
	buff_sel = dpp_buff_mngr->latest_buf[disp_panel_id];
	if (buff_sel >= DPP_BUF_MAX_COUNT) {
		DPU_FB_INFO("[effect] select buffer %d\n", buff_sel);
		goto exit;
	}

	/* last gmp queue work not work because panel is power off,
	 * so call gmp queue work here to config reg  again.
	 */
	if (dpufd->effect_info[disp_panel_id].dpp_cmdlist_type == DPP_CMDLIST_TYPE_NONE) {
		if (dpufd->gmp_lut_wq &&
			((dpp_buff_mngr->gmp_call_wq_scene & GMP_PANEL_OFF_SCENE) ||
			(dpp_buff_mngr->gmp_call_wq_scene & GMP_LAST_PARAM_SCENE))) {
			DPU_FB_INFO("[effect] scene=%d,call gmp workqueue again!\n",
				dpp_buff_mngr->gmp_call_wq_scene);
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].dpp_config_mode = DPP_BUF_WORKQ_CONFIG;
			effect_dbm_call_workqueue(dpufd);
		}
	}

	if (dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].dpp_config_mode != DPP_BUF_WORKQ_CONFIG &&
		dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].dpp_config_mode != DPP_BUF_ONLINE_CONFIG) {
		goto exit;
	}

	dpu_effect_dpp_sw_channel(dpufd, disp_glb_base, buff_sel);

exit:
	up(&dpufd->effect_gmp_sem);
	return;
}

void dpufb_effect_dpp_config(struct dpu_fb_data_type *dpufd, bool enable_cmdlist)
{
	void_unused(enable_cmdlist);
	uint32_t buff_sel;
	uint32_t dst_channel;
	uint32_t *ch_status = NULL;
	struct dpp_buf_maneger *dpp_buff_mngr = NULL;
	char __iomem *disp_glb_base = NULL;
	uint32_t gmp_mode;
	int disp_panel_id;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");
	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("[effect] not primary panel\n");
		return;
	}

	down(&dpufd->blank_sem_effect_gmp);

	disp_panel_id = dpufd->panel_info.disp_panel_id;
	disp_glb_base = dpufd->dss_base + DSS_DISP_GLB_OFFSET;

	dpp_buff_mngr = &(dpufd->effect_info[disp_panel_id].dpp_buf);

	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("[effect] fb%d, panel is power off!\n", dpufd->index);
		dpp_buff_mngr->gmp_call_wq_scene |= GMP_PANEL_OFF_SCENE;
		up(&dpufd->blank_sem_effect_gmp);
		return;
	}

	dpufb_activate_vsync(dpufd);
	down(&dpufd->effect_gmp_sem);

	ch_status = dpufd->effect_info[disp_panel_id].dpp_chn_status;
	buff_sel = dpp_buff_mngr->latest_buf[disp_panel_id];

	if (buff_sel >= DPP_BUF_MAX_COUNT) {
		DPU_FB_INFO("[effect] select buffer %d\n", buff_sel);
		goto exit;
	}

	if (dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].dpp_config_mode != DPP_BUF_WORKQ_CONFIG) {
		DPU_FB_INFO("[effect] dpp buffer config mode %d is not correct\n",
			dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].dpp_config_mode);
		goto exit;
	}

	effect_dbm_dpp_buf_lock(buff_sel, disp_panel_id);

	/* check dpp config status, make sure both dpp is connectted */
	effect_dpp_ch_both_connect(dpufd, disp_glb_base);

	/* chosse channel to config effect */
	dst_channel = (g_dyn_sw_default & 0x1) ? 0 : 1;
	effect_dbm_dpp_chn_lock(dst_channel);
	gmp_mode = dpp_buff_mngr->buf_info_list[disp_panel_id][buff_sel].gmp.gmp_mode;
	DPU_FB_DEBUG("[effect]1 buf_sel=%d, dst_channel=%d, dst_ch_status=%d,"
		"another_ch_status=%d, gmp_mode=%d, gmp_call_wq_scene=%d\n",
		buff_sel, dst_channel, *(ch_status + dst_channel),
		*(ch_status + ((~dst_channel) & 0x1)), gmp_mode,
		dpp_buff_mngr->gmp_call_wq_scene);

	if (*(ch_status + dst_channel) != DPP_CHN_UPDATE_READY ||
		g_dyn_sw_default != inp32(disp_glb_base + DYN_SW_DEFAULT)) {
		DPU_FB_DEBUG("this channel status is %d,not ready for config!\n",
			*(ch_status + dst_channel));
		if ((gmp_mode & GMP_ENABLE) && (gmp_mode & GMP_LAST_PARAM_SCENE))
			dpp_buff_mngr->gmp_call_wq_scene |= GMP_LAST_PARAM_SCENE;
		effect_dbm_dpp_chn_unlock(dst_channel);
		effect_dbm_dpp_buf_unlock(buff_sel, disp_panel_id);
		goto exit;
	}

	*(ch_status + dst_channel) = DPP_CHN_CONFIG_DOING;

	if (effect_dpp_ch_config_degamma(dpufd, dst_channel, buff_sel))
		goto err_ret;

	if (effect_dpp_ch_config_gama(dpufd, dst_channel, buff_sel))
		goto err_ret;

	if (effect_dpp_ch_config_xcc(dpufd, dst_channel, buff_sel, false))
		goto err_ret;

	if (effect_dpp_ch_config_gmp(dpufd, dst_channel, buff_sel))
		goto err_ret;

	*(ch_status + dst_channel) = DPP_CHN_CONFIG_READY;
	*(ch_status + ((~dst_channel) & 0x1)) = DPP_CHN_NEED_CONFIG;
	dpp_buff_mngr->gmp_call_wq_scene = 0;

	DPU_FB_DEBUG("[effect]2 buf_sel=%d, dst_channel=%d, dst_ch_status=%d,"
		"another_ch_status=%d, gmp_mode=%d, gmp_call_wq_scene=%d\n",
		buff_sel, dst_channel, *(ch_status + dst_channel),
		*(ch_status + ((~dst_channel) & 0x1)), gmp_mode,
		dpp_buff_mngr->gmp_call_wq_scene);

	effect_dbm_dpp_chn_unlock(dst_channel);
	effect_dbm_dpp_buf_unlock(buff_sel, disp_panel_id);

	goto exit;
err_ret:
	*(ch_status + dst_channel) = DPP_CHN_NEED_CONFIG;
	DPU_FB_DEBUG("[effect]3 buf_sel=%d, dst_channel=%d, dst_ch_status=%d,"
		"another_ch_status=%d, gmp_mode=%d, gmp_call_wq_scene=%d\n",
		buff_sel, dst_channel, *(ch_status + dst_channel),
		*(ch_status + ((~dst_channel) & 0x1)), gmp_mode,
		dpp_buff_mngr->gmp_call_wq_scene);

	effect_dbm_dpp_chn_unlock(dst_channel);
	effect_dbm_dpp_buf_unlock(buff_sel, disp_panel_id);

exit:
	up(&dpufd->effect_gmp_sem);
	dpufb_deactivate_vsync(dpufd);
	up(&dpufd->blank_sem_effect_gmp);
}

void dpufb_effect_gmp_lut_workqueue_handler(struct work_struct *work)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (work == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}
	dpufd = container_of(work, struct dpu_fb_data_type, gmp_lut_work);
	if (dpufd == NULL) {
		DPU_FB_ERR("[effect] dpufd is NULL\n");
		return;
	}

	dpufb_effect_dpp_config(dpufd, false);
}

void dpu_effect_color_dimming_acm_reg_init(struct dpu_fb_data_type *dpufd)
{
	(void)dpufd;
}

#define ARSR1P_MIN_SIZE 16
#define ARSR1P_MAX_SIZE 8192
#define ARSR1P_MAX_SRC_WIDTH_SIZE 3840

static void effect_arsr_pre_config_param(struct dpu_fb_data_type *dpufd, struct dss_arsr2p *arsr2p)
{
	int disp_panel_id = dpufd->panel_info.disp_panel_id;

	if (dpufd->effect_info[disp_panel_id].arsr2p[0].update == 1) {
		memcpy(&(arsr2p->arsr2p_effect),
			&(dpufd->effect_info[disp_panel_id].arsr2p[0]), sizeof(struct arsr2p_info));
		dpufd->effect_info[disp_panel_id].arsr2p[0].update = 0;
	}

	if (dpufd->effect_info[disp_panel_id].arsr2p[1].update == 1) {
		memcpy(&(arsr2p->arsr2p_effect_scale_up),
			&(dpufd->effect_info[disp_panel_id].arsr2p[1]), sizeof(struct arsr2p_info));
		dpufd->effect_info[disp_panel_id].arsr2p[1].update = 0;
	}

	if (dpufd->effect_info[disp_panel_id].arsr2p[2].update == 1) {
		memcpy(&(arsr2p->arsr2p_effect_scale_down),
			&(dpufd->effect_info[disp_panel_id].arsr2p[2]), sizeof(struct arsr2p_info));
		dpufd->effect_info[disp_panel_id].arsr2p[2].update = 0;
	}
}

int dpu_effect_arsr2p_config(struct arsr2p_info *arsr2p_effect_dst,
	int ih_inc, int iv_inc)
{
	struct dpu_fb_data_type *dpufd_primary = NULL;
	struct dss_arsr2p *arsr2p = NULL;

	dpufd_primary = dpufd_list[PRIMARY_PANEL_IDX];
	if (dpufd_primary == NULL) {
		DPU_FB_ERR("[effect]dpufd_primary is NULL pointer, return!\n");
		return -EINVAL;
	}

	if (arsr2p_effect_dst == NULL) {
		DPU_FB_ERR("[effect]arsr2p_effect_dst is NULL pointer, return!\n");
		return -EINVAL;
	}

	arsr2p = &(dpufd_primary->dss_module_default.arsr2p[DSS_RCHN_V0]);

	effect_arsr_pre_config_param(dpufd_primary, arsr2p);

	if ((ih_inc == ARSR2P_INC_FACTOR) &&
		(iv_inc == ARSR2P_INC_FACTOR)) {
		memcpy(arsr2p_effect_dst,
			&(arsr2p->arsr2p_effect), sizeof(struct arsr2p_info));
	} else if ((ih_inc < ARSR2P_INC_FACTOR) ||
		(iv_inc < ARSR2P_INC_FACTOR)) {
		memcpy(arsr2p_effect_dst,
			&(arsr2p->arsr2p_effect_scale_up), sizeof(struct arsr2p_info));
	} else {
		memcpy(arsr2p_effect_dst,
			&(arsr2p->arsr2p_effect_scale_down), sizeof(struct arsr2p_info));
	}

	return 0;
}

void deinit_effect(struct dpu_fb_data_type *dpufd)
{
	uint32_t *ch_status = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	ch_status = dpufd->effect_info[dpufd->panel_info.disp_panel_id].dpp_chn_status;
	*ch_status = DPP_CHN_NEED_CONFIG;
	*(ch_status + 1) = DPP_CHN_NEED_CONFIG;
	g_dyn_sw_default = 0x0;
	dpufd->effect_info[dpufd->panel_info.disp_panel_id].dpp_buf.gmp_call_wq_scene = 0;
}

/* lint +e571, +e573, +e737, +e732, +e850, +e730, +e713, +e574, +e732, +e845, +e570,
+e774 +e568 +e587 +e685 */
#pragma GCC diagnostic pop
