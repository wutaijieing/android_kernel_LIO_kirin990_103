/*
 * npu_comm_sqe_fmt.c
 *
 * about npu communication sqe format
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
#include "npu_comm_sqe_fmt.h"

#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <securec.h>

#include "npu_common.h"
#include "npu_log.h"
#include "npu_user_common.h"

#define ts_sqe_entry(ts_sq_addr, offset) \
	((ts_sq_addr) + (offset) * NPU_RT_TASK_SIZE)

int npu_format_ts_sqe(void *ts_sqe_addr, npu_rt_task_t *ts_task,
	u32 index)
{
	npu_rt_task_t *comm_sqe_addr = NULL;

	if (ts_sqe_addr == NULL)
		return -1;
	if (index >= NPU_MAX_SQ_DEPTH)
		return -2;
	/* runtime task structure same as comm ts sqe, add to tail sqe */
	comm_sqe_addr = (npu_rt_task_t *)ts_sqe_entry(ts_sqe_addr, index);
	if (memcpy_s(comm_sqe_addr, NPU_RT_TASK_SIZE, ts_task,
		NPU_RT_TASK_SIZE) != 0)
		return -3;

	return 0;
}
