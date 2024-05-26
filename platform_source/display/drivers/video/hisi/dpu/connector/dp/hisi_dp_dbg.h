/* Copyright (c) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef __HISI_DP_DBG_H__
#define __HISI_DP_DBG_H__

#include <linux/kernel.h>
#include <linux/types.h>
#include <huawei_platform/dp_source/dp_dsm.h>
#include <huawei_platform/dp_source/dp_factory.h>
#include <huawei_platform/dp_source/dp_source_switch.h>
#include <huawei_platform/dp_source/dp_debug.h>

/*******************************************************************************
 * DP GRAPHIC DEBUG TOOL
 */
#define CONFIG_DPU_FB_ENG_DBG
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


/*******************************************************************************
 * Interface provided by cbg, include data monitor, event report, and etc.
 */
//#define HUAWEI_DP_DSM_ENABLE

#ifdef HUAWEI_DP_DSM_ENABLE
void huawei_dp_imonitor_set_param(enum dp_imonitor_param param, void *data);
void huawei_dp_imonitor_set_param_aux_rw(bool rw,
	bool i2c,
	uint32_t addr,
	uint32_t len,
	int retval);
void huawei_dp_imonitor_set_param_timing(uint16_t h_active,
	uint16_t v_active,
	uint32_t pixel_clock,
	uint8_t vesa_id);
void huawei_dp_imonitor_set_param_err_count(uint16_t lane0_err,
	uint16_t lane1_err,
	uint16_t lane2_err,
	uint16_t lane3_err);
void huawei_dp_imonitor_set_param_vs_pe(int index, uint8_t *vs, uint8_t *pe);
void huawei_dp_imonitor_set_param_resolution(uint8_t *user_mode, uint8_t *user_format);

void huawei_dp_set_dptx_vr_status(bool dptx_vr);

bool huawei_dp_factory_mode_is_enable(void);
void huawei_dp_factory_link_cr_or_ch_eq_fail(bool is_cr);
bool huawei_dp_factory_is_4k_60fps(uint8_t rate,
	uint8_t lanes,
	uint16_t h_active,
	uint16_t v_active,
	uint8_t fps);

int huawei_dp_get_current_dp_source_mode(void);
int huawei_dp_update_external_display_timming_info(uint32_t width,
	uint32_t high,
	uint32_t fps);
void huawei_dp_send_event(enum dp_event_type event);

void huawei_dp_debug_init_combophy_pree_swing(uint32_t *pv, int count);
#else
static inline void huawei_dp_imonitor_set_param(enum dp_imonitor_param param, void *data)
{
}

static inline void huawei_dp_imonitor_set_param_aux_rw(bool rw,
	bool i2c,
	uint32_t addr,
	uint32_t len,
	int retval)
{
}

static inline void huawei_dp_imonitor_set_param_timing(uint16_t h_active,
	uint16_t v_active,
	uint32_t pixel_clock,
	uint8_t vesa_id)
{
}

static inline void huawei_dp_imonitor_set_param_err_count(uint16_t lane0_err,
	uint16_t lane1_err,
	uint16_t lane2_err,
	uint16_t lane3_err)
{
}

static inline void huawei_dp_imonitor_set_param_vs_pe(int index,
	uint8_t *vs,
	uint8_t *pe)
{
}

static inline void huawei_dp_imonitor_set_param_resolution(uint8_t *user_mode,
	uint8_t *user_format)
{
}

static inline void huawei_dp_set_dptx_vr_status(bool dptx_vr) {}

static inline bool huawei_dp_factory_mode_is_enable(void)
{
	return false;
}

static inline void huawei_dp_factory_link_cr_or_ch_eq_fail(bool is_cr) {}

static inline bool huawei_dp_factory_is_4k_60fps(uint8_t rate,
	uint8_t lanes,
	uint16_t h_active,
	uint16_t v_active,
	uint8_t fps)
{
	return true;
}

static inline int huawei_dp_get_current_dp_source_mode(void)
{
	return SAME_SOURCE;
}

static inline int huawei_dp_update_external_display_timming_info(uint32_t width,
	uint32_t high,
	uint32_t fps)
{
	return 0;
}

static inline void huawei_dp_send_event(enum dp_event_type event) {}

static inline void huawei_dp_debug_init_combophy_pree_swing(uint32_t *pv, int count) {}
#endif /* HUAWEI_DP_DSM_ENABLE */

#endif /* HISI_DP_DBG_H */

