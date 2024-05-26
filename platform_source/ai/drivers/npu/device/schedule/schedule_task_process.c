/*
 * schedule_task_process.c
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
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

#include "schedule_task_process.h"
#include <linux/bitmap.h>
#include <linux/mutex.h>
#include "schedule_stream_engine.h"
#include "event_manager.h"
#include "schedule_model_manager.h"
#include "schedule_stream_manager.h"
#include "npu_shm.h"
#include "npu_platform_resource.h"
#include "schedule_model.h"
#include "hwts_sq_cq.h"
#include "npu_log.h"
#include "npu_user_common.h"
#include "hwts_sq_cq_manager.h"
#include "hwts_channel_manager.h"
#include "schedule_model_engine.h"
#include "hwts_driver.h"
#include "hwts_interrupt.h"
#include "placeholder.h"
#include "mutex_ops.h"
#include "npu_dfx_black_box.h"
#include "npu_platform_exception.h"

int sched_task_proc_event_record(struct schedule_stream *sched_stream, struct npu_rt_task *comm_task)
{
	struct list_head temporary_list;
	struct schedule_stream *next = NULL;
	struct schedule_stream *wait_stream = NULL;

	unused(sched_stream);

	INIT_LIST_HEAD(&temporary_list);
	cond_return_error(comm_task == NULL, -1, "comm_task is null");

	struct event *e = event_mngr_get_event(comm_task->u.event_task.event_id);
	cond_return_error(e == NULL, -1, "event is null");

	event_record(e, &temporary_list);

	list_for_each_entry_safe(wait_stream, next, &temporary_list, node) {
		list_del(&wait_stream->node);
		cond_return_error(wait_stream->status != SCHED_STREAM_STATUS_IDLE, SCHED_TASK_STATUS_FAIL,
			"stream status err %d", wait_stream->status);
		sched_stream_engine_push_stream(wait_stream);
	}

	return SCHED_TASK_STATUS_DONE;
}

int sched_task_proc_event_wait(struct schedule_stream *sched_stream, struct npu_rt_task *comm_task)
{
	cond_return_error(comm_task == NULL, -1, "comm_task is null");

	struct event *e = event_mngr_get_event(comm_task->u.event_task.event_id);
	cond_return_error(e == NULL, -1, "event is null");

	if (e->flag == 1)
		return SCHED_TASK_STATUS_DONE;

	event_wait(e, sched_stream);
	return SCHED_TASK_STATUS_RUNNING;
}

static void destory_sched_stream(struct schedule_stream *sched_stream)
{
	sched_stream->status = SCHED_STREAM_STATUS_IDLE;
	sched_stream->id = MAX_UINT16_NUM;

	INIT_LIST_HEAD(&sched_stream->sched_task_list);
}

static void destory_event(struct npu_rt_task *comm_task)
{
	struct event *e = event_mngr_get_event(comm_task->u.maintenance_task.target_id);
	cond_return_void(e == NULL, "event is null");

	event_init(e);
}

int sched_task_proc_maintenance(struct schedule_stream *sched_stream, struct npu_rt_task *comm_task)
{
	cond_return_error(comm_task == NULL, -1, "comm_task is null");

	switch (comm_task->u.maintenance_task.goal) {
	case NPU_MT_STREAM_DESTROY:
		destory_sched_stream(sched_stream);
		break;
	case NPU_MT_EVENT_DESTROY:
		destory_event(comm_task);
		break;
	default:
		npu_drv_err("maintenance goal type is invalid");
		return SCHED_TASK_STATUS_FAIL;
	}

	return SCHED_TASK_STATUS_DONE;
}

static struct hwts_sq_cq* create_hwts_sq_cq(uint16_t sink_stream_id)
{
	uint8_t devid = 0;
	uint16_t hwts_sq_id;
	uint16_t hwts_cq_id;
	struct npu_stream_info *sink_stream_info = NULL;
	struct npu_hwts_sq_info *hwts_sq_info = NULL;
	struct npu_hwts_cq_info *hwts_cq_info = NULL;
	struct hwts_sq_cq *hwts_sqcq = NULL;

	cond_return_error(sink_stream_id - NPU_MAX_STREAM_INFO_NUM >= NPU_MAX_SINK_STREAM_INFO_NUM, NULL,
		"sink stream id invalid, sink_stream_id = %u", sink_stream_id);

	sink_stream_info = npu_calc_stream_info(devid, sink_stream_id);
	cond_return_error(sink_stream_info == NULL, NULL,
		"get stream info failed, sink_stream_id = %u", sink_stream_id);
	cond_return_error(sink_stream_info->priority >= HWTS_PRIORITY_GRP_MAX, NULL,
		"priority is invalid, priority = %u", sink_stream_info->priority);

	hwts_sq_id = sink_stream_info->sq_index;
	hwts_cq_id = sink_stream_info->cq_index;
	cond_return_error(hwts_sq_id >= NPU_MAX_HWTS_SQ_NUM || hwts_cq_id >= NPU_MAX_HWTS_CQ_NUM, NULL,
		"sq id or cq id is invalid, hwts_sq_id = %u, hwts_cq_id = %u", hwts_sq_id, hwts_cq_id);

	hwts_sq_info = npu_calc_hwts_sq_info(devid, hwts_sq_id);
	cond_return_error(hwts_sq_info == NULL, NULL, "get sq info failed, sq_id = %u", hwts_sq_id);
	cond_return_error(hwts_sq_info->head != 0 || hwts_sq_info->tail > hwts_sq_info->length, NULL,
		"hwts sq info is invalid, head = %u, tail = %u, length = %u",
		hwts_sq_info->head, hwts_sq_info->tail, hwts_sq_info->length);

	hwts_cq_info = npu_calc_hwts_cq_info(devid, hwts_cq_id);
	cond_return_error(hwts_cq_info == NULL, NULL, "get cq info failed, hwts_cq_id = %u", hwts_cq_id);

	hwts_sqcq = hwts_sq_cq_mngr_get_sqcq(hwts_sq_id);
	cond_return_error(hwts_sqcq == NULL, NULL, "get hwts sqcq info failed, hwts_sq_id = %u", hwts_sq_id);

	hwts_sq_cq_set_sq(hwts_sqcq, hwts_sq_info);
	hwts_sq_cq_set_cq(hwts_sqcq, hwts_cq_info);
	hwts_sqcq->priority = sink_stream_info->priority;
	hwts_sqcq->schedule_cq.id = hwts_cq_id;

	npu_drv_info("create hwts sq cq, sq_id = %u, sq_head = %u, sq_tail = %u",
				hwts_sq_id, hwts_sqcq->schedule_sq.head, hwts_sqcq->schedule_sq.tail);
	npu_drv_info("create hwts sq cq, cq_id = %u, cq_head = %u, cq_tail = %u",
				hwts_cq_id, hwts_sqcq->schedule_cq.head, hwts_sqcq->schedule_cq.tail);

	return hwts_sqcq;
}

static int create_sqcq_list(struct schedule_model *sched_model, struct npu_model_desc_info *model_desc_info)
{
	uint16_t idx;
	uint16_t sink_stream_id;
	struct hwts_sq_cq *hwts_sqcq = NULL;

	for (idx = 0; idx < sched_model->sqcq_count; idx++) {
		sink_stream_id = model_desc_info->stream_id[idx];
		npu_drv_info("schedule model binds sink stream id %u", sink_stream_id);
		hwts_sqcq = create_hwts_sq_cq(sink_stream_id);
		cond_return_error(hwts_sqcq == NULL, -1,
			"create hwts sq cq failed, model_id = %u, sink_stream_id = %u,", sched_model->id, sink_stream_id);

		hwts_sqcq->model_id = sched_model->id;
		sched_model_push_sqcq(sched_model, hwts_sqcq);
	}

	return 0;
}

static void create_event_list(struct schedule_model *sched_model, struct npu_model_desc_info *model_desc_info)
{
	uint16_t idx;
	uint8_t event_flag;
	struct hwts_event *event = NULL;

	for (idx = 0; idx < NPU_MAX_HWTS_EVENT_ID; idx++) {
		event_flag = model_desc_info->event_info[idx / 8] & (1 << (idx % 8));
		if (event_flag != 0) {
			event = hwts_event_mngr_get_event(idx);
			if (event->node.next != &event->node)
				npu_drv_err("event has been used by other model, event_id = %u", idx);
			sched_model_push_event(sched_model, event);
		}
	}
}

static int create_sched_model(uint16_t model_id)
{
	struct schedule_model *sched_model = NULL;
	struct npu_model_desc_info *model_desc_info = NULL;
	uint8_t devid = 0;
	int ret;

	sched_model = sched_model_mngr_get_model(model_id);
	cond_return_error(sched_model == NULL, -1, "get schedule model failed, model_id = %u", model_id);
	cond_return_error(sched_model->is_valid == true, 0, "schedule model already create, model_id = %u", model_id);

	model_desc_info = npu_calc_model_desc_info(devid, model_id);
	cond_return_error(model_desc_info == NULL, -1, "get model desc info failed, model_id = %u", model_id);

	sched_model->sqcq_count = model_desc_info->stream_cnt;
	cond_return_error(sched_model->sqcq_count == 0 || sched_model->sqcq_count > MODEL_STREAM_MAX_NUM, -1,
		"sqcq count is invalid, sqcq_count = %u", sched_model->sqcq_count);

	sched_model->rtsq_type = model_desc_info->compute_type;
	cond_return_error(sched_model->rtsq_type >= HWTS_RTSQ_TYPE_RSV, -1, "rtsq_type is invalid, rtsq_type = %u",
		sched_model->rtsq_type);

	ret = create_sqcq_list(sched_model, model_desc_info);
	cond_return_error(ret != 0, -1, "create sqcq list failed");

	create_event_list(sched_model, model_desc_info);

	sched_model->is_valid = true;
	return 0;
}

static void reset_sched_model_sqcq_list(struct schedule_model *sched_model)
{
	struct hwts_sq_cq *hwts_sqcq = NULL;
	struct hwts_sq_cq *next = NULL;

	if (list_empty_careful(&sched_model->sqcq_list)) {
		npu_drv_info("hwts sq cq list is empty, model_id = %u", sched_model->id);
		return;
	}

	list_for_each_entry_safe(hwts_sqcq, next, &sched_model->sqcq_list, node) {
		hwts_sqcq->model_id = MAX_UINT16_NUM;
		list_del(&hwts_sqcq->node);
	}
}

static void reset_sched_model_event_list(struct schedule_model *sched_model)
{
	struct hwts_event *event = NULL;
	struct hwts_event *next = NULL;

	if (list_empty_careful(&sched_model->hwts_event_list)) {
		npu_drv_info("hwts event list is empty, model_id = %u", sched_model->id);
		return;
	}

	list_for_each_entry_safe(event, next, &sched_model->hwts_event_list, node) {
		list_del_init(&event->node);
	}
}

static int destory_sched_model(uint16_t model_id)
{
	struct schedule_model *sched_model = sched_model_mngr_get_model(model_id);
	cond_return_error(sched_model == NULL, -1, "shcedule mode is null, model_id = %u", model_id);

	sched_model->sqcq_count = 0;
	sched_model->sqcq_exec_count = 0;
	sched_model->is_valid = false;
	sched_model->rtsq_type = HWTS_RTSQ_TYPE_RSV;

	reset_sched_model_sqcq_list(sched_model);
	reset_sched_model_event_list(sched_model);

	return 0;
}

static int unbind_stream(uint16_t model_id, uint16_t stream_id)
{
	cond_return_error(stream_id < NPU_MAX_STREAM_INFO_NUM ||
		stream_id - NPU_MAX_STREAM_INFO_NUM >= NPU_MAX_SINK_STREAM_INFO_NUM, -1,
		"sink_stream_id invaild, stream_id = %u", stream_id);

	struct npu_stream_info *sink_stream_info = npu_calc_stream_info(0, stream_id );
	cond_return_error(sink_stream_info == NULL, NULL,
		"get stream info failed, sink_stream_id = %u", stream_id);

	uint16_t sq_id = sink_stream_info->sq_index;
	cond_return_error(sq_id >= NPU_MAX_HWTS_SQ_NUM, -1, "sq_id = %u is invalid", sq_id);

	struct hwts_sq_cq *hwts_sqcq = hwts_sq_cq_mngr_get_sqcq(sq_id);
	cond_return_error(hwts_sqcq == NULL, -1, "hwts_sq_cq is null");

	cond_return_error(hwts_sqcq->model_id != model_id, -1,
		"model_id is not equal sq model_id %u, model_id %u", hwts_sqcq->model_id, model_id);

	struct schedule_model *sched_model = sched_model_mngr_get_model(model_id);
	cond_return_error(sched_model == NULL, -1, "model is null, model_id = %u", model_id);

	cond_return_error(sched_model->is_valid == false, -1, "model is not create");

	list_del(&hwts_sqcq->node);
	hwts_sqcq->model_id = MAX_UINT16_NUM;

	return 0;
}

int sched_task_proc_model_maintenance(struct schedule_stream *sched_stream, struct npu_rt_task *comm_task)
{
	int ret = 0;
	cond_return_error(comm_task == NULL, -1, "comm_task is null");

	uint16_t model_id = comm_task->u.model_maintenance_task.model_id;
	uint16_t stream_id = comm_task->u.model_maintenance_task.stream_id;
	uint16_t operation_type = comm_task->u.model_maintenance_task.operation_type;
	switch(operation_type) {
	case NPU_MMT_MODEL_CREATE:
		ret = create_sched_model(model_id);
		break;
	case NPU_MMT_STREAM_UNBIND:
		ret = unbind_stream(model_id, stream_id);
		break;
	case NPU_MMT_MODEL_DESTROY:
		ret = destory_sched_model(model_id);
		break;
	default:
		npu_drv_err("model maintenance operation type is invalid");
		return SCHED_TASK_STATUS_FAIL;
	}

	return (ret == 0 ? SCHED_TASK_STATUS_DONE : SCHED_TASK_STATUS_FAIL);
}

int sched_task_proc_model_execute(struct schedule_stream *sched_stream, struct npu_rt_task *comm_task)
{
	int ret;
	cond_return_error(comm_task == NULL, SCHED_TASK_STATUS_FAIL, "comm_task is null");

	uint16_t model_id = comm_task->u.model_execute_task.model_id;
	struct schedule_model *sched_model = sched_model_mngr_get_model(model_id);
	cond_return_error(sched_model == NULL, SCHED_TASK_STATUS_FAIL,
		"get schedule model faield, model_id = %u", model_id);

	if (sched_model->is_valid == false) {
		npu_drv_err("schedule model is invalid, need to create, model_id = %u", model_id);
		ret = create_sched_model(model_id);
		cond_return_error(ret != 0, SCHED_TASK_STATUS_FAIL, "create schedule model failed, model_id = %u", model_id);
	}

	cond_return_error(sched_model->schedule_status == SHCED_MODEL_STATUS_RUNNING, SCHED_TASK_STATUS_FAIL,
		"schedule model already executing, model_id = %u", model_id);
	cond_return_error(sched_model->sqcq_count == 0, SCHED_TASK_STATUS_FAIL, "model with no sink stream, model_id = %u",
		model_id);

	sched_model->sqcq_exec_count = sched_model->sqcq_count;
	sched_model->pid = comm_task->u.model_execute_task.pid;
	sched_model->priority = comm_task->u.model_execute_task.priority;
	npu_drv_info("model_id = %u, pid= %u, priority = %u, sqcq_exec_count = %u",
				model_id, sched_model->pid, sched_model->priority, sched_model->sqcq_exec_count);

	sched_model_push_stream(sched_model, sched_stream);
	ret = sched_model_engine_push_model(sched_model);
	cond_return_error(ret != 0, SCHED_TASK_STATUS_FAIL, "push schedule model to model list failed, model_id = %u",
		model_id);

	ret = sched_model_engine_run(sched_model->priority);
	if (ret != 0)
		npu_drv_err("run model failed, priority = %u", sched_model->priority);

	return SCHED_TASK_STATUS_RUNNING;
}

static int sched_model_exec_done(struct schedule_model *sched_model, uint64_t result)
{
	int ret;
	struct schedule_stream *sched_stream = NULL;
	struct npu_id_entity *sched_task = NULL;
	struct response resp;
	uint16_t priority_group = sched_model_engine_priority_to_group(sched_model->priority);
	cond_return_error(priority_group >= NPU_MODEL_PRIORITY_GROUP_MAX, -1, "invalid priority");

	swts_mutex_acquire();

	if (!list_empty_careful(&g_sched_model_engine.executing_model_list)) {
		struct placeholder *pholder = list_first_entry(&g_sched_model_engine.executing_model_list,
			struct placeholder, node);
		pholder_mngr_free_node(pholder);
	}

	g_sched_model_engine.waiting_model[priority_group].count++;

	sched_stream = list_first_entry(&sched_model->schedule_stream_list, struct schedule_stream, node);
	if (sched_stream->is_default_stream) {
		list_move_tail(&sched_stream->node, &g_sched_stream_mngr.sched_stream_list[SCHED_STREAM_PRIORITY_H]);
	} else {
		list_move_tail(&sched_stream->node, &g_sched_stream_mngr.sched_stream_list[SCHED_STREAM_PRIORITY_L]);
	}

	if (!list_empty_careful(&sched_stream->sched_task_list)) {
		sched_task = list_first_entry(&sched_stream->sched_task_list, struct npu_id_entity, list);
		list_del_init(&sched_task->list);
		resp.stream_id = sched_stream->id;
		resp.task_id = ((struct npu_rt_task*)(sched_task->data))->task_id;
		resp.payload = result;
		ret = resp_queue_push_back(sched_stream->resp_queue, &resp);
		cond_goto_error(ret != 0, EXCEPTION, ret, ret, "response push queue failed");

		ret = task_pool_free_task(sched_task);
		cond_goto_error(ret != 0, EXCEPTION, ret, ret, "free task failed, task pool id = %u", sched_task->id);
		sched_model->schedule_status = SCHED_MODEL_STATUS_IDLE;
		ret = sched_stream_engine_run();
		cond_goto_error(ret != 0, EXCEPTION, ret, ret, "schedule stream engine run failed");
	}
	swts_mutex_release();
	return 0;

EXCEPTION:
	swts_mutex_release();
	return ret;
}

static int sched_model_sq_done(struct hwts_sq_cq *hwts_sqcq, uint64_t result)
{
	int ret;
	uint16_t model_id = hwts_sqcq->model_id;
	struct schedule_model *sched_model = sched_model_mngr_get_model(model_id);
	cond_return_error(sched_model == NULL, -1, "schedule model is null, model_id = %u", model_id);
	sched_model->sqcq_exec_count--;
	if (result != SUCCESS)
		npu_drv_err("model execute failed, model_id = %u", model_id);
	npu_drv_info("schedule model sq done, sqcq_exec_count = %u, result = %u", sched_model->sqcq_exec_count, result);
	if (sched_model->sqcq_exec_count == 0) {
		ret = sched_model_exec_done(sched_model, result);
		cond_return_error(ret != 0, -1, "schedule model execute failed, model_id = %u, ret = %u", sched_model->id, ret);
	}

	return 0;
}

void sched_task_proc_hwts_sq_done(uint8_t irq_type, uint64_t channel_bitmap)
{
	npu_drv_info("process sq done, type = %u, channel_bitmap = %llx", irq_type, channel_bitmap);
	int ret;
	uint16_t channel_id = 0;
	struct hwts_channel *channel = NULL;
	struct hwts_sq_cq *hwts_sqcq = NULL;

	for (; channel_bitmap != 0; bitmap_clear((unsigned long *)&channel_bitmap, channel_id, 1)) {
		channel_id = find_first_bit((unsigned long *)&channel_bitmap, HWTS_CHANNEL_NUM);
		channel = hwts_mngr_get_channel(channel_id);
		if (channel->sq_id < NPU_MAX_HWTS_SQ_NUM) {
			hwts_sqcq = hwts_sq_cq_mngr_get_sqcq(channel->sq_id);
			npu_drv_info("process sq done, channel_id=%u, sq_id=%u, priority=%u",
				channel_id, channel->sq_id, hwts_sqcq->priority);

			ret = sched_model_sq_done(hwts_sqcq, SUCCESS);
			if (ret != 0) {
				npu_drv_err("schedule model sq done failed, model_id, ret = %u",
					hwts_sqcq->model_id, ret);
				return;
			}

			hwts_driver_reset_cq_head(channel_id);
			hwts_driver_clear_channel_sq_en(channel_id);
			hwts_channel_mngr_free_channel(channel);
		}
	}
}

void sched_task_proc_hwts_error(uint8_t event_type, uint64_t channel_bitmap)
{
	npu_drv_warn("process hwts error, event type = %u, channel_bitmap = %llx", event_type, channel_bitmap);
	uint16_t channel_id = 0;
	struct hwts_channel *channel = NULL;
	struct hwts_sq_cq *hwts_sqcq = NULL;
	struct npu_dev_ctx *dev_ctx = get_dev_ctx_by_id(0);
	struct hwts_exception_report_info report = {0};

	cond_return_void(event_type >= NPU_EXCEPTION_TYPE_MAX, "event_type %u error", event_type);

	report.exception_type = event_type;

	// 1. The RDR is reported except for the SQE error and sw status error.
	if ((event_type == NPU_EXCEPTION_TYPE_HWTS_SQE_ERROR) || (event_type == NPU_EXCEPTION_TYPE_HWTS_SW_STATUS_ERROR)) {
		npu_drv_warn("hwts sqe error or sw status error\n");
	} else if (event_type == NPU_EXCEPTION_TYPE_HWTS_BUS_ERROR) {
		npu_rdr_exception_report(RDR_TYPE_HWTS_BUS_ERROR);
	} else {
		npu_rdr_exception_report(RDR_TYPE_HWTS_EXCEPTION_ERROR);
	}

	// There is no l2 interrupt for the bus error and pool conflict. Therefore, the value of channel_bitmap is 0
	if (channel_bitmap == 0) {
		npu_exception_report_proc(dev_ctx, &report);
		return;
	}

	// 2. print exception info
	for (; channel_bitmap != 0; bitmap_clear((unsigned long *)&channel_bitmap, channel_id, 1)) {
		channel_id = find_first_bit((unsigned long*)&channel_bitmap, HWTS_CHANNEL_NUM);
		if (event_type != NPU_EXCEPTION_TYPE_HWTS_TIMEOUT_EXCEPTION) {
			report.channel_id = hwts_get_rtsq_id_by_acsq_id(channel_id);
		} else {
			report.channel_id = channel_id;
		}
		channel = hwts_mngr_get_channel(report.channel_id);
		cond_return_void(channel == NULL, "channel is null, channel id: %u", report.channel_id);
		npu_drv_info("channel_id: %u, report.channel_id: %u, HWTS_CHANNEL_NUM: %d, sq_id: %u, NPU_MAX_HWTS_SQ_NUM: %d\n",
			channel_id, report.channel_id, HWTS_CHANNEL_NUM, channel->sq_id, NPU_MAX_HWTS_SQ_NUM);

		hwts_sqcq = hwts_sq_cq_mngr_get_sqcq(channel->sq_id);
		npu_drv_warn("process hwts error, rtsq_id=%u, sq_id=%u, priority=%u",
			report.channel_id, channel->sq_id, hwts_sqcq->priority);

		report.hwts_sq_id = channel->sq_id;
		npu_exception_report_proc(dev_ctx, &report);
	}
}

static int sched_task_proc_register_hanlder(void)
{
	uint32_t idx;
	int ret;

	for (idx = 0; idx < NPU_TASK_RESERVED; idx++) {
		ret = sched_stream_engine_register_handler(idx, NULL);
		cond_return_error(ret != 0, -1, "register hanlder fail");
	}

	ret = sched_stream_engine_register_handler(NPU_TASK_EVENT_RECORD, sched_task_proc_event_record);
	cond_return_error(ret != 0, -1, "event record register handler failed");

	ret = sched_stream_engine_register_handler(NPU_TASK_STREAM_WAIT_EVENT, sched_task_proc_event_wait);
	cond_return_error(ret != 0, -1, "event wait register handler failed");

	ret = sched_stream_engine_register_handler(NPU_TASK_MAINTENANCE, sched_task_proc_maintenance);
	cond_return_error(ret != 0, -1, "maintenance register handler failed");

	ret = sched_stream_engine_register_handler(NPU_TASK_MODEL_MAINTAINCE, sched_task_proc_model_maintenance);
	cond_return_error(ret != 0, -1, "model maintenance register handler failed");

	ret = sched_stream_engine_register_handler(NPU_TASK_MODEL_EXECUTE, sched_task_proc_model_execute);
	cond_return_error(ret != 0, -1, "model exec register handler failed");

	return 0;
}

static int sched_task_proc_resource_init(void)
{
	int ret;

	sched_stream_engine_init();

	ret = task_pool_init();
	cond_return_error(ret != 0, -1, "task_pool_init failed");

	event_mngr_init();
	sched_model_mngr_init();
	hwts_event_mngr_init();
	hwts_sq_cq_mngr_init();
	sched_model_engine_init();

	ret = hwts_irq_init();
	cond_return_error(ret != 0, -1, "hwts_irq_init failed");

	return 0;
}

int sched_task_proc_init(void)
{
	int ret;

	ret = sched_task_proc_register_hanlder();
	cond_return_error(ret != 0, -1, "register_hanlder failed");

	ret = sched_task_proc_resource_init();
	cond_return_error(ret != 0, -1, "resource init failed");
	return 0;
}

void sched_task_proc_deinit(void)
{
	(void)task_pool_deinit();
	hwts_irq_deinit();
}
