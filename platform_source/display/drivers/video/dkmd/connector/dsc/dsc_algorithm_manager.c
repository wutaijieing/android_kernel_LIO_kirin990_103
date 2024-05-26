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
#include <linux/slab.h>
#include "dsc_debug.h"
#include "dsc_algorithm_manager.h"

static dsc_algorithm_manager_t *_dsc_algorithm_manager;
static struct dpu_dsc_algorithm g_dsc_algorithm;

void init_dsc_algorithm_manager(dsc_algorithm_manager_t *p)
{
	dpu_pr_info("[DSC]+!\n");

	init_dsc_algorithm(&g_dsc_algorithm);
	p->vesa_dsc_info_calc = g_dsc_algorithm.vesa_dsc_info_calc;
}

dsc_algorithm_manager_t *get_dsc_algorithm_manager_instance(void)
{
	dpu_pr_info("[DSC]+!\n");
	if (_dsc_algorithm_manager != NULL) {
		return _dsc_algorithm_manager;
	} else {
		_dsc_algorithm_manager = (dsc_algorithm_manager_t *) kzalloc(sizeof(dsc_algorithm_manager_t), GFP_KERNEL);
		if (!_dsc_algorithm_manager) {
			dpu_pr_err("[DSC] malloc fail!\n");
			return NULL;
		}
		init_dsc_algorithm_manager(_dsc_algorithm_manager);
		return _dsc_algorithm_manager;
	}
	return NULL;
}
