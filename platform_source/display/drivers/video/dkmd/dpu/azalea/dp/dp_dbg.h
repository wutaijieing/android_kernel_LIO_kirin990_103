/* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef DP_DBG_H
#define DP_DBG_H

#include <linux/kernel.h>
#if defined(CONFIG_HUAWEI_DSM)
#include <huawei_platform/dp_source/dp_dsm.h>
#endif
#if defined(CONFIG_HW_DP_SOURCE)
#include <huawei_platform/dp_source/dp_factory.h>
#include <huawei_platform/dp_source/dp_source_switch.h>
#include <huawei_platform/dp_source/dp_debug.h>
#endif

/*
 * DP GRAPHIC DEBUG TOOL
 */
#ifdef CONFIG_DPU_FB_ENG_DBG
#define CONFIG_DP_GRAPHIC_DEBUG_TOOL
#endif
#ifdef CONFIG_DP_GRAPHIC_DEBUG_TOOL
struct dp_ctrl;
void dp_graphic_debug_node_init(struct dp_ctrl *dptx);
void dp_graphic_debug_set_debug_params(struct dp_ctrl *dptx);
#endif
extern int g_dp_debug_mode_enable;
extern int g_dp_aux_ronselp;

/*
 * DP CDC SLT TEST
 */
struct dp_ctrl;
enum DP_SLT_RESULT {
	INIT = 0,
	SUCC,
	FAIL,
};

#ifdef CONFIG_DP_CDC
void dpu_dp_cdc_slt(struct dp_ctrl *dptx);
void dpu_dp_cdc_slt_reset(void);
#else
static inline void dpu_dp_cdc_slt(struct dp_ctrl *dptx) {}
static inline void dpu_dp_cdc_slt_reset(void) {}
#endif /* CONFIG_DP_CDC */


#if defined(CONFIG_HUAWEI_DSM)
/*
 * Interface provided by cbg, include data monitor, event report, and etc.
 */
void dpu_dp_imonitor_set_param(enum dp_imonitor_param param, void *data);
void dpu_dp_imonitor_set_param_aux_rw(bool rw,
	bool i2c,
	uint32_t addr,
	uint32_t len,
	int retval);
void dpu_dp_imonitor_set_param_timing(uint16_t h_active,
	uint16_t v_active,
	uint32_t pixel_clock,
	uint8_t vesa_id);
void dpu_dp_imonitor_set_param_err_count(uint16_t lane0_err,
	uint16_t lane1_err,
	uint16_t lane2_err,
	uint16_t lane3_err);
void dpu_dp_imonitor_set_param_vs_pe(int index, uint8_t *vs, uint8_t *pe);
void dpu_dp_imonitor_set_param_resolution(uint8_t *user_mode, uint8_t *user_format);

#if defined(CONFIG_VR_DISPLAY)
void dpu_dp_set_dptx_vr_status(bool dptx_vr);
#endif
#endif

#if defined(CONFIG_HW_DP_SOURCE)
bool dpu_dp_factory_mode_is_enable(void);
void dpu_dp_factory_link_cr_or_ch_eq_fail(bool is_cr);
bool dpu_dp_factory_is_4k_60fps(uint8_t rate,
	uint8_t lanes,
	uint16_t h_active,
	uint16_t v_active,
	uint8_t fps);

int dpu_dp_get_current_dp_source_mode(void);
int dpu_dp_update_external_display_timming_info(uint32_t width,
	uint32_t high,
	uint32_t fps);
void dpu_dp_send_event(enum dp_event_type event);

void dpu_dp_debug_init_combophy_pree_swing(uint32_t *pv, int count);

void dpu_dp_save_edid(uint8_t *edid_buf, uint32_t buf_len);
#endif

#endif /* DP_DBG_H */

