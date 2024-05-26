/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef DPU_FRAME_BUFFER_MGR_H
#define DPU_FRAME_BUFFER_MGR_H

#include "dpu_disp_merger_mgr.h"

void wait_buffer_available(struct disp_merger_mgr *merger_mgr);

void dpu_lock_buf_sync(struct disp_merger_mgr *merger_mgr);

void dpu_unlock_buf_sync(void);

void init_buf_sync_mgr(void);

void dpu_init_buf_timeline(void);

#endif
