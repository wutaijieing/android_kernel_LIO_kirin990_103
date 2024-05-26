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

#include <linux/string.h>
#include "regist.h"
#include "test.h"
#include "xml.h"


/** Test case registry */
static ku_ptest_registry f_ptest_registry = NULL;

/**
 * @brief The cancellation of a test case
 *
 * @param ptest
 */
static void cleanup_test(ku_ptest ptest)
{
	if (ptest->pname != NULL)
		KU_FREE(ptest->pname);

	ptest->pname = NULL;
}

/**
 * @brief The cancellation of a test package
 *
 * @param psuite
 */
static void cleanup_suite(ku_psuite psuite)
{
	ku_ptest pcur_test = NULL;
	ku_ptest pnext_test = NULL;

	pcur_test = psuite->ptest;
	while (pcur_test != NULL) {
		pnext_test = pcur_test->pnext;
		cleanup_test(pcur_test);
		KU_FREE(pcur_test);
		pcur_test = pnext_test;
	}

	if (psuite->pname != NULL)
		KU_FREE(psuite->pname);

	psuite->pname = NULL;
	psuite->ptest = NULL;
	psuite->ui_number_of_tests = 0;
}

/**
 * @brief The cancellation of the registers
 *
 * @param pregistry
 */
static void cleanup_test_registry(ku_ptest_registry pregistry)
{
	ku_psuite pcur_suite = NULL;
	ku_psuite pnext_suite = NULL;

	pcur_suite = pregistry->psuite;
	while (pcur_suite) {
		pnext_suite = pcur_suite->pnext;
		cleanup_suite(pcur_suite);
		KU_FREE(pcur_suite);
		pcur_suite = pnext_suite;
	}

	pregistry->psuite = NULL;
	pregistry->ui_number_of_suites = 0;
	pregistry->ui_number_of_tests = 0;
}

/**
 * @brief Cancellation of registration, free memory
 *
 * @param pp_registry
 * @return ku_error_code
 */
ku_error_code ku_destroy_existing_registry(ku_ptest_registry *pp_registry)
{
	ku_error_code result;
	result = KUE_SUCCESS;

	if (!pp_registry) {
		result = KUE_WRONGPARA;
		return result;
	}

	if (*pp_registry)
		cleanup_test_registry(*pp_registry);

	KU_FREE(*pp_registry);
	*pp_registry = NULL;
	return result;
}

/**
 * @brief Cancellation of registration, empty failure form
 *
 * @return ku_error_code
 */
ku_error_code ku_cleanup_registry(void)
{
	ku_error_code result = KUE_SUCCESS;

	if (ku_is_test_running()) {
		result = KUE_FAILED;
		return result;
	}

	ku_set_error(KUE_SUCCESS);
	result = ku_destroy_existing_registry(&f_ptest_registry);
	result = ku_clear_previous_results();
	return result;
}

/**
 * @brief Create a registry
 *
 * @return ku_ptest_registry
 */
ku_ptest_registry ku_create_new_registry(void)
{
	ku_ptest_registry pregistry = (ku_ptest_registry)KU_MALLOC(sizeof(ku_test_registry));
	if (pregistry) {
		pregistry->psuite = NULL;
		pregistry->ui_number_of_suites = 0;
		pregistry->ui_number_of_tests = 0;
	}

	return pregistry;
}

/**
 * @brief Initializes the registry
 *
 * @return ku_error_code
 */
ku_error_code ku_initialize_registry(void)
{
	ku_error_code result;

	if (ku_is_test_running() == KU_TRUE) {
		ku_set_error(result = KUE_TEST_RUNNING);
		return result;
	}
	ku_set_error(result = KUE_SUCCESS);

	if (f_ptest_registry)
		result = ku_cleanup_registry();

	if(result != KUE_SUCCESS)
		return result;

	f_ptest_registry = ku_create_new_registry();
	if (!f_ptest_registry)
		ku_set_error(result = KUE_NOMEMORY);

	return result;
}

/**
 * @brief Access to the registry
 *
 * @return ku_ptest_registry
 */
ku_ptest_registry ku_get_registry(void)
{
	return f_ptest_registry;
}

/**
 * @brief Registry in the presence of the test package
 *
 * @param pregistry
 * @param sz_suite_name
 * @return KU_BOOL
 */
static KU_BOOL suite_exists(ku_ptest_registry pregistry, const char* sz_suite_name)
{
	ku_psuite psuite = pregistry->psuite;

	while (psuite) {
		if ((psuite->pname) && (strcmp(sz_suite_name, psuite->pname) == 0))
			return KU_TRUE;

		psuite = psuite->pnext;
	}
	return KU_FALSE;
}

/**
 * @brief Create a suite object
 *
 * @param str_name
 * @param pinit
 * @param pclean
 * @return ku_psuite
 */
static ku_psuite create_suite(const char* str_name, ku_initialize_func pinit, ku_cleanup_func pclean)
{
	ku_psuite pret_value = (ku_psuite)KU_MALLOC(sizeof(ku_suite));

	if (pret_value) {
		pret_value->pname = (char *)KU_MALLOC(strlen(str_name) + 1);
		if (pret_value->pname) {
			strcpy(pret_value->pname, str_name);
			pret_value->pinitialize_func = pinit;
			pret_value->pcleanup_func = pclean;
			pret_value->ptest = NULL;
			pret_value->pnext = NULL;
			pret_value->pprev = NULL;
			pret_value->ui_number_of_tests = 0;
		} else {
			KU_FREE(pret_value);
			pret_value = NULL;
		}
	}
	return pret_value;
}

/**
 * @brief Add the specified test package to the registry
 *
 * @param pregistry
 * @param psuite
 */
static void insert_suite(ku_ptest_registry pregistry, ku_psuite psuite)
{
	ku_psuite pcur_suite = pregistry->psuite;

	psuite->pnext = NULL;
	pregistry->ui_number_of_suites++;

	/* If it is the first test in the registry */
	if (!pcur_suite) {
		pregistry->psuite = psuite;
		psuite->pprev = NULL;
	} else { /* Or added to the test package list tail */
		while (pcur_suite->pnext)
			pcur_suite = pcur_suite->pnext;

		pcur_suite->pnext = psuite;
		psuite->pprev = pcur_suite;
	}
}

/**
 * @brief Add test package
 *
 * @param str_name
 * @param pinit
 * @param pclean
 * @return ku_psuite
 */
ku_psuite ku_add_suite(const char* str_name, ku_initialize_func pinit, ku_cleanup_func pclean)
{
	ku_psuite pret_value = NULL;
	ku_error_code error = KUE_SUCCESS;

	if (ku_is_test_running() == KU_TRUE) {
		error = KUE_TEST_RUNNING;
		return pret_value;
	}

	if (!f_ptest_registry) {
		error = KUE_NOREGISTRY;
	} else if (!str_name) {
		error = KUE_NO_SUITENAME;
	} else if (suite_exists(f_ptest_registry, str_name) == KU_TRUE) {
		error = KUE_DUP_SUITE;
	} else {
		pret_value = create_suite(str_name, pinit, pclean);
		if (!pret_value)
			error = KUE_NOMEMORY;
		else
			insert_suite(f_ptest_registry, pret_value);
	}

	ku_set_error(error);
	return pret_value;
}

/**
 * @brief Specify the test whether there is the test case in the package
 *
 * @param psuite
 * @param sz_test_name
 * @return KU_BOOL
 */
static KU_BOOL test_exists(ku_psuite psuite, const char* sz_test_name)
{
	ku_ptest ptest = psuite->ptest;

	while (ptest) {
		if ((ptest->pname) && (strcmp(sz_test_name, ptest->pname) == 0))
			return KU_TRUE;

		ptest = ptest->pnext;
	}
	return KU_FALSE;
}

/**
 * @brief Create a test object
 *
 * @param str_name
 * @param ptest_func
 * @return ku_ptest
 */
static ku_ptest create_test(const char* str_name, ku_test_func ptest_func)
{
	ku_ptest pret_value = (ku_ptest)KU_MALLOC(sizeof(ku_test));

	if (pret_value) {
		pret_value->pname = (char *)KU_MALLOC(strlen(str_name) + 1);
		if (pret_value->pname) {
			strcpy(pret_value->pname, str_name);
			pret_value->ptest_func = ptest_func;
			pret_value->pnext = NULL;
			pret_value->pprev = NULL;
		} else {
			KU_FREE(pret_value);
			pret_value = NULL;
		}
	}
	return pret_value;
}

/**
 * @brief Add test cases to the specified test package
 *
 * @param psuite
 * @param ptest
 */
static void insert_test(ku_psuite psuite, ku_ptest ptest)
{
	ku_ptest pcur_test = psuite->ptest;

	psuite->ui_number_of_tests++;

	if (!pcur_test) {
		psuite->ptest = ptest;
		ptest->pprev = NULL;
	} else {
		while (NULL != pcur_test->pnext)
			pcur_test = pcur_test->pnext;

		pcur_test->pnext = ptest;
		ptest->pprev = pcur_test;
	}
}

/**
 * @brief Add test cases to the specified test package
 *
 * @param psuite
 * @param str_name
 * @param ptest_func
 * @return ku_ptest
 */
ku_ptest ku_add_test(ku_psuite psuite, const char* str_name, ku_test_func ptest_func)
{
	ku_ptest pret_value = NULL;
	ku_error_code error = KUE_SUCCESS;

	if (ku_is_test_running() == KU_TRUE) {
		error = KUE_TEST_RUNNING;
		return pret_value;
	}

	if (!f_ptest_registry) {
		error = KUE_NOREGISTRY;
	} else if (!psuite) {
		error = KUE_NOSUITE;
	} else if (!str_name) {
		error = KUE_NO_TESTNAME;
	} else if(!ptest_func) {
		error = KUE_NOTEST;
	} else if (test_exists(psuite, str_name) == KU_TRUE) {
		error = KUE_DUP_TEST;
	} else {
		pret_value = create_test(str_name, ptest_func);
		if (!pret_value) {
			error = KUE_NOMEMORY;
		} else {
			f_ptest_registry->ui_number_of_tests++;
			insert_test(psuite, pret_value);
		}
	}
	ku_set_error(error);
	return pret_value;
}

/**
 * @brief Registered test package array
 *
 * @param suite_info
 * @return ku_error_code
 */
ku_error_code ku_register_suites(ku_suite_info suite_info[])
{
	ku_suite_info *psuite_item = NULL;
	ku_test_info *ptest_item = NULL;
	ku_psuite psuite = NULL;

	psuite_item = suite_info;
	if (!psuite_item) {
		return ku_get_error();
	}

	for ( ; NULL != psuite_item->pname; psuite_item++) {
		if (psuite_item->bregist == KU_FALSE)
			continue;

		if ((psuite = ku_add_suite(psuite_item->pname, psuite_item->pinit_func, psuite_item->pcleanup_func)) == NULL)
			return ku_get_error();

		for (ptest_item = psuite_item->ptests; NULL != ptest_item->pname; ptest_item++) {
			if (ku_add_test(psuite, ptest_item->pname, ptest_item->ptest_func) == NULL)
				return ku_get_error();
		}
	}

	return ku_get_error();
}
