/*
 * hwts_interrupt.c
 *
 * about hwts interrupt
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
#include <linux/irq.h>
#include "hwts_interrupt_handler.h"
#include "irq_bottom_half.h"
#include "npu_log.h"
#include "npu_platform.h"
#include "schedule_task_process.h"
#include "hwts_interrupt.h"

static struct hwts_irq g_hwts_normal_irq;
static struct hwts_irq g_hwts_error_irq;

int hwts_irq_init(void)
{
	int ret = 0;
	uint32_t irq_normal_num;
	uint32_t irq_error_num;
	enum npu_exception_type type;

	// register top half irq
	struct npu_platform_info *plat_info = npu_plat_get_info();
	cond_return_error(plat_info == NULL, -1, "npu_plat_get_info failed");

	irq_normal_num = (uint32_t)plat_info->dts_info.irq_swts_hwts_normal;
	irq_error_num = (uint32_t)plat_info->dts_info.irq_swts_hwts_error;
	hwts_irq_get_ctx((uint64_t)(uintptr_t)plat_info->dts_info.reg_vaddr[NPU_REG_HWTS_BASE],
		irq_normal_num, &g_hwts_normal_irq);
	hwts_error_irq_get_ctx((uint64_t)(uintptr_t)plat_info->dts_info.reg_vaddr[NPU_REG_HWTS_BASE],
		irq_error_num, &g_hwts_error_irq);
	ret = request_irq(irq_normal_num, hwts_irq_handler, IRQF_TRIGGER_NONE, "hwts_irq_handler", &g_hwts_normal_irq);
	cond_return_error(ret != 0, ret, "request_irq for normal irq error");
	ret = request_irq(irq_error_num, hwts_irq_handler, IRQF_TRIGGER_NONE, "hwts_irq_handler", &g_hwts_error_irq);
	cond_goto_error(ret != 0, error_unregister_nor_irq, ret, ret, "request_irq for error irq error");

	// register bottom half irq
	ret = irq_bh_init();
	cond_goto_error(ret != 0, error_unregister_err_irq, ret, ret, "irq_bh_init error");

	ret = irq_bh_register_handler(HWTS_IRQ_NORMAL, HWTS_SQ_DONE_NS, sched_task_proc_hwts_sq_done);
	cond_goto_error(ret != 0, error_deinit, ret, ret, "irq_bh_register_handler error");

	for (type = NPU_EXCEPTION_TYPE_HWTS_TASK_EXCEPTION; type < NPU_EXCEPTION_TYPE_MAX; type++) {
		ret = irq_bh_register_handler(HWTS_IRQ_ERROR, type, sched_task_proc_hwts_error);
		cond_goto_error(ret != 0, error_deinit, ret, ret, "irq_bh_register_handler error");
	}
	return ret;

error_deinit:
	irq_bh_deinit();
error_unregister_err_irq:
	free_irq(irq_error_num, &g_hwts_error_irq);
error_unregister_nor_irq:
	free_irq(irq_normal_num, &g_hwts_normal_irq);
	return ret;
}

void hwts_irq_deinit(void)
{
	int ret = 0;
	enum npu_exception_type type;

	free_irq(g_hwts_normal_irq.irq_num, &g_hwts_normal_irq);
	free_irq(g_hwts_error_irq.irq_num, &g_hwts_error_irq);

	ret = irq_bh_unregister_handler(HWTS_IRQ_NORMAL, HWTS_SQ_DONE_NS);
	if (ret != 0)
		npu_drv_err("unregister bh failed, irq_type = %u, event_type = %u.\n", HWTS_IRQ_NORMAL, HWTS_SQ_DONE_NS);

	for (type = NPU_EXCEPTION_TYPE_HWTS_TASK_EXCEPTION; type < NPU_EXCEPTION_TYPE_MAX; type++) {
		ret = irq_bh_unregister_handler(HWTS_IRQ_ERROR, type);
		npu_drv_err("unregister bh failed, irq_type = %u, event_type = %u.\n", HWTS_IRQ_ERROR, type);
	}

	irq_bh_deinit();
}
