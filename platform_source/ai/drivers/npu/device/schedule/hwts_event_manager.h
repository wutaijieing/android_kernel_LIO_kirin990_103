/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-08
 */
#ifndef __HWTS_EVENT_MANAGER_H
#define __HWTS_EVENT_MANAGER_H

#include <linux/types.h>
#include <linux/list.h>
struct hwts_event{
	struct list_head node;
	uint16_t id;
};

void hwts_event_mngr_init(void);
struct hwts_event* hwts_event_mngr_get_event(uint16_t id);

#endif