/*
 * npu_pm_framework.h
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
#ifndef __NPU_PM_FRAMEWORK_H
#define __NPU_PM_FRAMEWORK_H

#include "npu_proc_ctx.h"
#include "npu_platform_pm.h"

#define NPU_SUBIP_ALL \
	(~((0xFFFFFFFFU >> (unsigned int)(NPU_SUBIP_MAX)) << (unsigned int)(NPU_SUBIP_MAX)))

enum npu_power_stage {
	NPU_PM_DOWN,
	NPU_PM_NPUCPU,
	NPU_PM_SMMU,
	NPU_PM_TS,
	NPU_PM_UP
};

enum npu_work_status {
	WORK_IDLE,
	WORK_ADDING,
	WORK_CANCELING,
	WORK_ADDED
};

int npu_sync_ts_time(void);

int npu_ctrl_core(u32 dev_id, u32 core_num);

int npu_pm_proc_ioctl_enter_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode);

int npu_pm_proc_ioctl_exit_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode);

int npu_pm_proc_send_task_enter_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode);

int npu_pm_proc_release_task_exit_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx, u32 work_mode, u32 num);

int npu_pm_proc_release_exit_wm(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx);

int npu_pm_vote(struct npu_dev_ctx *dev_ctx, u32 workmode);

int npu_pm_unvote(struct npu_dev_ctx *dev_ctx, u32 workmode);

int npu_pm_reboot(struct npu_proc_ctx *proc_ctx,
	struct npu_dev_ctx *dev_ctx);

int npu_pm_exception_powerdown(struct npu_dev_ctx *dev_ctx);

int npu_pm_resource_init(struct npu_dev_ctx *dev_ctx);

int npu_pm_get_delta_subip_set(struct npu_power_manage *handle, u32 work_mode,
	int pm_ops);

void npu_pm_delete_idle_timer(struct npu_dev_ctx *dev_ctx);

void npu_pm_add_idle_timer(struct npu_dev_ctx *dev_ctx);

int npu_interframe_enable(struct npu_proc_ctx *proc_ctx, uint32_t flag);

void npu_pm_adapt_init(struct npu_dev_ctx *dev_ctx);
#endif
