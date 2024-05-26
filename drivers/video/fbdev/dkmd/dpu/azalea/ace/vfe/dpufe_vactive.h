/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef DPUFE_VACTIVE_H
#define DPUFE_VACTIVE_H

#include <linux/types.h>
#include <linux/wait.h>

struct dpufe_vstart_mgr {
	uint32_t allowable_isr_loss_num;
};

void dpufe_vstart_isr_register(struct dpufe_vstart_mgr *vstart_ctl);
void dpufe_check_last_frame_start_working(struct dpufe_vstart_mgr *vstart_ctl);
void dpufe_vstart_isr_handler(struct dpufe_vstart_mgr *vstart_ctl);

#endif

