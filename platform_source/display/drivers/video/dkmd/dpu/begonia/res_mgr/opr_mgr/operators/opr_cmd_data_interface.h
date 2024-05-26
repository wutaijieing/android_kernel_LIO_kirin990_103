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

#ifndef OPR_CMD_DATA_INTERFACE_H
#define OPR_CMD_DATA_INTERFACE_H

#include <linux/types.h>
#include "dkmd_opr_id.h"
#include "opr_cmd_data.h"

void init_opr_cmd_data(void);
void deinit_opr_cmd_data(void);
struct opr_cmd_data *get_opr_cmd_data(union dkmd_opr_id opr_id);

#endif