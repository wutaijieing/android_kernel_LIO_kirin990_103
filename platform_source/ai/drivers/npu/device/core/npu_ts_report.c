/*
 * npu_ts_report.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
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
#include "npu_log.h"
#include "npu_calc_cq.h"
#include "npu_user_common.h"
#include "npu_proc_ctx.h"
#include "npu_common.h"
#include "npu_shm.h"
#include "npu_message.h"
#include "npu_pm_framework.h"
#include "npu_irq_adapter.h"
#include "npu_platform.h"

static u64 g_recv_cq_int_num; // use for debug
static u64 g_find_cq_index_called_num; // use for debug
struct npu_cq_report_int_ctx g_cq_int_ctx;

static void npu_find_cq_index(unsigned long data)
{
	struct npu_cq_sub_info *cq_sub_info = NULL;
	struct npu_cq_sub_info *next_cq_sub_info = NULL;
	struct npu_ts_cq_info *cq_info = NULL;
	struct npu_proc_ctx *proc_ctx = NULL;
	struct npu_proc_ctx *next_proc_ctx = NULL;
	struct npu_dev_ctx *cur_dev_ctx = NULL;

	g_find_cq_index_called_num++; // user for debug,compare with ts side
	cur_dev_ctx = get_dev_ctx_by_id(((struct npu_cq_report_int_ctx *)(uintptr_t)data)->dev_id);
	if (cur_dev_ctx == NULL)
		return;

	spin_lock(&cur_dev_ctx->proc_ctx_lock);
	if (list_empty_careful(&cur_dev_ctx->proc_ctx_list)) {
		spin_unlock(&cur_dev_ctx->proc_ctx_lock);
		return;
	}

	/* walk proc_ctx */
	list_for_each_entry_safe(proc_ctx, next_proc_ctx, &cur_dev_ctx->proc_ctx_list,
		dev_ctx_list) {
		/* walk cq_list */
		list_for_each_entry_safe(cq_sub_info, next_cq_sub_info, &proc_ctx->cq_list,
			list) {
			cq_info = npu_calc_cq_info(proc_ctx->devid, cq_sub_info->index);
			if (cq_info == NULL) {
				/* condition is true, continue */
				npu_drv_debug("cq info is null, pid = %d\n", proc_ctx->pid);
				continue;
			}

			npu_update_cq_tail_sq_head(proc_ctx, cq_sub_info, cq_info);
			/* do not have valid cqe */
			if (cq_info->head == cq_info->tail)
				continue;

			npu_drv_debug("receive report irq, cq id: %u, cq_head = %u "
				"cq_tail = %u report is valid, wake up runtime thread\n",
				cq_info->index, cq_info->head, cq_info->tail);
			wake_up(&proc_ctx->report_wait);
		}
	}

	spin_unlock(&cur_dev_ctx->proc_ctx_lock);
}

irqreturn_t npu_irq_handler(int irq, void *data)
{
	struct npu_cq_report_int_ctx *int_context = NULL;
	unsigned long flags;
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	unused(irq);

	int_context = (struct npu_cq_report_int_ctx *)data;
	cur_dev_ctx = get_dev_ctx_by_id(int_context->dev_id);
	if (cur_dev_ctx == NULL)
		return IRQ_HANDLED;

	if (cur_dev_ctx->power_stage != NPU_PM_UP)
		return IRQ_HANDLED;

	local_irq_save(flags);
	g_recv_cq_int_num++; // user for debug,compare with ts side
	npu_plat_handle_irq_tophalf(NPU_IRQ_CALC_CQ_UPDATE0);

	tasklet_schedule(&int_context->find_cq_task);

	local_irq_restore(flags);

	return IRQ_HANDLED;
}

static int __npu_request_cq_report_irq_bh(
	struct npu_cq_report_int_ctx *cq_int_ctx)
{
	int ret;
	unsigned int cq_irq;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail\n");
		return -EINVAL;
	}

	if (cq_int_ctx == NULL) {
		npu_drv_err("cq report int_context is null ");
		return -1;
	}

	cq_int_ctx->dev_id = 0;
	tasklet_init(&cq_int_ctx->find_cq_task, npu_find_cq_index,
		(uintptr_t)cq_int_ctx);

	cq_irq = (unsigned int)plat_info->dts_info.irq_cq_update;

	ret = request_irq(cq_irq, npu_irq_handler, IRQF_TRIGGER_NONE,
		"npu_cq_report_handler", cq_int_ctx);
	if (ret != 0) {
		npu_drv_err("request cq report irq failed\n");
		goto request_failed0;
	}
	npu_drv_debug("request cq report irq %d success\n", cq_irq);

	return ret;

request_failed0:
	tasklet_kill(&cq_int_ctx->find_cq_task);
	return ret;
}

static int __npu_free_cq_report_irq_bh(
	struct npu_cq_report_int_ctx *cq_int_ctx)
{
	unsigned int cq_irq;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail\n");
		return -EINVAL;
	}

	if (cq_int_ctx == NULL) {
		npu_drv_err("cq report int_context is null ");
		return -EINVAL;
	}

	cq_irq = (unsigned int)plat_info->dts_info.irq_cq_update;
	free_irq(cq_irq, cq_int_ctx);
	tasklet_kill(&cq_int_ctx->find_cq_task);
	return 0;
}

int npu_request_cq_report_irq_bh(void)
{
	return __npu_request_cq_report_irq_bh(&g_cq_int_ctx);
}

int npu_free_cq_report_irq_bh(void)
{
	return __npu_free_cq_report_irq_bh(&g_cq_int_ctx);
}
