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
#ifndef MDFX_EVENT_MANAGER_H
#define MDFX_EVENT_MANAGER_H

#include <platform_include/display/linux/dpu_mdfx.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/wait.h>

#define MDFX_EVENT_NAME_PREFIX "event["
#define MDFX_NORMAL_NAME_PREFIX "normal"

#define MDFX_REPORT_EVENT_INTERVAL_SEC  5

struct mdfx_pri_data;

typedef int (*listener_act_func)(struct mdfx_pri_data *mdfx_data, struct mdfx_event_desc *desc);

struct mdfx_event_listener {
	struct list_head node;

	uint32_t actor_type;
	listener_act_func handle;
};

struct mdfx_event_node {
	struct list_head node;

	char event_name[MDFX_EVENT_NAME_MAX];
	struct timespec time;
};

struct mdfx_event_manager_t {
	struct semaphore event_sem;
	struct mdfx_event_desc event_desc;
	wait_queue_head_t event_wait;
	uint32_t need_report_event;

	struct list_head event_list;
	struct list_head listener_list;
	struct mdfx_event_desc_t default_event[DEF_EVENT_MAX];
};

extern struct mdfx_pri_data *g_mdfx_data;

void mdfx_event_deinit(struct mdfx_event_manager_t *event_manager);
void mdfx_event_init(struct mdfx_pri_data *mdfx_data);
int mdfx_event_register_listener(uint32_t actor_type, listener_act_func cb);
int mdfx_deliver_event(struct mdfx_pri_data *mdfx_data, const void __user *argp);
uint64_t mdfx_event_get_action_detail(const struct mdfx_event_desc *desc, uint32_t actor_type);
int mdfx_event_check_parameter(struct mdfx_event_desc *desc);
struct mdfx_event_desc *mdfx_copy_event_desc(struct mdfx_event_desc *desc);


#endif
