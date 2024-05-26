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
#include <linux/module.h>
#include "dkmd_log.h"

uint32_t g_dkmd_log_level = DPU_LOG_LVL_INFO;
module_param_named(debug_msg_level, g_dkmd_log_level, uint, 0644);
MODULE_PARM_DESC(debug_msg_level, "dpu msg level");

uint32_t g_debug_fence_timeline = 0;
module_param_named(debug_fence_timeline, g_debug_fence_timeline, uint, 0644);
MODULE_PARM_DESC(debug_fence_timeline, "dpu fence timeline debug");

uint32_t g_ldi_data_gate_en = 1;
module_param_named(enable_ldi_data_gate, g_ldi_data_gate_en, uint, 0644);
MODULE_PARM_DESC(enable_ldi_data_gate, "dpu dsi ldi data gate enable");

uint32_t g_debug_vsync_dump = 0;
module_param_named(debug_vsync_dump, g_debug_vsync_dump, uint, 0644);
MODULE_PARM_DESC(debug_vsync_dump, "debug vsync dump timestamp");

uint32_t g_debug_force_update = 0;
module_param_named(debug_force_update, g_debug_force_update, uint, 0644);
MODULE_PARM_DESC(debug_force_update, "debug force update");

uint32_t g_debug_vsync_delay_time = 0;
module_param_named(debug_vsync_delay, g_debug_vsync_delay_time, uint, 0644);
MODULE_PARM_DESC(debug_vsync_delay, "debug vsync delay time");

uint32_t g_debug_dpu_clear_enable = 1;
module_param_named(debug_dpu_clear, g_debug_dpu_clear_enable, uint, 0644);
MODULE_PARM_DESC(debug_dpu_clear, "debug dpu composer clear config");

uint32_t g_debug_idle_enable = 1;
module_param_named(debug_idle_enable, g_debug_idle_enable, uint, 0644);
MODULE_PARM_DESC(debug_idle_enable, "dpu idle enable");

uint32_t g_debug_legacy_dvfs_enable = 1;
module_param_named(debug_legacy_dvfs_enable, g_debug_legacy_dvfs_enable, uint, 0644);
MODULE_PARM_DESC(debug_legacy_dvfs_enable, "dpu legacy dvfs enable");

/* dynamic on and off are not supported */
uint32_t g_dpu_lp_enable_flag = 1;

uint32_t g_pan_display_frame_index;
bool g_dpu_complete_start;
