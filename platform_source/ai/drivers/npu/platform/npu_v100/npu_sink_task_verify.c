/*
 * npu_sqe_fmt.c
 *
 * about npu rt task
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
#include "npu_sink_task_verify.h"

#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <securec.h>

#include "npu_common.h"
#include "npu_log.h"
#include "npu_user_common.h"
#include "npu_rt_task.h"

#define rt_task_entry(stream_buf_addr, offset) \
	((stream_buf_addr) + (offset) * NPU_RT_TASK_SIZE)
typedef int (*verify_rt_task_func)(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *rt_task);

static int verify_rt_sink_task_comm_field(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *rt_task)
{
	if (rt_task->type >= NPU_TASK_RESERVED) {
		npu_drv_err("invalid task type:%u, task_id:%u\n",
			rt_task->type, rt_task->task_id);
		return -1;
	}
	if (rt_task->stream_id < NPU_MAX_NON_SINK_STREAM_ID ||
		npu_proc_check_stream_id(proc_ctx, rt_task->stream_id) != 0) {
		npu_drv_err("invalid task stream_id:%u, task_id:%u\n",
			rt_task->stream_id, rt_task->task_id);
		return -1;
	}
	if (npu_proc_check_task_id(proc_ctx, rt_task->task_id) != 0) {
		npu_drv_err("invalid task_id:%u\n", rt_task->task_id);
		return -1;
	}
	return 0;
}

int verify_evt_rec_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *ts_task)
{
	unused(proc_ctx);
	unused(ts_task);
	return 0;
}

int verify_wait_evt_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *ts_task)
{
	unused(proc_ctx);
	unused(ts_task);
	return 0;
}

int verify_kernel_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *rt_task)
{
	unused(proc_ctx);
	if (rt_task->u.kernel_task.block_dim == 0 ||
		rt_task->u.kernel_task.block_dim > MAX_UINT16_NUM) {
		npu_drv_err("invalid block_dim:%u, task_id:%u\n",
			rt_task->u.kernel_task.block_dim, rt_task->task_id);
		return -1;
	}
	if (rt_task->u.kernel_task.pc_start == 0) {
		npu_drv_err("invalid pc_start is 0, task_id:%u\n",
			rt_task->task_id);
		return -1;
	}
	if (rt_task->u.kernel_task.param_base == 0) {
		npu_drv_err("invalid param_base is 0, task_id:%u\n",
			rt_task->task_id);
		return -1;
	}
	if (rt_task->u.kernel_task.rd_cond == 1 &&
		rt_task->u.kernel_task.block_dim != 1) {
		npu_drv_err("rd_cond is 1, invalid block_dim:%u, task_id:%u\n",
			rt_task->u.kernel_task.block_dim, rt_task->task_id);
		return -1;
	}
	return 0;
}

int verify_memcpy_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *rt_task)
{
	unused(proc_ctx);
	if ((rt_task->u.memcpy_task.src_addr & npu_bit_mask(4)) ||
		rt_task->u.memcpy_task.src_addr == 0) {
		npu_drv_err("invalid src_addr not 16B align or is 0, sdma task_id:%u\n",
			rt_task->task_id);
		return -1;
	}
	if ((rt_task->u.memcpy_task.dst_addr & npu_bit_mask(4)) ||
		rt_task->u.memcpy_task.dst_addr == 0) {
		npu_drv_err("invalid dst_addr not 16B align or is 0, sdma task_id:%u\n",
			rt_task->task_id);
		return -1;
	}
	if ((rt_task->u.memcpy_task.length & npu_bit_mask(4)) ||
		rt_task->u.memcpy_task.length == 0) {
		npu_drv_err("invalid length:%llu not 16B align or is 0, sdma task_id:%u\n",
			rt_task->u.memcpy_task.length, rt_task->task_id);
		return -1;
	}
	return 0;
}

int verify_event_wait_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *rt_task)
{
	unused(proc_ctx);
	unused(rt_task);
	return 0;
}

int verify_pctrace_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *rt_task)
{
	unused(proc_ctx);
	unused(rt_task);
	return 0;
}

int verify_cteate_l2_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *rt_task)
{
	unused(proc_ctx);
	unused(rt_task);
	return 0;
}

int verify_fusion_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *ts_task)
{
	unused(proc_ctx);
	if (ts_task->u.fusion_task.flag > NPU_FUSION_END) {
		npu_drv_err("invalid fusion_task.flag:%u, task_id:%u\n",
			ts_task->u.fusion_task.flag, ts_task->task_id);
		return -1;
	}
	return 0;
}

int verify_maintenance_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *ts_task)
{
	unused(proc_ctx);
	if (ts_task->u.maintenance_task.goal > NPU_MT_EVENT_DESTROY) {
		npu_drv_err("invalid maintenance_task.goal:%u, task_id:%u\n",
			ts_task->u.maintenance_task.goal, ts_task->task_id);
		return -1;
	}
	return 0;
}

int verify_create_stream_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *ts_task)
{
	unused(proc_ctx);
	unused(ts_task);
	return 0;
}

int verify_model_maintenance_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *ts_task)
{
	unused(proc_ctx);
	unused(ts_task);
	return 0;
}

int verify_model_execute_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *ts_task)
{
	unused(proc_ctx);
	unused(ts_task);
	return 0;
}

int verify_profiling_enable_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *ts_task)
{
	unused(proc_ctx);
	unused(ts_task);
	return 0;
}

int verify_profiling_disable_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *ts_task)
{
	unused(proc_ctx);
	unused(ts_task);
	return 0;
}

// lack of aicpu task verify
static verify_rt_task_func verify_rt_task_map[] = {
	/* 0                        1                    2                    3 */
	verify_kernel_task, verify_kernel_task, verify_evt_rec_task, verify_wait_evt_task,
	/* 4                        5                       6                  7 */
	verify_fusion_task, verify_memcpy_task, verify_maintenance_task, verify_create_stream_task,
	/* 8                          9                         10    11 */
	NULL, verify_event_wait_task, verify_pctrace_task, verify_cteate_l2_task,
	/* 12                         13                        14    15 */
	verify_model_maintenance_task, verify_model_execute_task, NULL, NULL,
	/* 16                         17                        18    19 */
	NULL,                         NULL,                     NULL, NULL,
	/* 20                         21                            22           23 */
	NULL,                         NULL,                         NULL, NULL,
	/* 24                         25                            26    27 */
	NULL,                         NULL,                        NULL, NULL,
	/* 28                         29                            30    31 */
	NULL,                         NULL,                         NULL, NULL,
	/* 32                         33                            34    35 */
	NULL,                         NULL,                         NULL, NULL,
	/* 36                         37                            38    39 */
	NULL,                         NULL,                         NULL, NULL,
	/* 40                         41                            42    43 */
	NULL,                         NULL,                         NULL, NULL,
	/* 44                         45                            46    47 */
	NULL,                         NULL,                         NULL, NULL,
	/* 48                         49                            50    51 */
	NULL,                         NULL,                         NULL, NULL,
	/* 52                         53                            54    55 */
	NULL,                         NULL,                         NULL, NULL,
	/* 56                         57                            58    59 */
	NULL,                         NULL,                         NULL, NULL,
	/* 60                         61                            62    63 */
	NULL,                         NULL,                         NULL, NULL,
	/* 64                         65                            66    67 */
	verify_profiling_enable_task, verify_profiling_disable_task, NULL, NULL,
};

static int npu_verify_rt_task(struct npu_proc_ctx *proc_ctx,
	npu_rt_task_t *rt_task)
{
	verify_rt_task_func verify_func = NULL;
	int ret;

	npu_drv_debug("rt_task addr:0x%pK", rt_task);
	ret = verify_rt_sink_task_comm_field(proc_ctx, rt_task);
	if (ret != 0) {
		npu_drv_err("verify sink task field failed, ret:%d, task_id:%u, type:%u\n",
			ret, rt_task->task_id, rt_task->type);
		return -1;
	}

	verify_func = verify_rt_task_map[rt_task->type];
	if (verify_func == NULL) {
		npu_drv_err("invalid task_id:%u, type:%u\n", rt_task->task_id,
			rt_task->type);
		return -1;
	}
	ret = verify_func(proc_ctx, rt_task);
	if (ret != 0) {
		npu_drv_err("verify rt task failed, ret:%d, task_id:%u, type:%u\n",
			ret, rt_task->task_id, rt_task->type);
		return -1;
	}

	npu_drv_debug("end\n");
	return 0;
}

int npu_verify_rt_tasks(struct npu_proc_ctx *proc_ctx,
	void *stream_buf_addr, u64 rt_task_len)
{
	u64 rt_task_num = rt_task_len / NPU_RT_TASK_SIZE;
	u8 *stream_buf_addr_base = stream_buf_addr;
	npu_rt_task_t *task = NULL;
	int ret = 0;
	u32 i;

	if (stream_buf_addr == NULL) {
		npu_drv_err("stream buf addr is null\n");
		return -1;
	}

	npu_drv_debug("stream_buf_addr:0x%pK, rt_task_len:%llu, rt_task_num:%llu",
		(uintptr_t)stream_buf_addr, rt_task_len, rt_task_num);
	for (i = 0; i < rt_task_num; i++) {
		task =
			(npu_rt_task_t *)rt_task_entry(stream_buf_addr_base, i);

		ret = npu_verify_rt_task(proc_ctx, task);
		if (ret != 0) {
			npu_drv_err("verify rt task failed, ret:%d, task_id:%u, type:%u\n",
				ret, task->task_id, task->type);
			return -1;
		}
	}

	npu_drv_debug("verify rt tasks end\n");
	return 0;
}
