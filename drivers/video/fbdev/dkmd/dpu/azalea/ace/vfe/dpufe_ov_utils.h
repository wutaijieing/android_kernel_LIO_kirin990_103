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

#ifndef DPUFE_OV_UTILS_H
#define DPUFE_OV_UTILS_H

#include <linux/time.h>

#include "dpufe_queue.h"
#include "dpufe.h"
#include "../include/dpu_communi_def.h"

#ifndef BIT
#define BIT(x)  (1 << (x))
#endif

#define DBG_QUEUE_CAP 5

enum dbg_queue_type {
	DBG_DPU0_PLAY = 0,
	DBG_DPU0_VSYNC,
	DBG_DPU1_PLAY,
	DBG_DPU1_VSYNC,
	DBG_Q_NUM
};

extern struct dpufe_queue g_dpu_dbg_queue[DBG_Q_NUM];

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
typedef timeval_compatible timeval_compatible;
#else
typedef struct __kernel_sock_timeval timeval_compatible;
#endif

static inline void dpufe_get_timestamp(timeval_compatible *tv)
{
	struct timespec64 ts;

	ktime_get_ts64(&ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / NSEC_PER_USEC;
}

static inline uint32_t dpufe_timestamp_diff(timeval_compatible *lasttime, timeval_compatible *curtime)
{
	return 1000000 * (curtime->tv_sec - lasttime->tv_sec) +
		curtime->tv_usec - lasttime->tv_usec;
}

#define dpufe_trace_ts_begin(start_ts) dpufe_get_timestamp(start_ts)
#define dpufe_trace_ts_end(start_ts, timeout, msg, ...)  do { \
	timeval_compatible end_ts; \
	dpufe_get_timestamp(&end_ts); \
	if (dpufe_timestamp_diff(start_ts, &end_ts) >= timeout) \
		DPUFE_INFO(msg"%u", ##__VA_ARGS__, dpufe_timestamp_diff(start_ts, &end_ts)); \
	} while (0)

int dpufe_play_pack_data(struct dpufe_data_type *dfd, struct dpu_core_disp_data *ov_info);
int dpufe_play_send_info(struct dpufe_data_type *dfd, struct dpu_core_disp_data *ov_info);
int dpufe_overlay_get_ov_data_from_user(struct dpufe_data_type *dfd,
	dss_overlay_t *pov_req, const void __user *argp);
void dump_dbg_queues(int index);
void dpufe_dbg_queue_free(void);
void empty_dbg_queues(uint32_t index);

#endif
