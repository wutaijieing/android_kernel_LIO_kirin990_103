/*
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include "dpu_hihdr.h"
#include "dpu_hihdr_itm.h"
#include "dpu_hihdr_gtm.h"

#define HIHDR_MEAN_LIGHT 384

static void dpu_hihdr_top_init(struct dpu_fb_data_type *dpufd, char __iomem *hdr_base)
{
	uint64_t inv_size;

	dpu_check_and_no_retval(!hdr_base, ERR, "hdr_base is NULL\n");
	dpu_check_and_no_retval(!dpufd->panel_info.xres, ERR, "panel xres is 0\n");
	dpu_check_and_no_retval(!dpufd->panel_info.yres, ERR, "panel yres is 0\n");

	inv_size = (1UL << 36) / dpufd->panel_info.xres / dpufd->panel_info.yres;
	inv_size = inv_size > 0x3ffff ? 0x3ffff : inv_size; // 0x3ffff is the max number.

	/* HIHDR CONTROL */
	set_reg(hdr_base + DSS_HDR_RD_SHADOW, 0x0, 32, 0);
	set_reg(hdr_base + DSS_HDR_REG_DEFAULT, 0x0, 32, 0);
	set_reg(hdr_base + DSS_HDR_MEM_READ, 0x0, 32, 0);
	set_reg(hdr_base + DSS_HDR_CLK_SEL, 0xFFFFFFFF, 32, 0);
	set_reg(hdr_base + DSS_HDR_MEM_CTRL, 0x88888, 32, 0);
	set_reg(hdr_base + DSS_HDR_CLK_EN, 0xFFFFFFFF, 32, 0);
	set_reg(hdr_base + DSS_HDR_CTROL, 0x0, 32, 0);

	/* HIHDR_ROI set left,top > right,bottom to disable ROI */
	set_reg(hdr_base + DSS_HDR_ROI_WIN1_STA, 0x10001, 32, 0);
	set_reg(hdr_base + DSS_HDR_ROI_WIN1_END, 0, 32, 0);
	set_reg(hdr_base + DSS_HDR_ROI_WIN2_STA, 0x10001, 32, 0);
	set_reg(hdr_base + DSS_HDR_ROI_WIN2_END, 0, 32, 0);
	set_reg(hdr_base + DSS_HDR_STAT_WIN1_STA, 0x10001, 32, 0);
	set_reg(hdr_base + DSS_HDR_STAT_WIN1_END, 0, 32, 0);
	set_reg(hdr_base + DSS_HDR_STAT_WIN2_STA, 0x10001, 32, 0);
	set_reg(hdr_base + DSS_HDR_STAT_WIN2_END, 0, 32, 0);

	set_reg(hdr_base + DSS_HDR_INV_SIZE, inv_size, 32, 0 );
	set_reg(hdr_base + DSS_HDR_STAT_WIN1_INV_SIZE, 0x0, 32, 0 );
	set_reg(hdr_base + DSS_HDR_STAT_WIN2_INV_SIZE, 0x0, 32, 0 );
}

void init_hihdr(struct dpu_fb_data_type *dpufd)
{
	char __iomem *hdr_base = NULL;
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL!\n");

	if (g_debug_effect_hihdr)
		DPU_FB_INFO("[hihdr] init");

	dpufd->hihdr_mean = HIHDR_MEAN_LIGHT;
	dpufd->hihdr_basic_ready = 0;

	if (dpufd->hdr_info != NULL)
		memset(dpufd->hdr_info, 0x0, sizeof(dss_hihdr_info_t));

	hdr_base = dpufd->dss_base + DSS_HDR_OFFSET;

	dpu_hihdr_top_init(dpufd, hdr_base);
	dpu_hihdr_itm_init(dpufd, hdr_base);

	/* for each module */
	set_reg(hdr_base + DSS_LTM_CTROL, 0x0, 32, 0);
	set_reg(hdr_base + DSS_ITM_SLF_CTRL, 0x0, 32, 0);
	set_reg(hdr_base + DSS_GTM_SLF_CTRL, 0x0, 32, 0);
}

void deinit_hihdr(struct dpu_fb_data_type *dpufd)
{
	if (g_debug_effect_hihdr)
		DPU_FB_INFO("[hihdr] deinit");

	dpufd->hihdr_mean = HIHDR_MEAN_LIGHT;
	dpufd->hihdr_basic_ready = 0;

	if (dpufd->hdr_info != NULL)
		memset(dpufd->hdr_info, 0x0, sizeof(dss_hihdr_info_t));
}

void dpu_get_hihdr_mean(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL!\n");

	dpufd->hihdr_mean = inp32(dpufd->dss_base + DSS_HDR_OFFSET + DSS_LTM_MEAN_STT);

	if (g_debug_effect_hihdr)
		DPU_FB_INFO("[hihdr] mean = %d", dpufd->hihdr_mean);
}

static void dpu_hihdr_global_control(struct dpu_fb_data_type *dpufd, dss_hihdr_info_t *hdr_info)
{
	char __iomem *hdr_base = NULL;
	hdr_base = dpufd->dss_base + DSS_HDR_OFFSET;

	/* Default status: sdr */
	if ((hdr_info->itm_info.flag == SDR_EH_CLOSE || hdr_info->itm_info.flag == SDR_EH_DEFAULT) &&
		hdr_info->hihdr_status == SDR) {
		dpufd->set_reg(dpufd, hdr_base + DSS_LTM_CTROL, 0x0, 32, 0);
		dpufd->hihdr_mean = HIHDR_MEAN_LIGHT;
		if (hdr_info->itm_info.csc_flag == CSC_DEFAULT || hdr_info->itm_info.csc_flag == CSC_2_P3_STOP)
			dpufd->set_reg(dpufd, hdr_base + DSS_HDR_CTROL, 0x0, 32, 0);
		else
			dpufd->set_reg(dpufd, hdr_base + DSS_HDR_CTROL, 0x1, 32, 0);
	} else {
		dpufd->set_reg(dpufd, hdr_base + DSS_HDR_CTROL, 0x1, 32, 0);
		dpufd->set_reg(dpufd, hdr_base + DSS_LTM_CTROL, 0x1, 32, 0);
	}

	if (hdr_info->hihdr_status == SDR) {
		dpufd->set_reg(dpufd, hdr_base + DSS_GTM_SLF_CTRL, 0x0, 32, 0);
		dpufd->set_reg(dpufd, hdr_base + DSS_GTM_ROI, 0x0, 32, 0);
	}

	if (hdr_info->hihdr_status == HDR) {
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_SLF_CTRL, 0x0, 32, 0);
		dpufd->set_reg(dpufd, hdr_base + DSS_ITM_ROI, 0x0, 32, 0);
	}
}

void dpu_hihdr_set_reg(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	dpu_check_and_no_retval((dpufd == NULL), ERR, "dpufd is NULL\n");

	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;

	pinfo = &(dpufd->panel_info);
	if (pinfo->post_hihdr_support == 0)
		return;

	dpu_check_and_no_retval((dpufd->hdr_info == NULL), ERR, "hdr_info is NULL\n");

	dpu_hihdr_global_control(dpufd, dpufd->hdr_info);

	if (g_debug_effect_hihdr)
		DPU_FB_INFO("[hihdr] hihdr_status = %d", dpufd->hdr_info->hihdr_status);

	dpu_hihdr_itm_set_reg(dpufd, &dpufd->hdr_info->itm_info);
	dpu_hihdr_gtm_set_reg(dpufd, &dpufd->hdr_info->gtm_info);
}

bool dpu_check_hihdr_active(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	dpu_check_and_return((dpufd == NULL), false, ERR, "dpufd is NULL!\n");

	if (dpufd->index != PRIMARY_PANEL_IDX)
		return false;

	pinfo = &(dpufd->panel_info);

	if (pinfo->post_hihdr_support == 0)
		return false;

	dpu_check_and_return((dpufd->hdr_info == NULL), false, ERR, "hdr_info is NULL!\n");

	if ((dpufd->hdr_info->itm_info.csc_flag == CSC_2_P3_START) ||
		(dpufd->hdr_info->itm_info.csc_flag == CSC_2_P3_STOP))
		return true;

	if ((dpufd->hdr_info->itm_info.flag == SDR_EH_BASIC) ||
		(dpufd->hdr_info->itm_info.flag == SDR_EH_DYNAMIC) ||
		(dpufd->hdr_info->itm_info.flag == SDR_EH_CLOSE))
		return true;

	if ((dpufd->hdr_info->gtm_info.flag == HDR_BASIC) ||
		(dpufd->hdr_info->gtm_info.flag == HDR_DYNAMIC) ||
		(dpufd->hdr_info->gtm_info.flag == HDR_CLOSE))
		return true;

	return false;
}

