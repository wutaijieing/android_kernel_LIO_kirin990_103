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

#include "test.h"
#include <linux/string.h>

/**** Global variable definition ****/

/** Test cases run summary information */
static ku_run_summary f_run_summary = {0, 0, 0, 0, 0, 0, 0};

/** Failure record list */
static ku_pfailure_record f_failure_list = NULL;

/** The latest failure record */
static ku_pfailure_record f_last_failure = NULL;

/** Whether the test cases are running */
static KU_BOOL   f_btest_is_running = KU_FALSE;

/** The current test package */
static ku_psuite f_pcur_suite = NULL;

/** The current test cases */
static ku_ptest  f_pcur_test = NULL;


/** Call a function to test package execution before. */
static ku_test_start_message_handler f_ptest_start_message_handler = NULL;

/** To call a function after a single test case execution. */
static ku_test_complete_message_handler f_ptest_complete_message_handler = NULL;

/** To call a function after all test case execution. */
static ku_all_tests_complete_message_handler f_pall_tests_complete_message_handler = NULL;

/** To call a function after test package initialization failed. */
static ku_suite_init_failure_message_handler f_psuite_init_failure_message_handler = NULL;

/** To call a function after test package clear the failure. */
static ku_suite_cleanup_failure_message_handler f_psuite_cleanup_failure_message_handler = NULL;

/**
 * @brief Is there a test case is running
 *
 * @return KU_BOOL
 */
KU_BOOL ku_is_test_running(void)
{
	return f_btest_is_running;
}

/**
 * @brief Set the test suite execution before calling a function handle
 *
 * @param ptest_start_handler
 */
void ku_set_test_start_handler(ku_test_start_message_handler ptest_start_handler)
{
	f_ptest_start_message_handler = ptest_start_handler;
}

/**
 * @brief Setup test case execution after calling a function handle
 *
 * @param ptest_complete_handler
 */
void ku_set_test_complete_handler(ku_test_complete_message_handler ptest_complete_handler)
{
	f_ptest_complete_message_handler = ptest_complete_handler;
}

/**
 * @brief Set all the test case execution after calling a function handle
 *
 * @param pall_tests_complete_handler
 */
void ku_set_all_test_complete_handler(ku_all_tests_complete_message_handler pall_tests_complete_handler)
{
	f_pall_tests_complete_message_handler = pall_tests_complete_handler;
}

/**
 * @brief Set the test package handle to call a function after initialization failed
 *
 * @param psuite_init_failure_handler
 */
void ku_set_suite_init_failure_handler(ku_suite_init_failure_message_handler psuite_init_failure_handler)
{
	f_psuite_init_failure_message_handler = psuite_init_failure_handler;
}

/**
 * @brief Set the test package clear the handle to call a function after failure
 *
 * @param psuite_cleanup_failure_handler
 */
void ku_set_suite_cleanup_failure_handler(ku_suite_cleanup_failure_message_handler psuite_cleanup_failure_handler)
{
	f_psuite_cleanup_failure_message_handler = psuite_cleanup_failure_handler;
}

/**
 * @brief Get the information
 *
 * @return ku_prun_summary
 */
ku_prun_summary ku_get_run_summary(void)
{
	return &f_run_summary;
}

/**
 * @brief Add the failure information to record
 *
 * @param pp_failure
 * @param prun_summary
 * @param ui_line_number
 * @param sz_condition
 * @param sz_file_name
 * @param psuite
 * @param ptest
 */
void add_failure(ku_pfailure_record* pp_failure, ku_prun_summary prun_summary, unsigned int ui_line_number,
		char sz_condition[], char sz_file_name[], ku_psuite psuite, ku_ptest ptest)
{
	ku_pfailure_record pfailure_new = NULL;
	ku_pfailure_record ptemp = NULL;

	pfailure_new = (ku_pfailure_record)KU_MALLOC(sizeof(ku_failure_record));
	if (!pfailure_new)
		return;

	pfailure_new->str_file_name = NULL;
	pfailure_new->str_condition = NULL;
	if (sz_file_name) {
		pfailure_new->str_file_name = (char*)KU_MALLOC(strlen(sz_file_name) + 1);
		if (!pfailure_new->str_file_name) {
			KU_FREE(pfailure_new);
			return;
		}
		strcpy(pfailure_new->str_file_name, sz_file_name);
	}

	if (sz_condition) {
		pfailure_new->str_condition = (char*)KU_MALLOC(strlen(sz_condition) + 1);
		if (!(pfailure_new->str_condition)) {
			if (pfailure_new->str_file_name)
				KU_FREE(pfailure_new->str_file_name);

			KU_FREE(pfailure_new);
			return;
		}
		strcpy(pfailure_new->str_condition, sz_condition);
	}

	pfailure_new->ui_line_number = ui_line_number;
	pfailure_new->ptest = ptest;
	pfailure_new->psuite = psuite;
	pfailure_new->pnext = NULL;
	pfailure_new->pprev = NULL;
	ptemp = *pp_failure;

	if (ptemp) {
		while (ptemp->pnext)
			ptemp = ptemp->pnext;

		ptemp->pnext = pfailure_new;
		pfailure_new->pprev = ptemp;
	} else {
		*pp_failure = pfailure_new;
	}

	if (prun_summary)
		++(prun_summary->n_failure_records);

	f_last_failure = pfailure_new;
}

/**
 * @brief Release failure records linked list of memory space
 *
 * @param pp_failure
 */
static void cleanup_failure_list(ku_pfailure_record* pp_failure)
{
	ku_pfailure_record pcur_failure = NULL;
	ku_pfailure_record pnext_failure = NULL;

	pcur_failure = *pp_failure;
	while (pcur_failure) {
		if (pcur_failure->str_condition)
			KU_FREE(pcur_failure->str_condition);

		if (pcur_failure->str_file_name)
			KU_FREE(pcur_failure->str_file_name);

		pnext_failure = pcur_failure->pnext;
		KU_FREE(pcur_failure);
		pcur_failure = pnext_failure;
	}
	*pp_failure = NULL;
}

/**
 * @brief Initialization operation information & clear failure form
 *
 * @param prun_summary
 * @param pp_failure
 */
static void clear_previous_results(ku_prun_summary prun_summary, ku_pfailure_record* pp_failure)
{
	prun_summary->n_suites_run = 0;
	prun_summary->n_suites_failed = 0;
	prun_summary->n_tests_run = 0;
	prun_summary->n_tests_failed = 0;
	prun_summary->n_asserts = 0;
	prun_summary->n_asserts_failed = 0;
	prun_summary->n_failure_records = 0;
	if (*pp_failure)
		cleanup_failure_list(pp_failure);

	f_last_failure = NULL;
}

/**
 * @brief Run a single test case
 *
 * @param ptest
 * @param prun_summary
 * @return ku_error_code
 */
ku_error_code run_single_test(ku_ptest ptest, ku_prun_summary prun_summary)
{
	volatile unsigned int n_start_failures;

	/* Run the test to obtain the latest failure to record before */
	volatile ku_pfailure_record plast_failure = f_last_failure;

	n_start_failures = prun_summary->n_asserts_failed;
	ku_set_error(KUE_SUCCESS);
	f_pcur_test = ptest;
	if (f_ptest_start_message_handler)
		(*f_ptest_start_message_handler)(f_pcur_test, f_pcur_suite);

	/* Run the test function */
	if (ptest->ptest_func)
		(*ptest->ptest_func)();

	prun_summary->n_tests_run++;

	/* If you have a new assertion */
	if (prun_summary->n_asserts_failed > n_start_failures) {
		prun_summary->n_tests_failed++;
		if (plast_failure) {
			/* The current existing new failure record: plast_failure->pnext*/
			plast_failure = plast_failure->pnext;
		} else {
			/* Not the failure of the old records, jump to record head node failure */
			plast_failure = f_failure_list;
		}
	} else {
		/* No new assertion failure */
		plast_failure = NULL;
	}

	if (f_ptest_complete_message_handler)
		(*f_ptest_complete_message_handler)(f_pcur_test, f_pcur_suite, plast_failure);

	f_pcur_test = NULL;
	return ku_get_error();
}

/**
 * @brief Run a single test package
 *
 * @param psuite
 * @param prun_summary
 * @return ku_error_code
 */
static ku_error_code run_single_suite(ku_psuite psuite, ku_prun_summary prun_summary)
{
	ku_ptest ptest = NULL;
	ku_error_code result;
	ku_error_code result2;

	f_pcur_test = NULL;
	f_pcur_suite = psuite;
	ku_set_error(result = KUE_SUCCESS);

	/* failed to initiate the test package */
	if ((psuite->pinitialize_func) && ((*psuite->pinitialize_func)() != 0)) {
		if (f_psuite_init_failure_message_handler)
			(*f_psuite_init_failure_message_handler)(psuite);

		prun_summary->n_suites_failed++;
		add_failure(&f_failure_list, &f_run_summary, 0, "Suite Initialization failed - Suite Skipped", "KUnit System",
					psuite, NULL);
		ku_set_error(result = KUE_SINIT_FAILED);
	} else { /* Test package to initialize the success or not initialized */
		ptest = psuite->ptest;
		while (ptest) {
			result2 = run_single_test(ptest, prun_summary);
			result = (KUE_SUCCESS == result) ? result2 : result;
			ptest = ptest->pnext;
		}

		prun_summary->n_suites_run++;
		/* Test failed */
		if ((psuite->pcleanup_func) && ((*psuite->pcleanup_func)() != 0)) {
			if (NULL != f_psuite_cleanup_failure_message_handler)
				(*f_psuite_cleanup_failure_message_handler)(psuite);

			prun_summary->n_suites_failed++;
			add_failure(&f_failure_list, &f_run_summary, 0, "Suite cleanup failed.", "KUnit System", psuite, ptest);
			ku_set_error(KUE_SCLEAN_FAILED);
			result = (KUE_SUCCESS == result) ? KUE_SCLEAN_FAILED : result;
		}
	}

	f_pcur_suite = NULL;
	return result;
}

/**
 * @brief Run all test cases in the registry
 *
 * @return ku_error_code
 */
ku_error_code ku_run_all_tests(void)
{
	ku_ptest_registry pregistry = ku_get_registry();
	ku_psuite psuite = NULL;
	ku_error_code result;
	ku_error_code result2;

	ku_set_error(result = KUE_SUCCESS);
	if (!pregistry) {
		ku_set_error(result = KUE_NOREGISTRY);
		return result;
	}

	/* Test case is up and running, set up the label */
	f_btest_is_running = KU_TRUE;

	/* Remove failure record */
	clear_previous_results(&f_run_summary, &f_failure_list);
	psuite = pregistry->psuite;
	while (psuite) {
		/* With the test cases in test package, then run the test case */
		if (psuite->ui_number_of_tests > 0) {
			result2 = run_single_suite(psuite, &f_run_summary);
			result = (KUE_SUCCESS == result) ? result2 : result;
		}
		psuite = psuite->pnext;
	}

	/* Test cases run, reset the labels */
	f_btest_is_running = KU_FALSE;
	if (f_pall_tests_complete_message_handler)
		(*f_pall_tests_complete_message_handler)(f_failure_list);

	return result;
}

/**
 * @brief Assertion execution function
 *
 * @param value
 * @param ui_line
 * @param str_condition
 * @param str_file
 * @param str_function
 * @return KU_BOOL
 */
KU_BOOL ku_assert_implementation(KU_BOOL value,
	unsigned int ui_line, char str_condition[], char str_file[], char str_function[])
{
	++f_run_summary.n_asserts;
	if (value == KU_FALSE) {
		++f_run_summary.n_asserts_failed;
		add_failure(&f_failure_list, &f_run_summary, ui_line, str_condition,
		str_file, f_pcur_suite, f_pcur_test);
	}
	return value;
}
EXPORT_SYMBOL(ku_assert_implementation);

/**
 * @brief Remove failure records and operation information
 *
 * @return ku_error_code
 */
ku_error_code ku_clear_previous_results(void)
{
	clear_previous_results(&f_run_summary, &f_failure_list);
	return KUE_SUCCESS;
}
