/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: dsc algorithm manager
 * Create: 2020-01-22
 * Notes: null
 */
#ifndef __DPTX_DSC_MANAGER_H__
#define __DPTX_DSC_MANAGER_H__
#include "dsc_algorithm.h"

typedef struct dsc_algorithm_manager {
	void (*vesa_dsc_info_calc)(const struct input_dsc_info *input_para,
		struct dsc_info *output_para, struct dsc_info *exist_para);
	void (*dsc_config_rc_table)(struct dsc_info *dsc_info, struct rc_table_param *user_rc_table, uint32_t rc_table_len);
} dsc_algorithm_manager_t;

dsc_algorithm_manager_t *get_dsc_algorithm_manager_instance(void);

#endif
