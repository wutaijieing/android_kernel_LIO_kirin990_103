/* Copyright (c) 2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "dsc_config_base.h"
#include "../dp_core_interface.h"
#include "../hidptx/hidptx_dp_avgen.h"
#include "../hidptx/hidptx_dp_core.h"
#include "../hidptx/hidptx_reg.h"
#include "dsc/dsc_algorithm.h"
#include "dsc/dsc_algorithm_manager.h"

#define DSS_DSC_MAX_LINE_BUF_DEP 11
#define DSS_SLICE_HEIGHT_LIMIT 2048
#define PIC_HEIGHT_DIV_INIT 1
#define DP_BIT_PER_PIXEL_CLOCK 48
#define DSC_CLOCK_DIV_MIN 2

#define round1(x, y)  ((x) / (y) + ((x) % (y)  ? 1 : 0))

int dptx_slice_height_limit(struct dp_ctrl *dptx, uint32_t pic_height)
{
	int divide_val;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] NULL Pointer\n");
	divide_val = PIC_HEIGHT_DIV_INIT;

	disp_pr_info("[DP] %s pic_height = %x\n", __func__, pic_height);

	while (!((pic_height / divide_val) < DSS_SLICE_HEIGHT_LIMIT))
		divide_val++; /* divide to 2 */

	disp_pr_debug("[DP] divide_val = %d\n", divide_val);

	return divide_val;
}

int dptx_line_buffer_depth_limit(uint8_t line_buf_depth)
{
	if (line_buf_depth > DSS_DSC_MAX_LINE_BUF_DEP)
		line_buf_depth = DSS_DSC_MAX_LINE_BUF_DEP;

	return line_buf_depth;
}

int dptx_dsc_get_clock_div(struct dp_ctrl *dptx)
{
	uint32_t dsc_bpp;
	uint16_t h_total;
	int div_temp;
	struct dtd *mdtd = NULL;
	struct video_params *vparams = NULL;
	int div_max;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL\n");

	if (!dptx->dsc) {
		disp_pr_err("[DP] this function cannot be called in non-DSC mode\n");
		return -EINVAL;
	}

	vparams= &(dptx->vparams);
	mdtd = &(dptx->vparams.mdtd);
	dsc_bpp = vparams->dp_dsc_info.dsc_info.dsc_bpp;

	if (dsc_bpp == 0) {
		disp_pr_err("[DP] dsc_bpp cannot be zero\n");
		return -EINVAL;
	}
	h_total = mdtd->h_active + mdtd->h_blanking;
	div_max = DP_BIT_PER_PIXEL_CLOCK / dsc_bpp;

	/* fpga only support div 2 */
	if (g_fpga_flag == 1) {
		return DSC_CLOCK_DIV_MIN;
	}

	for (div_temp = div_max; div_temp >= DSC_CLOCK_DIV_MIN; div_temp--) {
		if (h_total % div_temp == 0) {
			if (div_temp == DSC_CLOCK_DIV_MIN)
				disp_pr_err("[DP] div is 2, unable to compress bandwidth\n");
			return div_temp;
		}
	}

	return -EINVAL;
}

static void ldi_clock_delay_calculation(struct dp_ctrl *dptx)
{
	struct dtd *mdtd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	struct video_params *vparams = NULL;
	uint32_t h_active_new;
	uint32_t h_blank_new;
	uint32_t h_front_porch_new;
	uint32_t h_sync_width_new;
	uint32_t h_back_porch_new;
	uint32_t h_clock_div;

	disp_check_and_no_retval((dptx->connector_dev == NULL), err, "[DP] NULL Pointer\n");
	pinfo = dptx->connector_dev->fix_panel_info;
	disp_check_and_no_retval((pinfo == NULL), err, "[DP] NULL Pointer\n");
	disp_pr_debug("[DP] +\n");

	h_clock_div = (uint32_t)dptx_dsc_get_clock_div(dptx);
	disp_check_and_no_retval((h_clock_div == 0), err, "get clock div failed\n");

	vparams = &(dptx->vparams);
	mdtd = &(dptx->vparams.mdtd);

	if (vparams->dp_dsc_info.dsc_info.slice_width == 0) {
		disp_pr_err("[DP] slice_width cannot be zero");
		return;
	}
	vparams->dp_dsc_info.encoders =
		vparams->dp_dsc_info.dsc_info.pic_width / vparams->dp_dsc_info.dsc_info.slice_width;
	pinfo->ifbc_type = dptx->dsc_ifbc_type;
	pinfo->timing_info.pxl_clk_rate_div = 1;
	h_active_new = vparams->dp_dsc_info.encoders * round1(vparams->dp_dsc_info.dsc_info.chunk_size, 6);
	h_blank_new = (mdtd->h_active + mdtd->h_blanking) / h_clock_div - h_active_new;
	h_front_porch_new = round1(mdtd->h_sync_offset, h_clock_div);
	h_sync_width_new = round1(mdtd->h_sync_pulse_width, h_clock_div);
	h_back_porch_new = h_blank_new - h_front_porch_new - h_sync_width_new;

	disp_pr_debug("[HiDP] h_active_new is %d\n", h_active_new);
	disp_pr_debug("[HiDP] h_blank_new is %d\n", h_blank_new);
	disp_pr_debug("[HiDP] h_front_porch_new is %d\n", h_front_porch_new);
	disp_pr_debug("[HiDP] h_sync_width_new is %d\n", h_sync_width_new);
	disp_pr_debug("[HiDP] h_back_porch_new is %d\n", h_back_porch_new);

	pinfo->ldi.h_front_porch = h_front_porch_new * 2;
	pinfo->ldi.h_pulse_width = h_sync_width_new * 2;
	pinfo->ldi.h_back_porch = h_back_porch_new * 2;
	dptx->vparams.dp_dsc_info.h_active_new = h_active_new * 2;
	pinfo->panel_dsc_info.dsc_info.output_width = dptx->vparams.dp_dsc_info.h_active_new;
	pinfo->panel_dsc_info.dsc_info.output_height = mdtd->v_active;
	disp_pr_info("%u*%u\n", pinfo->panel_dsc_info.dsc_info.output_width, pinfo->panel_dsc_info.dsc_info.output_height);
	disp_pr_info("%d*%d\n", pinfo->panel_dsc_info.dsc_info.output_width, pinfo->panel_dsc_info.dsc_info.output_height);
}

void dptx_dsc_dss_config(struct dp_ctrl *dptx)
{
	struct dsc_algorithm_manager *p_dsc = NULL;
	struct hisi_panel_info *pinfo = NULL;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");
	disp_check_and_no_retval((dptx->connector_dev == NULL), err, "[DP] NULL Pointer\n");
	pinfo = dptx->connector_dev->fix_panel_info;
	disp_check_and_no_retval((pinfo == NULL), err, "[DP] NULL Pointer\n");

	p_dsc = get_dsc_algorithm_manager_instance();

	if (p_dsc == NULL) {
		disp_pr_err("[DP] get_dsc_instance NULL Pointer\n");
		return;
	}
	p_dsc->vesa_dsc_info_calc(NULL, &(pinfo->panel_dsc_info.dsc_info),
		&(dptx->vparams.dp_dsc_info.dsc_info));

	ldi_clock_delay_calculation(dptx);
}

void dptx_dsc_sdp_manul_send(struct dp_ctrl *dptx, bool enable)
{
	uint32_t reg;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	if (enable) {
		/* manual dsc sdp send */
		reg = dptx_readl(dptx, DPTX_SDP_CTRL(0));
		reg |= DPTX_CFG_STREAM_PPS_SDP_ENABLE;
		dptx_writel(dptx, DPTX_SDP_CTRL(0), reg);
	}
}

void dptx_dsc_enable(struct dp_ctrl *dptx, int stream)
{
	uint32_t reg;

	disp_check_and_no_retval(!dptx, err, "[DP] NULL Pointer\n");

	reg = dptx_readl(dptx, DPTX_VIDEO_CTRL_STREAM(stream));
	/* configure dsc */
	reg |= DPTX_CFG_STREAM_DSC_ENABLE; /* enable dsc */
	reg |= DPTX_CFG_STREAM_DSC_CRC_ENABLE; /* enable dsc crc */

	dptx_writel(dptx, DPTX_VIDEO_CTRL_STREAM(stream), reg);
}

static void dptx_dsc_calc_timestamp(struct dp_ctrl *dptx)
{
	uint32_t reg;

	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	/* Configure chunk size and time unit */
	reg = dptx_readl(dptx, DPTX_STREAM_DSC_CRC_CTRL(0));
	reg &= ~CFG_STREAM_CHUNK_SIZE;
	reg &= ~CFG_STREAM_SLICES_NUM;
	reg &= ~CFG_STREAM_TIMESTAMP_UNIT;
	disp_pr_debug("[HIDP dsc] chunk size is %d\n", dptx->vparams.dp_dsc_info.dsc_info.chunk_size);
	reg |= dptx->vparams.dp_dsc_info.dsc_info.chunk_size << 0;
	reg |= (dptx->vparams.dp_dsc_info.dsc_info.pic_width / dptx->vparams.dp_dsc_info.dsc_info.slice_width) << 16;
	reg |= (uint32_t)dptx_dsc_get_clock_div(dptx) << 19;
	disp_pr_err("[HIDP dsc] 0x25c write reg is %x\n", reg);
	dptx_writel(dptx, DPTX_STREAM_DSC_CRC_CTRL(0), reg);
}

void dptx_dsc_cfg(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	dptx_dsc_notice_rx(dptx);
	dptx_dsc_calc_timestamp(dptx);

	/* Program PPS table */
	dptx_program_pps_sdps(dptx);
}
