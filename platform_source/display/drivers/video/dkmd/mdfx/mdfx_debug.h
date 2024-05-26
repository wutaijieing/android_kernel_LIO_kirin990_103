/*
 * Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef MDFX_DEBUG_H
#define MDFX_DEBUG_H

#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/wait.h>

struct mdfx_pri_data;

// level manager
extern int mdfx_debug_level;

#define MDFX_UPDATE_DEBUGGER_STR "update="
#define MDFX_SET_LOG_FILE_COUNT_STR "log_file_count="
#define MDFX_DEBUG_SET_CAPS_STR "mdfx_caps="

#define MDFX_LOG_FILE_MAX_COUNT 100

struct mdfx_debugger_t {
	struct semaphore debugger_sem;
	wait_queue_head_t debugger_wait;
	uint32_t need_update_debugger;
};

void mdfx_debugger_init(struct mdfx_pri_data *mdfx_data);

#endif
