/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Test.h
*
* 版    本 :
*
* 功    能 : 实现运行测试用例相关结构体定义&  函数声明
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

#ifndef KUNIT_H_TEST
#define KUNIT_H_TEST

#include "Regist.h"
#include "Kerror.h"

/**** 结构体定义****/

/** 失败记录数据类型. */
typedef struct KU_FailureRecord
{
	unsigned int  uiLineNumber;         /** 失败所在行号. */
	char*         strFileName;          /** 失败所在文件名. */
	char*         strCondition;         /** 失败发生的条件. */
	KU_pTest      pTest;                /** 失败所属测试用例. */
	KU_pSuite     pSuite;               /** 失败所属测试包. */
	struct KU_FailureRecord* pNext;     /** 下一条失败记录. */
	struct KU_FailureRecord* pPrev;     /** 前一条失败记录. */
} KU_FailureRecord;
typedef KU_FailureRecord* KU_pFailureRecord;  /** KU_FailureRecord指针类型. */

/** 运行摘要数据类型. */
typedef struct KU_RunSummary
{
	unsigned int nSuitesRun;                 /** 测试包总数. */
	unsigned int nSuitesFailed;              /** 失败的测试包数. */
	unsigned int nTestsRun;                  /** 测试用例总数. */
	unsigned int nTestsFailed;               /** 失败的测试用例数. */
	unsigned int nAsserts;                   /** 断言总数. */
	unsigned int nAssertsFailed;             /** 失败的断言数. */
	unsigned int nFailureRecords;            /** 失败记录总数. */
} KU_RunSummary;
typedef KU_RunSummary* KU_pRunSummary;  /** KU_RunSummary指针类型. */


/**** 函数指针定义****/

/** 测试包执行前调用函数. */
typedef void (*KU_TestStartMessageHandler)(const KU_pTest pTest, const KU_pSuite pSuite);

/** 测试用例执行后调用函数. */
typedef void (*KU_TestCompleteMessageHandler)(const KU_pTest pTest, const KU_pSuite pSuite, \
		const KU_pFailureRecord pFailure);

/** 所有测试用例执行后调用函数. */
typedef void (*KU_AllTestsCompleteMessageHandler)(const KU_pFailureRecord pFailure);

/** 测试包初始化失败后调用函数. */
typedef void (*KU_SuiteInitFailureMessageHandler)(const KU_pSuite pSuite);

/** 测试包清理失败后调用函数. */
typedef void (*KU_SuiteCleanupFailureMessageHandler)(const KU_pSuite pSuite);


/**** 函数说明****/

/** 测试用例是否正在执行. */
KU_BOOL KU_is_test_running(void);

/** 获取运行摘要信息. */
KU_pRunSummary KU_get_run_summary(void);

/** 运行所有测试用例. */
KU_ErrorCode KU_run_all_tests(void);

/** 设置测试包执行前调用函数句柄. */
void KU_set_test_start_handler(KU_TestStartMessageHandler pTestStartHandler);

/** 设置测试用例执行后调用函数句柄. */
void KU_set_test_complete_handler(KU_TestCompleteMessageHandler pTestCompleteHandler);

/** 设置所有测试用例执行后调用函数句柄. */
void KU_set_all_test_complete_handler(KU_AllTestsCompleteMessageHandler pAllTestsCompleteHandler);

/** 设置测试包初始化失败后调用函数句柄. */
void KU_set_suite_init_failure_handler(KU_SuiteInitFailureMessageHandler pSuiteInitFailureHandler);

/** 设置测试包清理失败后调用函数句柄. */
void KU_set_suite_cleanup_failure_handler(KU_SuiteCleanupFailureMessageHandler pSuiteCleanupFailureHandler);

/** 断言执行函数. */
KU_BOOL KU_assertImplementation(KU_BOOL bValue, unsigned int uiLine, char strCondition[], \
	char strFile[], char strFunction[]);

/** 清除失败记录和运行摘要信息. */
KU_ErrorCode KU_clear_previous_results(void);

#endif
