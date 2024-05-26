/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include <linux/types.h>
#include <linux/module.h>
#include "hisi_drv_test.h"
#include "hisi_disp_debug.h"

struct hisi_drv_test_module g_hisi_disp_test_module;

void hisi_disp_test_register_test_module(uint32_t test_module_type, void *monitor_data)
{
	g_hisi_disp_test_module.dte_test_module[test_module_type] = monitor_data;

	disp_pr_info("type=%d register test data %p", test_module_type, monitor_data);
}

void *hisi_disp_test_get_module_data(uint32_t type)
{
	return g_hisi_disp_test_module.dte_test_module[type];
}
EXPORT_SYMBOL(hisi_disp_test_get_module_data);

