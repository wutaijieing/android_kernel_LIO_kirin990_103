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

#ifndef KUNIT_H_TEST
#define KUNIT_H_TEST

#include "regist.h"
#include "kerror.h"

/**** Structure definition ****/

/** Failure record data types. */
typedef struct ku_failure_record
{
	unsigned int  ui_line_number;         /** Failure is the line number. */
	char*         str_file_name;          /** Failure in the file name. */
	char*         str_condition;         /** Failure conditions. */
	ku_ptest      ptest;                /** Test cases to fail. */
	ku_psuite     psuite;               /** Test failed. */
	struct ku_failure_record* pnext;     /** Under a failure record. */
	struct ku_failure_record* pprev;     /** Before a failure record. */
} ku_failure_record;
typedef ku_failure_record* ku_pfailure_record;

typedef struct ku_run_summary
{
	unsigned int n_suites_run;                 /** The total number of test package. */
	unsigned int n_suites_failed;              /** The test package number of failure. */
	unsigned int n_tests_run;                  /** The total number of test cases. */
	unsigned int n_tests_failed;               /** The test cases of failure. */
	unsigned int n_asserts;                    /** The total number of claims. */
	unsigned int n_asserts_failed;             /** The assertion failure. */
	unsigned int n_failure_records;            /** The total number of failed record. */
} ku_run_summary;
typedef ku_run_summary* ku_prun_summary;

typedef void (*ku_test_start_message_handler)(const ku_ptest ptest, const ku_psuite psuite);
typedef void (*ku_test_complete_message_handler)(const ku_ptest ptest, const ku_psuite psuite, \
		const ku_pfailure_record pfailure);
typedef void (*ku_all_tests_complete_message_handler)(const ku_pfailure_record pfailure);
typedef void (*ku_suite_init_failure_message_handler)(const ku_psuite psuite);
typedef void (*ku_suite_cleanup_failure_message_handler)(const ku_psuite psuite);

KU_BOOL ku_is_test_running(void);
ku_prun_summary ku_get_run_summary(void);
ku_error_code ku_run_all_tests(void);
void ku_set_test_start_handler(ku_test_start_message_handler ptest_start_handler);
void ku_set_test_complete_handler(ku_test_complete_message_handler ptest_complete_handler);
void ku_set_all_test_complete_handler(ku_all_tests_complete_message_handler pall_tests_complete_handler);
void ku_set_suite_init_failure_handler(ku_suite_init_failure_message_handler psuite_init_failure_handler);
void ku_set_suite_cleanup_failure_handler(ku_suite_cleanup_failure_message_handler psuite_cleanup_failure_handler);
KU_BOOL ku_assert_implementation(KU_BOOL value, unsigned int ui_line, char str_condition[], \
	char str_file[], char str_function[]);
ku_error_code ku_clear_previous_results(void);

#endif
