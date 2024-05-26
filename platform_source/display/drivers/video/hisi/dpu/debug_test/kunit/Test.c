/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Test.c
*
* 版    本 :
*
* 功    能 : 运行测试用例功能
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

#include "Test.h"
#include <linux/string.h>

/**** 全局变量定义 ****/

/** 测试用例运行摘要信息. */
static KU_RunSummary f_run_summary = {0, 0, 0, 0, 0, 0, 0};

/** 失败记录链表. */
static KU_pFailureRecord f_failure_list = NULL;

/** 最新失败记录. */
static KU_pFailureRecord f_last_failure = NULL;

/** 测试用例是否正在运行. */
static KU_BOOL   f_bTestIsRunning = KU_FALSE;

/** 当前测试包. */
static KU_pSuite f_pCurSuite = NULL;

/** 当前测试用例. */
static KU_pTest  f_pCurTest  = NULL;


/** 指向测试包执行前调用函数. */
static KU_TestStartMessageHandler           f_pTestStartMessageHandler = NULL;

/** 指向单个测试用例执行后调用函数. */
static KU_TestCompleteMessageHandler        f_pTestCompleteMessageHandler = NULL;

/** 指向所有测试用例执行后调用函数. */
static KU_AllTestsCompleteMessageHandler    f_pAllTestsCompleteMessageHandler = NULL;

/** 指向测试包初始化失败后调用函数. */
static KU_SuiteInitFailureMessageHandler    f_pSuiteInitFailureMessageHandler = NULL;

/** 指向测试包清理失败后调用函数. */
static KU_SuiteCleanupFailureMessageHandler f_pSuiteCleanupFailureMessageHandler = NULL;


/********************************************************
  * 函数名称: KU_is_test_running
  *
  * 功能描述: 是否有测试用例正在运行
  *
  * 调用函数:
  *
  * 被调函数:
  *
  * 输入参数:
  *                         无
  *
  * 输出参数:
  *                         无
  *
  * 返 回 值:
  *                         KU_FALSE
  *                         KU_TRUE
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
KU_BOOL KU_is_test_running(void)
{
	return f_bTestIsRunning;
}

/********************************************************
  * 函数名称: KU_set_test_start_handler
  *
  * 功能描述: 设置测试包执行前
  *                         调用函数句柄
  *
  * 调用函数: 无
  *
  * 被调函数:KU_automated_run_tests
  *
  * 输入参数:
  *                       Param1: KU_TestStartMessageHandler
  *                                    pTestStartHandler
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
void KU_set_test_start_handler(KU_TestStartMessageHandler pTestStartHandler)
{
	f_pTestStartMessageHandler = pTestStartHandler;
}


/********************************************************
  * 函数名称: KU_set_test_complete_handler
  *
  * 功能描述: 设置测试用例执行后
  *                         调用函数句柄
  *
  * 调用函数: 无
  *
  * 被调函数:KU_automated_run_tests
  *
  * 输入参数:
  *                       Param1: KU_TestCompleteMessageHandler
  *                                   pTestCompleteHandler
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
void KU_set_test_complete_handler(KU_TestCompleteMessageHandler pTestCompleteHandler)
{
	f_pTestCompleteMessageHandler = pTestCompleteHandler;
}


/********************************************************
  * 函数名称: KU_set_all_test_complete_handler
  *
  * 功能描述: 设置所有测试用例执行后
  *                         调用函数句柄
  *
  * 调用函数: 无
  *
  * 被调函数:KU_automated_run_tests
  *
  * 输入参数:
  *                       Param1: KU_AllTestsCompleteMessageHandler
  *                                    pAllTestsCompleteHandler
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
void KU_set_all_test_complete_handler(KU_AllTestsCompleteMessageHandler pAllTestsCompleteHandler)
{
	f_pAllTestsCompleteMessageHandler = pAllTestsCompleteHandler;
}


/********************************************************
  * 函数名称: KU_set_suite_init_failure_handler
  *
  * 功能描述: 设置测试包初始化失败后
  *                         调用函数句柄
  *
  * 调用函数: 无
  *
  * 被调函数:KU_automated_run_tests
  *
  * 输入参数:
  *                       Param1: KU_SuiteInitFailureMessageHandler
  *                                   pSuiteInitFailureHandler
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
void KU_set_suite_init_failure_handler(KU_SuiteInitFailureMessageHandler pSuiteInitFailureHandler)
{
	f_pSuiteInitFailureMessageHandler = pSuiteInitFailureHandler;
}


/********************************************************
  * 函数名称: KU_set_suite_cleanup_failure_handler
  *
  * 功能描述: 设置测试包清理失败后
  *                         调用函数句柄
  *
  * 调用函数: 无
  *
  * 被调函数:KU_automated_run_tests
  *
  * 输入参数:
  *                       Param1: KU_SuiteCleanupFailureMessageHandler
  *                                   pSuiteCleanupFailureHandler
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
void KU_set_suite_cleanup_failure_handler(KU_SuiteCleanupFailureMessageHandler pSuiteCleanupFailureHandler)
{
	f_pSuiteCleanupFailureMessageHandler = pSuiteCleanupFailureHandler;
}


/********************************************************
  * 函数名称: KU_get_run_summary
  *
  * 功能描述: 获取运行摘要信息
  *
  * 调用函数: 无
  *
  * 被调函数:
  *
  * 输入参数:
  *                        无
  *
  * 输出参数:
  *
  * 返 回 值:    f_run_summary
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
KU_pRunSummary KU_get_run_summary(void)
{
	return &f_run_summary;
}


/********************************************************
  * 函数名称: add_failure
  *
  * 功能描述: 添加失败信息到失败记录表
  *
  * 调用函数: 无
  *
  * 被调函数:
  *
  * 输入参数: Param1: KU_pFailureRecord* ppFailure
  *                         Param2: KU_pRunSummary pRunSummary
  *                         Param3: unsigned int uiLineNumber
  *                         Param4: char szCondition[]
  *                         Param5: char szFileName[]
  *                         Param6: KU_pSuite pSuite
  *                         Param7: KU_pTest pTest
  *
  * 输出参数:
  *
  * 返 回 值:    无
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
void add_failure(KU_pFailureRecord* ppFailure, KU_pRunSummary pRunSummary, unsigned int uiLineNumber,
		char szCondition[], char szFileName[], KU_pSuite pSuite, KU_pTest pTest)
{
	KU_pFailureRecord pFailureNew = NULL;
	KU_pFailureRecord pTemp = NULL;

	pFailureNew = (KU_pFailureRecord)KU_MALLOC(sizeof(KU_FailureRecord));
	if (!pFailureNew)
		return;

	pFailureNew->strFileName = NULL;
	pFailureNew->strCondition = NULL;
	if (szFileName) {
		pFailureNew->strFileName = (char*)KU_MALLOC(strlen(szFileName) + 1);
		if (!pFailureNew->strFileName) {
			KU_FREE(pFailureNew);
			return;
		}
		strcpy(pFailureNew->strFileName, szFileName);
	}

	if (szCondition) {
		pFailureNew->strCondition = (char*)KU_MALLOC(strlen(szCondition) + 1);
		if (!(pFailureNew->strCondition)) {
			if (pFailureNew->strFileName)
				KU_FREE(pFailureNew->strFileName);

			KU_FREE(pFailureNew);
			return;
		}
		strcpy(pFailureNew->strCondition, szCondition);
	}

	pFailureNew->uiLineNumber = uiLineNumber;
	pFailureNew->pTest = pTest;
	pFailureNew->pSuite = pSuite;
	pFailureNew->pNext = NULL;
	pFailureNew->pPrev = NULL;
	pTemp = *ppFailure;

	if (pTemp) {
		while (pTemp->pNext)
			pTemp = pTemp->pNext;

		pTemp->pNext = pFailureNew;
		pFailureNew->pPrev = pTemp;
	} else {
		*ppFailure = pFailureNew;
	}

	if (pRunSummary)
		++(pRunSummary->nFailureRecords);

	f_last_failure = pFailureNew;
}


/********************************************************
  * 函数名称: cleanup_failure_list
  *
  * 功能描述: 释放失败记录链表的内存空间
  *
  * 调用函数: 无
  *
  * 被调函数:
  *
  * 输入参数: Param1: KU_pFailureRecord* ppFailure
  *
  * 输出参数:
  *
  * 返 回 值:    无
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
static void cleanup_failure_list(KU_pFailureRecord* ppFailure)
{
	KU_pFailureRecord pCurFailure = NULL;
	KU_pFailureRecord pNextFailure = NULL;

	pCurFailure = *ppFailure;
	while (pCurFailure) {
		if (pCurFailure->strCondition)
			KU_FREE(pCurFailure->strCondition);

		if (pCurFailure->strFileName)
			KU_FREE(pCurFailure->strFileName);

		pNextFailure = pCurFailure->pNext;
		KU_FREE(pCurFailure);
		pCurFailure = pNextFailure;
	}
	*ppFailure = NULL;
}


/********************************************************
  * 函数名称: clear_previous_results
  *
  * 功能描述: 初始化运行摘要信息
  *                          &  清空失败记录表
  *
  * 调用函数: cleanup_failure_list
  *
  * 被调函数: KU_run_all_tests  KU_clear_previous_results
  *
  * 输入参数: Param1: KU_pRunSummary pRunSummary
  *                         Param2: KU_pFailureRecord* ppFailure
  *
  * 输出参数:
  *
  * 返 回 值:    无
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
static void clear_previous_results(KU_pRunSummary pRunSummary, KU_pFailureRecord* ppFailure)
{
	pRunSummary->nSuitesRun = 0;
	pRunSummary->nSuitesFailed = 0;
	pRunSummary->nTestsRun = 0;
	pRunSummary->nTestsFailed = 0;
	pRunSummary->nAsserts = 0;
	pRunSummary->nAssertsFailed = 0;
	pRunSummary->nFailureRecords = 0;
	if (*ppFailure)
		cleanup_failure_list(ppFailure);

	f_last_failure = NULL;
}


/********************************************************
  * 函数名称: run_single_test
  *
  * 功能描述: 运行单个测试用例
  *
  * 调用函数:
  *
  * 被调函数:run_single_suite
  *
  * 输入参数: Param1: KU_pTest pTest
  *                         Param2: KU_pRunSummary pRunSummary
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
KU_ErrorCode run_single_test(KU_pTest pTest, KU_pRunSummary pRunSummary)
{
	volatile unsigned int nStartFailures;

	/* 运行测试用力前获取当前最新失败记录 */
	volatile KU_pFailureRecord pLastFailure = f_last_failure;

	nStartFailures = pRunSummary->nAssertsFailed;
	KU_set_error(KUE_SUCCESS);
	f_pCurTest = pTest;
	if (f_pTestStartMessageHandler)
		(*f_pTestStartMessageHandler)(f_pCurTest, f_pCurSuite);

	/* 运行测试函数*/
	if (pTest->pTestFunc)
		(*pTest->pTestFunc)();

	pRunSummary->nTestsRun++;

	/* 如果有新的断言失败*/
	if (pRunSummary->nAssertsFailed > nStartFailures) {
		pRunSummary->nTestsFailed++;
		if (pLastFailure) {
			/* 当前已有新的失败记录: pLastFailure->pNext*/
			pLastFailure = pLastFailure->pNext;
		} else {
			/* 没有旧的失败记录，跳转到失败记录表的头结点 */
			pLastFailure = f_failure_list;
		}
	} else {
		/* 没有新的断言失败 */
		pLastFailure = NULL;
	}

	if (f_pTestCompleteMessageHandler)
		(*f_pTestCompleteMessageHandler)(f_pCurTest, f_pCurSuite, pLastFailure);

	f_pCurTest = NULL;
	return KU_get_error();
}


/********************************************************
  * 函数名称: run_single_suite
  *
  * 功能描述: 运行单个测试包
  *
  * 调用函数:add_failure   run_single_test
  *
  * 被调函数:KU_run_all_tests
  *
  * 输入参数: Param1: KU_pSuite pSuite
  *                         Param2: KU_pRunSummary pRunSummary
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
static KU_ErrorCode run_single_suite(KU_pSuite pSuite, KU_pRunSummary pRunSummary)
{
	KU_pTest pTest = NULL;
	KU_ErrorCode result;
	KU_ErrorCode result2;

	f_pCurTest = NULL;
	f_pCurSuite = pSuite;
	KU_set_error(result = KUE_SUCCESS);

	/* 测试包初始化失败 */
	if ((pSuite->pInitializeFunc) && ((*pSuite->pInitializeFunc)() != 0)) {
		if (f_pSuiteInitFailureMessageHandler)
			(*f_pSuiteInitFailureMessageHandler)(pSuite);

		pRunSummary->nSuitesFailed++;
		add_failure(&f_failure_list, &f_run_summary, 0, "Suite Initialization failed - Suite Skipped", "KUnit System",
					pSuite, NULL);
		KU_set_error(result = KUE_SINIT_FAILED);
	} else { /* 测试包初始化成功或没有初始化 */
		pTest = pSuite->pTest;
		while (pTest) {
			result2 = run_single_test(pTest, pRunSummary);
			result = (KUE_SUCCESS == result) ? result2 : result;
			pTest = pTest->pNext;
		}

		pRunSummary->nSuitesRun++;
		/* 测试包清理失败 */
		if ((pSuite->pCleanupFunc) && ((*pSuite->pCleanupFunc)() != 0)) {
			if (NULL != f_pSuiteCleanupFailureMessageHandler)
				(*f_pSuiteCleanupFailureMessageHandler)(pSuite);

			pRunSummary->nSuitesFailed++;
			add_failure(&f_failure_list, &f_run_summary, 0, "Suite cleanup failed.", "KUnit System", pSuite, pTest);
			KU_set_error(KUE_SCLEAN_FAILED);
			result = (KUE_SUCCESS == result) ? KUE_SCLEAN_FAILED : result;
		}
	}

	f_pCurSuite = NULL;
	return result;
}


/********************************************************
  * 函数名称: KU_run_all_tests
  *
  * 功能描述: 运行注册器中所有测试用例
  *
  * 调用函数:KU_get_registry   clear_previous_results
  *                        run_single_suite
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
KU_ErrorCode KU_run_all_tests(void)
{
	KU_pTestRegistry pRegistry = KU_get_registry();
	KU_pSuite pSuite = NULL;
	KU_ErrorCode result;
	KU_ErrorCode result2;

	KU_set_error(result = KUE_SUCCESS);
	if (!pRegistry) {
		KU_set_error(result = KUE_NOREGISTRY);
		return result;
	}

	/* 测试用例开始运行，设置标签 */
	f_bTestIsRunning = KU_TRUE;

	/* 清除失败记录 */
	clear_previous_results(&f_run_summary, &f_failure_list);
	pSuite = pRegistry->pSuite;
	while (pSuite) {
		/* 测试包中有测试用例，则运行测试用例*/
		if (pSuite->uiNumberOfTests > 0) {
			result2 = run_single_suite(pSuite, &f_run_summary);
			result = (KUE_SUCCESS == result) ? result2 : result;
		}
		pSuite = pSuite->pNext;
	}

	/* 测试用例运行完毕，重新设置标签 */
	f_bTestIsRunning = KU_FALSE;
	if (f_pAllTestsCompleteMessageHandler)
		(*f_pAllTestsCompleteMessageHandler)(f_failure_list);

	return result;
}


/********************************************************
  * 函数名称: KU_assertImplementation
  *
  * 功能描述: 断言执行函数
  *
  * 调用函数: add_failure
  *
  * 被调函数:
  *
  * 输入参数: Param1: KU_BOOL bValue
  *                         Param2: unsigned int uiLine
  *                         Param3: char strCondition[]
  *                         Param4: char strFile[]
  *                         Param5: char strFunction[]
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       KU_TRUE                     执行成功
  *                       KU_FALSE                    执行失败
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
KU_BOOL KU_assertImplementation(KU_BOOL bValue, unsigned int uiLine, char strCondition[], char strFile[], char strFunction[])
{
	++f_run_summary.nAsserts;
	if (bValue == KU_FALSE) {
		++f_run_summary.nAssertsFailed;
		add_failure(&f_failure_list, &f_run_summary, uiLine, strCondition,
		strFile, f_pCurSuite, f_pCurTest);
	}
	return bValue;
}


/********************************************************
  * 函数名称: KU_clear_previous_results
  *
  * 功能描述: 清除失败记录和运行摘要信息
  *
  * 调用函数: clear_previous_results
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
KU_ErrorCode KU_clear_previous_results(void)
{
	clear_previous_results(&f_run_summary, &f_failure_list);
	return KUE_SUCCESS;
}


