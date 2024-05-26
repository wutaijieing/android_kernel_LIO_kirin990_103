/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-08
 */
#ifndef __SCHEDULE_MODEL_MANAGER_H
#define __SCHEDULE_MODEL_MANAGER_H

#include <linux/types.h>
#include <linux/list.h>

#include "schedule_model.h"

void sched_model_mngr_init(void);
struct schedule_model* sched_model_mngr_get_model(uint16_t schde_model_id);

#endif