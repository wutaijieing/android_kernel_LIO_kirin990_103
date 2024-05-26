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

#ifndef KUNIT_H_ERROR
#define KUNIT_H_ERROR

typedef enum
{
	 /* basic error code */
	KUE_SUCCESS           = 0,               /** No error */
	KUE_FAILED            = 1,               /** Failure */
	KUE_NOMEMORY          = 2,               /** Memory for failure */
	KUE_WRONGPARA         = 3,               /** Parameters of the fault */
	/* register error code */
	KUE_NOREGISTRY        = 10,              /** No registration */
	KUE_REGISTRY_EXISTS   = 11,              /** Registry already exists */
	KUE_TEST_RUNNING       =12,              /** Test cases are running */

	/* test suite error */
	KUE_NOSUITE           = 20,              /** No test package */
	KUE_NO_SUITENAME      = 21,              /** No test package name */
	KUE_SINIT_FAILED      = 22,              /** Failed to initiate the test package */
	KUE_SCLEAN_FAILED     = 23,              /** Test failed */
	KUE_DUP_SUITE         = 24,              /** Test package name repetition */
	/* test error code */
	KUE_NOTEST            = 30,              /** No test cases */
	KUE_NO_TESTNAME       = 31,              /** No test cases name */
	KUE_DUP_TEST          = 32,              /** The test case name repetition */
	KUE_TEST_NOT_IN_SUITE = 33,              /** No the test cases in the test package */
	/* file error code */
	KUE_FOPEN_FAILED      = 40,              /** File open failed */
	KUE_FCLOSE_FAILED     = 41,              /** File off failure */
	KUE_BAD_FILENAME      = 42,              /** The file name wrong */
	KUE_WRITE_ERROR       = 43               /** Write file error */
} ku_error_code;

void ku_set_error(ku_error_code error);
ku_error_code ku_get_error(void);

#endif
