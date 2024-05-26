/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef KUNIT_H_REGIST
#define KUNIT_H_REGIST

#include "kerror.h"
#include <linux/slab.h>
#include <linux/gfp.h>

#define KU_BOOL int
#define KU_TRUE  1
#define KU_FALSE 0

#define KU_MALLOC(x)            kmalloc((x), GFP_KERNEL)
#define KU_FREE(x)              kfree((x))


typedef int  (*ku_initialize_func)(void);
typedef int  (*ku_cleanup_func)(void);
typedef void (*ku_test_func)(void);

typedef struct ku_test
{
	char*           pname;                 /** The test cases name */
	ku_test_func    ptest_func;            /** Function test cases */
	struct ku_test* pnext;                 /** The next test case */
	struct ku_test* pprev;                 /** The previous test cases */
} ku_test;
typedef ku_test* ku_ptest;

typedef struct ku_suite
{
	char*             pname;                 /** The test package name */
	ku_ptest          ptest;                 /** The first test case */
	ku_initialize_func pinitialize_func;     /** Initialization function */
	ku_cleanup_func    pcleanup_func;        /** Clean up function */
	unsigned int      ui_number_of_tests;    /** The number of test cases */
	struct ku_suite*  pnext;                 /** The next test package */
	struct ku_suite*  pprev;                 /** Before a test package */
} ku_suite;
typedef ku_suite* ku_psuite;

typedef struct ku_test_registry
{
	unsigned int ui_number_of_suites;        /** The number of test package */
	unsigned int ui_number_of_tests;         /** The number of test cases */
	ku_psuite    psuite;                     /** The first test package */
} ku_test_registry;
typedef ku_test_registry* ku_ptest_registry;

typedef struct ku_test_info
{
	char       *pname;                        /** The test cases */
	ku_test_func ptest_func;                  /** Function test cases */
} ku_test_info;
typedef ku_test_info* ku_ptest_info;

typedef struct ku_suite_info
{
	char             *pname;                   /** The test package name */
	ku_initialize_func pinit_func;             /** Initialization function */
	ku_cleanup_func    pcleanup_func;          /** Clean up function */
	ku_test_info      *ptests;                 /** Test group cases */
	KU_BOOL    bregist;                        /** Registered test package labels */
} ku_suite_info;
typedef ku_suite_info* ku_psuite_info;

/** NULL CU_test_info_t to terminate arrays of tests. */
#define KU_TEST_INFO_NULL { NULL, NULL }

/** NULL CU_suite_info_t to terminate arrays of suites. */
#define KU_SUITE_INFO_NULL { NULL, NULL, NULL, NULL }

ku_error_code ku_initialize_registry(void);
ku_error_code ku_cleanup_registry(void);
ku_ptest_registry ku_get_registry(void);
ku_error_code ku_register_suites(ku_suite_info suite_info[]);

#endif
