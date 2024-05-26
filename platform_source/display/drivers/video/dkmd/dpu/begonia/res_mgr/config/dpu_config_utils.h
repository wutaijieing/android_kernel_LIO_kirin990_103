/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef DISP_CONFIG_H
#define DISP_CONFIG_H

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <dpu/soc_dpu_define.h>
#include <platform_include/display/dkmd/dkmd_res_mgr.h>
#include <platform_include/display/dkmd/dkmd_dpu.h>
#include "dksm_debug.h"

#define DTS_OFFLINE_SCENE_ID_MAX 3
struct dpu_config {
	char __iomem *ip_base_addrs[DISP_IP_MAX];
	uint32_t lbuf_size;
	union dpu_version version;
	uint32_t xres;
	uint32_t yres;
	uint32_t offline_scene_id_count;
	uint32_t offline_scene_ids[DTS_OFFLINE_SCENE_ID_MAX];

	uint32_t pmctrl_dvfs_enable;
	struct clk *clk_gate_edc;
};

extern struct dpu_config g_dpu_config_data;

int32_t dpu_init_config(struct platform_device *device);

static inline bool dpu_config_enable_pmctrl_dvfs()
{
	return (g_dpu_config_data.pmctrl_dvfs_enable == 1) && (g_debug_legacy_dvfs_enable == 0);
}

static inline char __iomem *dpu_config_get_ip_base(uint32_t ip)
{
	if (ip >= DISP_IP_MAX)
		return NULL;

	return g_dpu_config_data.ip_base_addrs[ip];
}

static inline uint32_t dpu_config_get_lbuf_size()
{
	return g_dpu_config_data.lbuf_size;
}

static inline uint64_t dpu_config_get_version_value(void)
{
	return g_dpu_config_data.version.value;
}

static inline void dpu_config_set_screen_info(uint32_t xres, uint32_t yres)
{
	if (g_dpu_config_data.xres == 0 && g_dpu_config_data.yres == 0) {
		g_dpu_config_data.xres = xres;
		g_dpu_config_data.yres = yres;
	}
}

static inline void dpu_config_get_screen_info(uint32_t *xres, uint32_t *yres)
{
	*xres = g_dpu_config_data.xres;
	*yres = g_dpu_config_data.yres;
}

static inline uint32_t* dpu_config_get_offline_scene_ids(uint32_t *count)
{
	*count = g_dpu_config_data.offline_scene_id_count;
	return g_dpu_config_data.offline_scene_ids;
}

uint32_t *dpu_config_get_scf_lut_addr_tlb(uint32_t *length);
uint32_t *dpu_config_get_arsr_lut_addr_tlb(uint32_t *length);
void dpu_config_dvfs_vote_exec(uint64_t clk_rate);
uint32_t dpu_config_get_version(void);

void dpu_enable_core_clock(void);
void dpu_disable_core_clock(void);
bool is_dpu_dvfs_enable(void);
void dpu_legacy_inter_frame_dvfs_vote(uint32_t level);
void dpu_intra_frame_dvfs_vote(struct intra_frame_dvfs_info *info);

#endif /* DISP_CONFIG_H */
