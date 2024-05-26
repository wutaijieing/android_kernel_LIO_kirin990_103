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

#ifndef DPUFE_OV_PLAY_H
#define DPUFE_OV_PLAY_H

#include "dpufe.h"

#define DPU_COMPOSER_HOLD_TIME (1000UL * 3600 * 24 * 5)

int dpufe_ov_online_play(struct dpufe_data_type *dfd, void __user *argp);
int dpufe_overlay_init(struct dpufe_data_type *dfd);
int dpufe_evs_switch(struct dpufe_data_type *dfd, const void __user *argp);

#endif
