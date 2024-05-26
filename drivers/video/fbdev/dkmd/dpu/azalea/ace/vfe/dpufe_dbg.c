/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#include "dpufe_dbg.h"
#include "dpufe_queue.h"

uint32_t g_dpufe_err_status;
uint32_t g_dpufe_msg_level = 7;

/* sys/module/dpufe/parameters */
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_msg_level, g_dpufe_msg_level, int, 0644);
MODULE_PARM_DESC(debug_msg_level, "dpufe msg level");
#endif

int g_dpufe_debug_fence_timeline;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_fence_timeline, g_dpufe_debug_fence_timeline, int, 0644);
MODULE_PARM_DESC(debug_fence_timeline, "dpufe debug_fence_timeline");
#endif

int g_dpufe_debug_layerbuf_sync;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_layerbuf_sync, g_dpufe_debug_layerbuf_sync, int, 0644);
MODULE_PARM_DESC(debug_layerbuf_sync, "dpufe debug_layerbuf_sync");
#endif

int g_dpufe_debug_ldi_underflow;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_ldi_underflow, g_dpufe_debug_ldi_underflow, int, 0644);
MODULE_PARM_DESC(debug_ldi_underflow, "dpu ldi_underflow debug");
#endif

uint32_t g_dpufe_debug_time_threshold = 20000; /* us */
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_dpufe_time_threshold, g_dpufe_debug_time_threshold, int, 0644);
MODULE_PARM_DESC(debug_dpufe_time_threshold, "dpu overlay time threshold debug");
#endif

/* wait asynchronous vactive0 start threshold 150ms */
uint32_t g_dpufe_debug_wait_asy_vactive0_thr = 150;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_wait_asy_vactive0_start_thr, g_dpufe_debug_wait_asy_vactive0_thr, int, 0644);
MODULE_PARM_DESC(debug_wait_asy_vactive0_start_thr, "dpu dss asynchronous vactive0 start threshold");
#endif

int g_dpufe_debug_vfb_transfer;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_vfb_transfer, g_dpufe_debug_vfb_transfer, int, 0644);
MODULE_PARM_DESC(debug_vfb_transfer, "dpufe debug vfb transfer");
#endif

int g_debug_overlay_info;
#ifdef CONFIG_DPU_FB_ENG_DBG
module_param_named(debug_overlay_info, g_debug_overlay_info, int, 0644);
MODULE_PARM_DESC(debug_overlay_info, "debug overlay info");
#endif

