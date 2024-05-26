/**
 * @file composer_test.c
 * @brief test unit for composer
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "kunit.h"
#include "stub.h"
#include "dkmd_comp.h"
#include "gfxdev_mgr.h"
#include "dpu_config_utils.h"
#include "dkmd_alsc_interface.h"
#ifdef CONFIG_BUILD_TESTCASE_SUITE
#include "testcases.h"
#endif

/**
 * @brief Test the following interface
 *    device_mgr_create_gfxdev
 *    device_mgr_destroy_gfxdev
 *    device_mgr_shutdown_gfxdev
 *    device_mgr_register_comp
 */
static void test_gfx_itf(void)
{
	struct composer comp = {
		.base = {
			.type = PANEL_WRITEBACK,
			.name = "test_gfx",
		},
	};

	device_mgr_register_comp(&comp);
	CHECK_EQUAL(device_mgr_create_gfxdev(&comp), 0);
	device_mgr_shutdown_gfxdev(&comp);
	device_mgr_destroy_gfxdev(&comp);
}

/**
 * @brief Test the following interface
 *    dpu_config_get_scf_lut_addr_tlb
 *    dpu_config_get_arsr_lut_addr_tlb
 *
 */
static void test_dpu_config_itf(void)
{
	int32_t length = 0;
	uint32_t i = 0;
	uint32_t *lut_addr_tlb = NULL;
	uint64_t level0 = 0;

	lut_addr_tlb = dpu_config_get_scf_lut_addr_tlb(&length);
	CHECK_PTR_NOT_NULL(lut_addr_tlb);
	CHECK_NOT_EQUAL(length, 0);

	lut_addr_tlb = NULL;
	length = 0;
	lut_addr_tlb = dpu_config_get_arsr_lut_addr_tlb(&length);
	CHECK_PTR_NOT_NULL(lut_addr_tlb);
	CHECK_NOT_EQUAL(length, 0);
}

/**
 * @brief Test the following interface
 *    dkmd_alsc_enable
 *    dkmd_alsc_disable
 *    dkmd_alsc_update_bl_param
 *    dkmd_alsc_update_degamma_coef
 *    dkmd_alsc_register_cb_func
 *
 */
static void test_dkmd_alsc_itf(void)
{
	struct dkmd_alsc *alsc = NULL;

	alsc = (struct dkmd_alsc *)kzalloc(sizeof(struct dkmd_alsc), GFP_KERNEL);
	CHECK_NOT_EQUAL(dkmd_alsc_enable(NULL), 0);
	CHECK_EQUAL(dkmd_alsc_enable(alsc), 0);

	CHECK_NOT_EQUAL(dkmd_alsc_update_bl_param(NULL), 0);
	CHECK_EQUAL(dkmd_alsc_update_bl_param(&alsc->bl_param), 0);

	CHECK_NOT_EQUAL(dkmd_alsc_update_degamma_coef(NULL), 0);
	CHECK_EQUAL(dkmd_alsc_update_degamma_coef(&alsc->degamma_coef), 0);

	CHECK_EQUAL(dkmd_alsc_disable(), 0);

	CHECK_NOT_EQUAL(dkmd_alsc_register_cb_func(NULL, NULL), 0);
	kfree(alsc);
}

ku_test_info composer_test[] = {
	{ "test_gfx_itf", test_gfx_itf },
	{ "test_dpu_config_itf", test_dpu_config_itf },
	{ "test_dkmd_alsc_itf", test_dkmd_alsc_itf },
	KU_TEST_INFO_NULL
};

int32_t composer_test_suite_init(void)
{
	pr_info("composer_test device init!\n");
	return 0;
}

int32_t composer_test_suite_clean(void)
{
	pr_info("composer_test device deinit!\n");
	return 0;
}

#ifndef CONFIG_BUILD_TESTCASE_SUITE
ku_suite_info composer_test_suites[]= {
	{"composer_test_init", composer_test_suite_init, composer_test_suite_clean, composer_test, KU_TRUE},
	KU_SUITE_INFO_NULL
};

static int32_t composer_test_init(void)
{
	pr_info("+++++++++++++++++++++++++ hello, composer_test kunit +++++++++++++++++++++++!\n");

	run_all_tests(composer_test_suites,"/data/log/composer_test");
	return 0;
}

static void composer_test_exit(void)
{
	pr_info("----------------------- bye, composer_test kunit --------------------!\n");
}

module_init(composer_test_init);
module_exit(composer_test_exit);

#endif

MODULE_LICENSE("GPL");