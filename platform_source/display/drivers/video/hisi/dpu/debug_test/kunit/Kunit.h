/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Kunit.h
*
* 版    本 :
*
* 功    能 : 用户调用接口的声明&  断言的定义
*
* 作    者 :
*
* 创建日期 : 2011-03-07
*
* 修改记录 :
*
*    1: <时  间> :
*
*       <作  者> :
*
*       <版  本> :
*
*       <内  容> :
*
*    2:
**********************************************************************************/

#ifndef KUNIT_H_MAIN
#define KUNIT_H_MAIN

#include "Test.h"
#include "Xml.h"
#include "Regist.h"


/** 运行所有测试用例. */
extern void RUN_ALL_TESTS(KU_SuiteInfo suite_info[],char* szFilenameRoot);
/** 断言执行函数. */
extern KU_BOOL KU_assertImplementation(KU_BOOL bValue, unsigned int uiLine, char strCondition[], char strFile[], char strFunction[]);

/****  断言定义****/

/** CHECK 类型断言:检查失败继续，程序不退出.*/

/** 非零.*/
#define CHECK(value) \
	do { KU_assertImplementation((value), __LINE__, #value, __FILE__, ""); } while (0)

/** 真.*/
#define CHECK_TRUE(value) \
	do { KU_assertImplementation((value), __LINE__, ("CHECK_TRUE(" #value ")"), __FILE__, ""); } while (0)

/** 假.*/
#define CHECK_FALSE(value) \
	do { KU_assertImplementation(!(value), __LINE__, ("CHECK_FALSE(" #value ")"), __FILE__, ""); } while (0)

/** 相等.*/
#define CHECK_EQUAL(actual, expected) \
	do { KU_assertImplementation(((actual) == (expected)), __LINE__, \
		("CHECK_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
	} while (0)

/** 不相等.*/
#define CHECK_NOT_EQUAL(actual, expected) \
	do { KU_assertImplementation(((actual) != (expected)), __LINE__, \
		("CHECK_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
	} while (0)

/** 指针相等.*/
#define CHECK_PTR_EQUAL(actual, expected) \
	do { KU_assertImplementation(((void*)(actual) == (void*)(expected)), __LINE__, \
		("CHECK_PTR_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
	} while (0)

/** 指针不相等.*/
#define CHECK_PTR_NOT_EQUAL(actual, expected) \
	do { KU_assertImplementation(((void*)(actual) != (void*)(expected)), __LINE__, \
		("CHECK_PTR_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
	} while (0)

/** 空指针.*/
#define CHECK_PTR_NULL(value) \
	do { KU_assertImplementation((NULL == (void*)(value)), __LINE__, \
		("CHECK_PTR_NULL(" #value")"), __FILE__, ""); \
	} while (0)

/** 非空指针.*/
#define CHECK_PTR_NOT_NULL(value) \
	do { KU_assertImplementation((NULL != (void*)(value)), __LINE__,  \
		("CHECK_PTR_NOT_NULL(" #value")"), __FILE__, ""); \
	} while (0)

/** 字符串相等.*/
#define CHECK_STRING_EQUAL(actual, expected) \
	do { KU_assertImplementation(!(strcmp((const char*)(actual), (const char*)(expected))), __LINE__, \
		("CHECK_STRING_EQUAL(" #actual ","  #expected ")"), __FILE__, ""); \
	} while (0)

/** 字符串不相等.*/
#define CHECK_STRING_NOT_EQUAL(actual, expected) \
	do { KU_assertImplementation((strcmp((const char*)(actual), (const char*)(expected))), __LINE__, \
		("CHECK_STRING_NOT_EQUAL(" #actual ","  #expected ")"), __FILE__, ""); \
	} while (0)

/** 定长字符串相等.*/
#define CHECK_NSTRING_EQUAL(actual, expected, count) \
	do { KU_assertImplementation(!(strncmp((const char*)(actual), (const char*)(expected), (size_t)(count))), __LINE__, \
		("CHECK_NSTRING_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, ""); \
	} while (0)

/** 定长字符串不相等.*/
#define CHECK_NSTRING_NOT_EQUAL(actual, expected, count) \
	do { KU_assertImplementation((strncmp((const char*)(actual), (const char*)(expected), (size_t)(count))), __LINE__,\
		("CHECK_NSTRING_NOT_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, ""); \
	} while (0)


/** ASSERT 类型断言:检查失败return，程序退出.*/

/** 非零.*/
#define ASSERT(value) \
	do { \
		if ((int)(value) == KU_FALSE) { \
			KU_assertImplementation((KU_BOOL)value, __LINE__, #value, __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 真.*/
#define ASSERT_TRUE(value) \
	do { \
		if ((value) == KU_FALSE) { \
			KU_assertImplementation(KU_FALSE, __LINE__, ("ASSERT_TRUE(" #value ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 假.*/
#define ASSERT_FALSE(value)  \
	do { \
		if ((value) != KU_FALSE) { \
			KU_assertImplementation(KU_FALSE, __LINE__, ("ASSERT_FALSE(" #value ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 相等.*/
#define ASSERT_EQUAL(actual, expected) \
	do {  \
		if ((actual) != (expected)) {  \
			KU_assertImplementation(KU_FALSE, __LINE__, ("ASSERT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 不相等.*/
#define ASSERT_NOT_EQUAL(actual, expected)  \
	do { \
		if ((void*)(actual) == (void*)(expected)) { \
			KU_assertImplementation(KU_FALSE, __LINE__, ("ASSERT_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 指针相等.*/
#define ASSERT_PTR_EQUAL(actual, expected)  \
	do { \
		if ((void*)(actual) != (void*)(expected)) { \
			KU_assertImplementation(KU_FALSE, __LINE__, ("ASSERT_PTR_EQUAL(" #actual "," #expected ")"), __FILE__, "");  \
			return; \
		} \
	} while (0)

/** 指针不相等.*/
#define ASSERT_PTR_NOT_EQUAL(actual, expected)  \
	do {  \
		if ((void*)(actual) == (void*)(expected)) { \
			KU_assertImplementation(KU_FALSE, __LINE__, ("ASSERT_PTR_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 空指针.*/
#define ASSERT_PTR_NULL(value) \
	do { \
		if ((void*)(value) != NULL) { \
			KU_assertImplementation(KU_FALSE, __LINE__, ("ASSERT_PTR_NULL(" #value")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 非空指针.*/
#define ASSERT_PTR_NOT_NULL(value) \
	do { \
		if ((void*)(value) == NULL) { \
			KU_assertImplementation(KU_FALSE, __LINE__, ("ASSERT_PTR_NOT_NULL(" #value")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 字符串相等.*/
#define ASSERT_STRING_EQUAL(actual, expected) \
	do { \
		if (strcmp((const char*)actual, (const char*)expected)) { \
			KU_assertImplementation(KU_FALSE, __LINE__, \
				("ASSERT_STRING_EQUAL(" #actual ","  #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 字符串不相等.*/
#define ASSERT_STRING_NOT_EQUAL(actual, expected) \
	do { \
		if (!strcmp((const char*)actual, (const char*)expected)) { \
			KU_assertImplementation(TRUE, __LINE__, \
				("ASSERT_STRING_NOT_EQUAL(" #actual ","  #expected ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 定长字符串相等.*/
#define ASSERT_NSTRING_EQUAL(actual, expected, count)  \
	do { \
		if (strncmp((const char*)actual, (const char*)expected, (size_t)count)) { \
			KU_assertImplementation(KU_FALSE, __LINE__,  \
				("ASSERT_NSTRING_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

/** 定长字符串不相等.*/
#define ASSERT_NSTRING_NOT_EQUAL(actual, expected, count)  \
	do { \
		if (!strncmp((const char*)actual, (const char*)expected, (size_t)count)) { \
			KU_assertImplementation(TRUE, __LINE__, \
				("ASSERT_NSTRING_NOT_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, ""); \
			return; \
		} \
	} while (0)

#endif


