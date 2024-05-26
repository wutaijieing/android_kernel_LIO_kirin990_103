/* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef DPU_OVERLAY_ASYNCHRONOUS_PLAY_H
#define DPU_OVERLAY_ASYNCHRONOUS_PLAY_H

#include "dpu_fb.h"

int dpu_get_release_and_retire_fence(struct dpu_fb_data_type *dpufd, void __user *argp);
int dpu_ov_asynchronous_play(struct dpu_fb_data_type *dpufd, void __user *argp);

#endif /* DPU_OVERLAY_ASYNCHRONOUS_PLAY_H */
