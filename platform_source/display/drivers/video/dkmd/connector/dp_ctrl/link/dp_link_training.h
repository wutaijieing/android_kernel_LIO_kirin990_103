/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef __DP_LINK_TRAINING_H__
#define __DP_LINK_TRAINING_H__

#include <linux/kernel.h>

struct dp_ctrl;

/**
 * Link training
 */
int dptx_link_training(struct dp_ctrl *dptx, uint8_t rate, uint8_t lanes);
int dptx_link_check_status(struct dp_ctrl *dptx);
int dptx_link_adjust_drive_settings(struct dp_ctrl *dptx, int *out_changed);
int dptx_link_retraining(struct dp_ctrl *dptx, u8 rate, u8 lanes);

#endif /* DP_LINK_TRAINING_H */
