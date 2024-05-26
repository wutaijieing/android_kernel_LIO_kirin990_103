/*
 * npu_platform_exception.c
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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

#include "npu_platform_exception.h"

#include <securec.h>

#include "npu_shm.h"
#include "npu_sink_sqe_fmt.h"
#include "npu_hwts_plat.h"
#include "npu_aicore_plat.h"
#include "npu_sdma_plat.h"
#include "npu_pm_framework.h"
#include "npu_pool.h"
#include "bbox/npu_dfx_black_box.h"
#include "npu_proc_ctx.h"
#include "npu_common.h"

u64 npu_exception_calc_sqe_addr(
	struct npu_dev_ctx *dev_ctx, u16 hwts_sq_id, u16 channel_id)
{
	u64 hwts_base = 0;
	u16 sq_head = 0;
	u64 sqe_addr = 0;
	int ret = 0;
	struct npu_hwts_sq_info *sq_info = NULL;
	struct npu_entity_info *hwts_sq_sub_info = NULL;

	cond_return_error(dev_ctx == NULL, 0, "dev_ctx is null\n");
	sq_info = npu_calc_hwts_sq_info(dev_ctx->devid, hwts_sq_id);
	cond_return_error(sq_info == NULL, 0,
		"hwts_sq_id= %u, hwts_sq_info is null\n", hwts_sq_id);

	hwts_sq_sub_info = (struct npu_entity_info *)(
		uintptr_t)sq_info->hwts_sq_sub;
	cond_return_error(hwts_sq_sub_info == NULL, 0,
		"hwts_sq_id= %u, hwts_sq_sub is null\n", hwts_sq_id);
	cond_return_error(hwts_sq_sub_info->vir_addr == 0, 0,
		"vir_addr is null\n");

	hwts_base = npu_hwts_get_base_addr();
	cond_return_error(hwts_base == 0, 0, "hwts_base is NULL\n");
	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_sq_head(channel_id, &sq_head), ret);
	cond_return_error(ret != 0, 0, "can't get sq_head\n");

	sqe_addr = hwts_sq_sub_info->vir_addr + sq_head *
		sizeof(struct hwts_kernel_sqe);
	npu_drv_debug("sqe_addr = 0x%pK\n", sqe_addr);
	return sqe_addr;
}

void npu_exception_pool_conflict_proc(struct npu_dev_ctx *dev_ctx,
	u16 hwts_sq_id, u16 channel_id)
{
	u8 i;
	u8 status = 0;
	(void)hwts_sq_id;
	(void)channel_id;

	for (i = 0; i < NPU_HWTS_MAX_AICORE_POOL_NUM; i++)
		npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
			(void)npu_hwts_query_aicore_pool_status(i, 0, &status));
	for (i = 0; i < NPU_HWTS_MAX_SDMA_POOL_NUM; i++)
		npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
			(void)npu_hwts_query_sdma_pool_status(i, 0, &status));
}

void npu_exception_bus_error_proc(struct npu_dev_ctx *dev_ctx,
	u16 hwts_sq_id, u16 channel_id)
{
	u8 status;
	(void)hwts_sq_id;
	(void)channel_id;

	npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
		(void)npu_hwts_query_bus_error_type(&status));
	npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
		(void)npu_hwts_query_bus_error_id(&status));
}

void npu_exception_sqe_error_proc(struct npu_dev_ctx *dev_ctx,
	u16 hwts_sq_id, u16 channel_id)
{
	u64 sqe_head_addr = npu_exception_calc_sqe_addr(dev_ctx, hwts_sq_id,
		channel_id);

	(void)npu_hwts_query_sqe_info(sqe_head_addr);
}

void npu_exception_sw_status_error_proc(struct npu_dev_ctx *dev_ctx,
	u16 hwts_sq_id, u16 channel_id)
{
	u32 status = 0;

	npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
		(void)npu_hwts_query_sw_status(channel_id, &status));
	npu_exception_sqe_error_proc(dev_ctx, hwts_sq_id, channel_id);
}

void npu_exception_bbox_dump_ub(struct npu_dev_ctx *dev_ctx, u8 aic_id)
{
	#define UB_SIZE 8
	static u64 ub_start[UB_SIZE] =
		{0, 0xD000, 0xE000, 0xF000, 0x14800, 0x18000, 0x1A000, 0x1C000};
	static u32 ub_length[UB_SIZE] =
		{2048, 2048, 2048, 1024, 1024, 2048, 4096, 2048};

	int ret = 0;
	u32 i;
	u32 offset;
	u64 reg_buf[4];
	enum npu_subip subip = aic_id == 0 ? NPU_AICORE0 : NPU_AICORE1;
	char log_buf[NPU_BUF_LEN_MAX + 1] = {0};

	ret = snprintf_s(log_buf, NPU_BUF_LEN_MAX + 1, NPU_BUF_LEN_MAX,
		"**********ub begin**********\n");
	cond_return_void(ret < 0, "snprintf_s ub begin fail, ret = %d\n", ret);
	(void)npu_mntn_copy_reg_to_bbox(log_buf, strlen(log_buf));
	for (i = 0; i < UB_SIZE; i++) {
		for (offset = 0; offset < ub_length[i]; offset += 32) {
			npu_pm_safe_call_with_return(dev_ctx, subip,
				npu_aicore_query_ub_flowtable(
				aic_id, ub_start[i] + offset, reg_buf, 4), ret);
			if (ret != 0) {
				npu_drv_warn("read ub flowtable failed. addr = 0x%016llx\n",
					ub_start[i]);
				return;
			}

			npu_drv_debug("%016llx, %016llx, %016llx, %016llx\n",
				reg_buf[0], reg_buf[1], reg_buf[2], reg_buf[3]);
			ret = snprintf_s(log_buf, NPU_BUF_LEN_MAX + 1, NPU_BUF_LEN_MAX,
				"%08lx: %016llx %016llx %016llx %016llx\n",
				ub_start[i] + offset, reg_buf[0], reg_buf[1],
				reg_buf[2], reg_buf[3]);
			if (ret < 0) {
				npu_drv_warn("snprintf_s is fail !, ret = %d, offset is %u\n", ret, offset);
				continue;
			}
			(void)npu_mntn_copy_reg_to_bbox(log_buf, strlen(log_buf));
		}
	}
	ret = snprintf_s(log_buf, NPU_BUF_LEN_MAX + 1, NPU_BUF_LEN_MAX,
		"***********ub end*************\n");
	cond_return_void(ret < 0, "snprintf_s ub end fail, ret = %d\n", ret);
	(void)npu_mntn_copy_reg_to_bbox(log_buf, strlen(log_buf));
}

void npu_exception_bbox_dump_aicore(u8 aic_id,
	struct aicore_exception_info *aic_info,
	struct aicore_exception_dbg_info *aic_dbg_info)
{
	int reg_id;
	char log_buf[NPU_BUF_LEN_MAX + 1] = {0};
	int ret;

	ret = snprintf_s(log_buf, NPU_BUF_LEN_MAX + 1, NPU_BUF_LEN_MAX,
		"aicore%u\nAIC_ERROR=0x%016llx\nifu start=0x%016llx\n",
		aic_id, aic_info->aic_error, (aic_info->ifu_start << 2));
	cond_return_void(ret < 0, "snprintf_s fail, ret = %d\n", ret);
	(void)npu_mntn_copy_reg_to_bbox(log_buf, strlen(log_buf));

	ret = snprintf_s(log_buf, NPU_BUF_LEN_MAX + 1, NPU_BUF_LEN_MAX,
		"ifu current=0x%016llx\n", aic_dbg_info->ifu_current);
	cond_return_void(ret < 0, "snprintf_s fail, ret = %d\n", ret);
	(void)npu_mntn_copy_reg_to_bbox(log_buf, strlen(log_buf));

	ret = snprintf_s(log_buf, NPU_BUF_LEN_MAX + 1, NPU_BUF_LEN_MAX,
		"***general purpose register of aicore%u***\n", aic_id);
	cond_return_void(ret < 0, "snprintf_s fail, ret = %d\n", ret);
	(void)npu_mntn_copy_reg_to_bbox(log_buf, strlen(log_buf));
	for (reg_id = 0; reg_id < 32; reg_id++) {
		ret = snprintf_s(log_buf, NPU_BUF_LEN_MAX + 1, NPU_BUF_LEN_MAX, "x%d = 0x%016llx\n", reg_id,
			(aic_dbg_info->general_register)[reg_id]);
		if (ret < 0) {
			npu_drv_warn("snprintf_s is fail !, ret = %d, reg_id is %d\n", ret, reg_id);
			continue;
		}
		(void)npu_mntn_copy_reg_to_bbox(log_buf, strlen(log_buf));
	}
}

void npu_exception_query_aicore_info(struct npu_dev_ctx *dev_ctx, u8 aic_id,
	struct aicore_exception_info *aicore_info,
	struct aicore_exception_error_info *aicore_error_info,
	struct aicore_exception_dbg_info *aicore_dbg_info)
{
	enum npu_subip subip = aic_id == 0 ? NPU_AICORE0 : NPU_AICORE1;

	npu_pm_safe_call(dev_ctx, subip,
		(void)npu_aicore_query_exception_info(aic_id, aicore_info));
	npu_pm_safe_call(dev_ctx, subip,
		(void)npu_aicore_query_exception_error_info(
		aic_id, aicore_error_info));
	npu_pm_safe_call(dev_ctx, subip,
		(void)npu_aicore_query_exception_dbg_info(
		aic_id, aicore_dbg_info));
	npu_exception_bbox_dump_aicore(aic_id, aicore_info, aicore_dbg_info);
	npu_exception_bbox_dump_ub(dev_ctx, aic_id);
}

void npu_exception_aicore_exception_proc(
	struct npu_dev_ctx *dev_ctx, u16 channel_id)
{
	u8 i;
	int ret = 0;
	u8 status = 0;
	u8 aic_own_bitmap = 0;
	u8 aic_exception_bitmap = 0;
	struct aicore_exception_info aicore_info = {0};
	struct aicore_exception_error_info aicore_error_info = {0};
	struct aicore_exception_dbg_info aicore_dbg_info = {0};

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_aic_own_bitmap(channel_id, &aic_own_bitmap),
		ret);
	cond_return_void(ret != 0, "cannot get aic_own_bitmap\n");

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_aic_exception_bitmap(
		channel_id, &aic_exception_bitmap), ret);
	cond_return_void(ret != 0, "cannot get aic_exception_bitmap\n");
	cond_return_void(aic_exception_bitmap == 0, "no aicore get exception\n");

	npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
		(void)npu_hwts_query_aic_task_config());

	for (i = 0; i < NPU_HWTS_MAX_AICORE_POOL_NUM; i++)
		npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
			(void)npu_hwts_query_aicore_pool_status(i, 0, &status));

	if (aic_exception_bitmap & (0x1 << 0))
		npu_exception_query_aicore_info(dev_ctx, 0, &aicore_info,
			&aicore_error_info, &aicore_dbg_info);
	if (aic_exception_bitmap & (0x1 << 1))
		npu_exception_query_aicore_info(dev_ctx, 1, &aicore_info,
			&aicore_error_info, &aicore_dbg_info);
}

void npu_exception_aicore_timeout_proc(
	struct npu_dev_ctx *dev_ctx, u16 channel_id)
{
	u8 i;
	int ret = 0;
	u8 status = 0;
	u8 aic_own_bitmap = 0;
	struct aicore_exception_info aicore_info = {0};
	struct aicore_exception_error_info aicore_error_info = {0};
	struct aicore_exception_dbg_info aicore_dbg_info = {0};

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_aic_own_bitmap(channel_id, &aic_own_bitmap),
		ret);
	cond_return_void(ret != 0, "cannot get aic_own_bitmap\n");

	if (aic_own_bitmap == 0) {
		npu_drv_warn("no aicore Occupied\n");
		return;
	}

	npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
		(void)npu_hwts_query_aic_task_config());

	for (i = 0; i < NPU_HWTS_MAX_AICORE_POOL_NUM; i++)
		npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
			(void)npu_hwts_query_aicore_pool_status(i, 0, &status));

	if (aic_own_bitmap & (0x1 << 0))
		npu_exception_query_aicore_info(dev_ctx, 0, &aicore_info,
			&aicore_error_info, &aicore_dbg_info);
	if (aic_own_bitmap & (0x1 << 1))
		npu_exception_query_aicore_info(dev_ctx, 1, &aicore_info,
			&aicore_error_info, &aicore_dbg_info);
}

void npu_exception_aicore_trap_proc(
	struct npu_dev_ctx *dev_ctx, u16 channel_id)
{
	int ret = 0;
	u8 aic_own_bitmap = 0;
	u8 aic_trap_bitmap = 0;
	struct aicore_exception_info aicore_info = {0};
	struct aicore_exception_error_info aicore_error_info = {0};
	struct aicore_exception_dbg_info aicore_dbg_info = {0};

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_aic_own_bitmap(channel_id, &aic_own_bitmap),
		ret);
	cond_return_void(ret != 0, "cannot get aic_own_bitmap\n");

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_aic_trap_bitmap(channel_id, &aic_trap_bitmap),
		ret);
	cond_return_void(ret != 0, "cannot get aic_trap_bitmap\n");

	cond_return_void(aic_trap_bitmap == 0, "no aicore get trap\n");
	if (aic_trap_bitmap & (0x1 << 0))
		npu_exception_query_aicore_info(dev_ctx, 0, &aicore_info,
			&aicore_error_info, &aicore_dbg_info);
	if (aic_trap_bitmap & (0x1 << 1))
		npu_exception_query_aicore_info(dev_ctx, 1, &aicore_info,
			&aicore_error_info, &aicore_dbg_info);
}

void npu_exception_task_trap_proc(struct npu_dev_ctx *dev_ctx,
	u16 hwts_sq_id, u16 channel_id)
{
	npu_exception_aicore_trap_proc(dev_ctx, channel_id);
	npu_exception_sqe_error_proc(dev_ctx, hwts_sq_id, channel_id);
}

void npu_exception_sdma_exception_proc(
	struct npu_dev_ctx *dev_ctx, u16 channel_id)
{
	u8 i;
	int ret = 0;
	u8 status = 0;
	u8 sdma_own_state = 0;
	u8 sdma_exception_id = 0;
	struct sdma_exception_info sdma_info = {0};

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_sdma_own_state(channel_id, &sdma_own_state),
		ret);
	cond_return_void(ret != 0, "cannot get sdma_own_state\n");
	for (sdma_exception_id = 0; sdma_exception_id < NPU_HWTS_SDMA_CHANNEL_NUM;
		sdma_exception_id++)
		if (sdma_own_state & (1 << sdma_exception_id))
			break;

	if (sdma_exception_id == NPU_HWTS_SDMA_CHANNEL_NUM) {
		npu_drv_warn("no sdma channel error\n");
		return;
	}

	for (i = 0; i < NPU_HWTS_MAX_SDMA_POOL_NUM; i++)
		npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
			(void)npu_hwts_query_sdma_pool_status(i, 0, &status));

	npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
		(void)npu_hwts_query_sdma_task_config());

	npu_pm_safe_call(dev_ctx, NPU_SUBSYS,
		(void)npu_sdma_query_exception_info(sdma_exception_id,
			&sdma_info));
}

void npu_exception_task_exception_proc(struct npu_dev_ctx *dev_ctx,
	u16 hwts_sq_id, u16 channel_id)
{
	int ret = 0;
	struct hwts_interrupt_info interrupt_info = {0};
	struct sq_exception_info sq_info = {0};
	(void)hwts_sq_id;

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_interrupt_info(&interrupt_info), ret);
	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_sq_info(channel_id, &sq_info), ret);
	npu_exception_aicore_exception_proc(dev_ctx, channel_id);
	npu_exception_sdma_exception_proc(dev_ctx, channel_id);
}

void npu_exception_task_timeout_proc(struct npu_dev_ctx *dev_ctx,
	u16 hwts_sq_id, u16 channel_id)
{
	int ret = 0;
	struct hwts_interrupt_info interrupt_info = {0};
	struct sq_exception_info sq_info = {0};
	(void)hwts_sq_id;

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_interrupt_info(&interrupt_info), ret);
	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_sq_info(channel_id, &sq_info), ret);
	npu_exception_aicore_timeout_proc(dev_ctx, channel_id);
	npu_exception_sdma_exception_proc(dev_ctx, channel_id);
}

void npu_exception_ispnn_error_proc(struct npu_dev_ctx *dev_ctx, u8 exception_type,
	u16 channel_id)
{
	int ret = 0;
	struct hwts_interrupt_info interrupt_info = {0};
	struct sq_exception_info sq_info = {0};

	npu_drv_warn("channel_id = %u, exception_type = %u\n", channel_id, exception_type);

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_interrupt_info(&interrupt_info), ret);
	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_sq_info(channel_id, &sq_info), ret);
	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_ispnn_info(channel_id), ret);
	npu_exception_aicore_timeout_proc(dev_ctx, channel_id);
	npu_exception_sdma_exception_proc(dev_ctx, channel_id);
}

void npu_exception_powerdown(struct npu_dev_ctx *dev_ctx, struct hwts_exception_report_info *report)
{
	struct npu_proc_ctx *proc_ctx = NULL;
	struct npu_proc_ctx *next_proc_ctx = NULL;
	cond_return_void(dev_ctx == NULL, "dev_ctx is null\n");
	cond_return_void(report == NULL, "report is null\n");

	down_write(&dev_ctx->pm.exception_lock);
	if (dev_ctx->pm.npu_exception_status == NPU_STATUS_EXCEPTION) {
		up_write(&dev_ctx->pm.exception_lock);
		return;
	}
	dev_ctx->pm.npu_exception_status = NPU_STATUS_EXCEPTION;
	dev_ctx->pm.exception_task.task_id = report->task_id;
	dev_ctx->pm.exception_task.persist_stream_id = report->persist_stream_id;
	dev_ctx->pm.exception_task.persist_task_id = report->persist_task_id;
	npu_pm_exception_powerdown(dev_ctx);

	mutex_lock(&dev_ctx->npu_power_mutex);
	if (dev_ctx->pm.wm_cnt[NPU_NONSEC] == 0 && dev_ctx->pm.npu_pm_vote == NPU_PM_VOTE_STATUS_PD)
		dev_ctx->pm.npu_exception_status = NPU_STATUS_NORMAL;
	list_for_each_entry_safe(proc_ctx, next_proc_ctx, &dev_ctx->proc_ctx_list,
		dev_ctx_list) {
		wake_up(&proc_ctx->report_wait);
	}
	mutex_unlock(&dev_ctx->npu_power_mutex);
	up_write(&dev_ctx->pm.exception_lock);
}

void npu_exception_report_proc(
	struct npu_dev_ctx *dev_ctx, struct hwts_exception_report_info *report)
{
	static void (*exception_func[NPU_EXCEPTION_TYPE_MAX])
		(struct npu_dev_ctx *, u16, u16) = {
		npu_exception_task_exception_proc,
		npu_exception_task_timeout_proc,
		npu_exception_task_trap_proc,
		npu_exception_sqe_error_proc,
		npu_exception_sw_status_error_proc,
		npu_exception_bus_error_proc,
		npu_exception_pool_conflict_proc
	};

	cond_return_void(report == NULL, "report is null\n");
	cond_return_void(report->exception_type >= NPU_EXCEPTION_TYPE_MAX,
		"unknown exception type\n");

	npu_drv_warn("get hwts exception report info: model_id = %u, "
		"persist_stream_id = %u, persist_task_id = %u, channel_id = %u, "
		"hwts_sq_id = %u, task_id = %u, exception_type = %u\n",
		report->model_id, report->persist_stream_id,
		report->persist_task_id, report->channel_id,
		report->hwts_sq_id, report->task_id, report->exception_type);

	if (report->service_type == NPU_SERVICE_TYPE_ISPNN) {
		npu_exception_ispnn_error_proc(dev_ctx, report->exception_type, report->channel_id);
		return;
	}

	// 1. print exception info
	(exception_func[report->exception_type])(dev_ctx,
		report->hwts_sq_id, report->channel_id);

	// 2.  set exception status + powerdown +wakeup
	if (list_empty_careful(&dev_ctx->proc_ctx_list)) {
		npu_drv_warn("the device has closed!\n");
		return;
	}
	npu_exception_powerdown(dev_ctx, report);
}

void npu_exception_timeout_record(struct npu_dev_ctx *dev_ctx)
{
	int ret = 0;
	uint32_t channel_id;
	uint32_t channel_num = NPU_HWTS_CHANNEL_NUM - 8; /* ISPNN&NON_SEC HWTS_CHANNEL*/
	struct hwts_interrupt_info interrupt_info = {0};
	struct sq_exception_info sq_info = {0};

	npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
		npu_hwts_query_interrupt_info(&interrupt_info), ret);

	/* timeout without hwts exception, check&&dump all channel info */
	for (channel_id = 0; channel_id < channel_num; channel_id++) {
		npu_drv_warn("channel_id = %u\n", channel_id);
		npu_pm_safe_call_with_return(dev_ctx, NPU_SUBSYS,
			npu_hwts_query_sq_info(channel_id, &sq_info), ret);
		npu_exception_aicore_timeout_proc(dev_ctx, channel_id);
		npu_exception_sdma_exception_proc(dev_ctx, channel_id);
	}
}
