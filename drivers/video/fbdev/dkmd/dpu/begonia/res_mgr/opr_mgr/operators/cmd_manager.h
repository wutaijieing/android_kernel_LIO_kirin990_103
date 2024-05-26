/**
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
 */

#ifndef CMD_MANAGER_H
#define CMD_MANAGER_H

#include <linux/types.h>

#include "dkmd_base_frame.h"
#include "dkmd_network.h"

int32_t generate_network_cmd(struct dkmd_base_frame *frame, struct dkmd_network *network);

#endif