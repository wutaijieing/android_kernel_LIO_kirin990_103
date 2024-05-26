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

#ifndef KUNIT_H_MAIN
#define KUNIT_H_MAIN

#include "test.h"
#include "xml.h"
#include "regist.h"

extern void run_all_tests(ku_suite_info suite_info[], char* sz_file_name_root);
extern KU_BOOL ku_assert_implementation(KU_BOOL value,
	unsigned int ui_line, char str_condition[], char str_file[], char str_function[]);

/** CHECK Type assertion: check failed to continue, the program does not exit. */

/** nonzero */
#define CHECK(value) \
	do { ku_assert_implementation((value), __LINE__, #value, __FILE__, ""); } while (0)

#define CHECK_TRUE(value) \
	do { ku_assert_implementation((value), __LINE__, ("CHECK_TRUE(" #value ")"), __FILE__, ""); } while (0)

#define CHECK_FALSE(value) \
	do { ku_assert_implementation(!(value), __LINE__, ("CHECK_FALSE(" #value ")"), __FILE__, ""); } while (0)

#define CHECK_EQUAL(actual, expected) \
	do { ku_assert_implementation(((actual) == (expected)), __LINE__, \
		("CHECK_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
	} while (0)

#define CHECK_NOT_EQUAL(actual, expected) \
	do { ku_assert_implementation(((actual) != (expected)), __LINE__, \
		("CHECK_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
	} while (0)

#define CHECK_PTR_EQUAL(actual, expected) \
	do { ku_assert_implementation(((void*)(actual) == (void*)(expected)), __LINE__, \
		("CHECK_PTR_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
	} while (0)

#define CHECK_PTR_NOT_EQUAL(actual, expected) \
	do { ku_assert_implementation(((void*)(actual) != (void*)(expected)), __LINE__, \
		("CHECK_PTR_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
	} while (0)

#define CHECK_PTR_NULL(value) \
	do { ku_assert_implementation((NULL == (void*)(value)), __LINE__, \
		("CHECK_PTR_NULL(" #value")"), __FILE__, ""); \
	} while (0)

#define CHECK_PTR_NOT_NULL(value) \
	do { ku_assert_implementation((NULL != (void*)(value)), __LINE__,  \
		("CHECK_PTR_NOT_NULL(" #value")"), __FILE__, ""); \
	} while (0)

#define CHECK_STRING_EQUAL(actual, expected) \
	do { ku_assert_implementation(!(strcmp((const char*)(actual), (const char*)(expected))), __LINE__, \
		("CHECK_STRING_EQUAL(" #actual ","  #expected ")"), __FILE__, ""); \
	} while (0)

#define CHECK_STRING_NOT_EQUAL(actual, expected) \
	do { ku_assert_implementation((strcmp((const char*)(actual), (const char*)(expected))), __LINE__, \
		("CHECK_STRING_NOT_EQUAL(" #actual ","  #expected ")"), __FILE__, ""); \
	} while (0)

#define CHECK_NSTRING_EQUAL(actual, expected, count) \
	do { ku_assert_implementation(!(strncmp((const char*)(actual), (const char*)(expected), (size_t)(count))), __LINE__, \
		("CHECK_NSTRING_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, ""); \
	} while (0)

#define CHECK_NSTRING_NOT_EQUAL(actual, expected, count) \
	do { ku_assert_implementation((strncmp((const char*)(actual), (const char*)(expected), (size_t)(count))), __LINE__,\
		("CHECK_NSTRING_NOT_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, ""); \
	} while (0)


/** ASSERT Type assertion: failed to check the return, the program exits. */

#define ASSERT(value) \
	do { \
		if ((int)(value) == KU_FALSE) { \
			ku_assert_implementation((KU_BOOL)value, __LINE__, #value, __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_TRUE(value) \
	do { \
		if ((value) == KU_FALSE) { \
			ku_assert_implementation(KU_FALSE, __LINE__, ("ASSERT_TRUE(" #value ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_FALSE(value)  \
	do { \
		if ((value) != KU_FALSE) { \
			ku_assert_implementation(KU_FALSE, __LINE__, ("ASSERT_FALSE(" #value ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_EQUAL(actual, expected) \
	do {  \
		if ((actual) != (expected)) {  \
			ku_assert_implementation(KU_FALSE, __LINE__, ("ASSERT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_NOT_EQUAL(actual, expected)  \
	do { \
		if ((void*)(actual) == (void*)(expected)) { \
			ku_assert_implementation(KU_FALSE, __LINE__, ("ASSERT_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_PTR_EQUAL(actual, expected)  \
	do { \
		if ((void*)(actual) != (void*)(expected)) { \
			ku_assert_implementation(KU_FALSE, __LINE__, ("ASSERT_PTR_EQUAL(" #actual "," #expected ")"), __FILE__, "");  \
			return; \
		} \
	} while (0)

#define ASSERT_PTR_NOT_EQUAL(actual, expected)  \
	do {  \
		if ((void*)(actual) == (void*)(expected)) { \
			ku_assert_implementation(KU_FALSE, __LINE__, ("ASSERT_PTR_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_PTR_NULL(value) \
	do { \
		if ((void*)(value) != NULL) { \
			ku_assert_implementation(KU_FALSE, __LINE__, ("ASSERT_PTR_NULL(" #value")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_PTR_NOT_NULL(value) \
	do { \
		if ((void*)(value) == NULL) { \
			ku_assert_implementation(KU_FALSE, __LINE__, ("ASSERT_PTR_NOT_NULL(" #value")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_STRING_EQUAL(actual, expected) \
	do { \
		if (strcmp((const char*)actual, (const char*)expected)) { \
			ku_assert_implementation(KU_FALSE, __LINE__, \
				("ASSERT_STRING_EQUAL(" #actual ","  #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_STRING_NOT_EQUAL(actual, expected) \
	do { \
		if (!strcmp((const char*)actual, (const char*)expected)) { \
			ku_assert_implementation(TRUE, __LINE__, \
				("ASSERT_STRING_NOT_EQUAL(" #actual ","  #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_NSTRING_EQUAL(actual, expected, count)  \
	do { \
		if (strncmp((const char*)actual, (const char*)expected, (size_t)count)) { \
			ku_assert_implementation(KU_FALSE, __LINE__,  \
				("ASSERT_NSTRING_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#define ASSERT_NSTRING_NOT_EQUAL(actual, expected, count)  \
	do { \
		if (!strncmp((const char*)actual, (const char*)expected, (size_t)count)) { \
			ku_assert_implementation(TRUE, __LINE__, \
				("ASSERT_NSTRING_NOT_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#endif
