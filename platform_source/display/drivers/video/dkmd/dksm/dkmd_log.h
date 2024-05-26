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

#ifndef _DKMD_LOG_H_
#define _DKMD_LOG_H_

#include <linux/types.h>
#include <linux/ktime.h>
#include "dksm_debug.h"

#define void_unused(x)    ((void)(x))

enum {
	DPU_LOG_LVL_ERR = 0,
	DPU_LOG_LVL_WARNING,
	DPU_LOG_LVL_INFO,
	DPU_LOG_LVL_DEBUG,
	DPU_LOG_LVL_MAX,
};

#undef pr_fmt
#define pr_fmt(fmt)  "dkmd: " fmt
#define dpu_dbg_nop(...)  ((void)#__VA_ARGS__)

#define dpu_pr_err(msg, ...)  do { \
		pr_err("[E] [%s:%d]"msg, __func__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#ifdef CONFIG_DKMD_DEBUG_ENABLE
#define dpu_pr_warn(msg, ...)  do { \
		if (g_dkmd_log_level >= DPU_LOG_LVL_WARNING) \
			pr_warn("[W] [%s:%d]"msg, __func__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#define dpu_pr_info(msg, ...)  do { \
		if (g_dkmd_log_level >= DPU_LOG_LVL_INFO) \
			pr_info("[I] [%s:%d]"msg, __func__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#define dpu_pr_debug(msg, ...)  do { \
		if (g_dkmd_log_level >= DPU_LOG_LVL_DEBUG) \
			pr_info("[D] [%s:%d]"msg, __func__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#else

#define dpu_pr_warn(msg, ...)  dpu_dbg_nop(msg, __VA_ARGS__)
#define dpu_pr_info(msg, ...)  dpu_dbg_nop(msg, __VA_ARGS__)
#define dpu_pr_debug(msg, ...) dpu_dbg_nop(msg, __VA_ARGS__)

#endif

#define dpu_check_and_return(condition, ret, level, msg, ...) \
	do { \
		if (condition) { \
			dpu_pr_##level(msg, ##__VA_ARGS__);\
			return ret; \
		} \
	} while (0)

#define dpu_check_and_no_retval(condition, level, msg, ...) \
	do { \
		if (condition) { \
			dpu_pr_##level(msg, ##__VA_ARGS__);\
			return; \
		} \
	} while (0)

#ifdef CONFIG_DKMD_ASSERT_ENABLE
#define dpu_assert(cond)  do { \
		if (cond) { \
			dpu_pr_err("assertion failed! %s,%s:%d,%s\n", #cond,  __FILE__, __LINE__, __func__); \
			BUG(); \
		} \
	} while (0)
#else
#define dpu_assert(ptr)
#endif

static inline void dpu_get_timestamp(uint64_t *start_ns)
{
	*start_ns = ktime_get_mono_fast_ns();
}

#define dpu_trace_ts_begin(start_ns) dpu_get_timestamp(start_ns)
#define dpu_trace_ts_end(start_ns, msg)  do { \
		uint64_t end_ns; \
		dpu_get_timestamp(&end_ns); \
		dpu_pr_debug("%s timediff=%llu us", msg, div_u64(end_ns - *start_ns, 1000)); \
	} while (0)

#endif /* DKMD_LOG_H */
