/** @file
 * Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
 *
 * 文件名称 : Regist.c
 *
 * 版    本 :
 *
 * 功    能 : 实现Kunit 注册和注销功能
 *
 * 作    者 :
 *
 * 创建日期 : 2010-03-07
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
 */

#include <linux/string.h>
#include "Regist.h"
#include "Test.h"
#include "Xml.h"

/**** 全局变量定义 ****/

/** 测试用例注册器. */
static KU_pTestRegistry f_pTestRegistry = NULL;

/********************************************************
  * 函数名称: cleanup_test
  *
  * 功能描述: 注销一个测试用例
  *
  * 调用函数: 无
  *
  * 被调函数: cleanup_suite
  *
  * 输入参数:
  *                       Param1: KU_pTest pTest
  *
  * 输出参数:
  *
  * 返 回 值: 无
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2011-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void cleanup_test(KU_pTest pTest)
{
	if (pTest->pName != NULL) {
		KU_FREE(pTest->pName);
	}

	pTest->pName = NULL;
}

/********************************************************
 * 函数名称: cleanup_suite
 *
 * 功能描述: 注销一个测试包
 *
 * 调用函数: cleanup_test
 *
 * 被调函数: cleanup_test_registry
 *
 * 输入参数:
 *                       Param1: KU_pSuite pSuite
 *
 * 输出参数:
 *
 * 返 回 值: 无
 *
 * 其    他
 *
 * 作    者:
 *
 * 日    期: 2010-03-07
 *
 * 修改日期
 *
 * 修改作者
 **********************************************************/
static void cleanup_suite(KU_pSuite pSuite)
{
	KU_pTest pCurTest = NULL;
	KU_pTest pNextTest = NULL;

	pCurTest = pSuite->pTest;
	while (pCurTest != NULL) {
		pNextTest = pCurTest->pNext;
		cleanup_test(pCurTest);
		KU_FREE(pCurTest);
		pCurTest = pNextTest;
	}

	if (pSuite->pName != NULL) {
		KU_FREE(pSuite->pName);
	}

	pSuite->pName = NULL;
	pSuite->pTest = NULL;
	pSuite->uiNumberOfTests = 0;
}

/********************************************************
  * 函数名称: cleanup_test_registry
  *
  * 功能描述: 注销注册器
  *
  * 调用函数: cleanup_suite
  *
  * 被调函数: KU_destroy_existing_registry
  *
  * 输入参数:
  *                       Param1: KU_pTestRegistry pRegistry
  *
  * 输出参数:
  *
  * 返 回 值: 无
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void cleanup_test_registry(KU_pTestRegistry pRegistry)
{
	KU_pSuite pCurSuite = NULL;
	KU_pSuite pNextSuite = NULL;

	pCurSuite = pRegistry->pSuite;
	while (pCurSuite) {
		pNextSuite = pCurSuite->pNext;
		cleanup_suite(pCurSuite);
		KU_FREE(pCurSuite);
		pCurSuite = pNextSuite;
	}

	pRegistry->pSuite = NULL;
	pRegistry->uiNumberOfSuites = 0;
	pRegistry->uiNumberOfTests = 0;
}

/********************************************************
  * 函数名称: KU_destroy_existing_registry
  *
  * 功能描述: 注销注册器，释放内存
  *
  * 调用函数: cleanup_test_registry
  *
  * 被调函数: KU_cleanup_registry
  *
  * 输入参数:
  *                       Param1: KU_pTestRegistry* ppRegistry
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KUE_SUCCESS                     执行成功
  *                       其它                              执行失败
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
KU_ErrorCode KU_destroy_existing_registry(KU_pTestRegistry* ppRegistry)
{
	KU_ErrorCode result;
	result = KUE_SUCCESS;

	if (!ppRegistry) {
		result = KUE_WRONGPARA;
		return result;
	}

	if (*ppRegistry) {
		cleanup_test_registry(*ppRegistry);
	}

	KU_FREE(*ppRegistry);
	*ppRegistry = NULL;
	return result;
}

/********************************************************
  * 函数名称: KU_cleanup_registry
  *
  * 功能描述: 注销注册器,清空失败记录表
  *
  * 调用函数: KU_is_test_running
  *                         KU_destroy_existing_registry
  *                         KU_clear_previous_results
  *
  * 被调函数:
  *
  * 输入参数: 无
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KUE_SUCCESS                     执行成功
  *                       其它                              执行失败
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
KU_ErrorCode KU_cleanup_registry(void)
{
	KU_ErrorCode result = KUE_SUCCESS;

	if (KU_is_test_running()) {
		result=KUE_FAILED;
		return result;
	}

	KU_set_error(KUE_SUCCESS);
	result = KU_destroy_existing_registry(&f_pTestRegistry);
	result = KU_clear_previous_results();
	return result;
}

/********************************************************
  * 函数名称: KU_create_new_registry
  *
  * 功能描述: 创建注册器
  *
  * 调用函数:
  *
  * 被调函数: KU_initialize_registry
  *
  * 输入参数: 无
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KU_pTestRegistry                 执行成功
  *                       NULL                                  执行失败
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
KU_pTestRegistry KU_create_new_registry(void)
{
	KU_pTestRegistry pRegistry = (KU_pTestRegistry)KU_MALLOC(sizeof(KU_TestRegistry));
	if (pRegistry) {
		pRegistry->pSuite = NULL;
		pRegistry->uiNumberOfSuites = 0;
		pRegistry->uiNumberOfTests = 0;
	}

	return pRegistry;
}

/********************************************************
  * 函数名称: KU_initialize_registry
  *
  * 功能描述: 初始化注册器
  *
  * 调用函数: KU_is_test_running   KU_cleanup_registry
  *                         KU_create_new_registry
  *
  * 被调函数:
  *
  * 输入参数: 无
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KUE_SUCCESS                     执行成功
  *                       其它                              执行失败
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
KU_ErrorCode KU_initialize_registry(void)
{
	KU_ErrorCode result;

	if (KU_is_test_running() == KU_TRUE) {
		KU_set_error(result = KUE_TEST_RUNNING);
		return result;
	}
	KU_set_error(result = KUE_SUCCESS);

	if (f_pTestRegistry)
		result = KU_cleanup_registry();

	if(result != KUE_SUCCESS)
		return result;

	f_pTestRegistry = KU_create_new_registry();
	if (!f_pTestRegistry)
		KU_set_error(result = KUE_NOMEMORY);

	return result;
}

/********************************************************
  * 函数名称: KU_get_registry
  *
  * 功能描述: 获取注册器
  *
  * 调用函数:
  *
  * 被调函数:
  *
  * 输入参数: 无
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KU_pTestRegistry                 执行成功
  *                       NULL                                  执行失败
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
KU_pTestRegistry KU_get_registry(void)
{
 return f_pTestRegistry;
}

/********************************************************
  * 函数名称: suite_exists
  *
  * 功能描述: 注册器中是否存在此测试包
  *
  * 调用函数:
  *
  * 被调函数: KU_add_suite
  *
  * 输入参数: Param1: KU_pTestRegistry pRegistry
  *                         Param2: const char* szSuiteName
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KU_TRUE
  *                       KU_FALSE
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static KU_BOOL suite_exists(KU_pTestRegistry pRegistry, const char* szSuiteName)
{
	KU_pSuite pSuite = pRegistry->pSuite;

	while (pSuite) {
		if ((pSuite->pName) && (strcmp(szSuiteName, pSuite->pName) == 0))
			return KU_TRUE;

		pSuite = pSuite->pNext;
	}
	return KU_FALSE;
}

/********************************************************
  * 函数名称: create_suite
  *
  * 功能描述: 创建测试包
  *
  * 调用函数:
  *
  * 被调函数: KU_add_suite
  *
  * 输入参数: Param1: const char* strName
  *                         Param2: KU_InitializeFunc pInit
  *                         Param3: KU_CleanupFunc pClean
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KU_pSuite                           执行成功
  *                       NULL                                  执行失败
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static KU_pSuite create_suite(const char* strName, KU_InitializeFunc pInit, KU_CleanupFunc pClean)
{
	KU_pSuite pRetValue = (KU_pSuite)KU_MALLOC(sizeof(KU_Suite));

	if (pRetValue) {
		pRetValue->pName = (char *)KU_MALLOC(strlen(strName) + 1);
		if (pRetValue->pName) {
			strcpy(pRetValue->pName, strName);
			pRetValue->pInitializeFunc = pInit;
			pRetValue->pCleanupFunc = pClean;
			pRetValue->pTest = NULL;
			pRetValue->pNext = NULL;
			pRetValue->pPrev = NULL;
			pRetValue->uiNumberOfTests = 0;
		} else {
			KU_FREE(pRetValue);
			pRetValue = NULL;
		}
	}
	return pRetValue;
}

/********************************************************
  * 函数名称: insert_suite
  *
  * 功能描述: 添加指定测试包到注册器
  *
  * 调用函数:
  *
  * 被调函数: KU_add_suite
  *
  * 输入参数: Param1: KU_pTestRegistry pRegistry
  *                         Param2: KU_pSuite pSuite
  *
  * 输出参数:
  *
  * 返 回 值:  无
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void insert_suite(KU_pTestRegistry pRegistry, KU_pSuite pSuite)
{
	KU_pSuite pCurSuite = pRegistry->pSuite;

	pSuite->pNext = NULL;
	pRegistry->uiNumberOfSuites++;

	/* 如果是注册器中第一个测试包 */
	if (!pCurSuite) {
		pRegistry->pSuite = pSuite;
		pSuite->pPrev = NULL;
	} else { /* 否则添加到测试包链表尾部*/
		while (pCurSuite->pNext)
			pCurSuite = pCurSuite->pNext;

		pCurSuite->pNext = pSuite;
		pSuite->pPrev = pCurSuite;
	}
}


/********************************************************
  * 函数名称: KU_add_suite
  *
  * 功能描述: 添加测试包
  *
  * 调用函数: KU_is_test_running  suite_exists
  *                         create_suite         insert_suite
  *
  * 被调函数: KU_register_suites
  *
  * 输入参数: Param1: const char* strName
  *                         Param2: KU_InitializeFunc pInit
  *                         Param3: KU_CleanupFunc pClean
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KU_pSuite                           执行成功
  *                       NULL                                  执行失败
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
KU_pSuite KU_add_suite(const char* strName, KU_InitializeFunc pInit, KU_CleanupFunc pClean)
{
	KU_pSuite pRetValue = NULL;
	KU_ErrorCode error = KUE_SUCCESS;

	if (KU_is_test_running() == KU_TRUE) {
		error = KUE_TEST_RUNNING;
		return pRetValue;
	}

	if (!f_pTestRegistry) {
		error = KUE_NOREGISTRY;
	} else if (!strName) {
		error = KUE_NO_SUITENAME;
	} else if (suite_exists(f_pTestRegistry, strName) == KU_TRUE) {
		error = KUE_DUP_SUITE;
	} else {
		pRetValue = create_suite(strName, pInit, pClean);
		if (!pRetValue)
			error = KUE_NOMEMORY;
		else
			insert_suite(f_pTestRegistry, pRetValue);
	}

	KU_set_error(error);
	return pRetValue;
}


/********************************************************
  * 函数名称: test_exists
  *
  * 功能描述: 指定测试包中是否存在此
  *                         测试用例
  *
  * 调用函数:
  *
  * 被调函数: KU_add_test
  *
  * 输入参数: Param1: KU_pSuite pSuite
  *                         Param2: const char* szTestName
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KU_TRUE
  *                       KU_FALSE
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static KU_BOOL test_exists(KU_pSuite pSuite, const char* szTestName)
{
	KU_pTest pTest = pSuite->pTest;

	while (pTest) {
		if ((pTest->pName) && (strcmp(szTestName, pTest->pName) == 0))
			return KU_TRUE;

		pTest = pTest->pNext;
	}
	return KU_FALSE;
}

/********************************************************
  * 函数名称: create_test
  *
  * 功能描述: 创建测试用例
  *
  * 调用函数:
  *
  * 被调函数: KU_add_test
  *
  * 输入参数: Param1: const char* strName
  *                         Param2: KU_TestFunc pTestFunc
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KU_pTest                            执行成功
  *                       NULL                                  执行失败
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static KU_pTest create_test(const char* strName, KU_TestFunc pTestFunc)
{
	KU_pTest pRetValue = (KU_pTest)KU_MALLOC(sizeof(KU_Test));

	if (pRetValue) {
		pRetValue->pName = (char *)KU_MALLOC(strlen(strName) + 1);
		if (pRetValue->pName) {
			strcpy(pRetValue->pName, strName);
			pRetValue->pTestFunc = pTestFunc;
			pRetValue->pNext = NULL;
			pRetValue->pPrev = NULL;
		} else {
			KU_FREE(pRetValue);
			pRetValue = NULL;
		}
	}
	return pRetValue;
}

/********************************************************
  * 函数名称: insert_test
  *
  * 功能描述: 将测试用例添加到指定测试包
  *
  * 调用函数:
  *
  * 被调函数: KU_add_test
  *
  * 输入参数: Param1: KU_pSuite pSuite
  *                         Param2: KU_pTest pTest
  *
  * 输出参数:
  *
  * 返 回 值: 无
  *
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
static void insert_test(KU_pSuite pSuite, KU_pTest pTest)
{
	KU_pTest pCurTest  = pSuite->pTest;

	pSuite->uiNumberOfTests++;
	/* 测试包的第一个测试用例*/
	if (!pCurTest) {
		pSuite->pTest = pTest;
		pTest->pPrev = NULL;
	} else {
		while (NULL != pCurTest->pNext)
			pCurTest = pCurTest->pNext;

		pCurTest->pNext = pTest;
		pTest->pPrev = pCurTest;
	}
}

/********************************************************
  * 函数名称: KU_add_test
  *
  * 功能描述: 添加测试用例到指定测试包
  *
  * 调用函数: KU_is_test_running    test_exists
  *                         create_test   insert_test
  *
  * 被调函数: KU_register_suites
  *
  * 输入参数: Param1: KU_pSuite pSuite
  *                         Param2: const char* strName
  *                         Param3: KU_TestFunc pTestFunc
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KU_pTest                            执行成功
  *                       NULL                                  执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
KU_pTest KU_add_test(KU_pSuite pSuite, const char* strName, KU_TestFunc pTestFunc)
{
	KU_pTest pRetValue = NULL;
	KU_ErrorCode error = KUE_SUCCESS;

	if (KU_is_test_running() == KU_TRUE) {
		error = KUE_TEST_RUNNING;
		return pRetValue;
	}

	if (!f_pTestRegistry) {
		error = KUE_NOREGISTRY;
	} else if (!pSuite) {
		error = KUE_NOSUITE;
	} else if (!strName) {
		error = KUE_NO_TESTNAME;
	} else if(!pTestFunc) {
		error = KUE_NOTEST;
	} else if (test_exists(pSuite, strName) == KU_TRUE) {
		error = KUE_DUP_TEST;
	} else {
		pRetValue = create_test(strName, pTestFunc);
		if (!pRetValue) {
			error = KUE_NOMEMORY;
		} else {
			f_pTestRegistry->uiNumberOfTests++;
			insert_test(pSuite, pRetValue);
		}
	}
	KU_set_error(error);
	return pRetValue;
}

/********************************************************
  * 函数名称: KU_register_suites
  *
  * 功能描述: 注册测试包数组
  *
  * 调用函数: KU_add_suite    KU_add_test
  *
  * 被调函数:
  *
  * 输入参数: Param1: KU_SuiteInfo suite_info[]
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KUE_SUCCESS                     执行成功
  *                       其它                              执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
KU_ErrorCode KU_register_suites(KU_SuiteInfo suite_info[])
{
	KU_SuiteInfo *pSuiteItem = NULL;
	KU_TestInfo  *pTestItem = NULL;
	KU_pSuite     pSuite = NULL;

	pSuiteItem = suite_info;
	if (!pSuiteItem) {
		return KU_get_error();
	}

	for ( ; NULL != pSuiteItem->pName; pSuiteItem++) {
		// 判断测试包是否注册
		if (pSuiteItem->bRegist == KU_FALSE)
			continue;

		if ((pSuite = KU_add_suite(pSuiteItem->pName, pSuiteItem->pInitFunc, pSuiteItem->pCleanupFunc)) == NULL)
			return KU_get_error();

		for (pTestItem = pSuiteItem->pTests; NULL != pTestItem->pName; pTestItem++) {
			if (KU_add_test(pSuite, pTestItem->pName, pTestItem->pTestFunc) == NULL)
				return KU_get_error();
		}
	}

	return KU_get_error();
}

