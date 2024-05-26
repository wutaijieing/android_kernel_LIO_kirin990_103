/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DKMD_SENSORHUB_BLPWM_H__
#define __DKMD_SENSORHUB_BLPWM_H__

#include "dkmd_backlight_common.h"

struct bl_sh_blpwm_data {
	struct file *file;
};

void dkmd_sh_blpwm_init(struct dkmd_bl_ctrl *bl_ctrl);
#endif
