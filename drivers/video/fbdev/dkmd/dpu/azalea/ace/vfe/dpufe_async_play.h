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

#ifndef DPUFE_ASYNC_PLAY_H
#define DPUFE_ASYNC_PLAY_H

#include "dpufe.h"

int dpufe_ov_async_play(struct dpufe_data_type *dfd, void __user *argp);
int dpufe_get_release_and_retire_fence(struct dpufe_data_type *dfd, void __user *argp);
void dpufe_resume_async_state(struct dpufe_data_type *dfds);
void wait_last_frame_start_working(struct dpufe_data_type *dfd);

#endif
