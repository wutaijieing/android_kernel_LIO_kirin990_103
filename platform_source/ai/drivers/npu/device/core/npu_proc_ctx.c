/*
 * npu_proc_ctx.c
 *
 * about npu proc ctx
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
#include "npu_proc_ctx.h"

#include <asm/barrier.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/bitops.h>
#include <securec.h>

#include "npu_shm.h"
#include "npu_calc_channel.h"
#include "npu_calc_cq.h"
#include "npu_message.h"
#include "npu_stream.h"
#include "npu_log.h"
#include "npu_event.h"
#include "npu_hwts_event.h"
#include "npu_model.h"
#include "npu_task.h"
#include "npu_platform.h"
#include "npu_calc_sq.h"
#include "npu_hwts.h"
#include "npu_adapter.h"
#include "npu_irq_adapter.h"
#include "npu_pm_framework.h"
#include "npu_sink_stream.h"
#include "npu_svm.h"
#include "npu_recycle.h"
#ifdef CONFIG_NPU_SWTS
#include "schedule_stream_manager.h"
#include "response.h"
#endif

int npu_proc_ctx_init(struct npu_proc_ctx *proc_ctx)
{
	INIT_LIST_HEAD(&proc_ctx->cq_list);
	INIT_LIST_HEAD(&proc_ctx->sq_list);
	INIT_LIST_HEAD(&proc_ctx->sink_stream_list);
	INIT_LIST_HEAD(&proc_ctx->stream_list);
	INIT_LIST_HEAD(&proc_ctx->event_list);
	INIT_LIST_HEAD(&proc_ctx->hwts_event_list);
	INIT_LIST_HEAD(&proc_ctx->model_list);
	INIT_LIST_HEAD(&proc_ctx->task_list);
	INIT_LIST_HEAD(&proc_ctx->task_set);
	INIT_LIST_HEAD(&proc_ctx->dev_ctx_list);
	INIT_LIST_HEAD(&proc_ctx->message_list_header);
	INIT_LIST_HEAD(&proc_ctx->l2_vma_list);
	INIT_LIST_HEAD(&proc_ctx->common_vma_list);

	proc_ctx->pid = current->tgid;
	proc_ctx->sink_stream_num = 0;
	proc_ctx->stream_num = 0;
	proc_ctx->event_num = 0;
	proc_ctx->hwts_event_num = 0;
	proc_ctx->cq_num = 0;
	proc_ctx->sq_num = 0;
	proc_ctx->model_num = 0;
	proc_ctx->task_num = 0;
	proc_ctx->proc_ctx_sub = NULL;
	proc_ctx->mailbox_message_count.counter = 0;
	proc_ctx->filep = NULL;
	proc_ctx->sq_round_index = -1;

	mutex_init(&proc_ctx->wm_lock);
	proc_ctx->wm_set = 0;
	proc_ctx->wm_cnt = 0;
	proc_ctx->manager_release = 0;

#ifdef CONFIG_NPU_SWTS
	proc_ctx->resp_queue = resp_queue_alloc();
	cond_return_error(proc_ctx->resp_queue == NULL, -1, "response queue alloc failed");
#endif
	init_waitqueue_head(&proc_ctx->report_wait);
	mutex_init(&proc_ctx->stream_mutex);
	mutex_init(&proc_ctx->event_mutex);
	mutex_init(&proc_ctx->model_mutex);
	mutex_init(&proc_ctx->task_mutex);
	bitmap_zero(proc_ctx->stream_bitmap, NPU_MAX_STREAM_ID);
	bitmap_zero(proc_ctx->event_bitmap, NPU_MAX_EVENT_ID);
	bitmap_zero(proc_ctx->hwts_event_bitmap, NPU_MAX_HWTS_EVENT_ID);
	bitmap_zero(proc_ctx->model_bitmap, NPU_MAX_MODEL_ID);
	bitmap_zero(proc_ctx->task_bitmap, NPU_MAX_SINK_TASK_ID);

	return npu_proc_ctx_sub_init(proc_ctx);
}

void npu_proc_ctx_destroy(struct npu_proc_ctx **proc_ctx_ptr)
{
	struct npu_platform_info *plat_info = npu_plat_get_info();

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail\n");
		return;
	}

	if (proc_ctx_ptr == NULL) {
		npu_drv_err("proc_ctx_ptr is NULL\n");
		return;
	}

	mutex_destroy(&((*proc_ctx_ptr)->wm_lock));
	mutex_destroy(&((*proc_ctx_ptr)->stream_mutex));
	mutex_destroy(&((*proc_ctx_ptr)->event_mutex));
	mutex_destroy(&((*proc_ctx_ptr)->model_mutex));
	mutex_destroy(&((*proc_ctx_ptr)->task_mutex));

#ifdef CONFIG_NPU_SWTS
	resp_queue_free((*proc_ctx_ptr)->resp_queue);
#endif
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_HWTS] == 1)
		npu_proc_ctx_sub_destroy(*proc_ctx_ptr);

	kfree(*proc_ctx_ptr);
	*proc_ctx_ptr = NULL;
}

void npu_release_proc_ctx(struct npu_proc_ctx *proc_ctx)
{
	int ret = 0;
	struct npu_platform_info *plat_info = npu_plat_get_info();
	cond_return_void(plat_info == NULL, "npu plat get info\n");

	if (plat_info->dts_info.feature_switch[NPU_FEATURE_HWTS] == 1)
		(void)npu_proc_free_proc_ctx_sub(proc_ctx);
	(void)npu_proc_free_cq(proc_ctx);

	ret = npu_clear_pid_ssid_table(proc_ctx->devid, proc_ctx->pid, 1);
	if (ret != 0)
		npu_drv_err("npu clear table failed, ret = %d\n", ret);

	if (proc_ctx->manager_release) {
		ret = npu_release_item_bypid(proc_ctx->devid, proc_ctx->pid, proc_ctx->pid);
		if (ret != 0)
			npu_drv_err("npu_release_item_bypid failed\n");
	}
	npu_proc_ctx_destroy(&proc_ctx);
}

void npu_recycle_rubbish_proc(struct npu_dev_ctx *dev_ctx)
{
	struct npu_proc_ctx *proc_ctx = NULL;
	struct npu_proc_ctx *next_proc_ctx = NULL;

	list_for_each_entry_safe(proc_ctx, next_proc_ctx,
		&dev_ctx->rubbish_proc_ctx_list, dev_ctx_list) {
		list_del(&proc_ctx->dev_ctx_list);
		npu_drv_warn("recycling pid = %d\n", proc_ctx->pid);
		npu_recycle_npu_resources(proc_ctx);
		npu_release_proc_ctx(proc_ctx);
	}
}

struct npu_ts_cq_info *npu_proc_alloc_cq(struct npu_proc_ctx *proc_ctx)
{
	struct npu_ts_cq_info *cq_info = NULL;
	struct npu_cq_sub_info *cq_sub_info = NULL;
	struct npu_dev_ctx *cur_dev_ctx = NULL;

	u8 dev_id;

	if (proc_ctx == NULL) {
		npu_drv_err("proc_ctx is null\n");
		return NULL;
	}
	dev_id = proc_ctx->devid;
	cur_dev_ctx = get_dev_ctx_by_id(dev_id);
	cond_return_error(cur_dev_ctx == NULL, NULL,
		"invalid dev id %u\n", dev_id);
	cq_info = npu_alloc_cq(dev_id, proc_ctx->pid);
	if (cq_info == NULL) {
		npu_drv_err("npu dev %d cq_info is null\n", dev_id);
		return NULL;
	}

	cq_sub_info = npu_get_cq_sub_addr(cur_dev_ctx, cq_info->index);
	cond_return_error(cq_sub_info == NULL, NULL,
		"invalid para cq info index %u\n", cq_info->index);
	list_add(&cq_sub_info->list, &proc_ctx->cq_list);
	cq_sub_info->proc_ctx = proc_ctx;
	proc_ctx->cq_num++;

	return cq_info;
}

int npu_proc_get_cq_id(struct npu_proc_ctx *proc_ctx, u32 *cq_id)
{
	struct npu_cq_sub_info *cq_sub = NULL;

	if (list_empty_careful(&proc_ctx->cq_list)) {
		npu_drv_err("cur process %d available cq list empty,"
			"left cq_num = %d !!!\n", proc_ctx->pid, proc_ctx->cq_num);
		return -1;
	}

	cq_sub = list_first_entry(&proc_ctx->cq_list, struct npu_cq_sub_info,
		list);
	cond_return_error(cq_sub == NULL, -1, "invalid cq_sub\n");

	if (cq_sub->index >= NPU_MAX_CQ_NUM) {
		npu_drv_err("invalid cq id = %u\n", cq_sub->index);
		return -1;
	}
	*cq_id = cq_sub->index;
	return 0;
}

int npu_proc_send_alloc_stream_mailbox(struct npu_proc_ctx *proc_ctx)
{
	struct npu_id_entity *stream_sub = NULL;
	struct npu_stream_info *stream_info = NULL;
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	u8 cur_dev_id = 0; // get from platform
	int ret;

	if (list_empty_careful(&proc_ctx->stream_list)) {
		npu_drv_warn("proc context stream list is empty\n");
		return 0;
	}

	list_for_each_safe(pos, n, &proc_ctx->stream_list) {
		stream_sub = list_entry(pos, struct npu_id_entity, list);
		if (stream_sub == NULL) {
			npu_drv_err("stream sub is null\n");
			return -1;
		}

		stream_info = npu_calc_stream_info(cur_dev_id, stream_sub->id);
		cond_return_error(stream_info == NULL, -1,
			"illegal npu stream info is null\n");
		if (stream_info->strategy & STREAM_STRATEGY_SINK) {
			npu_drv_debug("send no mailbox for sink stream\n");
			continue;
		}

		ret = npu_inc_cq_ref_by_communication_stream(cur_dev_id, stream_info->cq_index);
		if (ret != 0) {
			npu_drv_err("invalid param, cur_dev_id %d, cq_id %d", cur_dev_id, stream_info->cq_index);
			return ret;
		}
		ret = npu_send_alloc_stream_mailbox(cur_dev_id, stream_sub->id,
			stream_info->cq_index);
		if (ret) {
			npu_drv_err("send alloc stream %d mailbox failed\n",
				stream_info->id);
			return ret;
		}
	}

	return 0;
}

int npu_proc_clear_sqcq_info(struct npu_proc_ctx *proc_ctx)
{
	struct npu_id_entity *stream_sub = NULL;
	struct npu_stream_info *stream_info = NULL;
	struct npu_cq_sub_info *cq_sub = NULL;
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	u8 cur_dev_id = 0; // get from platform

	if (list_empty_careful(&proc_ctx->cq_list)) {
		npu_drv_debug("proc context cq list is empty, no need clear\n");
		return 0;
	}

	list_for_each_safe(pos, n, &proc_ctx->cq_list) {
		cq_sub = list_entry(pos, struct npu_cq_sub_info, list);
		if (cq_sub != NULL)
			(void)npu_clear_cq(cur_dev_id, cq_sub->index);
	}

	if (list_empty_careful(&proc_ctx->stream_list)) {
		npu_drv_debug("proc context sq list is empty, no need clear\n");
		return 0;
	}

	list_for_each_safe(pos, n, &proc_ctx->stream_list) {
		stream_sub = list_entry(pos, struct npu_id_entity, list);
		if (stream_sub != NULL) {
			stream_info = npu_calc_stream_info(cur_dev_id, stream_sub->id);
			cond_return_error(stream_info == NULL, -1,
				"illegal npu stream_info is null\n");
			if (stream_info->strategy & STREAM_STRATEGY_SINK) {
				npu_drv_debug("send no mailbox for sink stream\n");
				continue;
			}
			npu_drv_debug("stream_info->sq_index = 0x%x\n",
				stream_info->sq_index);
			(void)npu_clear_sq_info(cur_dev_id, stream_info->sq_index);
		}
	}

	return 0;
}

static int npu_proc_aval_sq_push(struct npu_proc_ctx *proc_ctx,
	u32 sq_id)
{
	struct npu_ts_sq_info *sq_info = NULL;
	struct npu_sq_sub_info *sq_sub_info = NULL;
	struct npu_dev_ctx *cur_dev_ctx = NULL;

	cond_return_error(proc_ctx == NULL, -1, "proc_ctx ptr is NULL\n");
	cur_dev_ctx = get_dev_ctx_by_id(proc_ctx->devid);
	cond_return_error(cur_dev_ctx == NULL, -1,
		"invalid dev id %u\n", proc_ctx->devid);

	sq_info = npu_calc_sq_info(proc_ctx->devid, sq_id);
	cond_return_error(sq_info == NULL, -1, "sq_info ptr is NULL\n");

	sq_sub_info = npu_get_sq_sub_addr(cur_dev_ctx, sq_info->index);
	cond_return_error(sq_sub_info == NULL, -1,
		"invalid para sq info index %u\n", sq_info->index);

	list_add_tail(&sq_sub_info->list, &proc_ctx->sq_list);
	proc_ctx->sq_num++;

	npu_drv_debug("proc_ctx->sq_num:%u sq_id:%u\n", proc_ctx->sq_num, sq_id);
	return 0;
}

static int npu_proc_aval_sq_pop(struct npu_proc_ctx *proc_ctx,
	u32 sq_id)
{
	struct npu_sq_sub_info *sq_sub_info = NULL;
	struct list_head *pos  = NULL;

	cond_return_error(proc_ctx == NULL, -1, "proc_ctx ptr is NULL\n");
	list_for_each(pos, &proc_ctx->sq_list) {
		sq_sub_info = container_of(pos, struct npu_sq_sub_info, list);
		if (sq_sub_info->index == sq_id)
			break;
	}

	list_del(&sq_sub_info->list);
	proc_ctx->sq_num--;
	npu_drv_debug("proc_ctx->sq_num:%d sq_id:%d\n", proc_ctx->sq_num, sq_id);
	return 0;
}

static int npu_proc_alloc_aval_sq_round(
	struct npu_proc_ctx *proc_ctx)
{
	int i;
	struct list_head *pos = NULL;
	struct npu_sq_sub_info *sq_sub = NULL;

	cond_return_error(proc_ctx == NULL, -1, "proc_ctx ptr is NULL\n");
	if (list_empty_careful(&proc_ctx->sq_list)) {
		npu_drv_err("sq list in curr proc is NULL\n");
		return -1;
	}

	cond_return_error(proc_ctx->sq_num == 0, -1, "npu sq_num is illegal\n");
	cond_return_error(proc_ctx->sq_round_index < -1, -1,
		"sq_round_index:%d is illegal\n", proc_ctx->sq_round_index);
	proc_ctx->sq_round_index = (proc_ctx->sq_round_index + 1) %
		(int)proc_ctx->sq_num;

	npu_drv_debug("proc_ctx->sq_round_index:%d\n", proc_ctx->sq_round_index);
	for (i = 0, pos = &(proc_ctx->sq_list); i < proc_ctx->sq_round_index;
		i++, pos = pos->next);

	sq_sub = list_first_entry(pos, struct npu_sq_sub_info, list);

	npu_drv_debug("sq_sub->index:%d\n", sq_sub->index);
	return sq_sub->index;
}

static int npu_proc_free_aval_sq_round(
	struct npu_proc_ctx *proc_ctx)
{
	cond_return_error(proc_ctx->sq_round_index <= -1, -1,
		"sq_round_index:%d is illegal\n", proc_ctx->sq_round_index);
	proc_ctx->sq_round_index = proc_ctx->sq_round_index - 1;
	return 0;
}

/*
 * v200:transmit hwts_cq_id to sink stream, and cq_id to non-sink stream
 * v100:transmit cq_id to nonsink_stream and sink_stream
 * [caustion] stream cq id is proc wide used without resource allocin or freein,
 *            and there is no relase func corresponding to this one
 */
static int npu_proc_get_stream_cq_id(struct npu_proc_ctx *proc_ctx,
	u32 strategy, u32 *stream_cq_id)
{
	int ret;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, -1, "plat_info ptr is NULL\n");
	cond_return_error(stream_cq_id == NULL, -1, "stream_cq_id ptr is NULL\n");
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_HWTS] == 1 &&
		(strategy & STREAM_STRATEGY_SINK))
		ret = npu_proc_get_hwts_cq_id(proc_ctx, stream_cq_id);
	else
		ret = npu_proc_get_cq_id(proc_ctx, stream_cq_id);
	if (ret != 0) {
		npu_drv_err("get cq_id from proc_ctx cq_list failed, strategy:%u\n",
			strategy);
		return -1;
	}
	return 0;
}

int npu_proc_get_stream_sq_id(struct npu_proc_ctx *proc_ctx, u32 strategy,
		u32 *stream_sq_id, u32 *stream_sq_used_round)
{
	int sq_id      = 0;
	int hwts_sq_id = 0;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, -1, "plat_info ptr is NULL\n");
	cond_return_error(stream_sq_id == NULL, -1, "stream_sq_id ptr is NULL\n");
	cond_return_error(stream_sq_used_round == NULL, -1,
		"stream_sq_used_round ptr is NULL\n");

	if (strategy & STREAM_STRATEGY_SINK) { /* v100 sink no alloc sq */
		if (plat_info->dts_info.feature_switch[NPU_FEATURE_HWTS] == 1) {
			hwts_sq_id = npu_alloc_hwts_sq(proc_ctx->devid,
				strategy & STREAM_STRATEGY_LONG);
			cond_return_error(hwts_sq_id < 0, -1,
				"alloc hwts_sq from dev %d failed\n", proc_ctx->devid);
			*stream_sq_id = (u32)hwts_sq_id;
		}
		return 0;
	}

	if (proc_ctx->sq_num < NPU_MAX_PROCESS_SQ_NUM) {
		sq_id = npu_alloc_sq(proc_ctx->devid, proc_ctx->pid);
		if (sq_id >= 0) { /* alloc sq from dev */
			*stream_sq_id = (u32)sq_id;
			(void)npu_proc_aval_sq_push(proc_ctx, *stream_sq_id);
			return 0;
		}
	}

	sq_id = npu_proc_alloc_aval_sq_round(proc_ctx); /* alloc sq from proc */
	cond_return_error(sq_id < 0, -1, "alloc sq from dev %d failed\n",
		proc_ctx->devid);

	*stream_sq_used_round = 1;
	*stream_sq_id = (u32)sq_id;
	npu_drv_warn("sq_num in curr proc or dev has reached limit, stream sq_id:%d is used round\n",
		*stream_sq_id);

	return 0;
}

int npu_proc_revert_stream_sq_id(struct npu_proc_ctx *proc_ctx,
	u32 strategy, u32 stream_sq_id, u32 stream_sq_used_round)
{
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, -1, "plat_info ptr is NULL\n");
	if (strategy & STREAM_STRATEGY_SINK) { /* v100 sink no alloc sq */
		/* hwtssq is bind to sink-stream */
		if (plat_info->dts_info.feature_switch[NPU_FEATURE_HWTS] == 1) {
			npu_free_hwts_sq(proc_ctx->devid, stream_sq_id);
			return 0;
		}
	} else {
		if (stream_sq_used_round == 0) {
			npu_proc_aval_sq_pop(proc_ctx, stream_sq_id);
			npu_free_sq(proc_ctx->devid, stream_sq_id);
		} else {
			npu_proc_free_aval_sq_round(proc_ctx);
		}
	}

	return 0;
}

int npu_proc_release_stream_sq_id(struct npu_proc_ctx *proc_ctx,
	u32 strategy, u32 stream_sq_id)
{
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, -1, "plat_info ptr is NULL\n");
	if (strategy & STREAM_STRATEGY_SINK) { /* v100 sink no alloc sq */
		/* hwtssq is bind to sink-stream */
		if (plat_info->dts_info.feature_switch[NPU_FEATURE_HWTS] == 1)
			npu_free_hwts_sq(proc_ctx->devid, stream_sq_id);
	} else {
		if (npu_get_sq_ref_by_stream(proc_ctx->devid, stream_sq_id) == 0) {
			npu_proc_aval_sq_pop(proc_ctx, stream_sq_id);
			npu_free_sq(proc_ctx->devid, stream_sq_id);
		}
	}

	return 0;
}

int npu_proc_check_stream_id(struct npu_proc_ctx *proc_ctx, u32 stream_id)
{
	DECLARE_BITMAP(tmp_bitmap, NPU_MAX_STREAM_ID);

	mutex_lock(&proc_ctx->stream_mutex);
	if (stream_id >= NPU_MAX_STREAM_ID) {
		mutex_unlock(&proc_ctx->stream_mutex);
		npu_drv_err("free_stream_id %d\n", stream_id);
		return -EFAULT;
	}

	bitmap_copy(tmp_bitmap, proc_ctx->stream_bitmap, NPU_MAX_STREAM_ID);
	bitmap_set(tmp_bitmap, stream_id, 1);
	// if tmp_bitmap == proc_ctx->stream_bitmap ,
	// bitmap_equal return 1, otherwise return 0
	if (bitmap_equal(tmp_bitmap, proc_ctx->stream_bitmap,
		NPU_MAX_STREAM_ID) == 0) {
		mutex_unlock(&proc_ctx->stream_mutex);
		npu_drv_err("invalidate id %d\n", stream_id);
		return -EFAULT;
	}
	mutex_unlock(&proc_ctx->stream_mutex);

	return 0;
}

int npu_proc_check_event_id(struct npu_proc_ctx *proc_ctx, u32 event_id)
{
	DECLARE_BITMAP(tmp_bitmap, NPU_MAX_EVENT_ID);

	mutex_lock(&proc_ctx->event_mutex);
	if (event_id >= NPU_MAX_EVENT_ID) {
		mutex_unlock(&proc_ctx->event_mutex);
		npu_drv_err("event_id %d\n", event_id);
		return -EFAULT;
	}

	bitmap_copy(tmp_bitmap, proc_ctx->event_bitmap, NPU_MAX_EVENT_ID);
	bitmap_set(tmp_bitmap, event_id, 1);
	if (bitmap_equal(tmp_bitmap, proc_ctx->event_bitmap,
		NPU_MAX_EVENT_ID) == 0) {
		mutex_unlock(&proc_ctx->event_mutex);
		npu_drv_err("invalidate id %d\n", event_id);
		return -EFAULT;
	}
	mutex_unlock(&proc_ctx->event_mutex);

	return 0;
}

int npu_proc_check_hwts_event_id(struct npu_proc_ctx *proc_ctx, u32 event_id)
{
	DECLARE_BITMAP(tmp_bitmap, NPU_MAX_HWTS_EVENT_ID);

	mutex_lock(&proc_ctx->event_mutex);
	if (event_id >= NPU_MAX_HWTS_EVENT_ID) {
		mutex_unlock(&proc_ctx->event_mutex);
		npu_drv_err("event_id %d\n", event_id);
		return -EFAULT;
	}

	bitmap_copy(tmp_bitmap, proc_ctx->hwts_event_bitmap, NPU_MAX_HWTS_EVENT_ID);
	bitmap_set(tmp_bitmap, event_id, 1);
	if (bitmap_equal(tmp_bitmap, proc_ctx->hwts_event_bitmap,
		NPU_MAX_HWTS_EVENT_ID) == 0) {
		mutex_unlock(&proc_ctx->event_mutex);
		npu_drv_err("invalidate id %d\n", event_id);
		return -EFAULT;
	}
	mutex_unlock(&proc_ctx->event_mutex);

	return 0;
}

int npu_proc_check_model_id(struct npu_proc_ctx *proc_ctx, u32 model_id)
{
	DECLARE_BITMAP(tmp_bitmap, NPU_MAX_MODEL_ID);

	mutex_lock(&proc_ctx->model_mutex);
	if (model_id >= NPU_MAX_MODEL_ID) {
		mutex_unlock(&proc_ctx->model_mutex);
		npu_drv_err("free_stream_id %d\n", model_id);
		return -EFAULT;
	}

	bitmap_copy(tmp_bitmap, proc_ctx->model_bitmap, NPU_MAX_MODEL_ID);
	bitmap_set(tmp_bitmap, model_id, 1);
	// if tmp_bitmap == proc_ctx->model_bitmap ,
	// bitmap_equal will return 1, otherwise return 0
	if (bitmap_equal(tmp_bitmap, proc_ctx->model_bitmap,
		NPU_MAX_MODEL_ID) == 0) {
		mutex_unlock(&proc_ctx->model_mutex);
		npu_drv_err("invalidate model id %u\n", model_id);
		return -EFAULT;
	}
	mutex_unlock(&proc_ctx->model_mutex);

	return 0;
}

int npu_proc_check_task_id(struct npu_proc_ctx *proc_ctx, u32 task_id)
{
	DECLARE_BITMAP(tmp_bitmap, NPU_MAX_SINK_TASK_ID);

	mutex_lock(&proc_ctx->task_mutex);
	if (task_id >= NPU_MAX_SINK_TASK_ID) {
		mutex_unlock(&proc_ctx->task_mutex);
		npu_drv_err("free_task_id %d\n", task_id);
		return -EFAULT;
	}

	bitmap_copy(tmp_bitmap, proc_ctx->task_bitmap, NPU_MAX_SINK_TASK_ID);
	bitmap_set(tmp_bitmap, task_id, 1);
	if (bitmap_equal(tmp_bitmap, proc_ctx->task_bitmap,
		NPU_MAX_SINK_TASK_ID) == 0) {
		mutex_unlock(&proc_ctx->task_mutex);
		npu_drv_err("invalidate id %d\n", task_id);
		return -EFAULT;
	}
	mutex_unlock(&proc_ctx->task_mutex);

	return 0;
}

// protect by proc_ctx->stream_mutex when called
int npu_proc_alloc_stream(struct npu_proc_ctx *proc_ctx,
	struct npu_stream_info **stream_info, u16 strategy, u16 priority)
{
	int ret;
	u32 stream_cq_id = 0;
	u32 stream_sq_id = 0;
	u32 stream_sq_used_round = 0;
	struct npu_dev_ctx *dev_ctx = NULL;
	struct npu_id_entity *stream_sub_info = NULL;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, -EINVAL,
		"get plat info fail\n");
	cond_return_error(proc_ctx == NULL, -1, "proc_ctx ptr is NULL\n");
	cond_return_error(stream_info == NULL, -1, "stream_info pptr is NULL\n");

	ret = npu_proc_get_stream_cq_id(proc_ctx, strategy, &stream_cq_id);
	cond_return_error(ret != 0, -1, "get stream_cq_id failed, strategy:%d\n",
		strategy);
	ret = npu_proc_get_stream_sq_id(proc_ctx, strategy, &stream_sq_id,
		&stream_sq_used_round);
	cond_return_error(ret != 0, -1, "get stream_sq_id failed, strategy:%d\n",
		strategy);

	npu_drv_debug("stream sq_id:%d cq_id:%d, strategy:%d\n", stream_sq_id,
		stream_cq_id, strategy);

	*stream_info = npu_alloc_stream(strategy, stream_sq_id, stream_cq_id);
	if (*stream_info == NULL) {
		npu_proc_revert_stream_sq_id(proc_ctx, strategy, stream_sq_id,
			stream_sq_used_round);
		npu_drv_err("get stream_info through cq %d failed\n", stream_cq_id);
		return -1;
	}
	(*stream_info)->create_tid = (u32)current->pid;
	(*stream_info)->priority = (priority > NPU_MAX_STREAM_PRIORITY) ?
		NPU_MAX_STREAM_PRIORITY : priority;
	npu_drv_debug("alloc stream success stream_id= %d, strategy= %u, stream_sq_id= %d, stream_cq_id= %d\n",
		(*stream_info)->id, strategy, (*stream_info)->sq_index,
		(*stream_info)->cq_index);

	dev_ctx = get_dev_ctx_by_id(proc_ctx->devid);
	cond_return_error(dev_ctx == NULL, -1, "invalid dev_ctx\n");
	if (strategy & STREAM_STRATEGY_SINK) {
		stream_sub_info = npu_get_sink_stream_sub_addr(
			dev_ctx, (*stream_info)->id);
		cond_return_error(stream_sub_info == NULL, -1,
			"invalid stream_sub_info id %d\n", (*stream_info)->id);
		list_add(&stream_sub_info->list, &proc_ctx->sink_stream_list);
		proc_ctx->sink_stream_num++;
	} else {
#ifdef CONFIG_NPU_SWTS
		struct schedule_stream *sched_stream = sched_stream_mngr_get_stream((*stream_info)->id);
		sched_stream_init(sched_stream, (*stream_info)->id, proc_ctx->resp_queue);
#endif
		stream_sub_info = npu_get_non_sink_stream_sub_addr(
			dev_ctx, (*stream_info)->id);
		cond_return_error(stream_sub_info == NULL, -1,
			"invalid stream_sub_info id %d\n", (*stream_info)->id);
		list_add(&stream_sub_info->list, &proc_ctx->stream_list);
		proc_ctx->stream_num++;
	}

	npu_drv_debug("npu process_id = %d thread_id %d own sink stream num = %d, non sink stream num = %d now\n",
		proc_ctx->pid, current->pid, proc_ctx->sink_stream_num,
		proc_ctx->stream_num);

	return 0;
}

static int npu_proc_alloc_ts_event(
	struct npu_proc_ctx *proc_ctx, u32 *event_id_ptr)
{
	struct npu_id_entity *event_info = npu_alloc_event(proc_ctx->devid);

	if (event_info == NULL) {
		npu_drv_err("event info is null\n");
		*event_id_ptr = NPU_MAX_EVENT_ID;
		return -ENODATA;
	}

	list_add(&event_info->list, &proc_ctx->event_list);
	proc_ctx->event_num++;
	npu_drv_debug("npu process %d own event id = %u num = %d now\n",
		current->pid, event_info->id, proc_ctx->event_num);

	*event_id_ptr = event_info->id;
	return 0;
}

static int npu_proc_alloc_hwts_event(struct npu_proc_ctx *proc_ctx,
	u32 *event_id_ptr)
{
	struct npu_id_entity *event_info = npu_alloc_hwts_event(proc_ctx->devid);

	if (event_info == NULL) {
		npu_drv_err("event info is null\n");
		*event_id_ptr = NPU_MAX_HWTS_EVENT_ID;
		return -ENODATA;
	}

	list_add(&event_info->list, &proc_ctx->hwts_event_list);
	proc_ctx->hwts_event_num++;
	npu_drv_debug("npu process %d own hwts event id = %u num = %d now\n",
		current->pid, event_info->id, proc_ctx->hwts_event_num);

	*event_id_ptr = event_info->id;
	return 0;
}

static int npu_proc_free_ts_event(struct npu_proc_ctx *proc_ctx,
	u32 event_id)
{
	int ret;

	if (event_id >= NPU_MAX_EVENT_ID || proc_ctx->event_num == 0) {
		npu_drv_err("invalid event_id is %u or event_num is %u\n",
			event_id, proc_ctx->event_num);
		return -EINVAL;
	}

	ret = npu_free_event_id(proc_ctx->devid, event_id);
	if (ret != 0) {
		npu_drv_err("free event id failed\n");
		return -ENODATA;
	}

	proc_ctx->event_num--;
	bitmap_clear(proc_ctx->event_bitmap, event_id, 1);
	npu_drv_debug("npu process %d free event id = %u, left num = %d\n",
		current->pid, event_id, proc_ctx->event_num);
	return 0;
}

static int npu_proc_free_hwts_event(struct npu_proc_ctx *proc_ctx,
	u32 event_id)
{
	int ret;

	if (event_id >= NPU_MAX_HWTS_EVENT_ID || proc_ctx->hwts_event_num == 0) {
		npu_drv_err("invalid hwts event_id is %u or hwts_event_num is %u\n",
			event_id, proc_ctx->hwts_event_num);
		return -EINVAL;
	}

	ret = npu_free_hwts_event_id(proc_ctx->devid, event_id);
	if (ret != 0) {
		npu_drv_err("free hwts event id failed\n");
		return -ENODATA;
	}

	proc_ctx->hwts_event_num--;
	bitmap_clear(proc_ctx->hwts_event_bitmap, event_id, 1);
	npu_drv_debug("npu process %d free hwts event id = %u, left num = %d\n",
		current->pid, event_id, proc_ctx->hwts_event_num);
	return 0;
}

int npu_proc_alloc_event(struct npu_proc_ctx *proc_ctx,
	u32 *event_id_ptr, u16 strategy)
{
	int ret;

	if (proc_ctx == NULL || event_id_ptr == NULL) {
		npu_drv_err("proc_ctx ptr or event id ptr is null\n");
		return -EINVAL;
	}

	if (strategy == EVENT_STRATEGY_TS) {
		npu_drv_debug("alloc v100 event, v200 nonsink event\n");
		ret = npu_proc_alloc_ts_event(proc_ctx, event_id_ptr);
	} else {
		ret = npu_proc_alloc_hwts_event(proc_ctx, event_id_ptr);
	}

	return ret;
}

int npu_proc_free_event(struct npu_proc_ctx *proc_ctx,
	u32 event_id, u16 strategy)
{
	int ret;

	if (proc_ctx == NULL) {
		npu_drv_err("proc_ctx ptr is null\n");
		return -EINVAL;
	}

	if (strategy == EVENT_STRATEGY_TS)
		ret = npu_proc_free_ts_event(proc_ctx, event_id);
	else
		ret = npu_proc_free_hwts_event(proc_ctx, event_id);

	return ret;
}

int npu_proc_alloc_model(struct npu_proc_ctx *proc_ctx, u32 *model_id_ptr)
{
	struct npu_id_entity *model_info = NULL;

	if (proc_ctx == NULL || model_id_ptr == NULL) {
		npu_drv_err("proc_ctx ptr or model id ptr is null\n");
		return -EINVAL;
	}

	if (proc_ctx->model_num >= NPU_MAX_PROCESS_MODEL_NUM) {
		npu_drv_err("no resource for current process\n");
		*model_id_ptr = NPU_MAX_MODEL_ID;
		return -ENODATA;
	}

	model_info = npu_alloc_model(proc_ctx->devid);
	if (model_info == NULL) {
		npu_drv_err("model info is null\n");
		*model_id_ptr = NPU_MAX_MODEL_ID;
		return -ENODATA;
	}

	list_add(&model_info->list, &proc_ctx->model_list);
	proc_ctx->model_num++;
	npu_drv_debug("npu process %d own model id = %u num = %d now\n",
		current->pid, model_info->id, proc_ctx->model_num);

	*model_id_ptr = model_info->id;
	return 0;
}

int npu_proc_free_model(struct npu_proc_ctx *proc_ctx, u32 model_id)
{
	int ret;

	if (proc_ctx == NULL || model_id >= NPU_MAX_MODEL_ID) {
		npu_drv_err("proc_ctx ptr or model id ptr is null\n");
		return -EINVAL;
	}

	if (proc_ctx->model_num == 0) {
		npu_drv_err("model_num is 0\n");
		return -EINVAL;
	}

	ret = npu_free_model_id(proc_ctx->devid, model_id);
	if (ret != 0) {
		npu_drv_err("free model id failed\n");
		return -ENODATA;
	}

	proc_ctx->model_num--;
	bitmap_clear(proc_ctx->model_bitmap, model_id, 1);
	npu_drv_debug("npu process %d free model id = %u, left num = %d",
		current->pid, model_id, proc_ctx->model_num);

	return 0;
}

int npu_proc_alloc_task(struct npu_proc_ctx *proc_ctx, u32 *task_id_ptr)
{
	struct npu_id_entity *task_info = NULL;

	if (proc_ctx == NULL || task_id_ptr == NULL) {
		npu_drv_err("proc_ctx ptr or task id ptr is null\n");
		return -EINVAL;
	}

	task_info = npu_alloc_task(proc_ctx->devid);
	if (task_info == NULL) {
		npu_drv_err("task info is null\n");
		*task_id_ptr = NPU_MAX_SINK_TASK_ID;
		return -ENODATA;
	}

	list_add(&task_info->list, &proc_ctx->task_list);
	proc_ctx->task_num++;
	npu_drv_debug("npu process %d own task id = %u num = %d now\n",
		current->pid, task_info->id, proc_ctx->task_num);

	*task_id_ptr = task_info->id;

	return 0;
}

int npu_proc_free_task(struct npu_proc_ctx *proc_ctx, u32 task_id)
{
	int ret;

	if (proc_ctx == NULL || task_id >= NPU_MAX_SINK_TASK_ID) {
		npu_drv_err("proc_ctx ptr or task id ptr is null\n");
		return -EINVAL;
	}

	if (proc_ctx->task_num == 0) {
		npu_drv_err("task_num id 0\n");
		return -ENODATA;
	}

	ret = npu_free_task_id(proc_ctx->devid, task_id);
	if (ret != 0) {
		npu_drv_err("free task id failed\n");
		return -ENODATA;
	}
	proc_ctx->task_num--;
	bitmap_clear(proc_ctx->task_bitmap, task_id, 1);

	npu_drv_debug("npu process %d free task id = %u, left num = %d",
		current->pid, task_id, proc_ctx->task_num);

	return 0;
}

int npu_proc_free_stream(struct npu_proc_ctx *proc_ctx, u32 stream_id)
{
	struct npu_stream_info *stream_info = NULL;
	u8 dev_id;
	int ret;

	if (proc_ctx == NULL || stream_id >= NPU_MAX_STREAM_ID) {
		npu_drv_err("proc_ctx ptr is null or illegal npu stream id. stream_id=%d\n",
			stream_id);
		return -1;
	}

	if (stream_id < NPU_MAX_NON_SINK_STREAM_ID &&
		proc_ctx->stream_num == 0) {
		npu_drv_err("stream_num is 0. stream_id=%d\n", stream_id);
		return -1;
	}

	if (stream_id >= NPU_MAX_NON_SINK_STREAM_ID &&
		proc_ctx->sink_stream_num == 0) {
		npu_drv_err("sink stream_num is 0 stream_id=%d\n", stream_id);
		return -1;
	}

	dev_id = proc_ctx->devid;
	stream_info = npu_calc_stream_info(dev_id, stream_id);
	if (stream_info == NULL) {
		npu_drv_err("stream_info is NULL. stream_id=%d\n", stream_id);
		return -1;
	}

	if (test_bit(stream_id, proc_ctx->stream_bitmap) == 0) {
		npu_drv_err(" has already been freed! stream_id=%d\n", stream_id);
		return -1;
	}

	ret = npu_free_stream(proc_ctx->devid, stream_id);
	if (ret != 0) {
		npu_drv_err("npu process %d free stream_id %d failed\n",
			current->pid, stream_id);
		return -1;
	}
	(void)npu_proc_release_stream_sq_id(proc_ctx, stream_info->strategy,
		stream_info->sq_index);

	bitmap_clear(proc_ctx->stream_bitmap, stream_id, 1);
	if (stream_id < NPU_MAX_NON_SINK_STREAM_ID)
		proc_ctx->stream_num--;
	else
		proc_ctx->sink_stream_num--;

	npu_drv_debug("npu process %d left stream num = %d "
		"(if stream'sq has been released) now\n",
		current->pid, proc_ctx->stream_num);
	return 0;
}

static int npu_proc_free_single_cq(struct npu_proc_ctx *proc_ctx,
	u32 cq_id)
{
	struct npu_ts_cq_info *cq_info = NULL;
	struct npu_cq_sub_info *cq_sub_info = NULL;
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	u8 dev_id;

	if (proc_ctx == NULL) {
		npu_drv_err("proc_ctx is null\n");
		return -1;
	}

	if (cq_id >= NPU_MAX_CQ_NUM) {
		npu_drv_err("illegal npu cq id = %d\n", cq_id);
		return -1;
	}

	if (proc_ctx->cq_num == 0) {
		npu_drv_err("cq_num is 0\n");
		return -1;
	}

	dev_id = proc_ctx->devid;
	cur_dev_ctx = get_dev_ctx_by_id(dev_id);
	cond_return_error(cur_dev_ctx == NULL, -1, "invalid cur_dev_ctx\n");

	cq_info = npu_calc_cq_info(dev_id, cq_id);
	if (cq_info == NULL) {
		npu_drv_err("cq_info is NULL\n");
		return -1;
	}

	cq_sub_info = npu_get_cq_sub_addr(cur_dev_ctx, cq_info->index);
	cond_return_error(cq_sub_info == NULL, -1,
		"invalid cq_sub index %u\n", cq_info->index);

	// del from proc_ctx->cq_list
	list_del(&cq_sub_info->list);
	proc_ctx->cq_num--;
	// add to dev_ctx->cq_available_list
	(void)npu_free_cq(dev_id, cq_id);

	npu_drv_info("proc_ctx pid %d cq_id %d total receive report"
		" proc current left cq num = %d\n", proc_ctx->pid,
		cq_id, proc_ctx->cq_num);
	return 0;
}

int npu_proc_free_cq(struct npu_proc_ctx *proc_ctx)
{
	struct npu_cq_sub_info *cq_sub = NULL;
	u32 cq_id;

	if (proc_ctx == NULL) {
		npu_drv_err("proc_ctx is null\n");
		return -1;
	}

	if (list_empty_careful(&proc_ctx->cq_list)) {
		npu_drv_err("cur process %d available cq list empty,"
			"left cq_num = %d !!!\n", proc_ctx->pid, proc_ctx->cq_num);
		return -1;
	}

	cq_sub = list_first_entry(&proc_ctx->cq_list, struct npu_cq_sub_info,
		list);
	cq_id = cq_sub->index;

	return npu_proc_free_single_cq(proc_ctx, cq_id);
}


// get phase of report from cq head or cq tail depending on report_pos
static u32 __npu_get_report_phase_from_cq_info(
	u8 dev_id, struct npu_ts_cq_info *cq_info, cq_report_pos_t report_pos)
{
	struct npu_cq_sub_info *cq_sub_info = NULL;
	struct npu_report *report = NULL;
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	u32 phase = CQ_INVALID_PHASE;

	cur_dev_ctx = get_dev_ctx_by_id(dev_id);
	cond_return_error(cur_dev_ctx == NULL, CQ_INVALID_PHASE,
		"invalid dev id %u\n", dev_id);

	cq_sub_info = npu_get_cq_sub_addr(cur_dev_ctx, cq_info->index);
	cond_return_error(cq_sub_info == NULL, CQ_INVALID_PHASE,
		"invalid cq_sub_info index %u\n", cq_info->index);

	if (report_pos == RREPORT_FROM_CQ_TAIL)
		report = (struct npu_report *)(uintptr_t)(cq_sub_info->virt_addr +
			(unsigned long)(cq_info->slot_size * cq_info->tail));
	if (report_pos == RREPORT_FROM_CQ_HEAD)
		report = (struct npu_report *)(uintptr_t)(cq_sub_info->virt_addr +
			(unsigned long)(cq_info->slot_size * cq_info->head));
	if (report != NULL)
		phase = report->phase;

	return phase;
}

static u32 __npu_get_report_phase(struct npu_proc_ctx *proc_ctx,
	cq_report_pos_t report_pos)
{
	struct npu_cq_sub_info *cq_sub_info = NULL;
	struct npu_ts_cq_info *cq_info = NULL;
	u32 phase = CQ_INVALID_PHASE;

	if (list_empty_careful(&proc_ctx->cq_list)) {
		npu_drv_err("proc ctx cq list is empty");
		return phase;
	}

	cq_sub_info = list_first_entry(&proc_ctx->cq_list,
		struct npu_cq_sub_info, list);
	cq_info = npu_calc_cq_info(proc_ctx->devid, cq_sub_info->index);
	if (cq_info == NULL) {
		npu_drv_err("proc ctx first cq_info is null");
		return phase;
	}

	phase = __npu_get_report_phase_from_cq_info(
		proc_ctx->devid, cq_info, report_pos);
	if (phase == CQ_INVALID_PHASE) {
		npu_drv_err("invalid phase dev id %u report_pos %d cq index %u\n",
			proc_ctx->devid, report_pos, cq_info->index);
		return phase;
	}

	return phase;
}

u32 npu_proc_get_cq_tail_report_phase(struct npu_proc_ctx *proc_ctx)
{
	return __npu_get_report_phase(proc_ctx, RREPORT_FROM_CQ_TAIL);
}

u32 npu_proc_get_cq_head_report_phase(struct npu_proc_ctx *proc_ctx)
{
	return __npu_get_report_phase(proc_ctx, RREPORT_FROM_CQ_HEAD);
}
