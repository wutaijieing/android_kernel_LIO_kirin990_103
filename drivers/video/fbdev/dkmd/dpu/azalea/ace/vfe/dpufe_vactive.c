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

#include <linux/wait.h>
#include <linux/delay.h>

#include "dpufe.h"
#include "dpufe_dbg.h"
#include "dpufe_vactive.h"
#include "dpufe_ov_utils.h"
#include "dpufe_fb_buf_sync.h"

#define MAX_ALLOW_LOSS_NUM 15

static struct dpufe_data_type *to_dfd(struct dpufe_vstart_mgr *vstart_ctl)
{
	return container_of(vstart_ctl, struct dpufe_data_type, vstart_ctl);
}

void dpufe_vstart_isr_register(struct dpufe_vstart_mgr *vstart_ctl)
{
	dpufe_check_and_no_retval(!vstart_ctl, "vstart_ctl is null\n");

	vstart_ctl->allowable_isr_loss_num = MAX_ALLOW_LOSS_NUM;
}

void dpufe_check_last_frame_start_working(struct dpufe_vstart_mgr *vstart_ctl)
{
	struct dpufe_data_type *dfd = NULL;

	dpufe_check_and_no_retval(!vstart_ctl, "vstart_ctl is null\n");
	dfd = to_dfd(vstart_ctl);

	if (vstart_ctl->allowable_isr_loss_num == 0) {
		DPUFE_ERR("fb%u wait vstart times timeout, resync fence!\n", dfd->index);
		dpufe_reset_all_buf_fence(dfd);
		dump_dbg_queues(dfd->index);
		vstart_ctl->allowable_isr_loss_num = MAX_ALLOW_LOSS_NUM;
	}

	vstart_ctl->allowable_isr_loss_num--;
}

void dpufe_vstart_isr_handler(struct dpufe_vstart_mgr *vstart_ctl)
{
	dpufe_check_and_no_retval(!vstart_ctl, "vstart_ctl is null\n");

	vstart_ctl->allowable_isr_loss_num = MAX_ALLOW_LOSS_NUM;
}
