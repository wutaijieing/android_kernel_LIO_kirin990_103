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

#ifndef DPUFE_VSYNC_H
#define DPUFE_VSYNC_H

#include <linux/types.h>
#include <linux/mutex.h>

#include "dpufe_sysfs.h"

struct dpufe_vsync {
	wait_queue_head_t vsync_wait;
	ktime_t timestamp;
	spinlock_t vsync_lock;
	int32_t enabled;
	int32_t vsync_created;
};

void dpufe_vsync_register(struct dpufe_vsync *vsync_ctrl, struct dpufe_attr *vattrs);
void dpufe_vsync_unregister(struct dpufe_vsync *vsync_ctrl, struct dpufe_attr *vattrs);
int dpufe_vsync_ctrl(struct dpufe_vsync *vsync_ctrl, const void __user *argp);
void dpufe_vsync_isr_handler(struct dpufe_vsync *vsync_ctrl, uint64_t timestamp, uint32_t fb_index);

#endif
