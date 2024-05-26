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

#ifndef OPR_CMD_DATA_H
#define OPR_CMD_DATA_H

#include <linux/types.h>
#include "dkmd_opr_id.h"
#include "dkmd_base_layer.h"
#include "dkmd_rect.h"

struct dpu_dm_param;
struct opr_cmd_data;

struct opr_common_data {
	int32_t format;
	struct dkmd_rect_coord rect;
};

struct opr_cfg_addr_info {
	uint32_t reg_addr;
	uint32_t reg_size;
	void *payload_addr;
};

struct opr_cmd_data_base {
	union dkmd_opr_id opr_id;
	int32_t scene_id;
	int32_t scene_mode;
	struct dpu_dm_param *dm_param;
	struct opr_common_data in_common_data;
	struct opr_common_data out_common_data;
	uint32_t module_offset;
	uint32_t cfg_addr_num;
	struct opr_cfg_addr_info *cfg_addr_info; // operator transport mode cmdlist config address
	uint32_t reg_cmdlist_id;
};

typedef struct opr_cmd_data *(*init_cmd_data_func)(union dkmd_opr_id);

struct opr_cmd_data {
	struct opr_cmd_data_base *data;

	/**
	* @brief set operator's register data function, each operator's instance has one implementation
	*
	* @param cmd_data operator's own cmd data struct with its necessary information and handle function
	* @param base_layer the layer used by this operator
	* @param pre_cmd_data pre operator's own cmd data struct
	* @param next_cmd_datas an array of next operators' own cmd data struct
	* @param next_oprs_num number of next operators
	* @return int32_t indicate success or fail
	*/
	int32_t (*set_data)(struct opr_cmd_data *cmd_data, const struct dkmd_base_layer *base_layer,
		const struct opr_cmd_data *pre_cmd_data, const struct opr_cmd_data **next_cmd_datas, uint32_t next_oprs_num);

	/**
	* @brief online composition with given parameters
	*
	* @param cfg_addr_info an array of operator's transport mode cmdlist config address
	* @param cfg_addr_num number of config address
	* @return int32_t indicate success or fail
	*/
	int32_t (*get_cfg_addr)(struct opr_cfg_addr_info **cfg_addr_info, uint32_t *cfg_addr_num);
};

void set_common_cmd_data(struct opr_cmd_data *cmd_data, const struct opr_cmd_data *pre_cmd_data);

int32_t opr_dpu_to_soc_type(int32_t opr_dpu_type);

#endif