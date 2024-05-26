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

#ifndef HISI_DISP_DEBUG_H
#define HISI_DISP_DEBUG_H

#include <linux/types.h>
#include <linux/ktime.h>

extern uint32_t g_disp_log_level;

enum {
	DISP_LOG_LVL_ERR = 0,
	DISP_LOG_LVL_WARNING,
	DISP_LOG_LVL_INFO,
	DISP_LOG_LVL_DEBUG,
	DISP_LOG_LVL_MAX,
};

typedef struct
{
	uint32_t src_type;
	uint32_t dsd_index;
} dss_layer_extend;

#define MAX_DSS_SRC_NUM (9)

static inline void hisi_disp_get_timestamp(struct timespec64 *ts)
{
	ktime_get_ts64(ts);
}

static inline uint32_t hisi_disp_timestamp_diff(struct timespec64 *lasttime, struct timespec64 *curtime)
{
	uint32_t ret;
	ret = (curtime->tv_nsec >= lasttime->tv_nsec) ?
		((curtime->tv_nsec - lasttime->tv_nsec) / 1000) :
		1000000 - ((lasttime->tv_nsec - curtime->tv_nsec) / 1000);  /* 1s */

	return ret;
}

#define DISP_LOG_TAG       "[DTE %s]"
#define DISP_DBG_NOP(...)  ((void)#__VA_ARGS__)

#define disp_pr_err(msg, ...)  \
	do { \
		pr_err(DISP_LOG_TAG"[%s:%d]"msg, "E", __func__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#ifdef CONFIG_DKMD_DPU_DEBUG
#define disp_pr_warn(msg, ...)  \
	do { \
		if (g_disp_log_level > DISP_LOG_LVL_WARNING) \
			pr_warn(DISP_LOG_TAG"[%s:%d]"msg, "W", __func__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#define disp_pr_info(msg, ...)  \
	do { \
		if (g_disp_log_level > DISP_LOG_LVL_INFO) \
			pr_err(DISP_LOG_TAG"[%s:%d]"msg, "I", __func__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#define disp_pr_debug(msg, ...)  \
	do { \
		if (g_disp_log_level > DISP_LOG_LVL_DEBUG) \
			pr_info(DISP_LOG_TAG"[%s:%d]"msg, "D", __func__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#define disp_trace_ts_begin(start_ts) hisi_disp_get_timestamp(start_ts)
#define disp_trace_ts_end(start_ts, msg)  \
	do { \
		struct timespec64 ts; \
		hisi_disp_get_timestamp(&ts); \
		disp_pr_debug("%s timediff=%u", msg, hisi_disp_timestamp_diff(&ts, start_ts)); \
	} while (0)

#else
#define disp_pr_warn(msg, ...)  DISP_DBG_NOP(msg, __VA_ARGS__)
#define disp_pr_info(msg, ...)  DISP_DBG_NOP(msg, __VA_ARGS__)
#define disp_pr_debug(msg, ...) DISP_DBG_NOP(msg, __VA_ARGS__)

#define disp_trace_ts_begin(start_ts)
#define disp_trace_ts_end(start_ts, msg)
#endif

#define disp_pr_rect(level, rect)    disp_pr_##level("%s(x:%d, y:%d, w:%u, h:%u)", #rect, (rect)->x, (rect)->y, (rect)->w, (rect)->h)
#define disp_pr_ops(level, ops_desc) disp_pr_##level("operator.idx:%u, operator.type:%u", ops_desc.info.idx, ops_desc.info.type)

#ifdef CONFIG_HISI_DISP_ENABLE_ASSERT
#define disp_assert_if_cond(cond)  \
	do { \
		if (cond) { \
			pr_err(DISP_LOG_TAG"assertion failed! %s,%s:%d,%s\n", #cond,  __FILE__, __LINE__, __func__); \
			BUG(); \
		} \
	} while (0)

#else
#define disp_assert_if_cond(ptr)
#endif

uint32_t get_ld_wch_vaddr(void);
void set_ld_wch_vaddr(uint32_t value);
uint32_t get_ld_continue_frm_wb(void);
uint32_t get_ld_stt2d_width(void);
uint32_t get_ld_led_numv(void);
uint32_t get_ld_led_numh(void);
uint32_t get_ld_sync_sel(void);
uint32_t get_ld_sync_en(void);
uint32_t get_ld_dither_en(void);
uint32_t get_ld_wch_last_vaddr(void);
void set_ld_wch_last_vaddr(uint32_t value);
uint32_t get_ld_frm_cnt(void);
void set_ld_frm_cnt(uint32_t value);
uint32_t get_dptx_vsync_cnt(void);
void set_dptx_vsync_cnt(uint32_t value);
uint32_t get_dss_layer_src_type(uint32_t num);
uint32_t get_dss_layer_dsd_index(uint32_t num);
void set_dss_layer_src_type(uint32_t layer_id, uint32_t type);
void set_dss_layer_dsd_index(uint32_t layer_id, uint32_t index);

#endif /* HISI_DISP_DEBUG_H */
