/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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

#ifndef DPU_DISP_RECORDER_H
#define DPU_DISP_RECORDER_H

#include <linux/hrtimer.h>

struct disp_state_recorder {
	struct hrtimer disp_timer;
	void *par;
};

void init_disp_recorder(struct disp_state_recorder *disp_recorder, void *data);
void enable_disp_recorder(struct disp_state_recorder *disp_recorder);
void disable_disp_recorder(struct disp_state_recorder *disp_recorder);

#endif
