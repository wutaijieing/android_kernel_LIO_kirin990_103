/**
 * @file dkmd_testcases.c
 * @brief test unit for dkmd
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
#include "testcases.h"

ku_suite_info dkmd_testcases_suites[]= {
	{"cmdlist_test_init", cmdlist_test_suite_init, cmdlist_test_suite_clean, test_cmdlist, KU_TRUE},
	{"connector_test_init", connector_test_suite_init, connector_test_suite_clean, connector_test, KU_TRUE},
	{"dksm_testcase_init", dksm_testcase_suite_init, dksm_testcase_suite_clean, dksm_testcase, KU_TRUE},
	{"composer_test_init", composer_test_suite_init, composer_test_suite_clean, composer_test, KU_TRUE},
	KU_SUITE_INFO_NULL
};

static int32_t dkmd_testcases_init(void)
{
	pr_info("+++++++++++++++++++++++++++++++++++++++ hello, dkmd_testcases kunit ++++++++++++++++++++++++++++++++++!\n");

	run_all_tests(dkmd_testcases_suites,"/data/log/dkmd_testcases");

	return 0;
}

static void dkmd_testcases_exit(void)
{
	pr_info("--------------------------------------- bye, dkmd_testcases kunit ----------------------------------!\n");
}

module_init(dkmd_testcases_init);
module_exit(dkmd_testcases_exit);

MODULE_LICENSE("GPL");