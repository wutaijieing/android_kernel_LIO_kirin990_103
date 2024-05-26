/**
 * @file connector_test.c
 * @brief test unit for connector
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "kunit.h"
#include "stub.h"
#include "dpu_conn_mgr.h"
#include "dkmd_rect.h"
#ifdef CONFIG_BUILD_TESTCASE_SUITE
#include "testcases.h"
#endif

/**
 * @brief Test the following connector initialized state
 *
 */
static void test_connector_init(void)
{
	struct dkmd_conn_handle_data *pdata = NULL;
	struct dpu_conn_manager *conn_mgr = g_conn_manager;

	CHECK_PTR_NOT_NULL(conn_mgr);

	CHECK_PTR_NOT_NULL(conn_mgr->device);

	CHECK_PTR_EQUAL(platform_get_drvdata(conn_mgr->device), conn_mgr);

	pdata = dev_get_platdata(&conn_mgr->device->dev);
	CHECK_PTR_NOT_NULL(pdata);

	CHECK_PTR_NOT_NULL(pdata->on_func);
	CHECK_PTR_NOT_NULL(pdata->off_func);
	CHECK_PTR_NOT_NULL(pdata->ops_handle_func);
}

/**
 * @brief Test connector power up and down
 *
 */
static void test_connector_power_onoff(void)
{
	int i = 0, backlight_level = 255;
	struct dkmd_conn_handle_data *pdata = NULL;
	struct dpu_conn_manager *conn_mgr = g_conn_manager;
	struct dpu_connector *connector = NULL;
	struct dkmd_rect rect = {
		.x = 0,
		.y = 0,
		.w = 480,
		.h = 720,
	};

	CHECK_PTR_NOT_NULL(conn_mgr);

	CHECK_PTR_NOT_NULL(conn_mgr->device);

	CHECK_PTR_EQUAL(platform_get_drvdata(conn_mgr->device), conn_mgr);

	pdata = dev_get_platdata(&conn_mgr->device->dev);
	CHECK_PTR_NOT_NULL(pdata);

	CHECK_PTR_NOT_NULL(pdata->on_func);
	CHECK_PTR_NOT_NULL(pdata->off_func);
	CHECK_PTR_NOT_NULL(pdata->ops_handle_func);

	for (i = PIPE_SW_POST_CH_DSI0; i < PIPE_SW_POST_CH_MAX; i++) {
		pr_info("check pipe_sw_post_ch index %d power onoff enter!\n", i);
		connector = conn_mgr->connector[i];
		if (connector && connector->conn_info) {
			CHECK_EQUAL(pdata->on_func(connector->conn_info), 0);
			CHECK_EQUAL(pdata->ops_handle_func(connector->conn_info, SET_FASTBOOT, &i), 0);
			CHECK_EQUAL(pdata->ops_handle_func(connector->conn_info, SET_BACKLIGHT, &backlight_level), 0);
			CHECK_EQUAL(pdata->ops_handle_func(connector->conn_info, LCD_SET_DISPLAY_REGION, &rect), 0);
			CHECK_EQUAL(pdata->off_func(connector->conn_info), 0);
		}
		pr_info("check pipe_sw_post_ch index %d power onoff exit!\n", i);
	}
}

ku_test_info connector_test[] = {
	{ "test_connector_init", test_connector_init },
	{ "test_connector_power_onoff", test_connector_power_onoff },
	KU_TEST_INFO_NULL
};

int32_t connector_test_suite_init(void)
{
	pr_info("connector_test device init!\n");
	return 0;
}

int32_t connector_test_suite_clean(void)
{
	pr_info("connector_test device deinit!\n");
	return 0;
}

#ifndef CONFIG_BUILD_TESTCASE_SUITE
ku_suite_info connector_test_suites[]= {
	{"connector_test_init", connector_test_suite_init, connector_test_suite_clean, connector_test, KU_TRUE},
	KU_SUITE_INFO_NULL
};

static int32_t connector_test_init(void)
{
	pr_info("+++++++++++++++++++++++++ hello, connector_test kunit ++++++++++++++++++++++!\n");

	run_all_tests(connector_test_suites,"/data/log/connector_test");
	return 0;
}

static void connector_test_exit(void)
{
	pr_info("----------------------- bye, connector_test kunit ---------------------!\n");
}

module_init(connector_test_init);
module_exit(connector_test_exit);
#endif

MODULE_LICENSE("GPL");