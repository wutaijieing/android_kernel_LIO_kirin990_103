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

#include "hdmirx_video_dsc.h"
#include "hdmirx_common.h"
#include <linux/delay.h>
#include "hdmirx_init.h"
#include "control/hdmirx_ctrl.h"

typedef enum {
	DSC_PROC_HW_STATUS,
	DSC_PROC_PPS,
	DSC_PROC_MAX
} dsc_proc_name;

int hdmirx_video_dsc_mask(struct hdmirx_ctrl_st *hdmirx)
{
	set_reg(hdmirx->hdmirx_sysctrl_base + 0x40, 0x1, 1, 1);
	return 0;
}

static void hdmirx_dsc_get_pps(struct hdmirx_ctrl_st *hdmirx, hdmirx_dsc_pps_info *pps)
{
	pps->dsc_version_major = inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 0 * 0x4) & 0xf;
	pps->dsc_version_minor = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 0 * 0x4) & 0xf0) >> 4 ;
	pps->pps_identifier    = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 0 * 0x4) & 0xff00) >> 8 ;
	pps->bits_per_component = (((inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 0 * 0x4) & 0xff000000) >> 24) & 0xf0) >> 4 ;
	pps->linebuf_depth     = ((inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 0 * 0x4) & 0xff000000) >> 24) & 0xf;
	pps->block_pred_enable = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 1 * 0x4) >> 5) & 0x1;
	pps->convert_rgb       = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 1 * 0x4) >> 4) & 0x1;
	pps->simple_422        = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 1 * 0x4) >> 3) & 0x1;
	pps->vbr_enable        = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 1 * 0x4) >> 2) & 0x1;
	pps->bits_per_pixel    = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 1 * 0x4) & 0x3) << 8;
	pps->bits_per_pixel   |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 1 * 0x4) & 0xff00) >> 8;
	pps->pic_height        = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 1 * 0x4) & 0xff0000) >> 8;
	pps->pic_height       |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 1 * 0x4) & 0xff000000) >> 24;
	pps->pic_width         = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 2 * 0x4) & 0xff) << 8;
	pps->pic_width        |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 2 * 0x4) & 0xff00) >> 8;
	pps->slice_heigh       = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 2 * 0x4) & 0xff0000) >> 8;
	pps->slice_heigh      |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 2 * 0x4) & 0xff000000) >> 24;
	pps->slice_width       = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 3 * 0x4) & 0xff) << 8;
	pps->slice_width      |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 3 * 0x4) & 0xff00) >> 8;
	pps->chunk_size        = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 3 * 0x4) & 0xff0000) >> 8;
	pps->chunk_size       |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 3 * 0x4) & 0xff000000) >> 24;
	pps->initial_scale_value = ((inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 4 * 0x4) & 0xff00) >> 8) & 0x3f;
	pps->scale_increment_interval  = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 4 * 0x4) & 0xff0000) >> 8;
	pps->scale_increment_interval |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 4 * 0x4) & 0xff000000) >> 24;
	pps->scale_decrement_interval  = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 5 * 0x4) & 0xff) << 8;
	pps->scale_decrement_interval |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 5 * 0x4) & 0xff00) >> 8;
	pps->first_line_bpg_offset     = ((inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 5 * 0x4) & 0xff0000) >> 24) & 0x1f;
	pps->nfl_bpg_offset            = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 6 * 0x4) & 0xff) << 8;
	pps->nfl_bpg_offset           |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 6 * 0x4) & 0xff00) >> 8;
	pps->slice_bpg_offset          = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 6 * 0x4) & 0xff0000) >> 8;
	pps->initial_offset           |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 6 * 0x4) & 0xff000000) >> 24;
	pps->final_offset              = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 7 * 0x4) & 0xff0000) >> 8;
	pps->final_offset             |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 7 * 0x4) & 0xff000000) >> 24;
	pps->flatness_min_qp = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 8 * 0x4) & 0xff) & 0x1f;
	pps->flatness_max_qp = ((inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 8 * 0x4) & 0xff00) >> 8) & 0x1f;
	pps->native_420 = ((inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 21 * 0x4) & 0xff) >> 1) & 0x1;
	pps->native_422 = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 21 * 0x4) & 0xff) & 0x1;
	pps->second_line_bpg_offset    = ((inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 21 * 0x4) & 0xff00) >> 8) & 0x1f;
	pps->nsl_bpg_offset            = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 21 * 0x4) & 0xff0000) >> 8;
	pps->nsl_bpg_offset           |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 21 * 0x4) & 0xff000000) >> 24;
	pps->second_line_offset_adj    = (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 22 * 0x4) & 0xff) << 8;
	pps->second_line_offset_adj   |= (inp32(hdmirx->hdmirx_sysctrl_base + 0x88 + 22 * 0x4) & 0xff00) >> 8;

	disp_pr_info( "%-16s: %-10u| ", "dsc_ver_major", pps->dsc_version_major);
	disp_pr_info( "%-16s: %-10u| ", "dsc_ver_minor", pps->dsc_version_minor);
	disp_pr_info( "%-16s: %-10u| ", "pps_identifiter", pps->pps_identifier);

	/* line 2 */
	disp_pr_info( "%-16s: %-10u| ", "linebuf_depth", pps->linebuf_depth);
	disp_pr_info( "%-16s: %-10u| ", "block_pred_en", pps->block_pred_enable);
	disp_pr_info( "%-16s: %-10s| ", "convert_rgb", pps->convert_rgb ? S_YES : S_NO);
	disp_pr_info( "%-16s: %-10s\n", "simple_422", pps->simple_422 ? S_YES : S_NO);

	/* line 3 */
	disp_pr_info( "%-16s: %-10s| ", "vbr_enable", pps->vbr_enable ? S_YES : S_NO);
	disp_pr_info( "%-16s: %-10u| ", "bits_per_pixel", pps->bits_per_pixel);
	disp_pr_info( "%-16s: %-10u| ", "pic_width", pps->pic_width);
	disp_pr_info( "%-16s: %-10u\n", "pic_height", pps->pic_height);

	/* line 4 */
	disp_pr_info( "%-16s: %-10u| ", "slice_width", pps->slice_width);
	disp_pr_info( "%-16s: %-10u| ", "slice_heigh", pps->slice_heigh);
	disp_pr_info( "%-16s: %-10u| ", "chunk_size", pps->chunk_size);
	disp_pr_info( "%-16s: %-10u\n", "init_xmit_delay", pps->initial_xmit_delay);

	/* line 5 */
	disp_pr_info( "%-16s: %-10u| ", "init_dec_delay", pps->initial_dec_delay);
	disp_pr_info( "%-16s: %-10u| ", "init_scale_val", pps->initial_scale_value);
	disp_pr_info( "%-16s: %-10u| ", "scale_inc_int", pps->scale_increment_interval);
	disp_pr_info( "%-16s: %-10u\n", "scale_dec_int", pps->scale_decrement_interval);

	/* line 6 */
	disp_pr_info( "%-16s: %-10u| ", "first_bpg_off", pps->first_line_bpg_offset);
	disp_pr_info( "%-16s: %-10u| ", "nfl_bpg_offset", pps->nfl_bpg_offset);
	disp_pr_info( "%-16s: %-10u| ", "slice_bpg_off", pps->slice_bpg_offset);
	disp_pr_info( "%-16s: %-10u\n", "initial_offset", pps->initial_offset);

	/* line 7 */
	disp_pr_info( "%-16s: %-10u| ", "final_offset", pps->final_offset);
	disp_pr_info( "%-16s: %-10u| ", "flat_min_qp", pps->flatness_min_qp);
	disp_pr_info( "%-16s: %-10u| ", "flat_max_qp", pps->flatness_max_qp);
	disp_pr_info("%-16s: %-10u\n", "sec_bpg_off", pps->second_line_bpg_offset);

	/* line 8 */
	disp_pr_info( "%-16s: %-10s| ", "native_420", pps->native_420 ? S_YES : S_NO);
	disp_pr_info( "%-16s: %-10s| ", "native_422", pps->native_422 ? S_YES : S_NO);
	disp_pr_info( "%-16s: %-10u| ", "nsl_bpg_offset", pps->nsl_bpg_offset);
	disp_pr_info( "%-16s: %-10u\n", "sec_offset_adj", pps->second_line_offset_adj);

	disp_pr_info( "\n");
}

int hdmirx_video_dsc_irq_proc(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t temp;
	hdmirx_dsc_pps_info pps = {0};

	temp = inp32(hdmirx->hdmirx_sysctrl_base + 0x44);
	if (!(temp & 0x2)) {
		disp_pr_warn("not dsc pps irq");
		return 0;
	}
	disp_pr_warn("dsc pps irq");
	hdmirx_dsc_get_pps(hdmirx, &pps);

	return 0;
}