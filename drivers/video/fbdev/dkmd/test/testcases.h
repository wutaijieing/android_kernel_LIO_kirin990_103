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

#ifndef __TEST_CASES__
#define __TEST_CASES__

/* cmdlist testcases */
extern ku_test_info test_cmdlist[5];
extern int cmdlist_test_suite_init(void);
extern int cmdlist_test_suite_clean(void);

/* connector testcases */
extern ku_test_info connector_test[3];
extern int connector_test_suite_init(void);
extern int connector_test_suite_clean(void);

/* dksm testcases */
extern ku_test_info dksm_testcase[6];
extern int dksm_testcase_suite_init(void);
extern int dksm_testcase_suite_clean(void);

/* composer testcases */
extern ku_test_info composer_test[4];
extern int composer_test_suite_init(void);
extern int composer_test_suite_clean(void);

#endif