/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-08
 */
#ifndef __EVENT_MANAGER_H
#define __EVENT_MANAGER_H

#include <linux/types.h>
#include "event.h"

void event_mngr_init(void);
struct event* event_mngr_get_event(uint16_t id);

#endif