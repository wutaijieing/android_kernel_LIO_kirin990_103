/*
 * schedule_interface.h
 *
 * about schedule interface
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
#include <linux/types.h>
#include "npu_proc_ctx.h"
#include "npu_comm_sqe_fmt.h"

int32_t task_sched_init(void);
void task_sched_deinit(void);
int32_t task_sched_put_request(struct npu_proc_ctx *proc_ctx, struct npu_rt_task *task);
int32_t task_sched_get_response(struct npu_proc_ctx *proc_ctx, struct npu_receive_response_info *report_info);
