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

#ifndef HISI_DISP_TEST_H
#define HISI_DISP_TEST_H

#include <linux/types.h>

enum {
	DTE_TEST_MODULE_FB = 0,
	DTE_TEST_MODULE_DISPLAY,
	DTE_TEST_MODULE_FACTORY,
	DTE_TEST_MODULE_OVERLAY_DRV,
	DTE_TEST_MODULE_OVERLAY_PIPELINE,
	DTE_TEST_MODULE_OVERLAY_OPS,
	DTE_TEST_MODULE_CONNECTOR,
	DTE_TEST_MODULE_PANEL,
	DTE_TEST_MODULE_FEATURE_IDLE,
	DTE_TEST_MODULE_FEATURE_AOD,

	/* we can add new test module here */

	DTE_TEST_MODULE_MAX,
};

struct hisi_drv_test_module {
	/* void* is a pointer which is assigned the test module's tested function struct */
	void *dte_test_module[DTE_TEST_MODULE_MAX];
};

#ifdef CONFIG_HISI_DISP_TEST_ENABLE
void hisi_disp_test_register_test_module(uint32_t test_module_type, void *monitor_data);
void *hisi_disp_test_get_module_data(uint32_t type);

#else
#define hisi_disp_test_register_test_module(test_module_type, monitor_data)
#endif

#endif /* HISI_DISP_TEST_H */
