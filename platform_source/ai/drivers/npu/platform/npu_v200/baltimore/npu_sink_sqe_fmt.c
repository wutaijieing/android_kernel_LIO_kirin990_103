/*
 * npu_sink_sqe_fmt.c
 *
 * about npu hwts sqe format
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
#include "npu_sink_sqe_fmt.h"

#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>

#include "npu_log.h"
#include "npu_sink_task_verify.h"
#include "npu_rt_task.h"


typedef void (*format_hwts_sqe_func)(void *hwts_sqe_addr,
	npu_rt_task_t *hwts_task, npu_model_desc_info_t *model_desc_info);


void format_aicore_sqe(void *hwts_sqe_addr, npu_rt_task_t *hwts_task,
	npu_model_desc_info_t *model_desc_info)
{
	struct hwts_kernel_sqe *kernel_sqe =
		(struct hwts_kernel_sqe *)hwts_sqe_addr;
	unused(model_desc_info);

	kernel_sqe->type = NPU_HWTS_SQE_AICORE;
	kernel_sqe->ie = 0;
	kernel_sqe->pre_p = 0;
	kernel_sqe->post_p = 0;
	kernel_sqe->wr_cqe = 0;
	kernel_sqe->rd_cond = hwts_task->u.kernel_task.rd_cond;
	kernel_sqe->res0 = 0;
	kernel_sqe->l2_lock = 0;
	kernel_sqe->l2_unlock = 0;
	kernel_sqe->block_dim = hwts_task->u.kernel_task.block_dim;
	kernel_sqe->stream_id = hwts_task->stream_id;
	kernel_sqe->task_id = hwts_task->task_id;

	kernel_sqe->pc_addr_low = (u32)(hwts_task->u.kernel_task.pc_start);
	kernel_sqe->pc_addr_high = (u16)((hwts_task->u.kernel_task.pc_start) >> 32);
	kernel_sqe->kernel_credit = 2;
	kernel_sqe->res2 = 0;
	kernel_sqe->icache_prefetch_cnt = 1;

	kernel_sqe->param_addr_low = (u32)(hwts_task->u.kernel_task.param_base);
	kernel_sqe->param_addr_high =
		(u16)((hwts_task->u.kernel_task.param_base) >> 32);
	kernel_sqe->l2_in_main = 0xFF; // dirty_code
	kernel_sqe->res3 = 0;

	kernel_sqe->literal_addr_low =
		(u32)(hwts_task->u.kernel_task.literal_src_addr);
	kernel_sqe->literal_addr_high =
		(u16)(hwts_task->u.kernel_task.literal_src_addr >> 32);
	kernel_sqe->res4 = 0;

	kernel_sqe->literal_base_ub = hwts_task->u.kernel_task.literal_dst_base;
	kernel_sqe->res5 = 0;

	kernel_sqe->literal_buff_len = hwts_task->u.kernel_task.literal_size;
	kernel_sqe->res6 = 0;

	npu_drv_debug("kernel_sqe= 0x%pK, struct size= %d, stream_id= %u, task_id= %u, pc_start= 0x%pK, param_base= 0x%pK, rd_cond= %u\n",
		kernel_sqe,
		sizeof(struct hwts_kernel_sqe),
		hwts_task->stream_id,
		hwts_task->task_id,
		hwts_task->u.kernel_task.pc_start,
		hwts_task->u.kernel_task.param_base,
		hwts_task->u.kernel_task.rd_cond);
}

void format_ph_sqe(void *hwts_sqe_addr, npu_rt_task_t *hwts_task,
	npu_model_desc_info_t *model_desc_info)
{
	struct hwts_ph_sqe *ph_sqe = (struct hwts_ph_sqe *)hwts_sqe_addr;
	unused(model_desc_info);

	ph_sqe->type = NPU_HWTS_SQE_PLACE_HOLDER;
	ph_sqe->ie = 0;
	ph_sqe->pre_p = 0;
	ph_sqe->post_p = 0;
	ph_sqe->wr_cqe = 0;
	ph_sqe->res0 = 0;
	ph_sqe->task_type = NPU_HWTS_PH_SQE_NORMAL;

	ph_sqe->stream_id = hwts_task->stream_id;
	ph_sqe->task_id   = hwts_task->task_id;

	npu_drv_debug("ph_sqe->task_id= %u\n", ph_sqe->task_id);
}

void format_label_switch_sqe(void *hwts_sqe_addr,
	npu_rt_task_t *hwts_task,
	npu_model_desc_info_t *model_desc_info)
{
	struct hwts_ph_sqe *ph_sqe = (struct hwts_ph_sqe *)hwts_sqe_addr;
	unused(model_desc_info);

	ph_sqe->type = NPU_HWTS_SQE_PLACE_HOLDER;
	ph_sqe->ie = 0;
	ph_sqe->pre_p = 1;
	ph_sqe->post_p = 0;
	ph_sqe->wr_cqe = 0;
	ph_sqe->res0 = 0;
	ph_sqe->task_type = NPU_HWTS_PH_SQE_LABEL_SWITCH;

	ph_sqe->stream_id = hwts_task->stream_id;
	ph_sqe->task_id   = hwts_task->task_id;

	ph_sqe->u.label_switch.condition =
		hwts_task->u.label_switch_task.condition;
	ph_sqe->u.label_switch.right = hwts_task->u.label_switch_task.right;
	ph_sqe->u.label_switch.true_label_idx =
		hwts_task->u.label_switch_task.true_label_idx;

	npu_drv_debug("ph_sqe->task_id= %u, label_idx= %u, right= %llu, condition= %u\n",
		ph_sqe->task_id,
		hwts_task->u.label_switch_task.true_label_idx,
		hwts_task->u.label_switch_task.right,
		hwts_task->u.label_switch_task.condition);
}

void format_label_goto_sqe(void *hwts_sqe_addr,
	npu_rt_task_t *hwts_task, npu_model_desc_info_t *model_desc_info)
{
	struct hwts_ph_sqe *ph_sqe = (struct hwts_ph_sqe *)hwts_sqe_addr;
	unused(model_desc_info);

	ph_sqe->type = NPU_HWTS_SQE_PLACE_HOLDER;
	ph_sqe->ie = 0;
	ph_sqe->pre_p = 1;
	ph_sqe->post_p = 0;
	ph_sqe->wr_cqe = 0;
	ph_sqe->res0 = 0;
	ph_sqe->task_type = NPU_HWTS_PH_SQE_LABEL_GOTO;

	ph_sqe->stream_id = hwts_task->stream_id;
	ph_sqe->task_id   = hwts_task->task_id;

	ph_sqe->u.label_goto.label_idx = hwts_task->u.label_goto_task.label_idx;

	npu_drv_debug("ph_sqe->task_id= %u, label_idx= %u\n",
		ph_sqe->task_id, hwts_task->u.label_goto_task.label_idx);
}

void add_model_event_id(npu_model_desc_info_t *model_desc_info, u16 event_id)
{
	// uint8_t bit map
	u16 group = event_id / 8;
	u8 offset = event_id % 8;
	uint8_t event_map = (uint8_t)0x1 << offset;

	model_desc_info->event_info[group] |= event_map;
}

void format_evt_rec_sqe(void *hwts_sqe_addr, npu_rt_task_t *hwts_task,
	npu_model_desc_info_t *model_desc_info)
{
	struct hwts_event_sqe *evt_rec_sqe =
		(struct hwts_event_sqe *)hwts_sqe_addr;

	evt_rec_sqe->type = NPU_HWTS_SQE_EVENT_RECORD;
	evt_rec_sqe->ie = 0;
	evt_rec_sqe->pre_p = 0;
	evt_rec_sqe->post_p = 0;
	evt_rec_sqe->wr_cqe = 0;
	evt_rec_sqe->res0 = 0;
	evt_rec_sqe->res1 = 0;

	evt_rec_sqe->stream_id = hwts_task->stream_id;
	evt_rec_sqe->task_id = hwts_task->task_id;
	evt_rec_sqe->event_id = hwts_task->u.event_task.event_id;
	evt_rec_sqe->res2 = 0;
	evt_rec_sqe->kernel_credit = 255;

	add_model_event_id(model_desc_info, evt_rec_sqe->event_id);
	npu_drv_debug("evt_rec_sqe->task_id= %u, event_id= %u\n",
		evt_rec_sqe->task_id, evt_rec_sqe->event_id);
}

void format_wait_evt_sqe(void *hwts_sqe_addr, npu_rt_task_t *hwts_task,
	npu_model_desc_info_t *model_desc_info)
{
	struct hwts_event_sqe *evt_wait_sqe = (struct hwts_event_sqe *)hwts_sqe_addr;
	unused(model_desc_info);

	evt_wait_sqe->type = NPU_HWTS_SQE_EVENT_WAIT;
	evt_wait_sqe->ie = 0;
	evt_wait_sqe->pre_p = 0;
	evt_wait_sqe->post_p = 0;
	evt_wait_sqe->wr_cqe = 0;
	evt_wait_sqe->res0 = 0;
	evt_wait_sqe->res1 = 0;

	evt_wait_sqe->stream_id = hwts_task->stream_id;
	evt_wait_sqe->task_id = hwts_task->task_id;
	evt_wait_sqe->event_id = hwts_task->u.event_task.event_id;
	evt_wait_sqe->res2 = 0;
	evt_wait_sqe->kernel_credit = 255;

	npu_drv_debug("evt_wait_sqe->task_id= %u, event_id= %u\n",
		evt_wait_sqe->task_id, evt_wait_sqe->event_id);
}

void format_notify_rec_task(void *hwts_sqe_addr,
	npu_rt_task_t *hwts_task, npu_model_desc_info_t *model_desc_info)
{
	struct hwts_notify_sqe *notify_rec_sqe =
		(struct hwts_notify_sqe *)hwts_sqe_addr;
	unused(model_desc_info);

	notify_rec_sqe->type = NPU_HWTS_SQE_NOTIFY_RECORD;
	notify_rec_sqe->ie = 0;
	notify_rec_sqe->pre_p = 0;
	notify_rec_sqe->post_p = 0;
	notify_rec_sqe->wr_cqe = 0;
	notify_rec_sqe->l2_lock = 0;
	notify_rec_sqe->l2_unlock = 0;

	notify_rec_sqe->stream_id = hwts_task->stream_id;
	notify_rec_sqe->task_id = hwts_task->task_id;
	notify_rec_sqe->notify_id = hwts_task->u.notify_task.notify_id;
	notify_rec_sqe->kernel_credit = 255;

	npu_drv_debug("notify_rec_sqe->task_id= %u, notify_id= %u\n",
		notify_rec_sqe->task_id, notify_rec_sqe->notify_id);
}

void format_wait_notify_task(void *hwts_sqe_addr,
	npu_rt_task_t *hwts_task, npu_model_desc_info_t *model_desc_info)
{
	struct hwts_notify_sqe *notify_wait_sqe =
		(struct hwts_notify_sqe *)hwts_sqe_addr;
	unused(model_desc_info);

	notify_wait_sqe->type = NPU_HWTS_SQE_NOTIFY_WAIT;
	notify_wait_sqe->ie = 0;
	notify_wait_sqe->pre_p = 0;
	notify_wait_sqe->post_p = 0;
	notify_wait_sqe->wr_cqe = 0;
	notify_wait_sqe->l2_lock = 0;
	notify_wait_sqe->l2_unlock = 0;

	notify_wait_sqe->stream_id = hwts_task->stream_id;
	notify_wait_sqe->task_id = hwts_task->task_id;
	notify_wait_sqe->notify_id = hwts_task->u.notify_task.notify_id;
	notify_wait_sqe->kernel_credit = 255;

	npu_drv_debug("notify_wait_sqe->task_id= %u, notify_id= %u\n",
		notify_wait_sqe->task_id, notify_wait_sqe->notify_id);
}

void format_memcpy_task(void *hwts_sqe_addr, npu_rt_task_t *hwts_task,
	npu_model_desc_info_t *model_desc_info)
{
	struct hwts_memcpy_sqe *memcpy_sqe =
		(struct hwts_memcpy_sqe *)hwts_sqe_addr;
	unused(model_desc_info);

	// part 1
	memcpy_sqe->type = NPU_HWTS_SQE_MEMCPY;
	memcpy_sqe->ie = 0;
	memcpy_sqe->pre_p = 0;
	memcpy_sqe->post_p = 0;
	memcpy_sqe->wr_cqe = 0;
	memcpy_sqe->l2_lock = 0;
	memcpy_sqe->l2_unlock = 0;
	memcpy_sqe->stream_id = hwts_task->stream_id;
	memcpy_sqe->task_id = hwts_task->task_id;
	memcpy_sqe->kernel_credit = 255;

	// part 2
	memcpy_sqe->ie_dma = 0;
	memcpy_sqe->mode = 0;
	memcpy_sqe->w_pattern = 0;
	memcpy_sqe->src_streamid = 1; // need svm sid
	memcpy_sqe->dst_streamid = 1; // need svm sid
	memcpy_sqe->src_substreamid = 1; // need svm ssid
	memcpy_sqe->dst_substreamid = 1; // need svm ssid

	memcpy_sqe->src_addr_low = (u32)(hwts_task->u.memcpy_task.src_addr &
		npu_bit_mask(32));
	memcpy_sqe->src_addr_high =
		(u32)((hwts_task->u.memcpy_task.src_addr >> 32) & npu_bit_mask(32));
	memcpy_sqe->src_addr_high_p = 0; // 0-va, 1-pa
	memcpy_sqe->dst_addr_low = (u32)(hwts_task->u.memcpy_task.dst_addr &
		npu_bit_mask(32));
	memcpy_sqe->dst_addr_high =
		(u32)((hwts_task->u.memcpy_task.dst_addr >> 32) & npu_bit_mask(32));
	memcpy_sqe->dst_addr_high_p = 0; // 0-va, 1-pa
	memcpy_sqe->length = hwts_task->u.memcpy_task.length;

	npu_drv_debug("memcpy_sqe->task_id= %u\n", memcpy_sqe->task_id);
}

format_hwts_sqe_func format_hwts_sqe_map[] = {
	/* 0               1               2                   3 */
	format_aicore_sqe, NULL,           format_evt_rec_sqe, format_wait_evt_sqe,
	/* 4           5                   6                   7 */
	format_ph_sqe, format_memcpy_task, NULL,               NULL,
	/* 8           9                   10                  11 */
	NULL,          NULL,               NULL,               NULL,
	/* 12          13    14                       15 */
	NULL,          NULL, format_wait_notify_task, format_notify_rec_task,
	/* 16          17                       18                     19 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 20          21                       22                     23 */
	format_ph_sqe, format_label_switch_sqe, format_label_goto_sqe, NULL,
	/* 24          25                       26                     27 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 28          29                       30                     31 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 32          33                       34                     35 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 36          37                       38                     39 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 40          41                       42                     43 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 44          45                       46                     47 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 48          49                       50                     51 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 52          53                       54                     55 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 56          57                       58                     59 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 60          61                       62                     63 */
	NULL,          NULL,                    NULL,                  NULL,
	/* 64          65                       66                     67 */
	NULL,          NULL,                    NULL,                  NULL,
};

static inline void npu_format_head_sqe(struct hwts_sqe_head *hwts_sqe_head)
{
	hwts_sqe_head->l2_lock = 1;
}

static inline void npu_format_tail_sqe(struct hwts_sqe_head *hwts_sqe_head)
{
	hwts_sqe_head->l2_unlock = 1;
	hwts_sqe_head->wr_cqe = 1;
}

int npu_format_hwts_sqe(void *hwts_sq_addr, void *stream_buf_addr,
	u64 ts_sq_len, npu_model_desc_info_t *model_desc_info)
{
	u64 ts_sqe_num = ts_sq_len / NPU_RT_TASK_SIZE;
	format_hwts_sqe_func format_func = NULL;
	u8 *hwts_sq_base = hwts_sq_addr;
	u8 *stream_buf_addr_base = stream_buf_addr;
	u8 *hwts_sqe = NULL;
	npu_rt_task_t *hwts_task = NULL;
	u32 i;

	if (hwts_sq_addr == NULL) {
		npu_drv_err("hwts sq addr is null\n");
		return -1;
	}
	if (stream_buf_addr == NULL) {
		npu_drv_err("stream buf addr is null\n");
		return -1;
	}
	if (ts_sqe_num == 0) {
		npu_drv_err("ts_sqe_num is 0\n");
		return 0;
	}

	npu_drv_debug("hwts_sq_addr:0x%pK, stream_buf_addr:0x%pK, ts_sq_len:%llu, ts_sqe_num:%llu",
		(uintptr_t)hwts_sq_addr, (uintptr_t)stream_buf_addr, ts_sq_len,
		ts_sqe_num);
	for (i = 0; i < ts_sqe_num; i++) {
		hwts_task = (npu_rt_task_t *)rt_task_entry(
			stream_buf_addr_base, i);
		hwts_sqe = hwts_sqe_entry(hwts_sq_base, i);

		format_func = format_hwts_sqe_map[hwts_task->type];
		if (format_func == NULL) {
			npu_drv_err("invalid task_id:%u, type:%u\n",
				hwts_task->task_id, hwts_task->type);
			return -1;
		}
		format_func((void *)hwts_sqe, hwts_task, model_desc_info);
		npu_drv_debug("type = %u\n", hwts_task->type);
	}

	npu_format_head_sqe((struct hwts_sqe_head *)hwts_sqe_entry(
		hwts_sq_base, 0));
	npu_format_tail_sqe((struct hwts_sqe_head *)hwts_sqe_entry(
		hwts_sq_base, ts_sqe_num - 1));
	npu_drv_debug("end\n");
	return ts_sqe_num;
}

int npu_hwts_query_sqe_info(u64 sqe)
{
	u32 i;
	u32 *usqe = NULL;
	struct hwts_sqe_head *sqe_head = (struct hwts_sqe_head *)(uintptr_t)(sqe);

	cond_return_error(sqe_head == NULL, -1, "sqe_head is null\n");
	npu_drv_warn("sqe.type = %u\n", sqe_head->type);
	npu_drv_warn("sqe.ie = %u\n", sqe_head->ie);
	npu_drv_warn("sqe.pre_p = %u\n", sqe_head->pre_p);
	npu_drv_warn("sqe.post_p = %u\n", sqe_head->post_p);
	npu_drv_warn("sqe.wr_cqe = %u\n", sqe_head->wr_cqe);
	npu_drv_warn("sqe.rd_cond = %u\n", sqe_head->rd_cond);
	npu_drv_warn("sqe.l2_lock = %u\n", sqe_head->l2_lock);
	npu_drv_warn("sqe.l2_unlock = %u\n", sqe_head->l2_unlock);
	npu_drv_warn("sqe.block_dim = %u\n", sqe_head->block_dim);
	npu_drv_warn("sqe.stream_id = %u\n", sqe_head->stream_id);
	npu_drv_warn("sqe.task_id = %u\n", sqe_head->task_id);
	usqe = (u32 *)(uintptr_t)(sqe);
	npu_drv_warn("sqe in Hex:\n");
	for (i = 0; i < sizeof(struct hwts_kernel_sqe) / sizeof(u32); i++)
		npu_drv_warn("0x%08x\n", usqe[i]);

	return 0;
}
