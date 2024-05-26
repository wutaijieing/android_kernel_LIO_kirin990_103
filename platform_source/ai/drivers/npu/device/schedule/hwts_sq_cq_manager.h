/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-08
 */
#ifndef __HWTS_SQ_CQ_MANAGER_H
#define __HWTS_SQ_CQ_MANAGER_H

#include <linux/types.h>
#include "hwts_sq_cq.h"

void hwts_sq_cq_mngr_init(void);
struct hwts_sq_cq* hwts_sq_cq_mngr_get_sqcq(uint16_t id);
#endif