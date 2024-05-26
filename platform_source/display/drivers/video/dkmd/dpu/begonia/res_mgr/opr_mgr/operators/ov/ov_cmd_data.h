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

#ifndef OV_CMD_DATA_H
#define OV_CMD_DATA_H

#include <linux/types.h>
#include "dkmd_opr_id.h"

struct dkmd_base_layer;
struct opr_cmd_data;

struct opr_cmd_data *init_ov_cmd_data(union dkmd_opr_id id);
int32_t opr_set_ov_data(struct opr_cmd_data *cmd_data, const struct dkmd_base_layer *base_layer,
	const struct opr_cmd_data *pre_cmd_data, const struct opr_cmd_data **next_cmd_datas, uint32_t next_oprs_num);

#endif