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

#include <linux/module.h>
#include <linux/types.h>
#include "hisi_disp_debug.h"

uint32_t g_vsync_cnt;

uint32_t g_disp_log_level = DISP_LOG_LVL_INFO;
uint32_t g_debug_layerbuf_log = 1;

uint32_t  g_ld_wch_vaddr = 0;
uint32_t  g_ld_continue_frm_wb = 0;

uint32_t  g_ld_stt2d_width = 0; // defaut 12bit, 1 is 14bit
uint32_t  g_ld_led_numv = 1;
uint32_t  g_ld_led_numh = 1;
uint32_t  g_ld_sync_sel = 1;
uint32_t  g_ld_sync_en = 1;
uint32_t  g_ld_dither_en = 0;
uint32_t g_ld_wch_last_vaddr = 0;
uint32_t g_ld_frm_cnt = 11;

dss_layer_extend g_dss_layer_extend[MAX_DSS_SRC_NUM] = {0}; // self test

#ifdef CONFIG_DKMD_DPU_DEBUG
module_param_named(disp_log_level, g_disp_log_level, int, 0644);
MODULE_PARM_DESC(disp_log_level, "disp log level");

module_param_named(ld_continue_frm_wb, g_ld_continue_frm_wb, int, 0644);
MODULE_PARM_DESC(ld_continue_frm_wb, "ld_continue_frm_wb");

module_param_named(ld_stt2d_width, g_ld_stt2d_width, int, 0644);
MODULE_PARM_DESC(ld_stt2d_width, "ld_stt2d_width");

module_param_named(ld_led_numv, g_ld_led_numv, int, 0644);
MODULE_PARM_DESC(ld_led_numv, "ld_led_numv");

module_param_named(ld_led_numh, g_ld_led_numh, int, 0644);
MODULE_PARM_DESC(ld_led_numh, "ld_led_numh");

module_param_named(ld_sync_sel, g_ld_sync_sel, int, 0644);
MODULE_PARM_DESC(ld_sync_sel, "ld_sync_sel");

module_param_named(ld_sync_en, g_ld_sync_en, int, 0644);
MODULE_PARM_DESC(ld_sync_en, "ld_sync_en");

module_param_named(ld_dither_en, g_ld_dither_en, int, 0644);
MODULE_PARM_DESC(ld_dither_en, "g_ld_dither_en");

module_param_named(ld_wch_last_vaddr, g_ld_wch_last_vaddr, int, 0644);
MODULE_PARM_DESC(ld_wch_last_vaddr, "g_ld_wch_last_vaddr");

module_param_named(ld_frm_cnt, g_ld_frm_cnt, int, 0644);
MODULE_PARM_DESC(ld_frm_cnt, "g_ld_frm_cnt");
#endif

void set_dss_layer_src_type(uint32_t layer_id, uint32_t type)
{
	if (layer_id >= MAX_DSS_SRC_NUM) {
		disp_pr_err("err layer id : %d\n", layer_id);
		return;
	}

	g_dss_layer_extend[layer_id].src_type = type;
}
EXPORT_SYMBOL_GPL(set_dss_layer_src_type); // self test

void set_dss_layer_dsd_index(uint32_t layer_id, uint32_t index)
{
	if (layer_id >= MAX_DSS_SRC_NUM) {
		disp_pr_err("err layer id : %d\n", layer_id);
		return;
	}

	g_dss_layer_extend[layer_id].dsd_index = index;
}
EXPORT_SYMBOL_GPL(set_dss_layer_dsd_index); // self test

uint32_t get_dss_layer_src_type(uint32_t layer_id) // self test
{
	if (layer_id >= MAX_DSS_SRC_NUM) {
		disp_pr_err("get err layer id : %d\n", layer_id);
		return 0;
	}
	return g_dss_layer_extend[layer_id].src_type;
}

uint32_t get_dss_layer_dsd_index(uint32_t layer_id) // self test
{
	if (layer_id >= MAX_DSS_SRC_NUM) {
		disp_pr_err("get err layer id: %d\n", layer_id);
		return 0;
	}
	return g_dss_layer_extend[layer_id].dsd_index;
}


void set_debug_level(uint32_t level)
{
	g_disp_log_level = level;
}
EXPORT_SYMBOL_GPL(set_debug_level);

uint32_t get_ld_wch_vaddr(void)
{
	return g_ld_wch_vaddr;
}

void set_ld_wch_vaddr(uint32_t value)
{
	g_ld_wch_vaddr = value;
}

uint32_t get_ld_continue_frm_wb(void)
{
	return g_ld_continue_frm_wb;
}

uint32_t get_ld_stt2d_width(void)
{
	return g_ld_stt2d_width;
}

uint32_t get_ld_led_numv(void)
{
	return g_ld_led_numv;
}

uint32_t get_ld_led_numh(void)
{
	return g_ld_led_numh;
}

uint32_t get_ld_sync_sel(void)
{
	return g_ld_sync_sel;
}

uint32_t get_ld_sync_en(void)
{
	return g_ld_sync_en;
}

uint32_t get_ld_dither_en(void)
{
	return g_ld_dither_en;
}

uint32_t get_ld_wch_last_vaddr(void)
{
	return g_ld_wch_last_vaddr;
}

void set_ld_wch_last_vaddr(uint32_t value)
{
	g_ld_wch_last_vaddr = value;
}

uint32_t get_ld_frm_cnt(void)
{
	return g_ld_frm_cnt;
}

void set_ld_frm_cnt(uint32_t value)
{
	g_ld_frm_cnt = value;
}

uint32_t get_dptx_vsync_cnt(void)
{
	return g_vsync_cnt;
}

void set_dptx_vsync_cnt(uint32_t value)
{
	g_vsync_cnt = value;
}
