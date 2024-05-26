/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: dsc algorithm manager
 * Author: yangbr
 * Create: 2020-01-22
 * Notes: null
 */
#include "dsc_algorithm_manager.h"
#include "hisi_fb.h"
static dsc_algorithm_manager_t *_dsc_algorithm_manager;
static struct hisi_dsc_algorithm g_dsc_algorithm;

void init_dsc_algorithm_manager(dsc_algorithm_manager_t *p)
{
	disp_pr_debug("[DSC]+!\n");

	init_dsc_algorithm(&g_dsc_algorithm);
	p->vesa_dsc_info_calc = g_dsc_algorithm.vesa_dsc_info_calc;
}

dsc_algorithm_manager_t *get_dsc_algorithm_manager_instance(void)
{
	disp_pr_debug("[DSC]+!\n");
	if (_dsc_algorithm_manager != NULL) {
		return _dsc_algorithm_manager;
	} else {
		_dsc_algorithm_manager = (struct dsc_algorithm_manager *)
			kmalloc(sizeof(struct dsc_algorithm_manager), GFP_KERNEL);
		if (!_dsc_algorithm_manager) {
			disp_pr_err("[DSC] malloc fail!\n");
			return NULL;
		}
		init_dsc_algorithm_manager(_dsc_algorithm_manager);
		return _dsc_algorithm_manager;
	}
	return NULL;
}

