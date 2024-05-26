/*
 * npu_sink_task_verify.h
 *
 * about npu sink task verify
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef _NPU_SINK_TASK_VERIFY_H_
#define _NPU_SINK_TASK_VERIFY_H_

#include <linux/types.h>

#include "npu_proc_ctx.h"
#include "npu_rt_task.h"

#define rt_task_entry(stream_buf_addr, offset) \
	((stream_buf_addr) + (offset) * NPU_RT_TASK_SIZE)
#define hwts_sqe_entry(hwts_sq_addr, offset) \
	((hwts_sq_addr) + (offset) * NPU_HWTS_SQ_SLOT_SIZE)

int npu_verify_hwts_sqe(struct npu_proc_ctx *proc_ctx,
	void *stream_buf_addr, u64 ts_sq_len);

#endif
