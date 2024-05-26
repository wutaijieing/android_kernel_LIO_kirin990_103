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

#ifndef DPUFE_DBG_H
#define DPUFE_DBG_H

#include <linux/printk.h>
#include <linux/types.h>

extern uint32_t g_dpufe_msg_level;
extern int g_dpufe_debug_fence_timeline;
extern int g_dpufe_debug_layerbuf_sync;
extern int g_dpufe_debug_ldi_underflow;
extern uint32_t g_dpufe_debug_time_threshold;
extern uint32_t g_dpufe_debug_wait_asy_vactive0_thr;
extern uint32_t g_dpufe_err_status;
extern int g_dpufe_debug_vfb_transfer;
extern int g_debug_overlay_info;

#define DPUFE_ERR(msg, ...) \
	do { \
		if (g_dpufe_msg_level > 3) \
			pr_err("[DPUFE E]:%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)

#define DPUFE_INFO(msg, ...) \
	do { \
		if (g_dpufe_msg_level > 6) \
			pr_err("[DPUFE I]:%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)

#define DPUFE_DEBUG(msg, ...) \
	do { \
		if (g_dpufe_msg_level > 7) \
			pr_err("[DPUFE D]:%s: "msg, __func__, ## __VA_ARGS__); \
	} while (0)

#define dpufe_check_and_return(condition, ret, msg, ...) \
	do { \
		if (condition) { \
			DPUFE_ERR(msg, ##__VA_ARGS__);\
			return ret; \
		} \
	} while (0)

#define dpufe_check_and_no_retval(condition, msg, ...) \
	do { \
		if (condition) { \
			DPUFE_ERR(msg, ##__VA_ARGS__);\
			return; \
		} \
	} while (0)
#endif
