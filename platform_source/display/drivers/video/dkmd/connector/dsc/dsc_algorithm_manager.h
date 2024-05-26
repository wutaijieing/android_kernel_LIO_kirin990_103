/**
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
#ifndef DPTX_DSC_MANAGER_H
#define DPTX_DSC_MANAGER_H

#include "dsc_algorithm.h"

typedef struct dsc_algorithm_manager {
	void (*vesa_dsc_info_calc)(const struct input_dsc_info *input_para,
		struct dsc_info *output_para, struct dsc_info *exist_para);
	void (*dsc_config_rc_table)(struct dsc_info *dsc_info, struct rc_table_param *user_rc_table, uint32_t rc_table_len);
} dsc_algorithm_manager_t;

dsc_algorithm_manager_t *get_dsc_algorithm_manager_instance(void);

#endif
